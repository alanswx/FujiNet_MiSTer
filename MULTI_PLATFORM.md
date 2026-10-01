# FujiNet on MiSTer: status, what we learned, and a multi-platform plan

This document records how FujiNet was brought up on the MiSTer CoCo 3 core, the
bugs that stood in the way, and a plan for supporting FujiNet on other MiSTer
cores (ADAM, Apple II, Atari, Lynx, PCs, ...). It is written for whoever picks
this up next, so it names repos, branches, files and exact behaviour.

---

## 1. How the pieces fit

```
 CoCo 3 core (FPGA)                         ARM HPS (Linux)
 +----------------------+   UART_TXD/RXD    +------------------------------+
 | 6809 + HDB-DOS cart  |------------------>| /dev/ttyS1                   |
 | RS-232 Pak (6551)    |<------------------| fujinet (FujiNet-PC, COCO)   |
 | at $FF68-$FF6B       |   UART_CTS<-RTS   |  started by /sbin/uartmode 8 |
 +----------------------+                   +------------------------------+
            ^                                        ^
            |  sys_top.v wires the core's UART_*     |  Main_MiSTer writes
            |  pins straight to the HPS UART,        |  /tmp/CORENAME and runs
            |  whatever the OSD UART mode is         |  "uartmode <mode>"
```

* **The core** exposes a serial port on its `UART_*` pins. `sys_top.v` connects
  those pins directly to the HPS UART (`/dev/ttyS1`); the OSD UART mode does not
  gate them.
* **The OSD UART mode** only decides which Linux service Main starts on
  `/dev/ttyS1`, via `/sbin/uartmode <n>`. Mode 7 is the printer daemon, mode 8
  is FujiNet.
* **FujiNet** is the upstream firmware's POSIX build ("FujiNet-PC"), compiled
  for one platform (see section 4). On the CoCo it speaks DriveWire.
* **Flow control:** the core's 6551 only transmits while its CTS input is low,
  and `sys_top.v` drives CTS from the HPS UART's RTS. FujiNet-PC turns off
  `CRTSCTS` and leaves RTS as Linux sets it on open (asserted), so this holds
  while FujiNet has the port open.

---

## 2. CoCo 3: status

**Working on hardware.** With the fixed core, the HDB-DOS SY6551 carts boot the
FujiNet CONFIG program from the MiSTer's own FujiNet daemon. From there,
disks mount into drive slots and networked programs (e.g. `weather.dsk`) run.

### Setup

1. Core: a CoCo 3 build with the fixes in section 3.
2. OSD → UART mode → **FujiNet**. Use **115,200** for `HDBDW3S3.CCC` or
   **19,200** for `HDBDW3L3_19200.CCC`. This needs a Main with mode 8 (section 5).
3. OSD → Multi-Pak → **(Slot 3) ECB / Cart**.
4. OSD → Load Cartridge → the HDB-DOS cart. It cold-boots straight into FujiNet
   CONFIG; no reset is needed.

### Using the drive slots

* DriveWire drive *N* is FujiNet device slot *N*+1 (`drivewire.cpp`,
  `get_disk(drive_num)`), four slots in all:

  | FujiNet slot | HDB-DOS drive |
  |---|---|
  | 1 | 0 |
  | 2 | 1 |
  | 3 | 2 |
  | 4 | 3 |

* **Whatever is in slot 1 autoboots** when you leave CONFIG with BREAK. CONFIG
  "mounts all", then HDB-DOS boots drive 0.
  * If slot 1 holds `autorun.dsk` (the CONFIG disk), CONFIG simply reloads.
    It looks like FujiNet restarting, but it's CONFIG booting itself.
  * Leave slot 1 empty to land at `OK` and use `DIR 1`, `RUN"X:2"` and so on.
* Reset brings CONFIG back (`boot_mode=0` serves CONFIG as drive 0 at boot).

---

## 3. What was wrong, and the fixes

All found and verified with the Verilator simulation of the core (section 6),
then confirmed on hardware.

### 3.1 CoCo 3 core (repo `CoCo3_MiSTer`)

