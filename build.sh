#!/bin/bash
# FujiNet_MiSTer - Cross-compilation build script for MiSTer ARM HPS (Cyclone V)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FUJINET_DIR="${SCRIPT_DIR}/fujinet-firmware"
PATCH_FILE="${SCRIPT_DIR}/patches/0001-cmake-expat-python3.patch"
TARGET="${1:-COCO}"
BUILD_DIR="${SCRIPT_DIR}/build_arm"
DIST_DIR="${SCRIPT_DIR}/dist/fujinet"

echo "========================================================"
echo " Building FujiNet for MiSTer HPS (Target: ${TARGET})"
echo "========================================================"

# 1. Verify Submodule
if [ ! -f "${FUJINET_DIR}/fujinet_pc.cmake" ]; then
    echo "Submodule not initialized. Initializing..."
    git -C "${SCRIPT_DIR}" submodule update --init --recursive
fi

# 2. Apply Portability Patch
echo "Checking patch: ${PATCH_FILE}..."
if git -C "${FUJINET_DIR}" apply --check "${PATCH_FILE}" 2>/dev/null; then
    echo "Applying patch..."
    git -C "${FUJINET_DIR}" apply "${PATCH_FILE}"
elif git -C "${FUJINET_DIR}" diff | grep -q "find_package(EXPAT REQUIRED)"; then
    echo "Patch already applied."
else
    echo "Warning: Patch could not be applied cleanly; verifying if already patched."
fi

# 3. Locate ARM Cross-Compiler
if [ -z "${ARM_PREFIX}" ]; then
    if [ -f "/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-gcc" ]; then
        ARM_PREFIX="/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-"
    elif which arm-linux-gnueabihf-gcc >/dev/null 2>&1; then
        ARM_PREFIX="arm-linux-gnueabihf-"
    else
        echo "Error: Could not locate ARM cross-compiler (arm-none-linux-gnueabihf-gcc or arm-linux-gnueabihf-gcc)."
        echo "Please set ARM_PREFIX environment variable, e.g.:"
        echo "  export ARM_PREFIX=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-"
        exit 1
    fi
fi

CC="${ARM_PREFIX}gcc"
CXX="${ARM_PREFIX}g++"
echo "Using Cross Compiler: ${CC}"

# 4. Dependency Directory Setup
# Defaults to $SCRIPT_DIR/deps if not externally provided
DEPS_DIR="${DEPS_DIR:-${SCRIPT_DIR}/deps}"
EXPAT_DIR="${EXPAT_DIR:-${DEPS_DIR}/expat-install-arm}"
MBEDTLS_DIR="${MBEDTLS_DIR:-${DEPS_DIR}/mbedtls-install-arm}"

CMAKE_ARGS=(
    "-DCMAKE_C_COMPILER=${CC}"
    "-DCMAKE_CXX_COMPILER=${CXX}"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DFUJINET_TARGET=${TARGET}"
    "-DWITH_ZLIB=OFF"
    "-DWITH_MBEDTLS=ON"
)

if [ -d "${EXPAT_DIR}" ]; then
    echo "Using custom Expat: ${EXPAT_DIR}"
    CMAKE_ARGS+=(
        "-DEXPAT_INCLUDE_DIR=${EXPAT_DIR}/include"
        "-DEXPAT_LIBRARY_RELEASE=${EXPAT_DIR}/lib/libexpat.a"
    )
fi

if [ -d "${MBEDTLS_DIR}" ]; then
    echo "Using custom mbedTLS: ${MBEDTLS_DIR}"
    CMAKE_ARGS+=(
        "-DMBEDTLS_ROOT_DIR=${MBEDTLS_DIR}"
    )
fi

# 5. Build
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "Configuring CMake..."
cmake "${FUJINET_DIR}" "${CMAKE_ARGS[@]}"

echo "Compiling..."
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# 6. Package Distribution Bundle
echo "Packaging distribution into ${DIST_DIR}..."
mkdir -p "${DIST_DIR}/SD"
cp "${BUILD_DIR}/fujinet" "${DIST_DIR}/"
${ARM_PREFIX}strip "${DIST_DIR}/fujinet" 2>/dev/null || true

# Copy Web UI & Assets
if [ -d "${BUILD_DIR}/data" ]; then
    cp -r "${BUILD_DIR}/data" "${DIST_DIR}/"
elif [ -d "${FUJINET_DIR}/data" ]; then
    cp -r "${FUJINET_DIR}/data" "${DIST_DIR}/"
fi

# Copy Config, Scripts & ROMs
cp "${SCRIPT_DIR}/config/fnconfig.ini" "${DIST_DIR}/"
cp "${SCRIPT_DIR}/scripts/start_fujinet.sh" "${DIST_DIR}/"
cp "${SCRIPT_DIR}/scripts/run-fujinet" "${DIST_DIR}/"
chmod +x "${DIST_DIR}/start_fujinet.sh" "${DIST_DIR}/run-fujinet" "${DIST_DIR}/fujinet"

if [ -d "${SCRIPT_DIR}/roms" ]; then
    cp -r "${SCRIPT_DIR}/roms" "${DIST_DIR}/"
fi

echo "========================================================"
echo " Build & Packaging Complete!"
echo " Output files are ready in: ${DIST_DIR}"
echo " Copy ${DIST_DIR} to /media/fat/fujinet/ on your MiSTer."
echo "========================================================"
