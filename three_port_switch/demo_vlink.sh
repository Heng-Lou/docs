#!/bin/bash
# Virtual Link Demo Script

COLOR_GREEN='\033[0;32m'
COLOR_BLUE='\033[0;34m'
COLOR_YELLOW='\033[1;33m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}Virtual Link Demonstration${COLOR_RESET}"
echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo ""

# Demo 1: Ring topology
echo -e "${COLOR_YELLOW}Demo 1: Ring Topology (4 switches)${COLOR_RESET}"
echo "Switches connected in a ring with test traffic"
echo "Running for 8 seconds..."
echo ""
timeout 8 ./vlink_switch_sim -n 4 -t ring -s || true
echo ""
echo -e "${COLOR_GREEN}✓ Ring demo completed${COLOR_RESET}"
echo ""
read -p "Press Enter to continue to next demo..."
echo ""

# Demo 2: Line topology
echo -e "${COLOR_YELLOW}Demo 2: Line Topology (6 switches)${COLOR_RESET}"
echo "Switches connected end-to-end"
echo "Running for 8 seconds..."
echo ""
timeout 8 ./vlink_switch_sim -n 6 -t line -s || true
echo ""
echo -e "${COLOR_GREEN}✓ Line demo completed${COLOR_RESET}"
echo ""
read -p "Press Enter to continue to next demo..."
echo ""

# Demo 3: Mesh topology
echo -e "${COLOR_YELLOW}Demo 3: Mesh Topology (4 switches)${COLOR_RESET}"
echo "Partial mesh with multiple connections"
echo "Running for 8 seconds..."
echo ""
timeout 8 ./vlink_switch_sim -n 4 -t mesh -s || true
echo ""
echo -e "${COLOR_GREEN}✓ Mesh demo completed${COLOR_RESET}"
echo ""
read -p "Press Enter to continue to final demo..."
echo ""

# Demo 4: Scale test
echo -e "${COLOR_YELLOW}Demo 4: Scale Test (12 switches)${COLOR_RESET}"
echo "Testing with larger topology"
echo "Running for 10 seconds..."
echo ""
timeout 10 ./vlink_switch_sim -n 12 -t ring -s || true
echo ""
echo -e "${COLOR_GREEN}✓ Scale demo completed${COLOR_RESET}"
echo ""

echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo -e "${COLOR_GREEN}All Demonstrations Completed!${COLOR_RESET}"
echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo ""
echo "You have seen:"
echo "  ✓ Ring topology - circular packet flow"
echo "  ✓ Line topology - end-to-end forwarding"
echo "  ✓ Mesh topology - redundant connections"
echo "  ✓ Scale test - 12 switches in ring"
echo ""
echo "Next steps:"
echo "  - Try custom configurations"
echo "  - Integrate with your switch code"
echo "  - Debug with GDB"
echo ""
