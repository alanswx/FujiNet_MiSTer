# FujiNet for MiSTer - TRS-80 Color Computer (CoCo / CoCo 3) Demo

This package provides a complete **FujiNet** implementation running on the MiSTer ARM HPS, providing full virtual networking, TNFS internet disk access, virtual printer, modem, real-time clock, and web management interface to the **CoCo 3 core** over internal serial (`/dev/ttyS1`).

---

## 1. Package Contents

* `fujinet`: FujiNet ARM executable for MiSTer Linux (Cyclone V ARMv7 Cortex-A9).
* `fnconfig.ini`: Configuration file configured for `/dev/ttyS1` at 19,200 baud, connecting to `tnfs.fujinet.online`.
* `data/`: Full Web UI assets and CoCo autorun disk images (`autorun.dsk`, `AUTOLOAD.DWL`, `CONFIG.DWL`).
* `roms/`:
  - `HDBDW3L3_19200.CCC`: HDB-DOS DW3 ROM for CoCo 3 with Deluxe RS-232 Pak (6551 ACIA at $FF68) at 19,200 baud (matches MiSTer default UART speed).
  - `HDBDW3S3.CCC`: HDB-DOS DW3 ROM for CoCo 3 with Deluxe RS-232 Pak at 115,200 baud.
  - `HDBDW3CC3.CCC`: Standard HDB-DOS DW3 for CoCo 3.
* `SD/`: Local virtual disk storage.
* `start_fujinet.sh`: Quick launch script for standalone testing.

---

## 2. Quick Setup on MiSTer

1. Copy the `fujinet` folder to `/media/fat/fujinet/` on your MiSTer SD card.
2. In the CoCo3 core on MiSTer:
   - Go to OSD Menu -> **UART Mode** -> Select **FujiNet** (or run `uartmode 8`).
   - In the CoCo3 OSD, set **Multi-Pak** to **(Slot 3) ECB / Cart**.
   - Select **Load Cartridge** and choose `/media/fat/fujinet/roms/HDBDW3L3_19200.CCC`.
3. Reset the CoCo (`F12` or Reset button).
4. CoCo boots with `HDB-DOS DW3`!
   - Type `DIR` to view the mounted virtual disk or FujiNet autoload menu.
   - Type `DOS` to launch the FujiNet config utility.
5. On your PC/phone browser on the same local network:
   - Navigate to `http://<mister-ip>:8000/` to open the FujiNet Web Management UI!
   - Browse TNFS servers (`tnfs.fujinet.online`), mount games/demos into virtual drives 1-4, configure WiFi/Network settings, and view printer output.

---

## 3. Supported Features

- **Full DriveWire 4 Protocol**: Sector read/write, named objects, time synchronization.
- **TNFS Client**: Direct network mounting of disk images from remote TNFS servers across the internet.
- **N: Network Device**: CoCo network applications (Weather, News, Telnet/Netcat, Wikipedia).
- **Virtual Printer**: Epson/file printer output.
- **Web UI**: Complete web interface hosted on port 8000.
