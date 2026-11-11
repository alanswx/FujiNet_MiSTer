#!/bin/sh
# Assemble dist/fujinet-studio2: the RS232 FujiNet-PC package plus the cart daemon.
#   ../build-docker.sh RS232 && make CROSS=... OUT=s2cartd-arm (see README.md), then ./package.sh
set -e
here=$(cd "$(dirname "$0")" && pwd)
src=$here/../dist/fujinet-rs232
out=$here/../dist/fujinet-studio2
[ -x "$src/fujinet" ] || { echo "no $src/fujinet: ../build-docker.sh RS232" >&2; exit 1; }
[ -x "$here/s2cartd-arm" ] || { echo "no s2cartd-arm" >&2; exit 1; }
rm -rf "$out"
cp -R "$src" "$out"
cp "$here/s2cartd-arm" "$out/s2cartd"
rm -f "$out/start_fujinet.sh" "$out/run-fujinet"
cp "$here/launch" "$here/fnconfig.ini" "$out/"
mkdir -p "$out/SD"
echo "$out: copy to /media/fat/fujinet/Studio-II/"
