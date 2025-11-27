#!/bin/bash

echo "=== Setting up Static ARP Entries ==="

# Detect number of namespaces
NUM_NS=$(sudo ip netns list | grep -c "^ns[0-9]")

if [ $NUM_NS -eq 0 ]; then
    echo "Error: No namespaces found!"
    exit 1
fi

echo "Found $NUM_NS namespaces"

# Get MAC addresses for all namespaces
echo ""
echo "Getting MAC addresses..."
declare -A MACS
declare -A IPS

for i in $(seq 1 $NUM_NS); do
    MAC=$(sudo ip netns exec ns$i ip link show veth_h${i}_s${i} | grep ether | awk '{print $2}')
    IP="10.0.$i.2"
    MACS[$i]=$MAC
    IPS[$i]=$IP
    echo "  ns$i (${IP}): ${MAC}"
done

# Clear existing ARP entries in all namespaces
echo ""
echo "Clearing existing ARP entries..."
for i in $(seq 1 $NUM_NS); do
    sudo ip netns exec ns$i ip neigh flush all 2>/dev/null
done

# Set up ARP entries in each namespace for all other namespaces
echo ""
for i in $(seq 1 $NUM_NS); do
    echo "Setting up ARP entries in ns$i..."
    for j in $(seq 1 $NUM_NS); do
        if [ $i -ne $j ]; then
            # Add static ARP entry for ns$j in ns$i
            sudo ip netns exec ns$i arp -s ${IPS[$j]} ${MACS[$j]} 2>/dev/null
        fi
    done
done

echo ""
echo "✓ Static ARP entries configured for $NUM_NS namespaces"

# Show ARP tables
echo ""
echo "=== ARP Tables ==="
for i in $(seq 1 $NUM_NS); do
    echo "ns$i:"
    sudo ip netns exec ns$i arp -n | grep -v "incomplete"
    echo ""
done

echo "Static ARP setup complete!"
echo ""
echo "Benefits:"
echo "  ✓ No ARP broadcasts (less flooding)"
echo "  ✓ Faster first ping (no ARP delay)"
echo "  ✓ Lower CPU on switches"