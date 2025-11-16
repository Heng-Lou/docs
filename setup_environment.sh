#!/bin/bash
#
# DOCA Development Environment Setup
# Source this file to configure your environment for DOCA development
#

# Set PKG_CONFIG_PATH to include DOCA libraries
export PKG_CONFIG_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH"

# Add DOCA libraries to LD_LIBRARY_PATH
export LD_LIBRARY_PATH="/opt/mellanox/doca/lib/x86_64-linux-gnu:/opt/mellanox/dpdk/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

# Add DOCA tools to PATH
export PATH="/opt/mellanox/doca/tools:/opt/mellanox/doca/bin:$PATH"

# Set DOCA installation path
export DOCA_HOME="/opt/mellanox/doca"

# Configure hugepages (requires sudo)
configure_hugepages() {
    echo "Configuring hugepages for DPDK..."
    sudo sh -c 'echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages'
    echo "Hugepages configured: $(cat /proc/meminfo | grep HugePages_Total)"
}

# Verify DOCA installation
verify_doca() {
    echo "DOCA Environment Configuration"
    echo "=============================="
    echo "DOCA Version: $(cat $DOCA_HOME/VERSION 2>/dev/null || echo 'Not found')"
    echo "DOCA Home: $DOCA_HOME"
    echo ""
    
    if pkg-config --exists doca-flow; then
        echo "✓ DOCA Flow library found"
        echo "  Version: $(pkg-config --modversion doca-flow)"
    else
        echo "✗ DOCA Flow library not found in pkg-config"
    fi
    
    if pkg-config --exists libdpdk; then
        echo "✓ DPDK library found"
        echo "  Version: $(pkg-config --modversion libdpdk)"
    else
        echo "✗ DPDK library not found in pkg-config"
    fi
    
    echo ""
    echo "Library paths configured:"
    echo "  PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
    echo "  LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
}

# Run verification
verify_doca

echo ""
echo "Environment configured!"
echo "Run 'configure_hugepages' to set up hugepages for DPDK"
echo ""
