#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT}"

DEVICE="${DEVICE:-/dev/ttyUSB0}"
BAUD="${BAUD:-115200}"
BUILD_DIR="${BUILD_DIR:-build}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-cmake/arm-gcc-toolchain.cmake}"

# 仅串口（不编译、不烧录）
ONLY_MINICOM=0
# 编译+烧录（默认 1；--minicom-only 时置 0）
DO_BUILD_FLASH=1
# 烧录成功后是否再打开 minicom（-m / --monitor）
OPEN_MINICOM_AFTER=0
SKIP_FLASH=0

usage() {
  echo "用法: ./burning.sh [选项]"
  echo ""
  echo "选项:"
  echo "  -d, --device <dev>    串口设备 (默认: /dev/ttyUSB0)"
  echo "  -b, --baud <rate>     串口波特率 (默认: 115200)"
  echo "  -m, --monitor         编译、烧录成功后打开 minicom"
  echo "      --flash-only      只编译并烧录（不打开串口；与默认相同）"
  echo "      --minicom-only    只打开串口（不编译、不烧录）"
  echo "      --no-flash        只编译，不烧录"
  echo "  -h, --help            显示帮助"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -d|--device)
      if [[ $# -lt 2 ]]; then
        echo "错误: $1 需要设备路径参数"
        exit 1
      fi
      DEVICE="$2"
      shift 2
      ;;
    -b|--baud)
      if [[ $# -lt 2 ]]; then
        echo "错误: $1 需要波特率参数"
        exit 1
      fi
      BAUD="$2"
      shift 2
      ;;
    -m|--monitor)
      OPEN_MINICOM_AFTER=1
      ONLY_MINICOM=0
      DO_BUILD_FLASH=1
      shift
      ;;
    --flash-only)
      OPEN_MINICOM_AFTER=0
      ONLY_MINICOM=0
      DO_BUILD_FLASH=1
      shift
      ;;
    --minicom-only)
      ONLY_MINICOM=1
      DO_BUILD_FLASH=0
      OPEN_MINICOM_AFTER=1
      shift
      ;;
    --no-flash)
      SKIP_FLASH=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "未知参数: $1"
      usage
      exit 1
      ;;
  esac
done

if [[ "${ONLY_MINICOM}" -eq 1 ]]; then
  echo "打开串口监视: ${DEVICE} @ ${BAUD}"
  minicom -D "${DEVICE}" -b "${BAUD}"
  exit 0
fi

if [[ "${DO_BUILD_FLASH}" -eq 1 ]]; then
  echo "[1/3] 配置 CMake..."
  cmake -S . -B "${BUILD_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"

  echo "[2/3] 编译固件..."
  cmake --build "${BUILD_DIR}"

  if [[ "${SKIP_FLASH}" -eq 0 ]]; then
    echo "[3/3] 烧录固件..."
    cmake --build "${BUILD_DIR}" --target flash
  else
    echo "[3/3] 跳过烧录 (--no-flash)"
  fi
fi

if [[ "${OPEN_MINICOM_AFTER}" -eq 1 ]]; then
  echo "打开串口监视: ${DEVICE} @ ${BAUD}"
  minicom -D "${DEVICE}" -b "${BAUD}"
fi
