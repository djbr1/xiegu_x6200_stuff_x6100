# usb_audio_full_duplex
This folder contains gstreamer commands and binaries with their sources for listening and transmitting audio through the radio's ‘host’ port. Works in SSB audio, U-DIG, and L-DIG.

## Method using gstreamer command
```
gst-launch-1.0 -e \
  alsasrc device=hw:1,0 ! audio/x-raw,format=S16LE,rate=48000,channels=1 \
    ! audioconvert ! audioresample ! audio/x-raw,channels=2 \
    ! volume volume=2.0 \
    ! queue ! alsasink device=mixout sync=false buffer-time=40000 latency-time=20000 \
  alsasrc device=mixcapture ! audio/x-raw,format=S16LE,rate=48000,channels=2 \
    ! audioconvert ! audioresample ! queue \
    ! alsasink device=hw:1,0 sync=false buffer-time=40000 latency-time=20000
```

## Method by calling the gstreamer API in C with Buildroot cross-compilation
The first method causes feedback in the headphones. To counter this, the listening volume must be reduced when using the microphone, which requires the compilation of a custom application.

The repository of [gdyuldin/AetherX6200Buildroot](https://github.com/gdyuldin/AetherX6200Buildroot) is used to configure the buildroot, which allows cross-compilation of our cmake project. Buildroot is also used to compile usefull packages for the radio.

### Custom application with peak detection:
[This application](BIN_FOR_TARGET/SWITCH_PEAK) activates the microphone only when voice is detected above a threshold of -30dB. 
It requires the addition of the `Level:GStreamer Good Plugins 1.0 Plugins` to the radio, which is [precompiled](LIB_FOR_TARGET/libgstlevel.so) and needs to be dropped in the right dir `/usr/lib/gstreamer-1.0/libgstlevel.so`.
 
### Custom application with PTT detection:
[This application](BIN_FOR_TARGET/SWITCH_PTT) activates the microphone only when PTT is pressed, with an event detection `/dev/input/event0`.
