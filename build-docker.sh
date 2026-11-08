#!/bin/bash
# FujiNet_MiSTer - run ./build.sh inside the Docker cross-build image, so no
# local ARM toolchain is needed (works on macOS and Linux hosts).
#
#   ./build-docker.sh [TARGET]      # TARGET defaults to COCO, as in build.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE="${FUJINET_BUILD_IMAGE:-fujinet-mister-build}"

echo "Building Docker image ${IMAGE} (toolchain + static Expat/Mbed TLS)..."
docker build -t "${IMAGE}" "${SCRIPT_DIR}/docker"

RUN_ARGS=(--rm -v "${SCRIPT_DIR}:/work" -w /work -e HOME=/tmp)
# On a Linux host, write the build output as the invoking user, not root.
if [ "$(uname -s)" = "Linux" ]; then
    RUN_ARGS+=(--user "$(id -u):$(id -g)")
fi

echo "Running ./build.sh $* in ${IMAGE}..."
docker run "${RUN_ARGS[@]}" "${IMAGE}" ./build.sh "$@"
