#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${ESP_IDF_DOCKER_IMAGE:-espressif/idf:v5.4.4}"

if ! command -v docker >/dev/null 2>&1; then
    echo "error: Docker was not found." >&2
    exit 127
fi

docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e IDF_GIT_SAFE_DIR=/project \
    -v "${ROOT_DIR}:/project" \
    -w /project \
    "${IMAGE}" \
    bash -lc 'source "$IDF_PATH/export.sh" && ./tools/build.sh'
