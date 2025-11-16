#!/bin/bash
# Monitor three-port switch statistics
# Uses built-in DOCA tools and standard Linux utilities

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
SWITCH_NAME=${1:-"doca_three_port_switch"}
REFRESH_INTERVAL=${2:-5}

echo "=========================================="
echo "Three-Port Switch Monitoring"
echo "=========================================="
echo "Switch: $SWITCH_NAME"
echo "Refresh: ${REFRESH_INTERVAL}s"
echo "Press Ctrl+C to stop"
echo "=========================================="
echo ""

# Check if switch is running
check_switch_running() {
    pgrep -f "doca_three" > /dev/null
    return $?
}

# Get port statistics using ethtool
get_port_stats() {
    local port=$1
    
    if [ -d "/sys/class/net/$port" ]; then
        echo "Port $port:"
        # RX stats
        rx_packets=$(cat /sys/class/net/$port/statistics/rx_packets 2>/dev/null || echo "0")
        rx_bytes=$(cat /sys/class/net/$port/statistics/rx_bytes 2>/dev/null || echo "0")
        rx_errors=$(cat /sys/class/net/$port/statistics/rx_errors 2>/dev/null || echo "0")
        rx_dropped=$(cat /sys/class/net/$port/statistics/rx_dropped 2>/dev/null || echo "0")
        
        # TX stats
        tx_packets=$(cat /sys/class/net/$port/statistics/tx_packets 2>/dev/null || echo "0")
        tx_bytes=$(cat /sys/class/net/$port/statistics/tx_bytes 2>/dev/null || echo "0")
        tx_errors=$(cat /sys/class/net/$port/statistics/tx_errors 2>/dev/null || echo "0")
        tx_dropped=$(cat /sys/class/net/$port/statistics/tx_dropped 2>/dev/null || echo "0")
        
        # Format bytes
        rx_mb=$((rx_bytes / 1048576))
        tx_mb=$((tx_bytes / 1048576))
        
        echo "  RX: $rx_packets pkts, $rx_mb MB, $rx_errors errors, $rx_dropped dropped"
        echo "  TX: $tx_packets pkts, $tx_mb MB, $tx_errors errors, $tx_dropped dropped"
    else
        echo "Port $port: Not found"
    fi
}

# Get process statistics
get_process_stats() {
    local pids=$(pgrep -f "doca_three")
    
    if [ -z "$pids" ]; then
        echo -e "${RED}Switch processes not running${NC}"
        return 1
    fi
    
    echo "Process Statistics:"
    
    for pid in $pids; do
        echo "  PID: $pid"
        
        # CPU and memory
        if [ -f "/proc/$pid/status" ]; then
            mem_kb=$(grep VmRSS /proc/$pid/status | awk '{print $2}')
            mem_mb=$((mem_kb / 1024))
            echo "    Memory: ${mem_mb} MB"
        fi
        
        # Get CPU usage
        cpu_usage=$(ps -p $pid -o %cpu= 2>/dev/null || echo "0")
        echo "    CPU: ${cpu_usage}%"
        echo ""
    done
}

# Get DOCA telemetry (if available)
get_doca_telemetry() {
    # Try to get DOCA counters
    # Note: This requires proper device and permissions
    
    echo "DOCA Telemetry:"
    
    # Check if we can access telemetry
    if sudo -n /opt/mellanox/doca/tools/doca_telemetry_utils get-counters 2>/dev/null | head -5; then
        echo "  Telemetry available"
    else
        echo "  Telemetry not accessible (requires sudo and device)"
    fi
}

# Get network interface status
get_interface_status() {
    echo "Network Interfaces:"
    
    # List all network interfaces
    for iface in $(ls /sys/class/net/ 2>/dev/null); do
        # Skip loopback
        if [ "$iface" = "lo" ]; then
            continue
        fi
        
        # Get link status
        operstate=$(cat /sys/class/net/$iface/operstate 2>/dev/null || echo "unknown")
        speed=$(cat /sys/class/net/$iface/speed 2>/dev/null || echo "?")
        
        if [ "$operstate" = "up" ]; then
            echo -e "  ${GREEN}$iface${NC}: UP ($speed Mbps)"
        else
            echo -e "  ${YELLOW}$iface${NC}: $operstate"
        fi
    done
}

# Main monitoring loop
monitor_loop() {
    while true; do
        clear
        
        echo "=========================================="
        echo "Three-Port Switch Monitor"
        echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "=========================================="
        echo ""
        
        # Check if switch is running
        if ! check_switch_running; then
            echo -e "${RED}WARNING: Switch process not running!${NC}"
            echo ""
        fi
        
        # Process stats
        get_process_stats
        echo ""
        
        # Interface status
        get_interface_status
        echo ""
        
        # Port statistics (example interfaces - adjust as needed)
        # These would be your actual Ethernet ports
        echo "Port Statistics:"
        echo "----------------------------------------"
        
        # Check common interface names
        for port in eth0 eth1 eth2 ens3 ens4 ens5 enp3s0f0 enp3s0f1; do
            if [ -d "/sys/class/net/$port" ]; then
                get_port_stats $port
                echo ""
            fi
        done
        
        echo "=========================================="
        echo "Refreshing in ${REFRESH_INTERVAL}s... (Ctrl+C to stop)"
        
        sleep $REFRESH_INTERVAL
    done
}

# Start monitoring
monitor_loop
