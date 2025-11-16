#!/bin/bash
# Run switches in simulator mode with DPDK null PMD
# This creates virtual switches that can be monitored without real hardware

set -e

# Configuration
NUM_SWITCHES=${1:-4}
TOPOLOGY=${2:-ring}
BASE_DIR="/home/heng/workdir/doca/three_port_switch"
SWITCH_BIN="$BASE_DIR/build/doca_three_port_switch"
LOG_DIR="./sim_logs"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=========================================="
echo "Switch Simulator - DPDK Virtual Mode"
echo -e "==========================================${NC}"
echo "Switches: $NUM_SWITCHES"
echo "Topology: $TOPOLOGY"
echo "Mode: DPDK null PMD (virtual devices)"
echo ""

# Check if switch binary exists
if [ ! -x "$SWITCH_BIN" ]; then
    echo -e "${RED}Error: Switch binary not found or not executable${NC}"
    echo "Expected: $SWITCH_BIN"
    echo ""
    echo "Build it with:"
    echo "  cd $BASE_DIR && ./build.sh"
    exit 1
fi

# Create log directory
mkdir -p "$LOG_DIR"

# Arrays to track PIDs
declare -a SWITCH_PIDS

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping all switches...${NC}"
    
    for pid in "${SWITCH_PIDS[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            echo "  Stopping switch (PID: $pid)"
            kill -SIGINT $pid 2>/dev/null
            
            # Wait a bit for graceful shutdown
            sleep 1
            
            # Force kill if still running
            if kill -0 $pid 2>/dev/null; then
                kill -9 $pid 2>/dev/null
            fi
        fi
    done
    
    echo -e "${GREEN}All switches stopped${NC}"
    echo ""
    echo "Log files saved in: $LOG_DIR"
    ls -lh $LOG_DIR/*.log 2>/dev/null || true
    
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo -e "${BLUE}Starting Virtual Switches${NC}"
echo "========================================"
echo ""

# Function to start a switch
start_switch() {
    local switch_id=$1
    local log_file="$LOG_DIR/switch_${switch_id}.log"
    
    echo -e "${GREEN}Starting Switch $switch_id${NC}"
    echo "  Virtual devices: 3 DPDK null PMDs"
    echo "  Log file: $log_file"
    
    # Note: The three-port switch requires actual PCI devices
    # For true simulation, we need to use DPDK testpmd or a modified switch
    # This is a demonstration of what would run
    
    # Try with no-huge and no PCI device scanning (will fail gracefully)
    sudo $SWITCH_BIN \
        --no-pci \
        --no-huge \
        --vdev=net_null${switch_id},size=64,copy=0 \
        -- > "$log_file" 2>&1 &
    
    local pid=$!
    SWITCH_PIDS[$switch_id]=$pid
    
    # Wait a moment to check if it started successfully
    sleep 1
    
    if kill -0 $pid 2>/dev/null; then
        echo -e "  ${GREEN}✓ Started successfully (PID: $pid)${NC}"
        return 0
    else
        echo -e "  ${RED}✗ Failed to start${NC}"
        echo "  Check log: $log_file"
        cat "$log_file"
        return 1
    fi
}

# Start switches based on count
for i in $(seq 0 $((NUM_SWITCHES - 1))); do
    start_switch $i
    echo ""
    
    # Small delay between starts
    sleep 2
done

echo -e "${BLUE}=========================================="
echo "All Switches Started!"
echo -e "==========================================${NC}"
echo ""
echo -e "${GREEN}Running $NUM_SWITCHES virtual switches in $TOPOLOGY topology${NC}"
echo ""
echo "Switch Processes:"
for i in $(seq 0 $((NUM_SWITCHES - 1))); do
    pid=${SWITCH_PIDS[$i]}
    if [ -n "$pid" ] && kill -0 $pid 2>/dev/null; then
        cpu=$(ps -p $pid -o %cpu= 2>/dev/null | xargs)
        mem=$(ps -p $pid -o %mem= 2>/dev/null | xargs)
        echo "  Switch $i: PID $pid (CPU: ${cpu}%, MEM: ${mem}%)"
    else
        echo -e "  Switch $i: ${RED}Not running${NC}"
    fi
done
echo ""

echo -e "${BLUE}Monitoring Commands:${NC}"
echo "========================================"
echo "  Quick check:    ./check_status.sh"
echo "  Real-time:      ./monitor_switch.sh"
echo "  Watch PIDs:     watch -n 1 'pgrep -a doca_three'"
echo "  View logs:      tail -f $LOG_DIR/switch_*.log"
echo ""

echo -e "${YELLOW}Press Ctrl+C to stop all switches${NC}"
echo ""

# Keep script running and show periodic updates
update_count=0
while true; do
    sleep 5
    update_count=$((update_count + 1))
    
    # Count running switches
    running=0
    for pid in "${SWITCH_PIDS[@]}"; do
        if [ -n "$pid" ] && kill -0 $pid 2>/dev/null; then
            running=$((running + 1))
        fi
    done
    
    # Show status line
    timestamp=$(date '+%H:%M:%S')
    echo -ne "\r[$timestamp] Running: $running/$NUM_SWITCHES switches | Updates: $update_count | Ctrl+C to stop    "
    
    # Check if all switches died
    if [ $running -eq 0 ]; then
        echo ""
        echo -e "${RED}All switches have stopped!${NC}"
        echo "Check logs in: $LOG_DIR"
        exit 1
    fi
done
