#!/bin/bash

# DPA GDB Debug Setup Script for Three-Port Switch
# This script prepares the debugging environment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="/home/heng/workdir/doca/three_port_switch"

echo "=========================================="
echo "DPA GDB Debug Environment Setup"
echo "=========================================="
echo

# Check if three_port_switch exists
if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Three-port switch directory not found"
    exit 1
fi

cd "$PROJECT_DIR"

# Step 1: Clean and rebuild with debug symbols
echo "Step 1: Building with debug symbols..."
echo "----------------------------------------"
rm -rf build
meson setup build -Dbuildtype=debug
ninja -C build

if [ $? -eq 0 ]; then
    echo "✓ Build successful with debug symbols"
else
    echo "✗ Build failed"
    exit 1
fi
echo

# Step 2: Create GDB command file
echo "Step 2: Creating GDB command file..."
echo "----------------------------------------"
cat > build/debug_commands.gdb << 'EOF'
# GDB Commands for Three-Port Switch Debugging

# Set source directory
directory /home/heng/workdir/doca/three_port_switch

# Connect to remote (when using dpa-gdbserver)
# Uncomment when GDB server is running:
# target remote localhost:1981

# Common breakpoints
# break main
# break process_packet
# break forward_packet

# Custom print functions
define print_stats
    printf "Port Statistics:\n"
    printf "Port 0: RX=%lu TX=%lu\n", port_stats[0].rx_packets, port_stats[0].tx_packets
    printf "Port 1: RX=%lu TX=%lu\n", port_stats[1].rx_packets, port_stats[1].tx_packets
    printf "Port 2: RX=%lu TX=%lu\n", port_stats[2].rx_packets, port_stats[2].tx_packets
end

# Auto-display settings
# set print pretty on
# set print array on

# Logging
set logging file debug_session.log
set logging on

# Ready message
echo \n
echo ========================================\n
echo GDB Ready for Three-Port Switch Debug\n
echo ========================================\n
echo \n
echo Commands:\n
echo   break main          - Break at main\n
echo   run                 - Start program\n
echo   print_stats         - Show port stats\n
echo   help                - GDB help\n
echo \n
EOF

echo "✓ GDB command file created: build/debug_commands.gdb"
echo

# Step 3: Create debug start script
echo "Step 3: Creating debug launcher..."
echo "----------------------------------------"
cat > build/start_debug.sh << 'EOF'
#!/bin/bash

# Three-Port Switch Debug Launcher

PROJECT_DIR="/home/heng/workdir/doca/three_port_switch"
cd "$PROJECT_DIR"

echo "=========================================="
echo "Starting Three-Port Switch Debugger"
echo "=========================================="
echo
echo "Debugging Options:"
echo "  1. Host-side debugging (available now)"
echo "  2. DPA debugging (needs dpa-gdbserver)"
echo
read -p "Select option (1 or 2): " option

if [ "$option" == "1" ]; then
    echo
    echo "Starting GDB for host-side debugging..."
    echo "Note: This debugs the host control code"
    echo
    gdb -x build/debug_commands.gdb build/doca_three_port_switch
    
elif [ "$option" == "2" ]; then
    echo
    echo "DPA Debugging requires:"
    echo "  1. dpa-gdbserver running"
    echo "  2. BlueField hardware or emulation"
    echo
    read -p "Is dpa-gdbserver running on localhost:1981? (y/n): " confirm
    
    if [ "$confirm" == "y" ]; then
        echo "Connecting to dpa-gdbserver..."
        dpa-gdb -x build/debug_commands.gdb build/doca_three_port_switch
    else
        echo
        echo "Please start dpa-gdbserver first:"
        echo "  sudo dpa-gdbserver -s 1981 -T debug_session"
        echo
    fi
else
    echo "Invalid option"
    exit 1
fi
EOF

chmod +x build/start_debug.sh
echo "✓ Debug launcher created: build/start_debug.sh"
echo

# Step 4: Create GDB server starter (for when hardware is available)
echo "Step 4: Creating GDB server script..."
echo "----------------------------------------"
cat > build/start_gdbserver.sh << 'EOF'
#!/bin/bash

# Start DPA GDB Server
# Use this when you have BlueField hardware or emulation

echo "=========================================="
echo "Starting DPA GDB Server"
echo "=========================================="
echo

# Check if dpa-gdbserver is available
if ! command -v dpa-gdbserver &> /dev/null; then
    echo "Error: dpa-gdbserver not found"
    echo "Make sure DOCA tools are in PATH"
    exit 1
fi

# Check for devices
echo "Checking for DPA devices..."
if [ -d /sys/class/infiniband ]; then
    DEVICES=$(ls /sys/class/infiniband/ 2>/dev/null)
    if [ -n "$DEVICES" ]; then
        echo "Found devices: $DEVICES"
        DEVICE=$(echo $DEVICES | awk '{print $1}')
        echo "Using device: $DEVICE"
    else
        echo "Warning: No InfiniBand devices found"
        echo "Will try to start server anyway (may work with emulation)"
        DEVICE=""
    fi
else
    echo "Warning: No /sys/class/infiniband directory"
    echo "Will try to start server anyway (may work with emulation)"
    DEVICE=""
fi

echo
echo "Starting GDB server..."
echo "Port: 1981"
echo "Session: debug_session"
echo

if [ -n "$DEVICE" ]; then
    sudo dpa-gdbserver "$DEVICE" -s 1981 -T debug_session
else
    sudo dpa-gdbserver -s 1981 -T debug_session
fi
EOF

chmod +x build/start_gdbserver.sh
echo "✓ GDB server script created: build/start_gdbserver.sh"
echo

# Step 5: Summary
echo "=========================================="
echo "Setup Complete!"
echo "=========================================="
echo
echo "Debug Environment Ready:"
echo "  ✓ Built with debug symbols (-g -O0)"
echo "  ✓ GDB command file created"
echo "  ✓ Debug launcher created"
echo "  ✓ GDB server script created"
echo
echo "Usage:"
echo "  1. Start debugging:"
echo "     cd $PROJECT_DIR"
echo "     ./build/start_debug.sh"
echo
echo "  2. Manual GDB (host-side):"
echo "     gdb -x build/debug_commands.gdb build/doca_three_port_switch"
echo
echo "  3. DPA GDB (when hardware available):"
echo "     Terminal 1: ./build/start_gdbserver.sh"
echo "     Terminal 2: ./build/start_debug.sh (option 2)"
echo
echo "Files created:"
echo "  - build/debug_commands.gdb  (GDB commands)"
echo "  - build/start_debug.sh      (Debug launcher)"
echo "  - build/start_gdbserver.sh  (GDB server starter)"
echo
echo "Next: Run './build/start_debug.sh' to begin debugging"
echo "=========================================="
