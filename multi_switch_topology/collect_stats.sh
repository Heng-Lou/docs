#!/bin/bash
# Simple statistics collector for multi-switch deployment
# Works without special DOCA tools, uses standard Linux utilities

# Configuration
OUTPUT_FILE="switch_stats_$(date +%Y%m%d_%H%M%S).log"
INTERVAL=5

echo "Multi-Switch Statistics Collector"
echo "=================================="
echo "Output: $OUTPUT_FILE"
echo "Interval: ${INTERVAL}s"
echo ""

# Log header
{
    echo "Multi-Switch Statistics Log"
    echo "Started: $(date)"
    echo "=================================="
} > $OUTPUT_FILE

collect_stats() {
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    {
        echo ""
        echo "[$timestamp]"
        echo "-----------------------------------"
        
        # Switch processes
        echo "Running Switches:"
        pgrep -a "doca_three_port_switch" | while read pid cmd; do
            cpu=$(ps -p $pid -o %cpu= 2>/dev/null || echo "0")
            mem=$(ps -p $pid -o %mem= 2>/dev/null || echo "0")
            echo "  PID $pid: CPU=${cpu}% MEM=${mem}%"
        done
        
        # Network statistics summary
        echo ""
        echo "Network Summary:"
        
        total_rx=0
        total_tx=0
        
        for iface in $(ls /sys/class/net/ 2>/dev/null); do
            if [ "$iface" != "lo" ]; then
                rx=$(cat /sys/class/net/$iface/statistics/rx_packets 2>/dev/null || echo "0")
                tx=$(cat /sys/class/net/$iface/statistics/tx_packets 2>/dev/null || echo "0")
                
                total_rx=$((total_rx + rx))
                total_tx=$((total_tx + tx))
                
                if [ $rx -gt 0 ] || [ $tx -gt 0 ]; then
                    echo "  $iface: RX=$rx TX=$tx"
                fi
            fi
        done
        
        echo "  Total: RX=$total_rx TX=$total_tx"
        
    } | tee -a $OUTPUT_FILE
}

# Main loop
echo "Collecting statistics... (Ctrl+C to stop)"
echo ""

while true; do
    collect_stats
    sleep $INTERVAL
done
