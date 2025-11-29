#!/bin/bash

echo "Real-time QoS Monitor (Ctrl+C to stop)"
echo "Press Enter to refresh stats..."
echo ""

while true; do
    clear
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║           DPDK Switch QoS Real-time Statistics                ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    
    for sw in 1 2 3; do
        if [ -f logs/sw${sw}.log ]; then
            echo "┌─── Switch $sw ───┐"
            tail -20 logs/sw${sw}.log | grep -A 12 "QoS Statistics" | tail -13
            echo ""
        fi
    done
    
    echo "Press Ctrl+C to exit, or wait 5 seconds for auto-refresh..."
    sleep 5
done