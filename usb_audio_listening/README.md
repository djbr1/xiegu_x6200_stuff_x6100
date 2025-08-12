# usb_audio_listening

This directory contains scripts and configuration files for automatically receive radio sound in USB-C audio devices such as headphones, earphones, speakers, etc.

A quick system idle check shows that the GStreamer bridge consumes very little CPU, making it suitable for continuous operation without noticeable performance impact. However, this remains a workaround while waiting for an official feature implementation. Use at your own risk.

## Overview

The setup includes:

- **`/usr/bin/usb_audio_control.sh`** – a script for starting or stopping a GStreamer audio bridge.
- **`/etc/udev/rules.d/99-usbaudio-gst.rules`** – a udev rule to automatically run the script when a specific USB audio device is plugged in or unplugged.

When the USB device is connected, the script launches a GStreamer pipeline bridging audio from the configured **`pcm.mixcapture`** device (defined in `/etc/asound.conf`) to the USB audio device (`hw:1,0`).  
When the USB device is removed, the bridge is stopped.

## Setup

Make the script executable:
```
 sudo chmod +x /usr/bin/usb_audio_control.sh
```

Adjust the ATTR{idVendor} value in the rules file to match your device and reload udev:
```
sudo udevadm control --reload-rules
sudo udevadm trigger
```