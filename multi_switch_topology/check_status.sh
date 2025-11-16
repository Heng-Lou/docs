#!/bin/bash
# Quick status check for three-port switch deployment

echo "=========================================="
echo "Three-Port Switch Status Check"
echo "=========================================="
echo ""

# Check if switches are running
echo "1. Running Switch Processes:"
echo "----------------------------"
if pgrep -f "doca_three" > /dev/null; then
    pgrep -f "doca_three" | while read pid; do
        echo "  ✓ PID $pid: Running"
    done
else
    echo "  ✗ No switch processes found"
fi
echo ""

# Check network interfaces
echo "2. Network Interface Status:"
echo "----------------------------"
for iface in $(ls /sys/class/net/ 2>/dev/null); do
    if [ "$iface" != "lo" ]; then
        operstate=$(cat /sys/class/net/$iface/operstate 2>/dev/null)
        if [ "$operstate" = "up" ]; then
            echo "  ✓ $iface: UP"
        else
            echo "  ✗ $iface: $operstate"
        fi
    fi
done
echo ""

# Check packet counts
echo "3. Packet Statistics:"
echo "----------------------------"
for iface in $(ls /sys/class/net/ 2>/dev/null); do
    if [ "$iface" != "lo" ]; then
        rx=$(cat /sys/class/net/$iface/statistics/rx_packets 2>/dev/null || echo "0")
        tx=$(cat /sys/class/net/$iface/statistics/tx_packets 2>/dev/null || echo "0")
        
        if [ $rx -gt 0 ] || [ $tx -gt 0 ]; then
            echo "  $iface: RX=$rx TX=$tx packets"
        fi
    fi
done
echo ""

# Check system resources
echo "4. System Resources:"
echo "----------------------------"
echo "  CPU Load: $(uptime | awk -F'load average:' '{print $2}')"
echo "  Memory: $(free -h | grep Mem | awk '{print $3 "/" $2}')"
echo "  Hugepages: $(grep HugePages /proc/meminfo | grep -v Zero)"
echo ""

# Check for errors
echo "5. Error Check:"
echo "----------------------------"
errors_found=0

for iface in $(ls /sys/class/net/ 2>/dev/null); do
    if [ "$iface" != "lo" ]; then
        rx_errors=$(cat /sys/class/net/$iface/statistics/rx_errors 2>/dev/null || echo "0")
        tx_errors=$(cat /sys/class/net/$iface/statistics/tx_errors 2>/dev/null || echo "0")
        
        if [ $rx_errors -gt 0 ] || [ $tx_errors -gt 0 ]; then
            echo "  ⚠ $iface: RX_ERR=$rx_errors TX_ERR=$tx_errors"
            errors_found=1
        fi
    fi
done

if [ $errors_found -eq 0 ]; then
    echo "  ✓ No errors detected"
fi

echo ""
echo "=========================================="
echo "Check complete: $(date)"
echo "=========================================="
