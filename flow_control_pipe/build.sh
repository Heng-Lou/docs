#!/bin/bash
# Build script for flow_control_pipe sample

set -e

# Setup environment
export PKG_CONFIG_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu:/opt/mellanox/dpdk/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

echo "Building flow_control_pipe sample..."
echo "======================================"

# Create build directory
rm -rf build
meson setup build

# Build
ninja -C build

echo ""
echo "Build successful!"
echo "Binary location: build/doca_flow_control_pipe"
echo ""
echo "To run (requires BlueField DPU or supported hardware):"
echo "  sudo build/doca_flow_control_pipe -a <PCI_ADDR> ..."
echo ""
