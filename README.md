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
├── docker/
│   └── Dockerfile                  # Cross-build image: Arm GCC 10.2 + static Expat/Mbed TLS
├── build.sh                        # Automated cross-compilation & packaging script
├── build-docker.sh                 # Runs build.sh inside the Docker image (no local toolchain needed)
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
   # optional third argument: the folder served as the SD host, e.g. /media/fat/games/CoCo3
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

### Option A: Docker (macOS or Linux, no toolchain to install)

```bash
./build-docker.sh COCO
```

This builds the `fujinet-mister-build` image from `docker/Dockerfile` (Arm GNU
toolchain 10.2-2020.11, the one MiSTer Main is built with, plus static Expat and
Mbed TLS cross-built for armhf) and runs `./build.sh COCO` inside it. The output
lands in `dist/fujinet/` exactly as with the native build below.

### Option B: Native Linux cross-build

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

## 4a. The Coleco ADAM target

The ADAM core (`ColecoAdam_MiSTer`, branch `fujinet`) is the AdamNet bus master for the
devices FujiNet-PC serves, over the same HPS UART. Build and install:

```bash
./build-docker.sh ADAM                      # -> dist/fujinet-adam/ (the COCO dist is untouched)
scp -r dist/fujinet-adam root@mister.local:/media/fat/fujinet/Adam
scp scripts/uartmode.fujinet root@mister.local:/media/fat/linux/
```

Then on the MiSTer install `uartmode.fujinet` as `/sbin/uartmode` (see the script's header: the
root filesystem is read-only). Each core has its own install in a folder named after the core
(`/tmp/CORENAME`): `/media/fat/fujinet/COCO3/`, `/media/fat/fujinet/Adam/`,
`/media/fat/fujinet/Studio-II/`. Main (`mac-printer-fujinet`) offers the FujiNet UART connection
only when the loaded core's folder holds `fujinet` or `launch`, and in FujiNet mode offers the
speeds the core declares in its CONF_STR. Mode 8 writes that speed into the folder's
`fnconfig.ini` (or passes it to `launch`) and serves the core's games folder as host 1. In the ADAM core: UART connection
**FujiNet**, speed **115200**, no image in floppy 1, reset: EOS boots FujiNet's CONFIG from disk 1.

What differs from the CoCo:
* `config/fnconfig-adam.ini`: the serial path (`[BOIP] enabled=0`, `[Serial] port=/dev/ttyS1`).
  The ADAM PC build defaults to Bus-over-IP, which is what the core's Verilator simulator uses.
* `patches/0002-adam-serial-port-and-baud-from-config.patch`: upstream's ADAM serial path probed
  `/dev/ttyUSB0` then `/dev/ttyS0` at a fixed 62,500 baud, waited only 2 ms for the rest of a
  packet, which a Linux tty does not meet (every block request was NAKed on a MiSTer), and
  discarded its input for 180 µs after each reply, meant for the one-wire bus's echo, which on
  Linux landed on the master's next command instead. It now takes `[Serial]` port and baud,
  waits 100 ms, and does not discard.
* `patches/0003-ndevice-adam-open-spec-and-parser-without-protocol.patch`: the ADAM's
  fujinet-lib puts a control byte before the `N:` of a network open spec; the firmware used to
  strip blindly up to the colon, now it refuses the open, and the program's next command then
  crashed on the missing protocol. Both fixed; found with `weather.ddp`.
* `[WiFi] enabled=1` in the config, although there is no WiFi: a TNFS mount stalls after
  resolving the host without it.
* No cartridge: CONFIG is on FujiNet's own disk 1, and the ADAM boots from disk 1 by itself.

## 5. Contributing & Upstream Synchronization

* **Upstream Patches**: The patch in `patches/0001-cmake-expat-python3.patch` ensures POSIX CMake builds correctly find Expat without pkg-config overrides and use `python3`. This will be submitted upstream to `FujiNetWIFI/fujinet-firmware`.
* **Updating Submodule** (upstream `master` is occasionally force-pushed, so reset rather than merge):
  ```bash
  git -C fujinet-firmware fetch origin
  git -C fujinet-firmware checkout -B master origin/master
  git -C fujinet-firmware apply --check ../patches/0001-cmake-expat-python3.patch   # confirm the patch still applies
  ./build-docker.sh COCO
  ```
