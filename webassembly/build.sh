#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required to run this build" >&2
    exit 1
fi

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
DIST_DIR="${SCRIPT_DIR}/dist"
IMAGE_NAME="libsidplayfp-wasm:latest"

mkdir -p "${DIST_DIR}"

docker build -f "${SCRIPT_DIR}/Dockerfile" -t "${IMAGE_NAME}" "${SCRIPT_DIR}"
docker run --rm -v "${DIST_DIR}:/dist" "${IMAGE_NAME}"

echo "Artifacts are available in ${DIST_DIR}"