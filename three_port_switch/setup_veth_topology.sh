#!/bin/bash
# filepath: /home/heng/workdir/doca/three_port_switch/setup_veth_topology.sh

# Accept number of switches as argument (default 3, max 10)
NUM_SWITCHES=${1:-3}
TOPOLOGY=${2:-line}  # line or ring

if [ "$NUM_SWITCHES" -lt 2 ] || [ "$NUM_SWITCHES" -gt 10 ]; then
    echo "Error: Number of switches must be between 2 and 10"
    echo "Usage: $0 <num_switches> <topology>"
    echo "  num_switches: 2-10 (default: 3)"
    echo "  topology: line or ring (default: line)"
    exit 1
fi

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}=== Setting up $NUM_SWITCHES-Switch $TOPOLOGY Topology ===${NC}"
echo ""

# Cleanup function
cleanup() {
    echo "Cleaning up existing setup..."
    
    # Delete all namespaces
    for i in $(seq 1 10); do
        sudo ip netns del ns$i 2>/dev/null || true
    done
    
    # Delete all veth pairs
    for i in $(seq 1 10); do
        sudo ip link del veth_s${i}_h${i} 2>/dev/null || true
        for j in $(seq 1 10); do
            sudo ip link del veth_s${i}_s${j} 2>/dev/null || true
        done
    done
    
    echo "Cleanup complete"
}

if [ "$1" == "cleanup" ]; then
    cleanup
    exit 0
fi

# Cleanup before setup
cleanup

echo "Creating namespaces and veth pairs..."

# Create namespaces and host links for all switches
for i in $(seq 1 $NUM_SWITCHES); do
    # Create namespace
    sudo ip netns add ns$i
    
    # Create veth pair for host connection
    VETH_SW="veth_s${i}_h${i}"
    VETH_NS="veth_h${i}_s${i}"
    
    sudo ip link add $VETH_SW type veth peer name $VETH_NS
    sudo ip link set $VETH_NS netns ns$i
    
    # Configure host interface in namespace
    sudo ip netns exec ns$i ip link set lo up
    sudo ip netns exec ns$i ip link set $VETH_NS up
    sudo ip netns exec ns$i ip addr add 10.0.${i}.2/16 dev $VETH_NS
    
    # Configure switch-side interface
    sudo ip link set $VETH_SW up
    sudo ip link set $VETH_SW promisc on
    
    echo -e "  ${GREEN}✓${NC} Created ns$i with host link $VETH_SW"
done

echo ""
echo "Creating switch-to-switch links..."

# Create switch interconnect links based on topology
if [ "$TOPOLOGY" == "line" ]; then
    # Line topology: S1 -- S2 -- S3 -- ... -- SN
    for i in $(seq 1 $((NUM_SWITCHES - 1))); do
        j=$((i + 1))
        VETH_A="veth_s${i}_s${j}"
        VETH_B="veth_s${j}_s${i}"
        
        sudo ip link add $VETH_A type veth peer name $VETH_B
        sudo ip link set $VETH_A up
        sudo ip link set $VETH_B up
        sudo ip link set $VETH_A promisc on
        sudo ip link set $VETH_B promisc on
        
        echo -e "  ${GREEN}✓${NC} Connected Switch $i <-> Switch $j"
    done
    
elif [ "$TOPOLOGY" == "ring" ]; then
    # Ring topology: S1 -- S2 -- S3 -- ... -- SN -- S1
    for i in $(seq 1 $NUM_SWITCHES); do
        j=$(( (i % NUM_SWITCHES) + 1 ))
        
        # Only create if i < j to avoid duplicates (except for the wrap-around)
        if [ $i -lt $j ] || [ $i -eq $NUM_SWITCHES ]; then
            VETH_A="veth_s${i}_s${j}"
            VETH_B="veth_s${j}_s${i}"
            
            sudo ip link add $VETH_A type veth peer name $VETH_B
            sudo ip link set $VETH_A up
            sudo ip link set $VETH_B up
            sudo ip link set $VETH_A promisc on
            sudo ip link set $VETH_B promisc on
            
            echo -e "  ${GREEN}✓${NC} Connected Switch $i <-> Switch $j"
        fi
    done
fi

echo ""
echo -e "${GREEN}=== Topology Setup Complete! ===${NC}"
echo ""
echo "Created $NUM_SWITCHES switches in $TOPOLOGY topology:"
for i in $(seq 1 $NUM_SWITCHES); do
    echo "  • Switch $i (ns$i): 10.0.${i}.2/16"
done

echo ""
echo "Verify setup with:"
echo -e "  ${YELLOW}sudo ip netns list${NC}"
echo -e "  ${YELLOW}ip link show | grep veth${NC}"
echo ""
echo "Next steps:"
echo -e "  ${YELLOW}./setup_static_arp.sh${NC}"
echo -e "  ${YELLOW}./generate_mac_tables.sh $NUM_SWITCHES $TOPOLOGY${NC}"
echo -e "  ${YELLOW}./build_veth.sh${NC}"
echo -e "  ${YELLOW}./deploy_switches.sh $NUM_SWITCHES $TOPOLOGY${NC}"