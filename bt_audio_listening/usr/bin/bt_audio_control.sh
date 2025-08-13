#!/bin/bash
PIDFILE="/tmp/bt_audio.pid"
GST_BIN="/usr/bin/gst-launch-1.0"

log() {
    echo "[BT-AUDIO] $1" > /dev/kmsg
}

log "Script de surveillance A2DP démarré"
echo "[BT-AUDIO] Starting A2DP watching script" > /dev/kmsg

pactl subscribe | while read -r line; do
    if echo "$line" | grep -q "Event 'new' on sink" && ! echo "$line" | grep -q "sink-input"; then
        sink=$(pactl list short sinks | grep "bluez_sink.*a2dp_sink" | head -n1)
        if [ -n "$sink" ]; then
            mac=$(echo "$sink" | awk -F'[.]' '{print $2}')
            
            if [ -f "$PIDFILE" ] && kill -0 $(cat "$PIDFILE") 2>/dev/null; then
                echo "[BT-AUDIO] Pipeline already running" > /dev/kmsg
            else
                echo "[BT-AUDIO] Launching gst pipeline with bt device $mac" > /dev/kmsg
                setsid $GST_BIN \
                    alsasrc device=mixcapture \
                    ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
                    ! audioconvert \
                    ! audioresample \
                    ! pulsesink device=bluez_sink.$mac.a2dp_sink sync=false \
                    >/dev/null 2>&1 </dev/null &
                echo $! > "$PIDFILE"
            fi
        fi
    fi
    if echo "$line" | grep -q "Event 'remove' on sink" && ! echo "$line" | grep -q "sink-input"; then
        if [ -n "$PIDFILE" ]; then
            echo "[BT-AUDIO] Device removed - stopping pipeline" > /dev/kmsg 
            PID=$(cat "$PIDFILE")
            kill $PID 2>/dev/null || true
            rm -f "$PIDFILE"
        fi
    fi
done
