#!/bin/bash
PIDFILE="/tmp/bt_audio.pid"
GST_BIN="/usr/bin/gst-launch-1.0"

echo "[BT-AUDIO] Starting A2DP watching script" > /dev/kmsg
sleep 1

pactl subscribe | while read -r line; do
    if [[ "$line" =~ Event\ \'new\'\ on\ sink && ! "$line" =~ sink-input ]]; then
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
    if [[ "$line" =~ Event\ \'remove\'\ on\ sink && ! "$line" =~ sink-input ]]; then
        if [ -n "$PIDFILE" ]; then
            echo "[BT-AUDIO] Device removed - stopping pipeline" > /dev/kmsg 
            PID=$(cat "$PIDFILE")
            kill $PID 2>/dev/null || true
            rm -f "$PIDFILE"
        fi
    fi
done
