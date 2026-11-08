#!/bin/sh
# FujiNet CoCo / DriveWire Launcher for MiSTer HPS
#
#   start_fujinet.sh [PORT] [BAUD] [SD_DIR]
#
# SD_DIR is the folder FujiNet serves as its "SD" host (host slot 1), e.g.
# /media/fat/games/CoCo3 to see the disk images already on the card. The
# default is the SD/ folder next to this script. PORT and BAUD are taken from
# fnconfig.ini ([Serial]); the arguments are only echoed.

BASEDIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyS1}"
BAUD="${2:-19200}"
SDDIR="${3:-$BASEDIR/SD}"

# Kill any existing fujinet or printer daemon instances
killall -q fujinet mister_printerd 2>/dev/null

echo "Starting FujiNet for CoCo / DriveWire on $PORT at $BAUD baud, SD host: $SDDIR"
cd "$BASEDIR"
exec ./fujinet -c "$BASEDIR/fnconfig.ini" -s "$SDDIR"
