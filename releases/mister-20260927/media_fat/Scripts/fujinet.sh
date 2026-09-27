#!/bin/bash
# FujiNet Start / Stop helper for MiSTer

if pidof fujinet >/dev/null; then
    echo "Stopping FujiNet..."
    killall fujinet
    echo "FujiNet stopped."
else
    echo "Starting FujiNet..."
    /media/fat/fujinet/start_fujinet.sh /dev/ttyS1 19200 >/media/fat/fujinet/fujinet.log 2>&1 &
    sleep 2
    if pidof fujinet >/dev/null; then
        echo "FujiNet started successfully!"
        echo "Web UI: http://$(hostname).local:8000/"
    else
        echo "Error: FujiNet failed to start. Check /media/fat/fujinet/fujinet.log"
    fi
fi
