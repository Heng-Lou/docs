#!/bin/bash
# filepath: /home/heng/workdir/doca/three_port_switch/disable_ipv6.sh

NUM_SWITCHES=${1:-3}

echo "Disabling IPv6 in all namespaces..."

for i in $(seq 1 $NUM_SWITCHES); do
    echo "Disabling IPv6 in ns$i..."
    sudo ip netns exec ns$i sysctl -w net.ipv6.conf.all.disable_ipv6=1
    sudo ip netns exec ns$i sysctl -w net.ipv6.conf.default.disable_ipv6=1
    sudo ip netns exec ns$i sysctl -w net.ipv6.conf.lo.disable_ipv6=1
    sudo ip netns exec ns$i sysctl -w net.ipv6.conf.veth_h${i}_s${i}.disable_ipv6=1
done

# Also disable on host side
echo "Disabling IPv6 on host veth interfaces..."
for i in $(seq 1 $NUM_SWITCHES); do
    sudo sysctl -w net.ipv6.conf.veth_s${i}_h${i}.disable_ipv6=1 2>/dev/null || true
done

for i in $(seq 1 $((NUM_SWITCHES - 1))); do
    j=$((i + 1))
    sudo sysctl -w net.ipv6.conf.veth_s${i}_s${j}.disable_ipv6=1 2>/dev/null || true
    sudo sysctl -w net.ipv6.conf.veth_s${j}_s${i}.disable_ipv6=1 2>/dev/null || true
done

echo "IPv6 disabled!"