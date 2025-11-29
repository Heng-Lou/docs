#!/bin/bash
# Quick QoS validation test

echo "=== Quick QoS Test ==="

# Ensure switches are running
if ! pgrep -f three_port_switch_veth_qos > /dev/null; then
    echo "❌ Switches not running. Start them first with:"
    echo "   ./deploy_switches.sh 3 line"
    exit 1
fi

echo "Sending traffic with different DSCP markings..."
echo ""

# Send 20 packets per priority
echo "Priority Queue | DSCP | Packets"
echo "---------------|------|--------"
echo "EF (Highest)   | 46   | 20"
sudo ip netns exec ns1 ping -c 20 -i 0.05 -Q 0xb8 10.0.3.2 >/dev/null 2>&1 &

echo "AF4x           | 34   | 20"
sudo ip netns exec ns1 ping -c 20 -i 0.05 -Q 0x88 10.0.3.2 >/dev/null 2>&1 &

echo "AF1x           | 10   | 20"
sudo ip netns exec ns1 ping -c 20 -i 0.05 -Q 0x28 10.0.3.2 >/dev/null 2>&1 &

echo "Best Effort    | 0    | 20"
sudo ip netns exec ns1 ping -c 20 -i 0.05 10.0.3.2 >/dev/null 2>&1 &

wait

echo ""
echo "Waiting for statistics update (10 seconds)..."
sleep 12

echo ""
echo "=== Switch 2 QoS Statistics ==="
tail -35 logs/sw2.log | grep -A 18 "QoS Statistics" | head -20

echo ""
echo "✓ Test complete. Check logs/ directory for detailed statistics"
