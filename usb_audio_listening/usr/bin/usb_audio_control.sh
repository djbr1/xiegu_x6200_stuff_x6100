#!/bin/sh
# usb_audio_control.sh
# This script starts or stops a GStreamer ALSA audio pipeline.
# Usage:
#   ./usb_audio_control.sh start  -> start the pipeline
#   ./usb_audio_control.sh stop   -> stop the pipeline

PIDFILE="/tmp/gst_usb_audio.pid"
GST_BIN="/usr/bin/gst-launch-1.0"   # Adjust if 'which gst-launch-1.0' returns a different path

case "$1" in
    start)
        # Check if pipeline is already running
        if [ -f "$PIDFILE" ] && kill -0 $(cat "$PIDFILE") 2>/dev/null; then
            echo "[USB-AUDIO] Pipeline already running" > /dev/kmsg
            exit 0
        fi
        
        echo "[USB-AUDIO] Detected device - starting pipeline" > /dev/kmsg 
        
        # Start pipeline in background, detached from udev (setsid)
        setsid $GST_BIN \
            alsasrc device=mixcapture \
            ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
            ! audioconvert \
            ! audioresample \
            ! alsasink device=hw:1,0 sync=false buffer-time=40000 latency-time=20000 \
            >/dev/null 2>&1 </dev/null &

	# Set headset volume
        PCM=$(amixer -c 1 | grep "Simple mixer control" | head -n1 | cut -d"'" -f2)
        amixer -c 1 sset $PCM 40%

        # Save PID of the background process
        echo $! > "$PIDFILE"
        ;;

    stop)
        if [ -f "$PIDFILE" ]; then
            echo "[USB-AUDIO] Device removed - stopping pipeline" > /dev/kmsg 
            PID=$(cat "$PIDFILE")
            kill $PID 2>/dev/null || true
            rm -f "$PIDFILE"
        fi
        ;;

    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0
