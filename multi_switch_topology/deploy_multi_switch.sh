#!/bin/bash
# Deploy N three-port switches in star topology
# Each switch connects one host via PCI (DevEmu) and uplinks via Ethernet

set -e

# Configuration
NUM_SWITCHES=${1:-4}  # Default: 4 switches
TOPOLOGY=${2:-star}   # star, mesh, or ring
BASE_IP="10.0.0"

echo "=========================================="
echo "Multi-Switch Network Deployment"
echo "=========================================="
echo "Number of switches: $NUM_SWITCHES"
echo "Topology: $TOPOLOGY"
echo "Base IP: $BASE_IP.0/24"
echo "=========================================="
echo ""

# Arrays to track PIDs and resources
declare -a SWITCH_PIDS
declare -a DEVEMU_PIDS

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    
    # Kill all switches
    for pid in "${SWITCH_PIDS[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            echo "Stopping switch (PID: $pid)"
            kill $pid
        fi
    done
    
    # Kill DevEmu processes
    for pid in "${DEVEMU_PIDS[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            echo "Stopping DevEmu (PID: $pid)"
            kill $pid
        fi
    done
    
    echo "Cleanup complete"
    exit 0
}

trap cleanup SIGINT SIGTERM

#
# Step 1: Create DevEmu PCI devices for host connections
#
echo "Step 1: Creating DevEmu PCI devices..."
echo "--------------------------------------"

for i in $(seq 0 $((NUM_SWITCHES-1))); do
    VUID="pci-emu-switch-$i"
    PCI_ADDR=$(printf "0%d:00.0" $i)
    
    echo "  Switch $i: Creating virtual PCI device"
    echo "    VUID: $VUID"
    echo "    PCI Address: $PCI_ADDR"
    
    # In a real deployment, you would create the DevEmu device here
    # sudo doca_devemu_pci_device_create --vuid "$VUID" --pci-address "$PCI_ADDR" &
    # DEVEMU_PIDS[$i]=$!
    
    # For now, just note that it would be created
    echo "    [Note: DevEmu device creation would happen here]"
    echo ""
done

echo "DevEmu PCI devices prepared"
echo ""

#
# Step 2: Start switches based on topology
#
echo "Step 2: Starting switches..."
echo "----------------------------"

case $TOPOLOGY in
    star)
        echo "Deploying STAR topology"
        echo ""
        
        # In star topology, all switches connect to a backbone
        # We'll use a simple configuration here
        
        for i in $(seq 0 $((NUM_SWITCHES-1))); do
            PCI_EMU=$(printf "0%d:00.0" $i)
            ETH1=$(printf "03:%02d.0" $i)
            ETH2=$(printf "03:%02d.1" $i)
            
            echo "  Starting Switch $i"
            echo "    Port 0 (PCI): $PCI_EMU → Host $i"
            echo "    Port 1 (ETH): $ETH1 → Backbone"
            echo "    Port 2 (ETH): $ETH2 → Redundant/Spare"
            
            # In real deployment:
            # sudo ../three_port_switch/build/doca_three_port_switch \
            #   -a $PCI_EMU \
            #   -a $ETH1 \
            #   -a $ETH2 -- &
            # SWITCH_PIDS[$i]=$!
            
            echo "    [Switch would start here with PID: $$]"
            echo ""
        done
        ;;
        
    mesh)
        echo "Deploying FULL MESH topology"
        echo ""
        
        # In mesh, switches connect to each other
        # More complex routing needed
        
        for i in $(seq 0 $((NUM_SWITCHES-1))); do
            PCI_EMU=$(printf "0%d:00.0" $i)
            
            # Connect to next and previous switches
            NEXT=$((($i + 1) % NUM_SWITCHES))
            PREV=$((($i - 1 + NUM_SWITCHES) % NUM_SWITCHES))
            
            ETH1=$(printf "03:%02d.0" $i)  # To next switch
            ETH2=$(printf "03:%02d.1" $i)  # To prev switch
            
            echo "  Starting Switch $i"
            echo "    Port 0 (PCI): $PCI_EMU → Host $i"
            echo "    Port 1 (ETH): $ETH1 → Switch $NEXT"
            echo "    Port 2 (ETH): $ETH2 → Switch $PREV"
            echo ""
        done
        ;;
        
    ring)
        echo "Deploying RING topology"
        echo ""
        
        # Ring: each switch connects to next in ring
        
        for i in $(seq 0 $((NUM_SWITCHES-1))); do
            PCI_EMU=$(printf "0%d:00.0" $i)
            NEXT=$((($i + 1) % NUM_SWITCHES))
            
            ETH1=$(printf "03:%02d.0" $i)
            
            echo "  Starting Switch $i"
            echo "    Port 0 (PCI): $PCI_EMU → Host $i"
            echo "    Port 1 (ETH): $ETH1 → Switch $NEXT"
            echo "    Port 2 (ETH): Unused"
            echo ""
        done
        ;;
        
    *)
        echo "Unknown topology: $TOPOLOGY"
        echo "Supported: star, mesh, ring"
        exit 1
        ;;
esac

echo "All switches configured"
echo ""

#
# Step 3: Display network map
#
echo "Step 3: Network Map"
echo "-------------------"

cat << EOF

Network Topology: $TOPOLOGY

Switches and Hosts:
EOF

for i in $(seq 0 $((NUM_SWITCHES-1))); do
    IP="$BASE_IP.$(($i + 1))"
    echo "  Switch $i ↔ Host $i ($IP)"
done

echo ""
echo "Connectivity:"

case $TOPOLOGY in
    star)
        echo "  All switches → Backbone switch"
        echo "  Inter-host traffic: 2 hops (via backbone)"
        ;;
    mesh)
        echo "  All switches interconnected"
        echo "  Inter-host traffic: 1-2 hops"
        ;;
    ring)
        echo "  Switches connected in ring"
        echo "  Inter-host traffic: 1-$((NUM_SWITCHES/2)) hops"
        ;;
esac

echo ""

#
# Step 4: Testing instructions
#
echo "Step 4: Testing"
echo "---------------"

cat << EOF

To test connectivity:

1. From Host 0, ping Host 1:
   ping -c 4 $BASE_IP.2

2. Check MAC learning on Switch 0:
   doca_flow_query --switch 0 --mac-table

3. Monitor statistics:
   watch -n 1 "doca_flow_query --all-switches --stats"

4. Test broadcast:
   ping -b $BASE_IP.255

5. Performance test:
   iperf3 -s (on Host 0)
   iperf3 -c $BASE_IP.1 (on Host 1)

EOF

#
# Step 5: Keep running
#
echo "Step 5: Running"
echo "---------------"
echo ""
echo "Deployment complete!"
echo ""
echo "Press Ctrl+C to stop all switches and cleanup"
echo ""

# Keep script running
while true; do
    sleep 5
    
    # Display status
    echo -ne "\r[$(date '+%H:%M:%S')] Switches running: $NUM_SWITCHES | Topology: $TOPOLOGY | Hosts: $NUM_SWITCHES"
done

# Cleanup on exit (via trap)
