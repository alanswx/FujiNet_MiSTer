# HOWTO: FujiNet on MiSTer FPGA

This document provides a comprehensive guide for developers on cross-compiling, configuring, deploying, and debugging **FujiNet** on the **MiSTer FPGA** platform.

---

## 1. Architecture Overview

```
+-----------------------------------------------------------------------+
|                             DE10-Nano                                 |
|                                                                       |
|  +----------------------------------+  Internal UART  +------------+  |
|  |           Cyclone V FPGA         | <============>  |  ARM HPS   |  |
|  |                                  |   /dev/ttyS1    |  (Linux)   |  |
|  |  [CoCo 3 Core]                   |   19,200 baud   |  fujinet   |  |
|  |   - Multi-Pak Interface          |                 |   daemon   |  |
|  |   - Deluxe RS-232 Pak ($FF68)    |                 +-----+------+  |
|  |   - HDB-DOS DW3 Cartridge ROM    |                       |         |
|  +----------------------------------+                 Web UI (8000)   |
|                                                       TNFS Client     |
|                                                       Virtual Disks   |
|                                                       Virtual Modem   |
|                                                       Virtual Printer |
+-----------------------------------------------------------------------+
```

* **FPGA Core**: Emulates the retro computer hardware (e.g. TRS-80 Color Computer 3 with 6809/6309 CPU, 512KB/2MB RAM, and Deluxe RS-232 Pak). The core exposes its serial communications port to the HPS via `UART_TX` / `UART_RX` multiplexed into `/dev/ttyS1`.
* **ARM HPS Daemon (`fujinet`)**: Built from FujiNet's POSIX PC port (`fujinet_pc.cmake`). Runs as a background service on the Cyclone V ARM Cortex-A9 under embedded Linux.
* **Network & Storage**: FujiNet connects to your home LAN/WiFi via the Linux network stack, providing the retro computer with transparent TNFS internet disk hosting (`tnfs.fujinet.online`), named objects, real-time clock syncing, modem emulation, and a browser-based Web UI on port 8000.

---

## 2. Prerequisites & Cross-Compilation Toolchain

Builds are performed on an x86_64 or aarch64 Linux development workstation (e.g. Ubuntu 22.04/24.04).

### 2.1 Required Tools
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 pkg-config libexpat1-dev
```

### 2.2 Standard MiSTer ARM Toolchain
Download the Linaro / Arm GNU toolchain for ARMv7-A (`arm-linux-gnueabihf`):
```bash
# Recommended MiSTer GCC 10.2 toolchain:
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz
sudo tar -xf gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz -C /opt/
```
Export the prefix:
```bash
export ARM_PREFIX=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
# Or if using distro toolchain:
# sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
# export ARM_PREFIX=arm-linux-gnueabihf-
```

### 2.3 Cross-Compiling Static Dependencies (Expat & mbedTLS)

FujiNet links against `expat` (XML parsing) and `mbedtls` (cryptography/TLS). For an embedded target like MiSTer, static libraries ensure zero external shared library dependencies.

#### Build Expat for ARM:
```bash
mkdir -p deps && cd deps
wget https://github.com/libexpat/libexpat/releases/download/R_2_6_2/expat-2.6.2.tar.gz
tar -xzf expat-2.6.2.tar.gz && cd expat-2.6.2
mkdir -p build_arm && cd build_arm
cmake .. \
  -DCMAKE_C_COMPILER=${ARM_PREFIX}gcc \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_shared=OFF \
  -DEXPAT_BUILD_DOCS=OFF \
  -DEXPAT_BUILD_EXAMPLES=OFF \
  -DEXPAT_BUILD_TESTS=OFF \
  -DEXPAT_BUILD_TOOLS=OFF
make -j$(nproc)
make DESTDIR=$(pwd)/../../expat-install-arm install
cd ../..
```

#### Build mbedTLS for ARM:
```bash
wget https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v3.6.0.tar.gz
tar -xzf v3.6.0.tar.gz && cd mbedtls-3.6.0
mkdir -p build_arm && cd build_arm
cmake .. \
  -DCMAKE_C_COMPILER=${ARM_PREFIX}gcc \
  -DCMAKE_CXX_COMPILER=${ARM_PREFIX}g++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTING=OFF \
  -DENABLE_PROGRAMS=OFF
make -j$(nproc)
make DESTDIR=$(pwd)/../../mbedtls-install-arm install
cd ../..
```

---

## 3. Building FujiNet for MiSTer

### 3.1 One-Shot Build Script
To build the complete package, apply the portability patch, and create the ready-to-deploy distribution directory:

```bash
# Build for TRS-80 Color Computer (CoCo)
./build.sh COCO

# Or via Makefile:
make TARGET=COCO
```

### 3.2 Manual CMake Invocation
If you prefer running CMake directly:

```bash
cd fujinet-firmware

# Ensure the Expat/Python3 patch is applied:
git apply --check ../patches/0001-cmake-expat-python3.patch && git apply ../patches/0001-cmake-expat-python3.patch

