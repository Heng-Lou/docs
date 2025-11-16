# Testing Mock Infrastructure - What You CAN and CANNOT Do

## Important: Mock Switches Are NOT Real Network Devices ❌

### What Mock Switches Actually Are

Mock switches are **bash processes that sleep** - they simulate the **process lifecycle** only:
```bash
# This is what's running:
while true; do
    sleep 10
done
```

**NOT real switches!** No packet forwarding, no networking, no data plane.

---

## ❌ What You CANNOT Test

### NO Network Traffic ❌

```bash
# These WON'T work:
ping <mock_switch>          ❌ No IP addresses
iperf -c <mock_switch>      ❌ No network stack
curl <mock_switch>          ❌ No services
tcpdump -i <mock_switch>    ❌ No interfaces
```

**Why?** Mock switches are just bash sleep loops, not network devices!

### NO Packet Forwarding ❌

```bash
# Mock switches do NOT:
❌ Forward packets
❌ Learn MAC addresses
❌ Create network interfaces
❌ Process traffic
❌ Run DOCA Flow
❌ Have ports
```

### NO Hardware Features ❌

```bash
# Mock switches do NOT:
❌ Use DPDK
❌ Access PCI devices
❌ Offload to hardware
❌ Collect packet statistics
❌ Perform switching
```

---

## ✅ What You CAN Test

### 1. Process Management ✅

**Test:** Can you start/stop multiple processes?

```bash
# Start 8 mock switches
./mock_simulator.sh 8 ring

# Verify all started
pgrep -f doca_three | wc -l
# Expected: 8

# Stop them (Ctrl+C in simulator terminal)
# Verify all stopped
pgrep -f doca_three | wc -l
# Expected: 0
```

**✅ Tests:**
- Multi-process launch
- PID tracking
- Signal handling (SIGINT)
- Graceful shutdown

---

### 2. Monitoring Tools ✅

**Test:** Do monitoring scripts detect processes?

```bash
# Terminal 1: Start switches
./mock_simulator.sh 4 ring

# Terminal 2: Test monitoring
./check_status.sh
# Expected: ✓ PID xxxx: Running (×4)

./monitor_switch.sh
# Expected: Shows 4 processes with CPU/memory

pgrep -af doca_three
# Expected: Lists 4 mock switch processes
```

**✅ Tests:**
- Process detection
- PID enumeration
- Status reporting
- Real-time updates

---

### 3. Resource Tracking ✅

**Test:** Are CPU/memory monitored correctly?

```bash
# Start switches
./mock_simulator.sh 4 ring

# Monitor resources
while true; do
    clear
    echo "=== Resource Tracking Test ==="
    for pid in $(pgrep -f doca_three); do
        echo "PID: $pid"
        ps -p $pid -o %cpu,%mem,vsz,rss
    done
    sleep 2
done
```

**✅ Tests:**
- CPU usage tracking
- Memory consumption
- Process state monitoring
- Resource allocation

---

### 4. Statistics Collection ✅

**Test:** Does stats collection work?

```bash
# Terminal 1: Start switches
./mock_simulator.sh 4 ring

# Terminal 2: Collect stats
./collect_stats.sh &
sleep 30

# Check log file
cat switch_stats_*.log
# Expected: Timestamped process stats
```

**✅ Tests:**
- Log file creation
- Periodic sampling
- Data persistence
- Background execution

---

### 5. Scalability ✅

**Test:** Can system handle many processes?

```bash
# Test with increasing counts
for n in 4 8 16 32; do
    echo "Testing with $n switches..."
    ./mock_simulator.sh $n ring &
    sleep 5
    
    count=$(pgrep -f doca_three | wc -l)
    echo "Started: $count switches"
    
    # Stop them
    pkill -f doca_three
    sleep 2
done
```

**✅ Tests:**
- Multi-process scaling
- System resource limits
- Process coordination
- Clean shutdown at scale

---

### 6. Lifecycle Management ✅

**Test:** Full start/monitor/stop cycle?

```bash
#!/bin/bash
# Full lifecycle test

echo "1. Starting switches..."
./mock_simulator.sh 4 ring &
SIM_PID=$!
sleep 3

echo "2. Verifying startup..."
./check_status.sh

echo "3. Monitoring (10 seconds)..."
timeout 10 ./monitor_switch.sh

echo "4. Collecting stats..."
./collect_stats.sh &
STATS_PID=$!
sleep 5

echo "5. Stopping switches..."
kill $SIM_PID
sleep 2

echo "6. Verifying shutdown..."
pgrep -f doca_three || echo "All stopped ✅"

kill $STATS_PID 2>/dev/null
echo "Test complete!"
```

**✅ Tests:**
- Complete operational cycle
- Monitoring integration
- Stats collection
- Clean shutdown

---

### 7. Concurrent Monitoring ✅

**Test:** Multiple monitoring tools simultaneously?

```bash
# Terminal 1: Start switches
./mock_simulator.sh 8 ring

# Terminal 2: Real-time monitor
./monitor_switch.sh

# Terminal 3: Status checks
watch -n 5 './check_status.sh | head -20'

# Terminal 4: Stats collection
./collect_stats.sh &

# Terminal 5: Process watch
watch -n 1 'pgrep -f doca_three | wc -l'
```

**✅ Tests:**
- Concurrent tool execution
- No race conditions
- Consistent reporting
- Independent monitoring

---

### 8. Error Handling ✅

**Test:** What happens with errors?

```bash
# Test 1: Kill individual processes
./mock_simulator.sh 4 ring &
sleep 3
pid=$(pgrep -f doca_three | head -1)
kill -9 $pid  # Forcefully kill one
./check_status.sh  # Should show 3 running

# Test 2: Resource exhaustion
ulimit -u 100  # Limit processes
./mock_simulator.sh 200 ring  # Try to start 200
# Should handle gracefully

# Test 3: Signal handling
./mock_simulator.sh 4 ring &
sleep 2
pkill -TERM -f mock_simulator  # Send SIGTERM
# Should cleanup all children
```

