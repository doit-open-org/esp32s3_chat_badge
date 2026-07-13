#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
PORT="${1:-${ESPPORT:-}}"

if ! command -v idf.py >/dev/null 2>&1; then
    echo "error: idf.py was not found. Run ESP-IDF export.sh first." >&2
    exit 127
fi
if [[ ! -f "${BUILD_DIR}/flasher_args.json" ]]; then
    echo "error: build output not found; run ./tools/build.sh first." >&2
    exit 2
fi
if [[ -z "${PORT}" ]]; then
    echo "usage: $0 <serial-port>" >&2
    echo "example: $0 /dev/ttyACM0" >&2
    exit 2
fi

cd "${ROOT_DIR}"
idf.py -B "${BUILD_DIR}" -p "${PORT}" flash monitor
