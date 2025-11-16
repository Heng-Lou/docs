#!/bin/bash
# Mock Switch Simulator - Test Infrastructure Without Hardware
# This creates mock processes that the monitoring tools can track

NUM_SWITCHES=${1:-8}
TOPOLOGY=${2:-ring}

echo "=========================================="
echo "Mock Switch Simulator"
echo "=========================================="
echo "Switches: $NUM_SWITCHES"
echo "Topology: $TOPOLOGY"
echo "Note: These are mock processes for testing"
echo "infrastructure, not real DOCA switches."
echo ""

# Array to track PIDs
declare -a PIDS

# Cleanup function
cleanup() {
    echo ""
    echo "Stopping all mock switches..."
    for pid in "${PIDS[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            kill $pid 2>/dev/null
        fi
    done
    echo "All mock switches stopped"
    exit 0
}

trap cleanup SIGINT SIGTERM

# Start mock switches
for i in $(seq 0 $((NUM_SWITCHES - 1))); do
    (
        # Detach from parent to survive backgrounding
        exec -a "doca_three_port_switch_mock_$i" bash -c "
            # Redirect to /dev/null to avoid output issues
            exec >/dev/null 2>&1
            while true; do
                sleep 10
            done
        " &
    ) &
    
    pid=$!
    PIDS[$i]=$pid
    echo "Started Mock Switch $i (PID: $pid)"
    sleep 0.2  # Shorter sleep for faster startup
done

echo ""
echo "=========================================="
echo "All Mock Switches Running!"
echo "=========================================="
echo ""
echo "Process List:"
for i in $(seq 0 $((NUM_SWITCHES - 1))); do
    echo "  Switch $i: PID ${PIDS[$i]}"
done
echo ""
echo "Test with monitoring tools:"
echo "  ./check_status.sh"
echo "  ./monitor_switch.sh"
echo "  pgrep -a doca_three"
echo ""
echo "Press Ctrl+C to stop all switches"
echo ""

# Keep running
while true; do
    sleep 5
    running=0
    for pid in "${PIDS[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            running=$((running + 1))
        fi
    done
    echo -ne "\r[$(date '+%H:%M:%S')] Running: $running/$NUM_SWITCHES mock switches | Press Ctrl+C to stop    "
done
