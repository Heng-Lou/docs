#!/bin/bash
# Quick fix for high-rate packet generation errors

echo "========================================="
echo "Fix High-Rate Queue Overflow"
echo "========================================="
echo ""

# Check current queue size
current_size=$(grep "^#define VLINK_QUEUE_SIZE" virtual_link.h | awk '{print $3}')
echo "Current queue size: $current_size"
echo ""

# Ask user what to do
echo "Solutions:"
echo "  1) Increase queue size to 4096 (for moderate tests, <1000 packets/host)"
echo "  2) Increase queue size to 16384 (for high-rate tests, <5000 packets/host)"
echo "  3) Increase queue size to 32768 (for stress tests, 10000+ packets/host)"
echo "  4) Show recommended test parameters for current queue size"
echo "  5) Exit (no changes)"
echo ""
read -p "Choose option (1-5): " choice

case $choice in
    1)
        echo ""
        echo "Changing VLINK_QUEUE_SIZE to 4096..."
        sed -i 's/^#define VLINK_QUEUE_SIZE.*$/#define VLINK_QUEUE_SIZE 4096  \/\* Moderate testing: <1000 pkts\/host *\//' virtual_link.h
        echo "✓ Updated virtual_link.h"
        echo ""
        echo "Rebuilding..."
        make -f Makefile.vhost clean all
        echo ""
        echo "✓ Done! Suitable for tests like:"
        echo "  ./vhost_switch_test -n 4 -p -r 200 -c 1000 -d 10"
        echo "  ./vhost_switch_test -n 8 -p -r 100 -c 800 -d 15"
        ;;
    2)
        echo ""
        echo "Changing VLINK_QUEUE_SIZE to 16384..."
        sed -i 's/^#define VLINK_QUEUE_SIZE.*$/#define VLINK_QUEUE_SIZE 16384  \/\* High-rate testing: <5000 pkts\/host *\//' virtual_link.h
        echo "✓ Updated virtual_link.h"
        echo ""
        echo "Rebuilding..."
        make -f Makefile.vhost clean all
        echo ""
        echo "✓ Done! Suitable for tests like:"
        echo "  ./vhost_switch_test -n 4 -p -r 500 -c 5000 -d 10"
        echo "  ./vhost_switch_test -n 8 -p -r 300 -c 3000 -d 15"
        ;;
    3)
        echo ""
        echo "Changing VLINK_QUEUE_SIZE to 32768..."
        sed -i 's/^#define VLINK_QUEUE_SIZE.*$/#define VLINK_QUEUE_SIZE 32768  \/\* Stress testing: 10000+ pkts\/host *\//' virtual_link.h
        echo "✓ Updated virtual_link.h"
        echo ""
        echo "Rebuilding..."
        make -f Makefile.vhost clean all
        echo ""
        echo "✓ Done! Suitable for extreme tests like:"
        echo "  ./vhost_switch_test -n 4 -p -r 1000 -c 10000 -d 15"
        echo "  ./vhost_switch_test -n 8 -p -r 500 -c 10000 -d 30"
        ;;
    4)
        echo ""
        echo "Recommended test parameters for queue size $current_size:"
        echo ""
        echo "4 hosts:"
        echo "  ./vhost_switch_test -n 4 -p -r 500 -c 5000 -d 10"
        echo ""
        echo "8 hosts:"
        echo "  ./vhost_switch_test -n 8 -p -r 200 -c 5000 -d 20"
        echo ""
        echo "16 hosts:"
        echo "  ./vhost_switch_test -n 16 -p -r 100 -c 5000 -d 30"
        echo ""
        echo "To support higher rates, choose option 1 or 2."
        ;;
    5)
        echo "No changes made."
        exit 0
        ;;
    *)
        echo "Invalid option."
        exit 1
        ;;
esac

