# FujiNet for the Apple IIgs core: options and plan

Status: design notes, nothing built yet (2026-11-10). Covers the MiSTer
`Apple-IIgs_MiSTer` core; the same card would suit the Apple-II core.
See [MULTI_PLATFORM.md](MULTI_PLATFORM.md) for how FujiNet runs on the other
cores.

---

## 1. What the Apple FujiNet-PC build expects

FujiNet-PC for `FUJINET_TARGET=APPLE` never sees IWM signals. It receives whole
**SmartPort commands** as packets, the protocol AppleWin's SmartPort-over-SLIP
card uses (`fujinet-firmware/lib/bus/iwm/iwm_slip.cpp`, `lib/devrelay/`):

* **Commands:** Status, ReadBlock, WriteBlock, Format, Control, Init, Open,
  Close, Read, Write (`devrelay/types/Command.h`, numbers 0-9).
* **Request:** `seq, command, param_count, unit, ...params` (ReadBlock: block
  number at offset 6, 11 bytes in all; WriteBlock adds 512 data bytes at
  offset 11; Control carries a length-prefixed list).
* **Response:** `seq, status, ...data` (ReadBlock: 2 + 512 bytes).
* **Framing:** each packet is SLIP-framed (`END $C0`, `ESC $DB`;
  `devrelay/slip/SLIP.cpp`). This borrows only SLIP's byte framing: there is
  no IP inside, so the Linux kernel's SLIP driver is no use here (and the
  MiSTer kernel has `CONFIG_SLIP` off anyway).
* **Transport:** CMake option `SLIP_PROTOCOL`:
  * `NET` (default): FujiNet **connects out** as a TCP client to
    `Config.get_boip_host()`/`get_boip_port()` (AppleWin listens).
  * `COM`: a serial port, but `connector_com.cpp` only picks a **USB** port
    from `sp_list_ports()` and never sets the baud, so it can't open
    `/dev/ttyS1` as is.

## 2. What the IIgs core has

* `rtl/smartport_dev.v`: a partial device on the **real SmartPort bus** (the
  IWM disk port, where a physical FujiApple plugs in). Packet receive and send,
  groups-of-7 encoding and checksums are there; the block I/O ports are tied
  off in `rtl/iwm_woz.v`, and the bus is deliberately made to look empty
  during the ROM's SmartPort scan.
* The MiSTer `UART_*` pins go to the SCC (OSD: Modem slot 2 / Printer slot 1).
* Uthernet II (`rtl/uthernet2.sv`) is a slot card enabled from the OSD
  (Off / Slot 3 / Slot 4) that reaches the network through DDRAM and Main.
  It's the pattern for a new card plus an ARM-side helper.

## 3. The core side: a FujiNet slot card

Both transports below use the same card, as AppleWin does:

* OSD: FujiNet card Off / Slot 4 / Slot 7 (Slot 7 shares with the HDD card
  only if the HDD is off).
* **Slot ROM** (`$Cn00` page plus a `$C800` expansion ROM if needed):
  ProDOS block-device signature and entry, SmartPort dispatch (standard
  calls, extended later), boot code. The ROM marshals parameters and buffers
  to and from the card's registers in ordinary 8-bit code. Writing and
  debugging this driver is the largest single piece of work.
* **Registers:** request write port, response read port, status/flags.
* **Back end:** a UART with SLIP framing (option A) or a DDRAM mailbox
  (option B).

What it gives the user:

* ProDOS 8: FujiNet CONFIG boots, FujiNet disks mount and run, and fujinet-lib
  network programs work.
* GS/OS: FujiNet disks should appear through GS/OS's generated driver for
  SmartPort slot cards. FujiNet networking from GS/OS needs a GS/OS driver;
  whether one exists hasn't been checked.

