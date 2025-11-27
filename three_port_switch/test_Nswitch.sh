#!/bin/bash

NUM_SWITCHES=${1:-3}

if [ "$NUM_SWITCHES" -lt 2 ] || [ "$NUM_SWITCHES" -gt 10 ]; then
    echo "Usage: $0 <num_switches>"
    echo "  num_switches: 2-10 (default: 3)"
    exit 1
fi

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}=== Testing $NUM_SWITCHES-Switch Connectivity ===${NC}"
echo ""

TOTAL=0
PASSED=0
FAILED=0

for i in $(seq 1 $NUM_SWITCHES); do
    for j in $(seq 1 $NUM_SWITCHES); do
        if [ $i -ne $j ]; then
            TOTAL=$((TOTAL + 1))
            echo -n "Test $TOTAL: ns$i -> ns$j (10.0.$j.2) ... "
            
            if sudo ip netns exec ns$i ping -c 2 -W 2 10.0.$j.2 > /dev/null 2>&1; then
                echo -e "${GREEN}PASS${NC}"
                PASSED=$((PASSED + 1))
            else
                echo -e "${RED}FAIL${NC}"
                FAILED=$((FAILED + 1))
            fi
        fi
    done
done

echo ""
echo -e "${YELLOW}=== Test Summary ===${NC}"
echo "Total tests:  $TOTAL"
echo -e "Passed:       ${GREEN}$PASSED${NC}"
echo -e "Failed:       ${RED}$FAILED${NC}"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✓ All tests PASSED!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Some tests FAILED${NC}"
    exit 1
fi