| # | Symptom | Cause | Fix |
|---|---|---|---|
| 1 | FujiNet never saw a byte. The screen stopped at `HDB-DOS 1.5 SY6551 COCO 3`, and `fujinet.log` recorded no DriveWire ops (the Sep 25 attempt ran 12 minutes). | `coco3fpga.sv` decoded `$FF68-$FF6B` only when the Multi-Pak I/O select was **slot 1**, which the OSD cannot select. HDB-DOS polled the 6551 status forever and read `$AA` from an empty bus. | Decode the RS-232 Pak in **any slot**, as on a real CoCo, where SCS covers only `$FF40-$FF5F`. |
| 2 | The 19,200 cart failed every sector checksum (`expected 65280, got 57344`). | `HDBDW3L3_19200.CCC` writes control `$0F`; bit 4 = 0 selects the "external" receive clock, which `uart_6551.v` tied to 115,200 baud. It transmitted at 19,200 and received at 115,200. | The receiver is clocked from the baud generator (`RX_CLK = TX_CLK`). |
| 3 | In slot 3, CONFIG hung after `CFGLOAD.BIN` (17 sector reads). A manual reset "fixed" it. | After a cart loads post-boot, slot 3 toggled the CART interrupt every CPU cycle forever. That's right for a program pak (CART tied to Q), but HDB-DOS is a disk ROM that never drives CART, and CFGLOAD re-enables that interrupt. The OSD option "Cart Interrupt Disabled" was not connected to anything. | A cart starting with `DK` gets a **cold boot** (the existing AMW/EE_Cold_Bt path) instead of the autostart interrupt. "Cart Interrupt Disabled" now works. Program paks are unchanged (tested with Arkanoid). |
| 4 | Current master shows **no video** on hardware (OSD fine). | The Dec 2025 sys update changed placement and exposed a latent bug. `video_mixer` samples `ce_pix` on the rising edge of `clk_sys`, but the core fed it the raw `CLK_14`, which changes on that same edge (both PLL outputs, phase 0). The scaler measured 43- or 243-pixel lines instead of 763 (black or garbage); one 2024 placement happened to work. | The core outputs **`PIX_CE`**, a one-cycle `clk_sys` enable registered with the pixel data, and `CoCo3.sv` feeds that to the mixer. Correct 763×254 video on the latest sys with two different fitter seeds. |

Supporting changes made along the way:
* the latest Template_MiSTer `sys/`;
* `CoCo3.sv` includes `sys/emu_ports.vh` (the new sys narrowed `HPS_BUS`);
* SDC clock groups for the pseudo-clocks (`img_mounted[*]`, `RESET_N`, `NMISample2`) and false paths for static OSD bits (worst 57 MHz slack went from −11 to about −5 ns);
* a video-fetch end-of-line compare moved off a failing 114 MHz path;
* two `wd1793.sv` declarations moved out of `generate` blocks (a Verilator requirement).

**Where the work is** (local; nothing pushed as of this writing):

| Checkout | Branch | Contents |
|---|---|---|
| `~/dev2/CoCo3_MiSTer` | `verilator-sim-fujinet-serial` | Verilator sim, fixes 1–3, plus fix 4 and the fetch change cherry-picked for testing |
| `~/dev2/CoCo3_MiSTer_sys` | `update-sys` | fresh master + latest sys + `emu_ports.vh` + SDC + fetch + fix 4 |

The plan is to rebase the serial/cart/sim commits onto `update-sys`, rebuild,
re-run the FujiNet hardware test, and open a PR. The sys/video part fixes
master for everyone and could go first on its own.

### 3.2 Main_MiSTer

The Main that was deployed (Mac/printer branch) did not know mode 8:
`GetUARTMode()` never checked `/tmp/uartmode8`, so the OSD showed "None" and
FujiNet could not be selected from the menu. It only started because a saved
mode 8 in `config/uartmode.COCO3` was passed straight to `/sbin/uartmode`.

