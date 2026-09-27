#!/bin/sh
# FujiNet CoCo / DriveWire Launcher for MiSTer HPS

BASEDIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyS1}"
BAUD="${2:-19200}"

# Kill any existing fujinet or printer daemon instances
killall -q fujinet mister_printerd 2>/dev/null

echo "Starting FujiNet on $PORT at $BAUD baud..."
if [ -n "$BAUD" ]; then
    sed -i "s/^baud=.*/baud=$BAUD/" "$BASEDIR/fnconfig.ini"
fi
if [ -n "$PORT" ]; then
    sed -i "s|^port=.*|port=$PORT|" "$BASEDIR/fnconfig.ini"
fi

cd "$BASEDIR"
exec ./fujinet -c "$BASEDIR/fnconfig.ini" -s "$BASEDIR/SD"
