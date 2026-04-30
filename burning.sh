#!/usr/bin/env bash
set -euo pipefail

DEVICE="${DEVICE:-/dev/ttyUSB0}"
BAUD="${BAUD:-115200}"
BUILD_DIR="${BUILD_DIR:-build}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-cmake/arm-gcc-toolchain.cmake}"
OPEN_MONITOR=0
SKIP_FLASH=0
FLASH_ONLY=0
MONITOR_ONLY=0

usage() {
  echo "用法: ./burning.sh [选项]"
  echo ""
  echo "选项:"
  echo "  -d, --device <dev>    串口设备 (默认: /dev/ttyUSB0)"
  echo "  -b, --baud <rate>     串口波特率 (默认: 115200)"
  echo "  -m, --monitor         烧录后自动打开 minicom"
  echo "      --flash-only      只编译并烧录（不打开串口）"
  echo "      --monitor-only    只打开串口工具（不编译不烧录）"
  echo "      --no-flash        只编译，不烧录"
  echo "  -h, --help            显示帮助"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -d|--device)
      DEVICE="$2"
      shift 2
      ;;
    -b|--baud)
      BAUD="$2"
      shift 2
      ;;
    -m|--monitor)
      OPEN_MONITOR=1
      shift
      ;;
    --flash-only)
      FLASH_ONLY=1
      OPEN_MONITOR=0
      shift
      ;;
    --monitor-only)
      MONITOR_ONLY=1
      OPEN_MONITOR=1
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

if [[ "${FLASH_ONLY}" -eq 1 && "${MONITOR_ONLY}" -eq 1 ]]; then
  echo "错误: --flash-only 和 --monitor-only 不能同时使用"
  exit 1
fi

if [[ "${MONITOR_ONLY}" -eq 0 ]]; then
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

if [[ "${OPEN_MONITOR}" -eq 1 ]]; then
  echo "打开串口监视: ${DEVICE} @ ${BAUD}"
  minicom -D "${DEVICE}" -b "${BAUD}"
fi