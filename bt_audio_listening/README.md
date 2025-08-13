# bt_audio_listening

This directory contains a script to automatically bridge Xiegu x6200 radio sound to Bluetooth audio devices such as headphones, earphones, speakers, etc.

Tested with Xiegu X6200 v1.0.6 and Bluetooth headset Shokz Openswim Pro.

A quick system idle check shows that the GStreamer bridge consumes very little CPU, making it suitable for continuous operation without noticeable performance impact. However, this remains a workaround while waiting for an official feature implementation. Use at your own risk.

## Overview

The setup includes:

- **`/usr/bin/bt_audio_control.sh`** – a script for starting or stopping a GStreamer audio bridge.

The script can be launched manually or automatically at startup after proper configuration  of the radio's operating system.

