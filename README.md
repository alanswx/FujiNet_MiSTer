# FujiNet for MiSTer FPGA

FujiNet virtual peripheral emulator running on the **MiSTer FPGA** Cyclone V ARM HPS.

This companion package runs FujiNet as an embedded Linux service on the MiSTer ARM processor, communicating with retro computer cores over internal hardware serial (`/dev/ttyS1`). It provides virtual network disks via TNFS, virtual modem, virtual printer, network utilities, real-time clock synchronization, and a browser-based Web UI on port 8000.

---

## 1. Cloning this Repository (Submodule Support)

This repository uses a Git submodule pointing to the official [`FujiNetWIFI/fujinet-firmware`](https://github.com/FujiNetWIFI/fujinet-firmware) upstream codebase.

### Option A: Clone with submodules initialized in one command (Recommended)
```bash
git clone --recurse-submodules https://github.com/alanswx/FujiNet_MiSTer.git
cd FujiNet_MiSTer
```

### Option B: If you already cloned without `--recurse-submodules`
If you cloned the repo normally and the `fujinet-firmware` directory is empty, run:
```bash
git submodule update --init --recursive
```

---

## 2. Repository Layout

```text
FujiNet_MiSTer/
├── .gitmodules                     # Tracks upstream FujiNetWIFI/fujinet-firmware
├── fujinet-firmware/               # Submodule: Upstream FujiNet codebase
├── patches/
│   └── 0001-cmake-expat-python3.patch  # Portability patch for CMake finding Expat & python3
├── config/
│   └── fnconfig.ini                # Preconfigured INI for MiSTer (/dev/ttyS1 @ 19,200 baud)
├── scripts/
│   ├── start_fujinet.sh            # Launch script for MiSTer
│   └── run-fujinet                 # Wrapper handling daemon restarts
├── roms/                           # Cartridge ROMs for CoCo 3
│   ├── HDBDW3L3_19200.CCC          # HDB-DOS DW3 for CoCo 3 at 19,200 baud
│   ├── HDBDW3S3.CCC                # HDB-DOS DW3 for CoCo 3 at 115,200 baud
│   └── HDBDW3CC3.CCC               # Standard HDB-DOS DW3
├── build.sh                        # Automated cross-compilation & packaging script
├── Makefile                        # Convenience make targets (build, clean, install)
├── HOWTO_MISTER.md                 # Detailed toolchain, build & developer guide
└── README.md
```

---

## 3. Quick Start (Pre-Built Deployment)

If you already have a compiled binary or distribution bundle:

1. Copy the `dist/fujinet` folder to `/media/fat/fujinet/` on your MiSTer SD card:
   ```bash
   scp -r dist/fujinet root@mister.local:/media/fat/
   ```

2. Start the daemon on MiSTer (SSH to `root@mister.local`):
   ```bash
   /media/fat/fujinet/start_fujinet.sh /dev/ttyS1 19200
   ```

3. Open the **TRS-80 Color Computer 3 (CoCo 3)** core on MiSTer:
   - In Core OSD (`F12`), set **UART Mode** -> **FujiNet** (maps serial to `/dev/ttyS1`).
   - Set **Multi-Pak** to **(Slot 3) ECB / Cart**.
   - Select **Load Cartridge** and choose `/media/fat/fujinet/roms/HDBDW3L3_19200.CCC`.
   - Reset the core (`F12` -> Reset).

4. Access the Web Management Interface:
   - In your web browser, navigate to:
     ```
     http://mister.local:8000/
     ```
   - Browse TNFS file servers (`tnfs.fujinet.online`), mount disk images, and manage devices.

---

## 4. Building from Source

To cross-compile the ARM binary from Linux:

```bash
# 1. Ensure the ARMv7 toolchain is in your PATH or set ARM_PREFIX:
export ARM_PREFIX=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-

# 2. Build for target (e.g. COCO):
./build.sh COCO

# 3. Deploy to MiSTer over SSH:
make install MISTER_HOST=mister.local
```

For full details on building static dependencies (`libexpat.a`, `libmbedtls.a`), toolchain setup, and debugging, refer to [HOWTO_MISTER.md](HOWTO_MISTER.md).

---

## 5. Contributing & Upstream Synchronization

* **Upstream Patches**: The patch in `patches/0001-cmake-expat-python3.patch` ensures POSIX CMake builds correctly find Expat without pkg-config overrides and use `python3`. This will be submitted upstream to `FujiNetWIFI/fujinet-firmware`.
* **Updating Submodule**:
  ```bash
  git submodule update --remote --merge
  ```