**✅ Tests:**
- Process failure detection
- Resource limit handling
- Signal propagation
- Cleanup on errors

---

## Comprehensive Test Suite

### Run All Tests

```bash
cat > /home/heng/workdir/doca/multi_switch_topology/test_mock_infrastructure.sh << 'SCRIPT'
#!/bin/bash
# Comprehensive Mock Infrastructure Test Suite

echo "=========================================="
echo "Mock Infrastructure Test Suite"
echo "=========================================="
echo ""

TEST_PASSED=0
TEST_FAILED=0

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
SCRIPT

chmod +x test_mock_infrastructure.sh
echo "Created: test_mock_infrastructure.sh"
```

**Run it:**
```bash
./test_mock_infrastructure.sh
```

---

## What About Network Testing?

### For Network Testing, You Need REAL Switches

Mock switches **cannot** be tested with network tools because they're not on the network!

### Option 1: Use Real DOCA Switches (Needs Hardware)

```bash
# On BlueField DPU with real switches
sudo ./doca_three_port_switch -a 03:00.0 -a 03:00.1 -a 03:00.2 --

# NOW you can test networking:
ping <switch_ip>             ✅ Works
iperf -c <switch_ip>         ✅ Works
tcpdump -i enp3s0f0          ✅ Works
doca_flow_query --stats      ✅ Works
```

### Option 2: Network Namespace Simulation

If you want to test networking without hardware, create **network namespaces**:

```bash
# Create network namespaces test script
cat > /home/heng/workdir/doca/multi_switch_topology/network_namespace_test.sh << 'NETNS'
#!/bin/bash
# Network Namespace Simulation (NOT for DOCA, but for network testing)

echo "Creating network namespace simulation..."

# Create namespaces
sudo ip netns add switch1
sudo ip netns add host1
sudo ip netns add host2

# Create veth pairs
sudo ip link add veth0 type veth peer name veth1
sudo ip link add veth2 type veth peer name veth3

# Move to namespaces
sudo ip link set veth1 netns switch1
sudo ip link set veth2 netns host1
sudo ip link set veth3 netns host2

# Configure IPs
sudo ip netns exec host1 ip addr add 10.0.1.10/24 dev veth2
sudo ip netns exec host2 ip addr add 10.0.2.10/24 dev veth3
sudo ip netns exec switch1 ip addr add 10.0.1.1/24 dev veth1

# Bring up
sudo ip netns exec host1 ip link set veth2 up
sudo ip netns exec host2 ip link set veth3 up
sudo ip netns exec switch1 ip link set veth1 up

echo "✅ Network namespaces created!"
echo ""
echo "Test with:"
echo "  sudo ip netns exec host1 ping 10.0.1.1"
echo "  sudo ip netns exec host1 iperf -s"
echo ""
echo "Cleanup with:"
echo "  sudo ip netns del switch1 host1 host2"
NETNS

chmod +x network_namespace_test.sh
```

**But remember:** This tests Linux networking, NOT DOCA switches!

---

## Summary Table

| Test Type | Mock Switches | Real DOCA Switches |
|-----------|---------------|-------------------|
| **Process Management** | ✅ YES | ✅ YES |
| **Monitoring Tools** | ✅ YES | ✅ YES |
| **Resource Tracking** | ✅ YES | ✅ YES |
| **Stats Collection** | ✅ YES | ✅ YES |
| **Scalability** | ✅ YES | ✅ YES |
| **Lifecycle** | ✅ YES | ✅ YES |
| **ping/iperf** | ❌ NO | ✅ YES |
| **Packet forwarding** | ❌ NO | ✅ YES |
| **DOCA Flow stats** | ❌ NO | ✅ YES |
| **MAC learning** | ❌ NO | ✅ YES |
| **Hardware offload** | ❌ NO | ✅ YES |

---

## Quick Answer to Your Question

### Can I ping or iperf mock switches?

**NO! ❌**

Mock switches are **bash sleep loops**, not network devices. They have:
- ❌ No IP addresses
- ❌ No network interfaces
- ❌ No packet processing
- ❌ No services to ping/connect to

### What CAN I test?

**Process infrastructure!** ✅
- Process launch/stop
- Monitoring detection
- Resource tracking
- Stats collection
- Scalability

### How do I test networking?

**You need real DOCA switches on real hardware!**

```bash
# Mock testing (process infrastructure)
./mock_simulator.sh 4 ring       ✅ Tests process mgmt

# Real testing (networking)
sudo ./doca_three_port_switch \
    -a 03:00.0 -a 03:00.1 --     ✅ Tests packet forwarding
```

---

## Run the Test Suite

```bash
# Create and run comprehensive tests
./test_mock_infrastructure.sh
```

**Expected output:**
```
==========================================
Mock Infrastructure Test Suite
==========================================

Test 1: Process Launch
  ✅ PASS: 4 processes started
Test 2: Process Detection
  ✅ PASS: Monitoring detects processes
Test 3: Graceful Shutdown
  ✅ PASS: Clean shutdown
Test 4: Resource Tracking
  ✅ PASS: Resource tracking works
Test 5: Scalability (8 switches)
  ✅ PASS: 8 processes scale test

==========================================
Test Results
==========================================
Passed: 5
Failed: 0
Total:  5

✅ ALL TESTS PASSED!
```

---

**Bottom Line:** Mock switches test **process infrastructure only**, NOT networking. For ping/iperf, you need real DOCA switches on BlueField hardware!
