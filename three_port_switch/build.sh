#!/bin/bash
# Build script for Three-Port Switch

set -e

# Setup environment
export PKG_CONFIG_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu:/opt/mellanox/dpdk/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

echo "========================================================"
echo " Building Three-Port Switch with DevEmu"
echo "========================================================"
echo " This is a software switch with 3 ports:"
echo "   - Port 0: PCI (emulated with DevEmu)"
echo "   - Port 1: Ethernet 0"
echo "   - Port 2: Ethernet 1"
echo ""
echo " Features:"
echo "   - MAC learning table"
echo "   - Packet forwarding between ports"
echo "   - DOCA Flow hardware offload"
echo "   - Statistics tracking"
echo ""

# Create build directory
rm -rf build
meson setup build

# Build
ninja -C build

echo ""
echo "========================================================"
echo " Build successful!"
echo "========================================================"
echo " Binary: build/doca_three_port_switch"
echo ""
echo " To run (requires at least 2 DPDK-compatible devices):"
echo "   sudo ./build/doca_three_port_switch -a <PCI_0> -a <PCI_1> -a <PCI_2> -- "
echo ""
echo " Example:"
echo "   sudo ./build/doca_three_port_switch \\"
echo "     -a 03:00.0,representor=pf0vf0 \\"
echo "     -a 03:00.1,representor=pf1vf0 \\"
echo "     -a 03:00.2,representor=pf2vf0 -- "
echo ""
echo " Note: Port 0 can be an emulated PCI device"
echo "========================================================"
echo ""
