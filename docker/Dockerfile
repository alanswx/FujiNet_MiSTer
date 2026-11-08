# Cross-build environment for FujiNet on the MiSTer ARM HPS (Cyclone V, armhf).
#
# Contains the Arm GNU toolchain 10.2-2020.11 (the toolchain MiSTer Main is
# built with) plus static Expat and Mbed TLS cross-built for armhf, so the
# resulting fujinet binary has no dependencies beyond the MiSTer's glibc.
#
# Build the image:   docker build -t fujinet-mister-build docker/
# Build FujiNet:     ./build-docker.sh COCO          (wraps the two steps)

FROM ubuntu:22.04

ARG TARGETARCH
ARG GCC_VER=10.2-2020.11
ARG EXPAT_VER=2.10.0
ARG MBEDTLS_VER=3.6.7

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git make pkg-config file ca-certificates \
        wget xz-utils bzip2 \
        python3 python3-jinja2 python3-yaml python3-jsonschema \
    && rm -rf /var/lib/apt/lists/*

# Arm GNU toolchain for arm-none-linux-gnueabihf, hosted on this image's CPU.
RUN set -e; \
    case "${TARGETARCH:-$(dpkg --print-architecture)}" in \
        arm64) HOST=aarch64 ;; \
        amd64) HOST=x86_64 ;; \
        *) echo "unsupported build host architecture: ${TARGETARCH}" >&2; exit 1 ;; \
    esac; \
    mkdir -p /opt/mister-toolchain && cd /opt/mister-toolchain && \
    wget -q "https://developer.arm.com/-/media/Files/downloads/gnu-a/${GCC_VER}/binrel/gcc-arm-${GCC_VER}-${HOST}-arm-none-linux-gnueabihf.tar.xz" -O toolchain.tar.xz && \
    tar -xf toolchain.tar.xz && rm toolchain.tar.xz && \
    ln -s "/opt/mister-toolchain/gcc-arm-${GCC_VER}-${HOST}-arm-none-linux-gnueabihf" /opt/mister-toolchain/gcc-arm

ENV ARM_PREFIX=/opt/mister-toolchain/gcc-arm/bin/arm-none-linux-gnueabihf-
ENV PATH=/opt/mister-toolchain/gcc-arm/bin:$PATH
ENV DEPS_DIR=/opt/deps

# Static Expat and Mbed TLS for armhf (layout expected by ../build.sh).
RUN set -e; mkdir -p /tmp/deps && cd /tmp/deps && \
    wget -q "https://github.com/libexpat/libexpat/releases/download/R_$(echo ${EXPAT_VER} | tr . _)/expat-${EXPAT_VER}.tar.gz" && \
    tar -xzf "expat-${EXPAT_VER}.tar.gz" && cd "expat-${EXPAT_VER}" && \
    cmake -S . -B build \
        -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm \
        -DCMAKE_C_COMPILER=${ARM_PREFIX}gcc \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${DEPS_DIR}/expat-install-arm -DCMAKE_INSTALL_LIBDIR=lib \
        -DEXPAT_SHARED_LIBS=OFF -DEXPAT_BUILD_DOCS=OFF -DEXPAT_BUILD_EXAMPLES=OFF \
        -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_TOOLS=OFF && \
    cmake --build build -j"$(nproc)" && cmake --install build && \
    cd /tmp/deps && \
    wget -q "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-${MBEDTLS_VER}/mbedtls-${MBEDTLS_VER}.tar.bz2" && \
    tar -xjf "mbedtls-${MBEDTLS_VER}.tar.bz2" && cd "mbedtls-${MBEDTLS_VER}" && \
    # Threading on: the bundled libssh initialises Mbed TLS for pthreads and
    # fails (stderr "Error in auto_init()", no SSH/SFTP) when it is built without.
    python3 scripts/config.py set MBEDTLS_THREADING_C && \
    python3 scripts/config.py set MBEDTLS_THREADING_PTHREAD && \
    cmake -S . -B build \
        -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm \
        -DCMAKE_C_COMPILER=${ARM_PREFIX}gcc -DCMAKE_CXX_COMPILER=${ARM_PREFIX}g++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${DEPS_DIR}/mbedtls-install-arm -DCMAKE_INSTALL_LIBDIR=lib \
        -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF -DGEN_FILES=OFF \
        -DMBEDTLS_FATAL_WARNINGS=OFF && \
    cmake --build build -j"$(nproc)" && cmake --install build && \
    rm -rf /tmp/deps

# The repository is bind-mounted at /work and owned by the host user.
RUN git config --system --add safe.directory '*'

WORKDIR /work
CMD ["./build.sh", "COCO"]
