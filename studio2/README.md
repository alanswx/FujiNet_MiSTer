# FujiNet for the StudioII core

The RCA Studio II FujiNet cart (fujinet-firmware `pico/studio2`) is two chips: an RP2354B
serving the console's bus, and an ESP32-S3 running FujiNet. On the MiSTer:

| Real cart | MiSTer |
|---|---|
| RP2354B core1: every bus read (`s2_bus_read`) | StudioII core, `rtl/fujinet/fujinet_cart.sv` (branch `fujinet`) |
| RP2354B core0: mailbox, text engine, image staging | `s2cartd` (this folder), on the HPS, over the core's UART |
| ESP32-S3: FujiNet (`fujiversal-studio2` = RS232) | FujiNet-PC `FUJINET_TARGET=RS232`, BoIP on 127.0.0.1 |

`s2cartd` compiles the firmware's own C (fujimail, fujibus, s2map, fuji_load, s2_text, the
baked CONFIG) as the MAME device does, and keeps the FPGA's tables, image buffers, arena and
raster in step with its view.

## Build

```sh
make                                   # host build, for the Verilator sim
../build-docker.sh RS232               # FujiNet-PC for the MiSTer -> ../dist/fujinet-rs232
docker run --rm -v "$PWD/..:/work" -w /work/studio2 fujinet-mister-build \
  make CROSS=/opt/mister-toolchain/gcc-arm/bin/arm-none-linux-gnueabihf- OUT=s2cartd-arm
./package.sh                           # -> ../dist/fujinet-studio2
```

## Install

- `../dist/fujinet-studio2/` to `/media/fat/fujinet/Studio-II/` (the core's name: Main and uartmode look for it there)
- `../scripts/uartmode.fujinet` to `/sbin/uartmode` (it runs `Studio-II/launch` for the Studio-II core)
- Core: OSD System > FujiNet Cartridge On; Main's UART menu: mode FujiNet, 1562500

## Test without a MiSTer

```sh
cd test && ./fujinet -c $PWD/fnconfig.ini -s $PWD/SD -u http://127.0.0.1:8017 &   # BoIP 9996
cd ~/dev2/StudioII_MiSTer/verilator && make headless
./obj_dir_headless/Vtop --bios studio2.rom --fujinet-pty /tmp/pty --frames 4000 --shot-every 250 ...
./s2cartd -d $(cat /tmp/pty) -t 127.0.0.1:9996 -v
```
