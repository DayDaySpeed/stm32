cmake_minimum_required(VERSION 3.20)

# 裸机工程不应在编译器检测阶段链接可执行文件
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 工程名与语言：C + 启动汇编
project(stm32f103_baremetal C ASM)

# 供 IDE/语言服务器使用（如 clangd）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认使用 Debug，便于调试和单步
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

option(DEBUG_LOG "Enable lightweight debug logging macros" OFF)
set(ASSERT_LEVEL "1" CACHE STRING "Assert strictness level (0-2)")
set(OLED_REFRESH_MODE "AUTO" CACHE STRING "OLED refresh mode: AUTO, FULL, REGION")

# C 语言标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MCU 相关编译/链接参数（Cortex-M3 + Thumb 指令集）
set(MCU_FLAGS
  -mcpu=cortex-m3
  -mthumb
  -mfloat-abi=soft
)

# 通用编译选项：节省空间 + 常规告警
set(COMMON_FLAGS
  ${MCU_FLAGS}
  -ffunction-sections
  -fdata-sections
  -fno-common
  -Wall
  -Wextra
  -Wpedantic
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_sources.cmake)

# 生成可执行固件（ELF）；源文件列表见 cmake/stm32_sources.cmake
add_executable(${PROJECT_NAME}.elf ${STM32_ELF_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/app
  ${CMAKE_SOURCE_DIR}/include/bsp
  ${CMAKE_SOURCE_DIR}/include/common
  ${CMAKE_SOURCE_DIR}/include/drivers
  ${CMAKE_SOURCE_DIR}/include/hal
)

# 目标专用编译选项
target_compile_options(${PROJECT_NAME}.elf PRIVATE
  ${COMMON_FLAGS}
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
  STM_DEBUG_LOG=$<BOOL:${DEBUG_LOG}>
  STM_ASSERT_LEVEL=${ASSERT_LEVEL}
  OLED_REFRESH_MODE_${OLED_REFRESH_MODE}=1
)

# 目标专用链接选项：
# - 使用 STM32F103C8 的链接脚本
# - --gc-sections 去掉未使用代码/数据
# - 生成 map 文件便于分析
target_link_options(${PROJECT_NAME}.elf PRIVATE
  ${MCU_FLAGS}
  -T${CMAKE_SOURCE_DIR}/linker/STM32F103C8TX_FLASH.ld
  -Wl,--gc-sections
  -Wl,-Map=${PROJECT_NAME}.map
)

# 每次生成 ELF 后，自动导出 HEX/BIN，并打印固件体积
if(NOT CMAKE_SIZE)
  find_program(CMAKE_SIZE arm-none-eabi-size)
endif()

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
  COMMENT "Generate .hex/.bin and print firmware size"
)

# OpenOCD 配置（可通过 -DOPENOCD_INTERFACE / -DOPENOCD_TARGET 覆盖）
set(OPENOCD_INTERFACE "interface/stlink.cfg" CACHE STRING "OpenOCD interface config")
set(OPENOCD_TARGET "target/stm32f1x.cfg" CACHE STRING "OpenOCD target config")

# 自定义烧录目标：
# cmake --build build --target flash
add_custom_target(flash
  COMMAND openocd
          -f ${OPENOCD_INTERFACE}
          -f ${OPENOCD_TARGET}
          -c "program ${PROJECT_NAME}.elf verify reset exit"
  DEPENDS ${PROJECT_NAME}.elf
  COMMENT "Flash with OpenOCD"
)

find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
  file(GLOB_RECURSE FORMAT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/include/*.h
  )
  add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXE} -i ${FORMAT_SOURCES}
    COMMENT "Format source files with clang-format"
  )
else()
  add_custom_target(format
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found, install to use target format"
  )
endif()

find_program(CPPCHECK_EXE cppcheck)
if(CPPCHECK_EXE)
  add_custom_target(lint
    COMMAND ${CPPCHECK_EXE} --enable=warning,style,performance --error-exitcode=1
            --std=c11 --quiet
            -I ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    COMMENT "Run cppcheck static analysis"
  )
else()
  add_custom_target(lint
    COMMAND ${CMAKE_COMMAND} -E echo "cppcheck not found, install to use target lint"
  )
endif()