mkdir -p build_arm && cd build_arm
cmake .. \
  -DCMAKE_C_COMPILER=${ARM_PREFIX}gcc \
  -DCMAKE_CXX_COMPILER=${ARM_PREFIX}g++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DFUJINET_TARGET=COCO \
  -DEXPAT_INCLUDE_DIR=$(pwd)/../../deps/expat-install-arm/include \
  -DEXPAT_LIBRARY_RELEASE=$(pwd)/../../deps/expat-install-arm/lib/libexpat.a \
  -DMBEDTLS_ROOT_DIR=$(pwd)/../../deps/mbedtls-install-arm

make -j$(nproc)
```

The resulting binary will be at `build_arm/fujinet`.

---

## 4. Configuration (`fnconfig.ini`)

FujiNet on MiSTer uses an INI configuration file to define ports, baud rates, and TNFS server bookmarks. 

Here is the recommended default configuration (`config/fnconfig.ini`):

```ini
[General]
devicename=MiSTer-CoCo
hsioindex=8
rotationsounds=1
configenabled=1
altconfigfile=
boot_mode=0
status_wait_enabled=1
printer_enabled=1
encrypt_passphrase=0

[WiFi]
enabled=0

[Bluetooth]
enabled=0

[Network]
sntpserver=pool.ntp.org

[Host1]
type=SD
name=SD

[Host2]
type=TNFS
name=tnfs.fujinet.online

[Host3]
type=TNFS
name=apps.irata.online

[Host4]
type=TNFS
name=fujinet.diller.org

[Modem]
modem_enabled=1
sniffer_enabled=0

[Cassette]
cassette_enabled=1

[CPM]
cpm_enabled=1

[ENABLE]
enable_device_slot_1=1
enable_device_slot_2=1
enable_device_slot_3=1
enable_device_slot_4=1
enable_device_slot_5=1
enable_device_slot_6=1
enable_device_slot_7=1
enable_device_slot_8=1
enable_apetime=1

[BOIP]
enabled=0

[Serial]
port=/dev/ttyS1
baud=19200
```

### Key Configuration Directives:
* `port`: Must match the HPS serial device connected to the FPGA core UART. On MiSTer, internal serial is `/dev/ttyS1`.
* `baud`: Must match the baud rate expected by the retro core firmware/ROM:
  * For CoCo 3 with `HDBDW3L3_19200.CCC`: `19200`
  * For CoCo 3 with `HDBDW3S3.CCC`: `115200`
* `Host2..Host4`: Remote TNFS file servers offering disk images and games over the internet.

---

## 5. Deployment to MiSTer

1. Copy the packaged `dist/fujinet/` directory to `/media/fat/fujinet/` on the MiSTer SD card:
   ```bash
   make install MISTER_HOST=mister.local
   # Or manually:
   scp -r dist/fujinet root@mister.local:/media/fat/
   ```

2. Directory structure on `/media/fat/fujinet/`:
   ```text
   /media/fat/fujinet/
   ├── fujinet             # Stripped ARM binary
   ├── fnconfig.ini        # Configuration
   ├── start_fujinet.sh    # Startup launcher
   ├── run-fujinet         # Wrapper with restart handling
   ├── data/               # Web UI assets & boot disks
   ├── roms/               # Boot cartridges (.CCC)
   └── SD/                 # Local disk image directory
   ```

---

## 6. Testing & Usage (TRS-80 Color Computer 3)

### Step 1: Core Setup
1. Boot the **CoCo3** core on MiSTer.
2. Open the Core OSD Menu (`F12`):
   * Set **UART Mode** -> **FujiNet** (or serial mode mapped to `/dev/ttyS1`).
   * Set **Multi-Pak** -> **(Slot 3) ECB / Cart**.
   * Select **Load Cartridge** and choose `/media/fat/fujinet/roms/HDBDW3L3_19200.CCC`.

### Step 2: Launch the FujiNet Daemon
Connect to the MiSTer terminal (via SSH or serial console):
```bash
/media/fat/fujinet/start_fujinet.sh /dev/ttyS1 19200
```
Inspect the startup log:
```text
### TTY initialized ###
TTY set_baudrate: 19200
setup(): DRIVEWIRE MODE
Starting web server http://0.0.0.0:8000
```

### Step 3: Boot CoCo 3
1. Press the Reset button or Core Reset (`F12`).
2. The CoCo will boot into **HDB-DOS DW3**:
   * Type `DIR` to list mounted virtual drives.
   * Type `DOS` to launch the interactive FujiNet configuration utility.

### Step 4: Web Management Interface
From any PC or mobile device on the same local network:
1. Open your web browser to:
   ```
   http://mister.local:8000/
   ```
   (or `http://<your-mister-ip>:8000/`)
2. Use the Web UI to:
   * Browse internet TNFS servers (`tnfs.fujinet.online`).
   * Mount games, OS images, and demos into virtual drives 1 through 4.
   * Access the virtual printer spool.
