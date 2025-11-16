#!/bin/bash

# GDB Debug Demonstration for Three-Port Switch
# This script shows how to use GDB to debug DPA-style programs

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "GDB Debugging Demo - Three-Port Switch"
echo "=========================================="
echo

# Check if simulation is built
if [ ! -f "switch_sim" ]; then
    echo "Building simulation..."
    make -f Makefile.debug
    echo
fi

echo "This demo shows how to debug your three-port switch"
echo "using GDB. The same techniques apply to DPA programs."
echo
echo "Available demo modes:"
echo "  1. Interactive GDB session"
echo "  2. Automated demo (shows breakpoints, stepping, etc.)"
echo "  3. Watch statistics demo"
echo "  4. Conditional breakpoint demo"
echo

read -p "Select demo mode (1-4): " mode

case $mode in
    1)
        echo
        echo "Starting interactive GDB..."
        echo "GDB will load with breakpoints pre-set."
        echo
        gdb -x gdb_commands.gdb ./switch_sim
        ;;
    
    2)
        echo
        echo "=== Automated GDB Demo ==="
        echo
        echo "This will run GDB with automated commands to show:"
        echo "  - Setting breakpoints"
        echo "  - Running the program"
        echo "  - Stepping through code"
        echo "  - Examining variables"
        echo
        
        cat > /tmp/gdb_demo.gdb << 'EOF'
# Automated GDB demo

# Load custom commands
source gdb_commands.gdb

# Run until first breakpoint
run test

# Show where we stopped
echo \n=== STOPPED AT FIRST BREAKPOINT ===\n
backtrace

# Show packet details
echo \n=== PACKET DETAILS ===\n
print pkt->port_id
print pkt->size

# Show statistics before processing
echo \n=== STATS BEFORE PROCESSING ===\n
show_all_stats

# Step through a few lines
echo \n=== STEPPING THROUGH CODE ===\n
step
step
step

# Continue to next breakpoint
echo \n=== CONTINUING TO NEXT BREAKPOINT ===\n
continue

# Show updated statistics
echo \n=== STATS AFTER PROCESSING ===\n
show_all_stats

# Quit
quit
EOF
        
        gdb -batch -x /tmp/gdb_demo.gdb ./switch_sim
        rm /tmp/gdb_demo.gdb
        ;;
    
    3)
        echo
        echo "=== Watch Statistics Demo ==="
        echo
        echo "This demo uses watchpoints to break when statistics change."
        echo
        
        cat > /tmp/gdb_watch.gdb << 'EOF'
source gdb_commands.gdb

# Set watchpoint on RX packet counter
watch port_stats[0].rx_packets

# Run
run test

# When it stops, show why
echo \n=== WATCHPOINT HIT ===\n
echo RX packets changed!\n
print port_stats[0].rx_packets
backtrace

# Continue to next change
continue

echo \n=== WATCHPOINT HIT AGAIN ===\n
print port_stats[0].rx_packets

quit
EOF
        
        gdb -batch -x /tmp/gdb_watch.gdb ./switch_sim 2>&1 | head -100
        rm /tmp/gdb_watch.gdb
        ;;
    
    4)
        echo
        echo "=== Conditional Breakpoint Demo ==="
        echo
        echo "This demo sets a breakpoint that only triggers for specific conditions."
        echo
        
        cat > /tmp/gdb_cond.gdb << 'EOF'
source gdb_commands.gdb

# Clear previous breakpoints
delete

# Set conditional breakpoint - only break for packets from port 1
break process_packet if pkt->port_id == 1

echo \n=== Breakpoint set: process_packet if pkt->port_id == 1 ===\n

run test

echo \n=== STOPPED (packet from port 1) ===\n
print pkt->port_id
print pkt->size

# Continue (will skip packets from other ports)
continue

quit
EOF
        
        gdb -batch -x /tmp/gdb_cond.gdb ./switch_sim
        rm /tmp/gdb_cond.gdb
        ;;
    
    *)
        echo "Invalid option"
        exit 1
        ;;
esac

echo
echo "=========================================="
echo "Demo Complete"
echo "=========================================="
echo
echo "To manually debug:"
echo "  gdb -x gdb_commands.gdb ./switch_sim"
echo
echo "To apply same techniques to actual three-port switch:"
echo "  1. Build with: meson setup build -Dbuildtype=debug"
echo "  2. Debug with: gdb build/doca_three_port_switch"
echo "  3. For DPA debugging: use dpa-gdb with dpa-gdbserver"
echo
echo "See DEBUG_QUICK_START.md for more details."
echo
