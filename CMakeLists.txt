cmake_minimum_required(VERSION 3.20)

# 工程名与语言：C + 启动汇编
project(stm32f103_baremetal C ASM)

# 供 IDE/语言服务器使用（如 clangd）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认使用 Debug，便于调试和单步
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

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

# 生成可执行固件（ELF）
add_executable(${PROJECT_NAME}.elf
  src/main.c
  src/KEY.c
  src/LED.c
  src/SYS.c
  startup/startup_stm32f103c8tx.s
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 目标专用编译选项
target_compile_options(${PROJECT_NAME}.elf PRIVATE
  ${COMMON_FLAGS}
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
