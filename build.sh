#!/bin/bash
# FujiNet_MiSTer - Cross-compilation build script for MiSTer ARM HPS (Cyclone V)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FUJINET_DIR="${SCRIPT_DIR}/fujinet-firmware"
TARGET="${1:-COCO}"
# One build tree and one dist per target, so an ADAM build does not clobber the
# COCO one; both can still be overridden from the environment.
target_lc=$(echo "${TARGET}" | tr 'A-Z' 'a-z')
if [ "${TARGET}" = "COCO" ]; then
    BUILD_DIR="${BUILD_DIR:-${SCRIPT_DIR}/build_arm}"
    DIST_DIR="${DIST_DIR:-${SCRIPT_DIR}/dist/fujinet}"
else
    BUILD_DIR="${BUILD_DIR:-${SCRIPT_DIR}/build_arm_${target_lc}}"
    DIST_DIR="${DIST_DIR:-${SCRIPT_DIR}/dist/fujinet-${target_lc}}"
fi

echo "========================================================"
echo " Building FujiNet for MiSTer HPS (Target: ${TARGET})"
echo "========================================================"

# 1. Verify Submodule
if [ ! -f "${FUJINET_DIR}/fujinet_pc.cmake" ]; then
    echo "Submodule not initialized. Initializing..."
    git -C "${SCRIPT_DIR}" submodule update --init --recursive
fi

# 2. Apply the patches: portability (0001) and whatever else patches/ holds, each
#    skipped when it is already in the tree (it then applies in reverse).
for p in "${SCRIPT_DIR}"/patches/*.patch; do
    [ -f "${p}" ] || continue
    echo "Checking patch: ${p}..."
    if git -C "${FUJINET_DIR}" apply --check "${p}" 2>/dev/null; then
        echo "Applying patch..."
        git -C "${FUJINET_DIR}" apply "${p}"
    elif git -C "${FUJINET_DIR}" apply --reverse --check "${p}" 2>/dev/null; then
        echo "Patch already applied."
    else
        echo "Warning: ${p} applies neither forwards nor backwards; continuing."
    fi
done

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
    "-DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=TRUE"
    # Upstream compiles with -Werror. On 32-bit ARM, GCC's -Wcast-align fires in
    # vendored libssh (void* casts that are fine on Cortex-A9); keep it a warning.
    "-DCMAKE_C_FLAGS=-Wno-error=cast-align"
    "-DCMAKE_CXX_FLAGS=-Wno-error=cast-align"
)

if [ -d "${EXPAT_DIR}" ]; then
    echo "Using custom Expat: ${EXPAT_DIR}"
    # CMake < 3.24 FindEXPAT reads EXPAT_LIBRARY; 3.24+ reads EXPAT_LIBRARY_RELEASE.
    CMAKE_ARGS+=(
        "-DEXPAT_INCLUDE_DIR=${EXPAT_DIR}/include"
        "-DEXPAT_LIBRARY=${EXPAT_DIR}/lib/libexpat.a"
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
# "fujinet" is the daemon (its unit tests are left out; they are host-only);
# "build_webui" generates ${BUILD_DIR}/data (web UI, printer fonts, CONFIG
# boot disks), which no default target produces.
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)" fujinet build_webui

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
# A target with its own config (config/fnconfig-<target>.ini) gets that, else the CoCo one
if [ -f "${SCRIPT_DIR}/config/fnconfig-${target_lc}.ini" ]; then
    cp "${SCRIPT_DIR}/config/fnconfig-${target_lc}.ini" "${DIST_DIR}/fnconfig.ini"
else
    cp "${SCRIPT_DIR}/config/fnconfig.ini" "${DIST_DIR}/"
fi
cp "${SCRIPT_DIR}/scripts/start_fujinet.sh" "${DIST_DIR}/"
cp "${SCRIPT_DIR}/scripts/run-fujinet" "${DIST_DIR}/"
chmod +x "${DIST_DIR}/start_fujinet.sh" "${DIST_DIR}/run-fujinet" "${DIST_DIR}/fujinet"

# The HDB-DOS cartridges are the CoCo's
if [ "${TARGET}" = "COCO" ] && [ -d "${SCRIPT_DIR}/roms" ]; then
    cp -r "${SCRIPT_DIR}/roms" "${DIST_DIR}/"
fi

echo "========================================================"
echo " Build & Packaging Complete!"
echo " Output files are ready in: ${DIST_DIR}"
echo " Copy ${DIST_DIR} to /media/fat/fujinet/ on your MiSTer."
echo "========================================================"
