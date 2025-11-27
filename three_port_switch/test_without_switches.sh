#!/bin/bash
# filepath: /home/heng/workdir/doca/three_port_switch/test_without_switches.sh

echo "=== Testing connectivity WITHOUT switches ==="
echo ""

# Stop switches
sudo pkill -9 three_port_switch_veth
sleep 1

# Create a simple Linux bridge
echo "Creating Linux bridge to verify basic connectivity..."
sudo ip link add br0 type bridge
sudo ip link set br0 up

# Add all veth interfaces to bridge
sudo ip link set veth_s1_h1 master br0
sudo ip link set veth_s1_s2 master br0
sudo ip link set veth_s2_s1 master br0
sudo ip link set veth_s2_h2 master br0
sudo ip link set veth_s2_s3 master br0
sudo ip link set veth_s3_s2 master br0
sudo ip link set veth_s3_h3 master br0

echo "Testing ping through Linux bridge..."
echo -n "ns1 -> ns2: "
if sudo ip netns exec ns1 ping -c 1 -W 2 10.0.2.2 > /dev/null 2>&1; then
    echo "OK"
else
    echo "FAIL"
fi

echo -n "ns1 -> ns3: "
if sudo ip netns exec ns1 ping -c 1 -W 2 10.0.3.2 > /dev/null 2>&1; then
    echo "OK"
else
    echo "FAIL"
fi

# Clean up bridge
echo ""
echo "Removing bridge..."
sudo ip link set veth_s1_h1 nomaster
sudo ip link set veth_s1_s2 nomaster
sudo ip link set veth_s2_s1 nomaster
sudo ip link set veth_s2_h2 nomaster
sudo ip link set veth_s2_s3 nomaster
sudo ip link set veth_s3_s2 nomaster
sudo ip link set veth_s3_h3 nomaster
sudo ip link del br0

echo "Bridge removed. Now test with switches again."

