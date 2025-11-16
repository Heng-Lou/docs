#!/bin/bash
# Build script for simple_fwd_vnf application

set -e

# Setup environment
export PKG_CONFIG_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu:/opt/mellanox/dpdk/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

echo "Building Simple Forward VNF Application..."
echo "==========================================="
echo "This is a production-grade BlueField DPU application"
echo ""

# Create build directory
rm -rf build
meson setup build

# Build
ninja -C build

echo ""
echo "Build successful!"
echo "Binary location: build/doca_simple_fwd_vnf"
echo ""
echo "This application demonstrates:"
echo "  - High-performance packet forwarding on BlueField DPU"
echo "  - Virtual Network Function (VNF) implementation"
echo "  - DOCA Flow integration for hardware offload"
echo "  - Multi-queue packet processing"
echo ""
echo "To run (requires BlueField DPU hardware):"
echo "  sudo build/doca_simple_fwd_vnf -a <PCI_ADDR_0> -a <PCI_ADDR_1> ..."
echo ""
