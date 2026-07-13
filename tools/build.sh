#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;main/boards/esp32s3-chat-badge/sdkconfig.defaults"

if ! command -v idf.py >/dev/null 2>&1; then
    echo "error: idf.py was not found. Run ESP-IDF export.sh first." >&2
    exit 127
fi

cd "${ROOT_DIR}"
idf.py \
    -B "${BUILD_DIR}" \
    -DSDKCONFIG="${BUILD_DIR}/sdkconfig" \
    -DSDKCONFIG_DEFAULTS="${SDKCONFIG_DEFAULTS}" \
    -DIDF_TARGET=esp32s3 \
    build
