#!/bin/bash
# Helper script to identify what needs to be synced between files

echo "================================================"
echo "Three Port Switch - Code Comparison Helper"
echo "================================================"
echo ""

MAIN_FILE="three_port_switch.c"
SIM_FILE="three_port_switch_sim.c"

echo "Main file: $MAIN_FILE ($(wc -l < $MAIN_FILE) lines)"
echo "Sim file:  $SIM_FILE ($(wc -l < $SIM_FILE) lines)"
echo ""

# Extract function names from both files
echo "Functions in $MAIN_FILE:"
grep -n "^[a-z_]*.*(" $MAIN_FILE | grep -v "^\s*//" | head -20

echo ""
echo "Functions in $SIM_FILE:"
grep -n "^[a-z_]*.*(" $SIM_FILE | grep -v "^\s*//" | head -20

echo ""
echo "================================================"
echo "To sync changes:"
echo "1. Edit $MAIN_FILE with your changes"
echo "2. Identify what logic changed (forwarding, validation, etc)"
echo "3. Manually port same logic to $SIM_FILE"
echo "4. Test simulation: gcc -g -O0 -o switch_sim $SIM_FILE && gdb switch_sim"
echo "================================================"
