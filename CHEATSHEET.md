# Xiegu X6200 Cheatsheet

##  PulseAudio sound server
### Lists audio sinks:
```
pactl list short sinks
```

## Alsa
### Lists Alsa audio devices:
```
aplay -l
```

## GStreamer
### Pipes the internal audio from the radio to the hw1 USB headset device:
```
gst-launch-1.0 \
    alsasrc device=mixcapture \
    ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
    ! audioconvert \
    ! audioresample \
    ! alsasink device=hw:1,0 sync=false buffer-time=40000 latency-time=20000
```

### Pipes the internal audio from the radio to the bluetooth headset:
```
gst-launch-1.0 \
    alsasrc device=mixcapture \
    ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
    ! audioconvert \
    ! audioresample \
    ! pulsesink device=bluez_sink.XX_XX_XX_XX_XX_XX.a2dp_sink sync=false
```

### Records the internal audio from the radio in a file called mix.wav:
```
gst-launch-1.0 \
    alsasrc device=mixcapture \
    ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
    ! wavenc \
    ! filesink location=mix.wav
```

### Plays recorded file to the hw1 USB headset device:
```
gst-launch-1.0 \
    filesrc location=mic.wav \
    ! wavparse \
    ! alsasink device=hw:1,0
```


### Records usb headset microphone in a file called mix.wav:
```
gst-launch-1.0 -e \
    alsasrc device=hw:1,0 \
    ! audio/x-raw,format=S16LE,rate=48000,channels=1 \
    ! audioconvert \
    ! audioresample \
    ! audio/x-raw,channels=2 \
    ! wavenc \
    ! filesink location=mic.wav
```


### Pipes the audio from the radio to the hw1 headset and direct the hw1 microphone stream to the radio.
```
gst-launch-1.0 -e \
  alsasrc device=hw:1,0 ! audio/x-raw,format=S16LE,rate=48000,channels=1 \
    ! audioconvert ! audioresample ! audio/x-raw,channels=2 \
    ! volume volume=4.0 \
    ! queue ! alsasink device=mixout sync=false buffer-time=40000 latency-time=20000 \
  alsasrc device=mixcapture ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
    ! audioconvert ! audioresample ! queue \
    ! alsasink device=hw:1,0 sync=false buffer-time=40000 latency-time=20000
```