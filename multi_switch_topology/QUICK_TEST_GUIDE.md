# Quick Testing Guide - Mock Infrastructure

## Direct Answer: NO ping/iperf ❌

**Mock switches are bash sleep loops - NOT network devices!**

```bash
# These DON'T work with mock switches:
ping <mock_switch>      ❌ No IP address
iperf -c <mock_switch>  ❌ No network stack
tcpdump                 ❌ No interfaces
```

---

## What You CAN Test ✅

### Quick Tests (5 minutes)

#### 1. Start Mock Switches
```bash
cd /home/heng/workdir/doca/multi_switch_topology
./mock_simulator.sh 4 ring
```

#### 2. Check They're Running (New Terminal)
```bash
# How many processes?
pgrep -f doca_three | wc -l
# Expected: 4

# Quick status
./check_status.sh

# Real-time monitor
./monitor_switch.sh
```

#### 3. Test Scalability
```bash
# Stop current (Ctrl+C in simulator)
# Try 8 switches
./mock_simulator.sh 8 ring

# Check
pgrep -f doca_three | wc -l
# Expected: 8
```

#### 4. Test Monitoring
```bash
# Resource tracking
for pid in $(pgrep -f doca_three); do
    echo "PID: $pid"
    ps -p $pid -o %cpu,%mem,vsz,rss
done

# Stats collection
./collect_stats.sh &
sleep 30
cat switch_stats_*.log
```

---

## Automated Test Suite ✅

### Run All Tests
```bash
./test_mock_infrastructure.sh
```

**Tests:**
- ✅ Process launch (4 switches)
- ✅ Process detection (monitoring tools)
- ✅ Graceful shutdown
- ✅ Resource tracking
- ✅ Scalability (8 switches)

**Expected:**
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

## What Each Test Does

### Test 1: Process Launch ✅
**Verifies:** Can start multiple switches
**Command:** `./mock_simulator.sh 4 ring`
**Check:** `pgrep -f doca_three | wc -l` returns 4

### Test 2: Process Detection ✅
**Verifies:** Monitoring tools find processes
**Command:** `./check_status.sh`
**Check:** Output shows "PID xxxx: Running"

### Test 3: Graceful Shutdown ✅
**Verifies:** Clean stop with SIGINT
**Command:** `kill -INT $PID`
**Check:** All processes exit cleanly

### Test 4: Resource Tracking ✅
**Verifies:** Can monitor CPU/memory
**Command:** `ps -p $PID -o %cpu,%mem`
**Check:** Returns valid statistics

### Test 5: Scalability ✅
**Verifies:** Can handle many switches
**Command:** `./mock_simulator.sh 8 ring`
**Check:** All 8 processes start successfully

---

## Manual Testing Examples

### Test Process Lifecycle
```bash
# Start
./mock_simulator.sh 4 ring &
PID=$!

# Check started
pgrep -f doca_three | wc -l

# Monitor
./monitor_switch.sh &
sleep 10

# Stop
kill $PID

# Verify stopped
pgrep -f doca_three | wc -l
```

### Test Concurrent Monitoring
```bash
# Terminal 1: Start switches
./mock_simulator.sh 4 ring

# Terminal 2: Monitor
./monitor_switch.sh

# Terminal 3: Status checks
watch -n 5 './check_status.sh | head -20'

# Terminal 4: Stats
./collect_stats.sh &
```

### Test Scale
```bash
# Try different counts
for n in 2 4 8 16; do
    echo "Testing $n switches..."
    ./mock_simulator.sh $n ring &
    sleep 5
    count=$(pgrep -f doca_three | wc -l)
    echo "Running: $count"
    pkill -f mock_simulator
    sleep 2
done
```

---

## What You CANNOT Test ❌

### Network Features (Need Real Hardware)

| Feature | Mock | Real DOCA |
|---------|------|-----------|
| ping | ❌ | ✅ |
| iperf | ❌ | ✅ |
| tcpdump | ❌ | ✅ |
| Packet forwarding | ❌ | ✅ |
| MAC learning | ❌ | ✅ |
| DOCA Flow stats | ❌ | ✅ |
| Hardware offload | ❌ | ✅ |

### Why Mock Switches Don't Support Networking

Mock switches are **bash processes**:
```bash
# Actual mock switch code:
while true; do
    sleep 10
done
```

**No network stack, no interfaces, no packet processing!**

---

## Testing Real DOCA Switches

### On BlueField DPU (When Available)

```bash
# Find devices
lspci | grep Mellanox

# Start real switch
sudo ./three_port_switch/build/doca_three_port_switch \
    -a 03:00.0 \
    -a 03:00.1 \
    -a 03:00.2 --

# NOW you can:
ping <switch_ip>              ✅ Works
iperf -s                      ✅ Works
tcpdump -i enp3s0f0           ✅ Works
doca_flow_query --stats       ✅ Works
```

---

## Summary

### ✅ Mock Testing Capabilities

**What works:**
- Process management
- Monitoring tools
- Resource tracking
- Statistics collection
- Scalability testing
- Lifecycle management

**Run test suite:**
```bash
./test_mock_infrastructure.sh
```

### ❌ Not Available (Needs Hardware)

**What doesn't work:**
- ping/iperf
- Network traffic
- Packet forwarding
- DOCA Flow features

**Need real switches:**
```bash
# Deploy to BlueField DPU
sudo ./doca_three_port_switch -a <devices> --
```

---

## Quick Commands

```bash
# Start mock testing
./mock_simulator.sh 4 ring

# Check status
./check_status.sh

# Monitor
./monitor_switch.sh

# Run tests
./test_mock_infrastructure.sh

# Cleanup
pkill -f doca_three
```

---

**Bottom Line:** Mock switches test **process infrastructure**, NOT networking. For ping/iperf, deploy to BlueField DPU!