Fixed on branch **`mac-printer-fujinet`** (commit `b4192cd`) in
`cottageubuntu:~/mister/Mac_Main_MiSTer`, on top of `mac-printer-writebuffer`:
* a "FujiNet" menu entry after "Printer", shown when `is_fujinet_available()`;
* `GetUARTMode()` returns 8;
* a saved mode 8 falls back to None when the daemon is missing;
* changing the baud in Printer/FujiNet mode restarts the service instead of
  sending it to MidiLink (modes 3–6 unchanged).

Deployed as `/media/fat/MiSTer`; the previous Main is saved as
`MiSTer.pre_fujinet_merge_20260928`. Not pushed.

### 3.3 `/sbin/uartmode` (on the MiSTer only)

Mode 8 was added by hand to `/sbin/uartmode` on the MiSTer (the original is
`/sbin/uartmode.bak`). **It is not in any repo, and a MiSTer Linux update will
overwrite it.** It should live in this repo (`scripts/`) with an installer. The
current version:

```sh
elif [ "$1" == "8" ]; then
	if [ ! -f /tmp/uartmode8 ]; then
		kill_all            # also gained: killall fujinet
		echo "1" >/tmp/uartmode8
		(
			while true; do
				if [ -x /media/fat/fujinet/fujinet ]; then
					sed -i "s/^baud=.*/baud=$conn_speed/" /media/fat/fujinet/fnconfig.ini
					cd /media/fat/fujinet && ./fujinet -c /media/fat/fujinet/fnconfig.ini \
						-s /media/fat/fujinet/SD >> /media/fat/fujinet/fujinet.log 2>&1
				fi
				[ ! -f /tmp/uartmode8 ] && exit 0
				sleep 1
			done
		) >/dev/null 2>&1 &
	fi
```

Note: Main runs `SetUARTMode(0)` then the saved mode on **every core load**,
so FujiNet restarts each time a core loads.

---

## 4. FujiNet variants

The platform is a **compile-time** choice: `FUJINET_TARGET` sets
`BUILD_<PLATFORM>`, and the bus code is `#ifdef`'d across dozens of files. One
binary speaks one machine's protocol, and the web UI and boot disks in `data/`
are per-platform too.

Only six targets build as **FujiNet-PC** (`fujinet_pc.cmake`), the version
that runs on the MiSTer's ARM: **ATARI, APPLE, COCO, ADAM, LYNX, RS232**.
Everything else exists only as ESP32 firmware and would have to be ported to
the PC build first.

The table lists every platform in `fujinet-firmware/build-platforms/`. The
MiSTer-core column is from general knowledge; entries marked *(?)* are
unverified.

