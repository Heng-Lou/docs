#!/bin/bash

echo "=== QoS Differentiation Test ==="
echo "This test demonstrates priority-based packet scheduling"
echo ""

# Clean start
sudo pkill -9 -f three_port_switch
./cleanup.sh
./setup_veth_topology.sh 3 line
./disable_ipv6.sh 3 >/dev/null 2>&1
./setup_static_arp.sh >/dev/null 2>&1
./generate_mac_tables.sh 3 line >/dev/null 2>&1

sleep 2

# Start switches
echo "Starting switches..."
sudo ./three_port_switch_veth_qos -s 1 -t line -n 3 2>&1 | tee logs/sw1.log &
sleep 2
sudo ./three_port_switch_veth_qos -s 2 -t line -n 3 2>&1 | tee logs/sw2.log &
sleep 2
sudo ./three_port_switch_veth_qos -s 3 -t line -n 3 2>&1 | tee logs/sw3.log &
sleep 3

echo "✓ Switches ready"
echo ""

# Test 1: Low load - all priorities work
echo "=== Test 1: Normal Load (all priorities get through) ==="
echo "Sending 100 packets per priority level..."

sudo ip netns exec ns1 ping -c 100 -i 0.01 10.0.3.2 >/dev/null 2>&1 &         # Queue 0
sudo ip netns exec ns1 ping -c 100 -i 0.01 -Q 0x28 10.0.3.2 >/dev/null 2>&1 & # Queue 3 (AF1x)
sudo ip netns exec ns1 ping -c 100 -i 0.01 -Q 0x88 10.0.3.2 >/dev/null 2>&1 & # Queue 6 (AF4x)
sudo ip netns exec ns1 ping -c 100 -i 0.01 -Q 0xb8 10.0.3.2 >/dev/null 2>&1 & # Queue 7 (EF)

wait
sleep 12

echo ""
echo "Results from Switch 2:"
tail -30 logs/sw2.log | grep -A 15 "QoS Statistics"
echo ""

# Test 2: High load - show queue drops
echo "=== Test 2: High Load (demonstrates queue overflow) ==="
echo "Flooding with 500 packets per priority..."

sudo ip netns exec ns1 ping -c 500 -f 10.0.3.2 >/dev/null 2>&1 &         # Queue 0
sudo ip netns exec ns1 ping -c 500 -f -Q 0x28 10.0.3.2 >/dev/null 2>&1 & # Queue 3
sudo ip netns exec ns1 ping -c 500 -f -Q 0x88 10.0.3.2 >/dev/null 2>&1 & # Queue 6
sudo ip netns exec ns1 ping -c 500 -f -Q 0xb8 10.0.3.2 >/dev/null 2>&1 & # Queue 7

wait
sleep 12

echo ""
echo "Results from Switch 2 (check for drops):"
tail -30 logs/sw2.log | grep -A 15 "QoS Statistics"
echo ""

# Test 3: Priority starvation test
echo "=== Test 3: Priority Starvation Test ==="
echo "Flooding low priority while sending high priority packets..."

# Flood with best effort
sudo ip netns exec ns1 ping -c 1000 -f 10.0.3.2 >/dev/null 2>&1 &

sleep 1

# Send high priority packets
echo "Sending 50 EF (highest priority) packets during flood..."
sudo ip netns exec ns1 ping -c 50 -i 0.02 -Q 0xb8 10.0.3.2 | grep -E "transmitted|loss"

echo ""
echo "High priority packets should have 0% loss even during flood"
echo ""

sleep 12

echo "Final stats from Switch 2:"
tail -30 logs/sw2.log | grep -A 15 "QoS Statistics"

echo ""
echo "=== QoS Test Complete ==="
echo ""
echo "Summary:"
echo "- Queue 0 (Best Effort): Lowest priority, drops first under load"
echo "- Queue 3 (AF1x): Medium-low priority"
echo "- Queue 6 (AF4x): High priority"
echo "- Queue 7 (EF): Highest priority, protected from starvation"
echo ""
echo "Check logs/sw2.log for detailed queue statistics"
