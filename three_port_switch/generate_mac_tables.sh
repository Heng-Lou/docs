#!/bin/bash
# filepath: /home/heng/workdir/doca/three_port_switch/generate_mac_tables.sh

NUM_SWITCHES=${1:-3}
TOPOLOGY=${2:-line}

if [ "$NUM_SWITCHES" -lt 2 ] || [ "$NUM_SWITCHES" -gt 10 ]; then
    echo "Error: Number of switches must be between 2 and 10"
    echo "Usage: $0 <num_switches> <topology>"
    exit 1
fi

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}=== Generating Static MAC Tables for $NUM_SWITCHES Switches ($TOPOLOGY) ===${NC}"
echo ""

mkdir -p mac_tables

# Get MAC addresses
get_mac() {
    local ns=$1
    local iface=$2
    sudo ip netns exec $ns ip link show $iface 2>/dev/null | grep -oP '(?<=link/ether )\S+' | head -1
}

echo "Getting MAC addresses..."
declare -A MACS
for i in $(seq 1 $NUM_SWITCHES); do
    MACS[$i]=$(get_mac ns$i veth_h${i}_s${i})
    if [ -z "${MACS[$i]}" ]; then
        echo -e "${RED}Error: Could not get MAC for ns$i${NC}"
        echo "Make sure namespaces are created: ./setup_veth_topology.sh $NUM_SWITCHES $TOPOLOGY"
        exit 1
    fi
    echo "  ns$i: ${MACS[$i]}"
done

echo ""

# Generate MAC tables based on topology
if [ "$TOPOLOGY" == "line" ]; then
    echo "Generating MAC tables for LINE topology..."
    echo ""
    
    for sw in $(seq 1 $NUM_SWITCHES); do
        MAC_FILE="mac_tables/switch_${sw}_line.txt"
        echo "Generating $MAC_FILE..."
        
        cat > $MAC_FILE << EOF
# Static MAC table for Switch $sw (Line Topology with $NUM_SWITCHES switches)
# Format: MAC_ADDRESS PORT_ID DESCRIPTION
# 
# Port assignment for Switch $sw:
#   Port 0: Local host (ns$sw)
EOF

        if [ $sw -eq 1 ]; then
            echo "#   Port 1: Link to Switch 2" >> $MAC_FILE
        elif [ $sw -eq $NUM_SWITCHES ]; then
            echo "#   Port 1: Link to Switch $((sw-1))" >> $MAC_FILE
        else
            echo "#   Port 1: Link to Switch $((sw-1)) (left/lower)" >> $MAC_FILE
            echo "#   Port 2: Link to Switch $((sw+1)) (right/higher)" >> $MAC_FILE
        fi
        
        echo "#" >> $MAC_FILE
        
        # Add local host on port 0
        echo "${MACS[$sw]} 0 ns${sw}_local" >> $MAC_FILE
        
        # Add remote hosts based on position
        for target in $(seq 1 $NUM_SWITCHES); do
            if [ $target -ne $sw ]; then
                
                if [ $sw -eq 1 ]; then
                    # First switch: everything goes right (port 1)
                    echo "${MACS[$target]} 1 ns${target}_via_right" >> $MAC_FILE
                    
                elif [ $sw -eq $NUM_SWITCHES ]; then
                    # Last switch: everything goes left (port 1)
                    echo "${MACS[$target]} 1 ns${target}_via_left" >> $MAC_FILE
                    
                else
                    # Middle switch: determine direction
                    if [ $target -lt $sw ]; then
                        # Target is to the left - use port 1 (connects to lower switch)
                        echo "${MACS[$target]} 1 ns${target}_via_left" >> $MAC_FILE
                    else
                        # Target is to the right - use port 2 (connects to higher switch)
                        echo "${MACS[$target]} 2 ns${target}_via_right" >> $MAC_FILE
                    fi
                fi
            fi
        done
        
        echo "  Created: $MAC_FILE ($(wc -l < $MAC_FILE) lines)"
    done
    
elif [ "$TOPOLOGY" == "ring" ]; then
    echo "Generating MAC tables for RING topology..."
    echo ""
    
    for sw in $(seq 1 $NUM_SWITCHES); do
        MAC_FILE="mac_tables/switch_${sw}_ring.txt"
        echo "Generating $MAC_FILE..."
        
        # Calculate next and previous switches
        NEXT=$(( (sw % NUM_SWITCHES) + 1 ))
        PREV=$(( (sw == 1) ? NUM_SWITCHES : sw - 1 ))
        
        cat > $MAC_FILE << EOF
# Static MAC table for Switch $sw (Ring Topology with $NUM_SWITCHES switches)
# Format: MAC_ADDRESS PORT_ID DESCRIPTION
# 
# Port assignment for Switch $sw:
#   Port 0: Local host (ns$sw)
#   Port 1: Link to Switch $NEXT (clockwise/next)
#   Port 2: Link to Switch $PREV (counter-clockwise/previous)
#
EOF
        
        # Add local host on port 0
        echo "${MACS[$sw]} 0 ns${sw}_local" >> $MAC_FILE
        
        # Add remote hosts - choose shortest path
        for target in $(seq 1 $NUM_SWITCHES); do
            if [ $target -ne $sw ]; then
                # Calculate clockwise distance
                if [ $target -gt $sw ]; then
                    CW_DIST=$((target - sw))
                else
                    CW_DIST=$((NUM_SWITCHES - sw + target))
                fi
                
                # Calculate counter-clockwise distance
                CCW_DIST=$((NUM_SWITCHES - CW_DIST))
                
                # Choose shorter path (or clockwise if equal)
                if [ $CW_DIST -le $CCW_DIST ]; then
                    # Go clockwise (port 1)
                    echo "${MACS[$target]} 1 ns${target}_cw${CW_DIST}_via_s${NEXT}" >> $MAC_FILE
                else
                    # Go counter-clockwise (port 2)
                    echo "${MACS[$target]} 2 ns${target}_ccw${CCW_DIST}_via_s${PREV}" >> $MAC_FILE
                fi
            fi
        done
        
        echo "  Created: $MAC_FILE ($(wc -l < $MAC_FILE) lines)"
    done
fi

echo ""
echo -e "${GREEN}✓ MAC tables generated successfully${NC}"
echo ""
echo "Generated files:"
ls -lh mac_tables/switch_*_${TOPOLOGY}.txt 2>/dev/null
echo ""
echo "Example: Switch 1 MAC table:"
echo "----------------------------------------"
cat mac_tables/switch_1_${TOPOLOGY}.txt
echo "----------------------------------------"