| FujiNet variant (platform) | Bus | FujiNet-PC? | MiSTer core(s) | How we'd hook it up | Effort |
|---|---|---|---|---|---|
| **CoCo / Dragon** (`COCO`); also Fujiversal-DriveWire and Foenix-DW boards | DriveWire over a UART | ✅ | CoCo2, **CoCo3** | 6551 RS-232 Pak → `UART_*` → `/dev/ttyS1`. **Done.** | done |
| **Atari 8-bit** (`ATARI`) | SIO: serial plus COMMAND/motor lines | ✅ (serial SIO or NetSIO; check which suits a tty) | Atari800 | Route SIO data to `UART_TXD/RXD`, and SIO COMMAND to a modem-control line (`UART_DTR`/`RTS`), like an SIO2PC cable. | medium |
| **Apple II / IIgs** (`APPLE`) | IWM SmartPort; the PC build uses **SmartPort over SLIP** (serial `connector_com` or network `connector_net`) | ✅ | Apple-II, Apple-IIgs | A new block acting as the SmartPort relay: a SmartPort device on the Apple bus, SLIP-framed onto `UART_*`. The IIgs core's UART is its SCC serial port today, a different thing. | high |
| **Coleco ADAM** (`ADAM`) | AdamNet: raw bytes over a UART, or Bus-over-IP | ✅ | ColecoAdam | Bridge AdamNet to `UART_*`; the core's `UART_TXD` currently carries only the printer (mode 7). AdamNet has a ~300 µs response window (`adamnet.h`); the PC build has a slow-host bypass that needs testing through a Linux tty. | medium–high |
| **Atari Lynx** (`LYNX`) | ComLynx, already a serial UART | ✅ | AtariLynx | Wire the core's ComLynx port to `UART_*` at the Lynx baud rate. | **low** |
| **Generic RS-232** (`RS232`): MS-DOS, CP/M, any COM port | FujiNet RS-232 packets (command frame + checksum, `lib/bus/rs232`) | ✅ | ao486, PCXT, CP/M-capable cores *(?)* | The core's COM port → `UART_*`; the guest runs FujiNet's RS-232 DOS/CP/M drivers. | **low** |
| **Fujiversal: Astrocade, Intellivision, MSX, Odyssey²** (all `RS232`) | FujiNet RS-232 packets from a cartridge-side adapter | ✅ (RS232 target) | Astrocade, Intellivision, MSX, Odyssey² *(?)* | Emulate the Fujiversal cartridge adapter in the core, speaking the RS-232 protocol on `UART_*`. One FujiNet binary serves all four. | medium each |
| **Commodore IEC** (`IEC`): C64, VIC-20, C128, Plus/4 | IEC serial bus (ATN/CLK/DATA) | ❌ ESP32 only | C64, C128, VIC-20, C16 | Port IEC to FujiNet-PC, then bridge the core's IEC bus (already there for its 1541) to the UART. The timing is tight. | high |
| **Macintosh** (`MAC`, "FujiMac") | Mac floppy/HD20 bus | ❌ ESP32 only | MacPlus, MacIIvi/Quadra | Port to FujiNet-PC, then a floppy/HD20-to-UART bridge in the core. | high |
| **Commander X16** (`CX16`) | I²C | ❌ | none known | — | n/a |
| **Heathkit H89** (`H89`) | H89 bus | ❌ | none known | — | n/a |
| **RC2014** (`RC2014`) | RC2014 SPI/SIO | ❌ | none known | — | n/a |
| **S-100** (`S100`) | S-100 bus | ❌ | none known | — | n/a |
| **TI-99/4A** | — | ❌ not in this firmware | TI-99/4A | A FujiNet port would come first. | n/a |

**Suggested order:**
1. Lynx and generic RS-232 (ao486/PCXT): mostly wiring.
2. Atari 800.
3. ADAM.
4. The Apple SmartPort relay and the Fujiversal cartridge adapters (real FPGA work).
5. IEC and Mac need FujiNet-PC ports before any core work.

---

## 5. One launcher for every platform

We need one binary per platform, but a **single UART mode** can pick the right
one from the core name. Main already writes `/tmp/CORENAME` before it runs
`/sbin/uartmode`.

### Layout

Keep the program in one place, and point each platform's FujiNet **SD host at
that core's games folder**:

```
/media/fat/fujinet/
  coco/    fujinet  data/  fnconfig.ini     # SD host -> /media/fat/games/CoCo3
  adam/    fujinet  data/  fnconfig.ini     # SD host -> /media/fat/games/<ADAM folder>
  apple/   ...
  rs232/   ...                              # ao486, PCXT, Fujiversal consoles
  lynx/    ...
  fujinet.log
```

**Why not `games/<core>/fujinet/`:**
* It's a Linux service that `/sbin/uartmode` and Main's
  `is_fujinet_available()` need at one fixed path; games folder names differ
  per core.
* The OSD file browser (Load Cartridge / Load Disk) would show the binary,
  `data/` and the web UI files.
* One tree is simpler to install, update and remove, and stays out of the way
  of scripts that manage `games/`.

**Why point the SD host at the games folder:**
* Users' existing images (e.g. `games/CoCo3/*.dsk`) appear under host 1 in
  CONFIG with no copying.
* Each platform only sees its own images.

**Caution:** images mounted read/write are written in place. To protect a
collection, use a subfolder (e.g. `games/CoCo3/fujinet-disks/`) or mount
read-only.

