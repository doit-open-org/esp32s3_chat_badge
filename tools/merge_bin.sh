#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"

if ! command -v idf.py >/dev/null 2>&1; then
    echo "error: idf.py was not found. Run ESP-IDF export.sh first." >&2
    exit 127
fi
if [[ ! -f "${BUILD_DIR}/flasher_args.json" ]]; then
    echo "error: build output not found; run ./tools/build.sh first." >&2
    exit 2
fi

cd "${ROOT_DIR}"
idf.py -B "${BUILD_DIR}" merge-bin

IMAGE="${BUILD_DIR}/merged-binary.bin"
if [[ ! -f "${IMAGE}" ]]; then
    echo "error: ${IMAGE} was not generated." >&2
    exit 3
fi

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${IMAGE}" | tee "${IMAGE}.sha256"
else
    shasum -a 256 "${IMAGE}" | tee "${IMAGE}.sha256"
fi
