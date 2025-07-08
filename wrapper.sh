#!/bin/bash

# Ensure we're running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (use sudo)"
    exit 1
fi

# Check and mount essential filesystems
if [ ! -e /proc/self ]; then
    echo "Mounting /proc..."
    mount -t proc /proc /proc
fi

if [ ! -e /sys/class ]; then
    echo "Mounting /sys..."
    mount -t sysfs /sys /sys
fi

if [ ! -e /dev/null ]; then
    echo "Mounting /dev..."
    mount -t devtmpfs /dev /dev
fi

# Run the builder
/usr/local/bin/builder "$@"