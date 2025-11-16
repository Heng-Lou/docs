#!/bin/bash
# Build script for DPA kernel launch sample

set -e

# Setup environment
export PKG_CONFIG_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu:/opt/mellanox/dpdk/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"
export PATH="/opt/mellanox/doca/tools:$PATH"

echo "Building DPA Kernel Launch Sample..."
echo "======================================"
echo "This demonstrates DOCA Programmable Acceleration (DPA)"
echo "DPA is NVIDIA's programmable data path for BlueField"
echo ""
echo "Using DPACC compiler for device code"
echo ""

# Create build directory
rm -rf build
meson setup build

# Build
ninja -C build

echo ""
echo "Build successful!"
echo "Binary location: build/doca_dpa_kernel_launch"
echo ""
echo "This application demonstrates:"
echo "  - DPA kernel programming for BlueField"
echo "  - Custom data path acceleration"
echo "  - Device-side code execution"
echo "  - Host-device interaction"
echo ""
echo "To run (requires BlueField-3 DPU hardware):"
echo "  sudo build/doca_dpa_kernel_launch -pf_dev <DEVICE_NAME>"
echo ""
