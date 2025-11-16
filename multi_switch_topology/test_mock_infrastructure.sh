#!/bin/bash
# Comprehensive Mock Infrastructure Test Suite

echo "=========================================="
echo "Mock Infrastructure Test Suite"
echo "=========================================="
echo ""

TEST_PASSED=0
TEST_FAILED=0

# Cleanup any existing processes
pkill -9 -f doca_three 2>/dev/null
sleep 1

# Test 1: Process Launch
echo "Test 1: Process Launch"
./mock_simulator.sh 4 ring >/dev/null 2>&1 &
SIM_PID=$!
sleep 3
COUNT=$(pgrep -f doca_three | wc -l)
if [ "$COUNT" -eq 4 ]; then
    echo "  ✅ PASS: 4 processes started"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo "  ❌ FAIL: Expected 4, got $COUNT"
    TEST_FAILED=$((TEST_FAILED + 1))
fi
kill $SIM_PID 2>/dev/null
sleep 2
pkill -9 -f doca_three 2>/dev/null
sleep 1

# Test 2: Process Detection
echo "Test 2: Process Detection"
./mock_simulator.sh 2 ring >/dev/null 2>&1 &
SIM_PID=$!
sleep 3
if ./check_status.sh 2>/dev/null | grep -q "PID.*Running"; then
    echo "  ✅ PASS: Monitoring detects processes"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo "  ❌ FAIL: Monitoring failed"
    TEST_FAILED=$((TEST_FAILED + 1))
fi
kill $SIM_PID 2>/dev/null
sleep 2
pkill -9 -f doca_three 2>/dev/null
sleep 1

# Test 3: Graceful Shutdown
echo "Test 3: Graceful Shutdown"
./mock_simulator.sh 3 ring >/dev/null 2>&1 &
SIM_PID=$!
sleep 3
kill -INT $SIM_PID 2>/dev/null
sleep 3
COUNT=$(pgrep -f doca_three | wc -l)
if [ "$COUNT" -eq 0 ]; then
    echo "  ✅ PASS: Clean shutdown"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo "  ❌ FAIL: $COUNT processes still running"
    TEST_FAILED=$((TEST_FAILED + 1))
    pkill -9 -f doca_three 2>/dev/null
fi
sleep 1

# Test 4: Resource Tracking
echo "Test 4: Resource Tracking"
./mock_simulator.sh 2 ring >/dev/null 2>&1 &
SIM_PID=$!
sleep 3
PID=$(pgrep -f doca_three | head -1)
if ps -p $PID -o %cpu,%mem >/dev/null 2>&1; then
    echo "  ✅ PASS: Resource tracking works"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo "  ❌ FAIL: Cannot track resources"
    TEST_FAILED=$((TEST_FAILED + 1))
fi
kill $SIM_PID 2>/dev/null
sleep 2
pkill -9 -f doca_three 2>/dev/null
sleep 1

# Test 5: Scalability
echo "Test 5: Scalability (8 switches)"
./mock_simulator.sh 8 ring >/dev/null 2>&1 &
SIM_PID=$!
sleep 5
COUNT=$(pgrep -f doca_three | wc -l)
if [ "$COUNT" -eq 8 ]; then
    echo "  ✅ PASS: 8 processes scale test"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo "  ❌ FAIL: Expected 8, got $COUNT"
    TEST_FAILED=$((TEST_FAILED + 1))
fi
kill $SIM_PID 2>/dev/null
sleep 2
pkill -9 -f doca_three 2>/dev/null

echo ""
echo "=========================================="
echo "Test Results"
echo "=========================================="
echo "Passed: $TEST_PASSED"
echo "Failed: $TEST_FAILED"
echo "Total:  $((TEST_PASSED + TEST_FAILED))"
echo ""

if [ $TEST_FAILED -eq 0 ]; then
    echo "✅ ALL TESTS PASSED!"
    exit 0
else
    echo "❌ SOME TESTS FAILED"
    exit 1
fi
