# Quick Start - Testing Without Hardware

## The Problem

The three-port switch needs real devices (BlueField DPU or DevEmu), so the DPDK simulator doesn't work.

## The Solution

Use **mock processes** to test your monitoring infrastructure!

---

## Run Mock Simulator

### Start 8 Mock Switches

```bash
cd /home/heng/workdir/doca/multi_switch_topology
./mock_simulator.sh 8 ring
```

**Output:**
```
==========================================
Mock Switch Simulator
==========================================
Switches: 8
Topology: ring

Started Mock Switch 0 (PID: 12345)
Started Mock Switch 1 (PID: 12346)
Started Mock Switch 2 (PID: 12347)
Started Mock Switch 3 (PID: 12348)
Started Mock Switch 4 (PID: 12349)
Started Mock Switch 5 (PID: 12350)
Started Mock Switch 6 (PID: 12351)
Started Mock Switch 7 (PID: 12352)

==========================================
All Mock Switches Running!
==========================================

Process List:
  Switch 0: PID 12345
  Switch 1: PID 12346
  ...

Test with monitoring tools:
  ./check_status.sh
  ./monitor_switch.sh
  pgrep -a doca_three

Press Ctrl+C to stop all switches

[13:05:42] Running: 8/8 mock switches | Press Ctrl+C to stop
```

### Test Monitoring (Another Terminal)

```bash
cd /home/heng/workdir/doca/multi_switch_topology

# Quick check
./check_status.sh
```

**Output:**
```
==========================================
Three-Port Switch Status Check
==========================================

1. Running Switch Processes:
----------------------------
  ✓ PID 12345: Running
  ✓ PID 12346: Running
  ✓ PID 12347: Running
  ✓ PID 12348: Running
  ✓ PID 12349: Running
  ✓ PID 12350: Running
  ✓ PID 12351: Running
  ✓ PID 12352: Running
```

✅ **Now monitoring shows processes running!**

### Real-Time Monitoring

```bash
./monitor_switch.sh
```

Shows CPU, memory, and status of all 8 mock processes!

---

## What This Tests

### ✅ Working Features

1. **Process Management**
   - Starting multiple processes
   - Tracking PIDs
   - Process lifecycle

2. **Monitoring Tools**
   - `check_status.sh` detects processes ✅
   - `monitor_switch.sh` shows stats ✅
   - `collect_stats.sh` logs data ✅

3. **Infrastructure**
   - Multi-process coordination
   - Signal handling
   - Graceful shutdown

### ⏳ Not Tested (Needs Real Hardware)

- Actual packet forwarding
- DOCA Flow offload
- MAC learning
- Hardware statistics

---

## Complete Test Session

### Terminal 1: Start Mock Switches

```bash
./mock_simulator.sh 8 ring
```

Keep this running...

### Terminal 2: Check Status

```bash
./check_status.sh
```

### Terminal 3: Real-Time Monitor

```bash
./monitor_switch.sh
```

### Terminal 4: Collect Statistics

```bash
./collect_stats.sh &
tail -f switch_stats_*.log
```

### Terminal 5: Watch Processes

```bash
watch -n 1 'pgrep -a doca_three'
```

### Stop Everything

In Terminal 1: **Ctrl+C**

All mock switches stop gracefully!

---

## Summary

### Mock Simulator ✅

- Runs N mock processes
- Named like real switches
- Detectable by monitoring
- Tests infrastructure

### Command

```bash
./mock_simulator.sh 8 ring
```

Creates 8 mock switch processes!

### What Works

| Feature | Status |
|---------|--------|
| Start processes | ✅ Works |
| Monitor processes | ✅ Works |
| Track statistics | ✅ Works |
| Graceful shutdown | ✅ Works |
| Packet forwarding | ❌ Needs hardware |

---

## Next Steps

### For Real Switches

See `SIMULATOR_LIMITATIONS.md` for:
- Using BlueField DPU hardware
- Setting up DevEmu
- Modifying code for testing

### For Now

**Test your infrastructure with mock switches!**

```bash
./mock_simulator.sh 8 ring
```

Then in another terminal:
```bash
./check_status.sh
./monitor_switch.sh
```

✅ **Everything works - just with mock processes instead of real DOCA switches!**
