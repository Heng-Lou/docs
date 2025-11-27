#!/bin/bash

NUM_SWITCHES=${1:-3}
TOPOLOGY=${2:-line}

if [ "$NUM_SWITCHES" -lt 2 ] || [ "$NUM_SWITCHES" -gt 10 ]; then
    echo "Error: Number of switches must be between 2 and 10"
    echo "Usage: $0 <num_switches> <topology>"
    exit 1
fi

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}=== Deploying $NUM_SWITCHES DOCA Switches ===${NC}"
echo "  Topology: $TOPOLOGY"
echo ""

# Create logs directory
mkdir -p logs

# Stop existing switches
echo "Stopping any existing switches..."
sudo pkill -9 three_port_switch_veth 2>/dev/null || true
sleep 1

# Start each switch
for i in $(seq 1 $NUM_SWITCHES); do
    echo "Starting switch $i..."
    
    # Start switch in background
    nohup sudo ./three_port_switch_veth \
        --switch-id $i \
        --topology $TOPOLOGY \
        --num-switches $NUM_SWITCHES \
        > logs/switch_${i}.log 2>&1 &
    
    sleep 1
    
    # Verify it started
    if pgrep -f "switch-id $i" > /dev/null; then
        echo -e "  ${GREEN}✓${NC} Switch $i started (PID: $!)"
    else
        echo -e "  ✗ Switch $i failed to start (check logs/switch_${i}.log)"
    fi
done

echo ""
echo -e "${GREEN}=== All Switches Started ===${NC}"
echo ""
echo "Switch processes:"
ps aux | grep three_port_switch_veth | grep -v grep
echo ""
echo "Log files:"
ls -1 logs/switch_*.log