The current install uses `/media/fat/fujinet/SD/` instead. That folder also
contains a stray copy of `autorun.dsk` (the CONFIG disk), which is how slot 1
ended up booting CONFIG again. FujiNet serves CONFIG from `data/`, not `SD/`.

### Launcher sketch (`/sbin/uartmode` mode 8)

```sh
case "$(cat /tmp/CORENAME)" in
	COCO3|COCO2)           plat=coco;  games=CoCo3 ;;
	ADAM)                  plat=adam;  games=Coleco ;;
	Apple-II*|Apple-IIgs)  plat=apple; games=Apple-II ;;
	ATARI800)              plat=atari; games=ATARI800 ;;
	AtariLynx)             plat=lynx;  games=AtariLynx ;;
	ao486|PCXT)            plat=rs232; games=AO486 ;;
	*)                     exit 0 ;;   # no FujiNet for this core
esac
dir=/media/fat/fujinet/$plat
[ -x $dir/fujinet ] || exit 0
sed -i "s/^baud=.*/baud=$conn_speed/" $dir/fnconfig.ini
cd $dir && ./fujinet -c fnconfig.ini -s /media/fat/games/$games >> ../fujinet.log 2>&1
```

The exact `CORENAME` strings and games folder names need checking on a real
MiSTer. `is_fujinet_available()` in Main should apply the same mapping, so the
menu offers FujiNet only for cores that have a binary.

---

## 6. How to develop a core-side bridge

The CoCo work used the core's **Verilator simulation** (`CoCo3_MiSTer/verilator`)
with the UART bridged to a **pty**, so the real FujiNet-PC binary talked to the
simulated machine:

* `--serial-pty` exposes the core's UART as `/dev/ttysNNN`.
* `--trace-acia` logs every 6551 register access and whether it decoded; this
  found bug 1.
* The TXD decoder prints each byte the core transmits; a warning flags
  receive-clock mismatches, which found bug 2.
* `--cart-frame N` loads a cart after boot the way the OSD does; this exposed
  bug 3.
* `tests/fujinet_boot.sh` runs the sim plus a FujiNet-PC build with
  `fnconfig.ini` pointed at the pty. `tests/dwserver.py` is a minimal DriveWire
  server for quick checks.

**Build FujiNet-PC natively** (macOS example):

```sh
brew install mbedtls@3
python3 -m venv venv && venv/bin/pip install jinja2 pyyaml
mkdir build && cd build
PATH=../venv/bin:$PATH cmake ../fujinet-firmware -DFUJINET_TARGET=COCO \
	-DCMAKE_BUILD_TYPE=Release -DMBEDTLS_ROOT_DIR=$(brew --prefix mbedtls@3)
PATH=../venv/bin:$PATH make -j dist      # -> build/dist/{fujinet,data/}
```

The firmware's build scripts call `python`; put a `python` → `python3` shim on
`PATH` if the system has none.

**Simulation caveat:** the sim runs ~50× slower than real time, and FujiNet
times out reads after 500 ms of wall-clock (`.readTimeout(500)` in
`drivewire.cpp`, per byte). For sim runs, build FujiNet with a large timeout
(e.g. 120000). On hardware the stock value is fine.

The same approach should carry over to the next platforms: add the bridge to
that core's Verilator sim, expose its UART as a pty, and run FujiNet-PC built
for that target against it before going to hardware.

---

## 7. Open items

* Move `/sbin/uartmode` mode 8 into this repo (`scripts/`) with an installer,
  and implement the per-core launcher (section 5).
* Update `README.md`:
  * UART mode **FujiNet** needs a Main with mode 8;
  * use Multi-Pak **slot 3**;
  * the baud must match the cart: 19,200 for L3, 115,200 for S3;
  * clear drive slot 1 unless you want it to autoboot.
* Remove the stray `/media/fat/fujinet/SD/autorun.dsk`, or re-point the SD host
  as in section 5.
* CoCo 3 core: combine the branches, open the PR(s), and report the master sys
  breakage and its fix (section 3.1 #4).
* Main: push `mac-printer-fujinet`. `is_fujinet_available()` should become
  per-core once the launcher exists.