The real-bus alternative (finish `smartport_dev.v` so FujiNet answers on the
IWM port and the ROM's slot 5 firmware drives it) is more authentic but needs
the full BSY/REQ handshake and multi-unit ID assignment inside the IWM code,
which has broken WOZ regressions before. It is not recommended for now.

## 4. Option A: the serial link

```
 IIgs core (FPGA)                 ARM HPS (Linux)
 FujiNet slot card --UART_*-->  /dev/ttyS1  fujinet (APPLE, SLIP_PROTOCOL=COM)
   ROM + SLIP + UART                         started by uartmode 8 (launcher)
```

* **Firmware:** upstream plus one small patch in `patches/`: take the port
  and baud from `fnconfig.ini`'s `[Serial]` section (like
  `0002-adam-serial-port-and-baud-from-config.patch`), 8N1, no flow
  control. About 20 lines; worth upstreaming. The Docker cross-build also
  needs libserialport.
* **Launcher:** add `Apple-IIgs` (and `Apple-II`) → `apple` in the mode-8
  launcher (`scripts/uartmode.fujinet`).
* **Core:** OSD switch giving the UART to either the SCC or the FujiNet card.
  Add faster speeds to the core's `CONF_STR` UART list (the core sets it).
  115,200 baud is about 11 KB/s, or 45 ms per block; 460,800 to 921,600 baud
  should be close to a real FujiNet, but the HPS side at those speeds needs
  confirming.
* **Sim:** `--serial-pty` like the CoCo 3 sim, with FujiNet-PC built natively
  on the Mac against the pty (long read timeout for sim speed).
* **Pros:** reuses the CoCo/ADAM setup (uartmode 8, launcher); no new ARM
  program. **Cons:** a firmware patch, serial speed limit, and the SCC
  serial port can't be used at the same time.

## 5. Option B: TCP and a bridge (stock FujiNet)

```
 IIgs core (FPGA)              ARM HPS (Linux)
 FujiNet slot card  <-->  bridge  <--TCP 127.0.0.1:1985-->  fujinet (stock APPLE, NET)
   ROM + mailbox      (listens; links devrelay/slip/SLIP.cpp)
```

* **Firmware:** stock upstream; `fnconfig.ini` points the BOIP host at
  127.0.0.1.
* **Bridge:** a small new Linux program (or code in Main) that listens, accepts
  FujiNet's connection, adds and strips SLIP framing, and moves each packet to
  and from the core.
* **Bridge to core:** a **DDRAM mailbox**, as Uthernet II uses: request
  buffer, response buffer, a few flags. Very fast (a block in microseconds),
  and both SCC ports stay free. The bridge needs DDRAM through `/dev/mem`, or
  lives in Main beside the Uthernet support. (Bridging over the UART instead
  would just be option A with an extra hop.)
* **Launcher:** start `fujinet` and the bridge together for the IIgs.
* **Sim:** `sim_main.cpp` listens on the port and acts as the bridge, so the
  stock Mac FujiNet-PC connects exactly as it does to AppleWin.
* **Pros:** no firmware patch, memory speed, serial ports untouched, simpler
  card back end (no UART/SLIP in the FPGA). **Cons:** a new bridge program
  (or a Main change) to write and ship.

## 6. Comparison

| | A: serial | B: TCP + bridge |
|---|---|---|
| FujiNet firmware | upstream + ~20-line patch | stock |
| New ARM software | launcher entry only | the bridge |
| Speed | ~11 KB/s at 115,200; ~4x at 460,800 | memory speed |
| SCC serial port | shared via an OSD switch | untouched |
| Core back end | UART + SLIP | mailbox registers |
| Sim | pty (CoCo pattern) | TCP listener in `sim_main` |

Leaning: **B** for the finished product. A is quicker to a first light
because the CoCo/ADAM infrastructure exists.

## 7. Suggested first steps

1. Build FujiNet-PC APPLE natively on the Mac (stock, `SLIP_PROTOCOL=NET`).
2. In the IIgs vsim, add a TCP listener that speaks SLIP and a minimal card
   (registers plus a stub ROM that issues one Status and one ReadBlock).
3. Get both round trips through the real FujiNet-PC, then grow the ROM into
   a ProDOS/SmartPort driver and boot FujiNet CONFIG in the sim.
4. Hardware: the bridge on the ARM, the DDRAM mailbox, the launcher entry.
