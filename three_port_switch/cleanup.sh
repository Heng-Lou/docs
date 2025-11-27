#!/bin/bash
# Cleanup all switches and veth topology

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}=== Cleaning Up DOCA veth Topology ===${NC}"
echo ""

# Stop all switches
echo "Stopping DOCA switches..."
sudo pkill -9 -f three_port_switch_veth 2>/dev/null || true
sudo pkill -9 iperf3 2>/dev/null || true
sleep 1

# Clean DPDK runtime
echo "Cleaning DPDK runtime..."
sudo rm -rf /var/run/dpdk/switch* 2>/dev/null || true

# Cleanup veth topology
echo "Cleaning up veth interfaces and namespaces..."
if [ -f ./setup_veth_topology.sh ]; then
    ./setup_veth_topology.sh cleanup 2>/dev/null || true
else
    echo "Warning: setup_veth_topology.sh not found"
fi

# Force reset terminal settings
stty sane 2>/dev/null || true

echo ""
echo -e "${GREEN}Cleanup complete!${NC}"

# Ensure terminal echo is back on
stty echo 2>/dev/null || true