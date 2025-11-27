#!/bin/bash

echo "Building three_port_switch_veth..."

gcc -O3 -g -Wall -Wextra $(pkg-config --cflags libdpdk) \
    -o three_port_switch_veth three_port_switch_veth.c \
    $(pkg-config --libs libdpdk)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    ls -lh three_port_switch_veth
else
    echo "Build failed!"
    exit 1
fi
