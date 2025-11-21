# Testing Quick Start Guide

## Problem: No TX/RX Traffic Shown

If you run `./vhost_switch_test -n 4 -d 5` and see all zeros in statistics, it's because **packet generation is disabled by default**.

## Solution: Add the `-p` Flag

**Wrong (no traffic):**
```bash
./vhost_switch_test -n 4 -d 5
# Result: All zeros - topology created but no packets sent
```

**Correct (with traffic):**
```bash
./vhost_switch_test -n 4 -p -d 5
# Result: Packets flowing, statistics show TX/RX counts
```

---

## Command-Line Options

```
Usage: ./vhost_switch_test [OPTIONS]

Options:
  -n NUM      Number of switches/hosts (default: 4, max: 32)
  -p          Enable packet generation ⭐ REQUIRED FOR TRAFFIC
  -r RATE     Packet generation rate in pps (default: 100)
  -c COUNT    Number of packets to send (default: 100, 0=infinite)
  -d DURATION Run duration in seconds (default: 10)
  -h          Show this help
```

---

## Common Test Scenarios

### 1. Basic Test with Traffic
```bash
./vhost_switch_test -n 4 -p -d 5
```
**Result:**
- 4 hosts, 4 switches in ring
- Each host sends 100 packets (default)
- Runs for 5 seconds
- Shows TX/RX statistics ✓

### 2. Quick Topology Check (No Traffic)
```bash
./vhost_switch_test -n 4 -d 5
```
**Result:**
- Creates topology
- No packet generation
- Just verifies setup works
- All stats will be 0 (this is expected)

### 3. High Rate Stress Test
```bash
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15
```
**Result:**
- 8 hosts/switches
- 1000 packets per second
- 10,000 packets total per host
- 15 second duration

### 4. Continuous Traffic
```bash
./vhost_switch_test -n 4 -p -r 100 -c 0 -d 30
```
**Result:**
- Infinite packet generation (-c 0)
- 100 pps rate
- Runs for 30 seconds
- Press Ctrl+C to stop

### 5. Large Ring Topology
```bash
./vhost_switch_test -n 16 -p -r 50 -d 10
```
**Result:**
- 16 hosts/switches in ring
- 50 pps (slower rate for large topology)
- 10 second test

---

## Using Demo Scripts

The `demo_vhost.sh` script has packet generation **enabled by default**.

### Basic Demo
```bash
./demo_vhost.sh pktgen
```
**Equivalent to:**
```bash
./vhost_switch_test -n 4 -p -r 100 -c 100 -d 15
```

### Other Demo Modes
```bash
./demo_vhost.sh basic      # 4 hosts, no traffic
./demo_vhost.sh ring       # 8 hosts, with traffic
./demo_vhost.sh stress     # High rate test
```

---

## Expected Output (With `-p` Flag)

### Switch Statistics (Should Show Non-Zero)
```
Switch 0: Switch-0
  Port 0 (PCI):  RX 101 pkts/5342 bytes, TX 101 pkts/5342 bytes, Drops 0
  Port 1 (Eth0): RX 0 pkts/0 bytes, TX 101 pkts/5342 bytes, Drops 0
  Port 2 (Eth1): RX 101 pkts/5342 bytes, TX 0 pkts/0 bytes, Drops 0
```

### Host Statistics (Should Show Non-Zero)
```
Host 0: Host-0
  MAC: 02:00:00:00:00:00
  IP: 192.168.1.10
  TX: 100 pkts / 5300 bytes (errors: 0)
  RX: 101 pkts / 5342 bytes (errors: 0, drops: 0)
```

**Note:** Each host sends 100 packets but receives 101 because in a ring topology, it receives packets from other hosts too.

---

## Traffic Flow in Ring Topology

With 4 hosts in ring:

```
Host 0 → Switch 0 → Switch 1 → Host 1
Host 1 → Switch 1 → Switch 2 → Host 2
Host 2 → Switch 2 → Switch 3 → Host 3
Host 3 → Switch 3 → Switch 0 → Host 0
```

Each switch:
- **Port 0 (PCI):** Receives from local host, sends to local host
- **Port 1 (Eth0):** Receives from previous switch in ring
- **Port 2 (Eth1):** Sends to next switch in ring

---

## Troubleshooting

### Problem: All zeros in statistics
**Cause:** Missing `-p` flag
**Solution:** Add `-p` to enable packet generation

### Problem: Fewer packets received than sent
**Cause:** TTL expiration (normal in ring topology)
**Solution:** This is expected behavior - TTL prevents infinite loops

### Problem: Test exits immediately
**Cause:** Invalid parameters
**Solution:** Check that 2 ≤ num ≤ 32

### Problem: Compilation error
**Cause:** Missing build
**Solution:** 
```bash
make -f Makefile.vhost clean all
```

---

## Quick Reference

| What You Want | Command |
|---------------|---------|
| Basic test with traffic | `./vhost_switch_test -n 4 -p -d 5` |
| Topology check only | `./vhost_switch_test -n 4 -d 5` |
| High rate test | `./vhost_switch_test -n 4 -p -r 1000 -c 5000` |
| Large topology | `./vhost_switch_test -n 16 -p -r 50` |
| Continuous traffic | `./vhost_switch_test -n 4 -p -c 0 -d 30` |
| Use demo script | `./demo_vhost.sh pktgen` |

---

## Key Takeaway

**Always use `-p` flag if you want to see TX/RX statistics!**

```bash
# Without -p: Topology only, no packets
./vhost_switch_test -n 4 -d 5           ❌ No traffic

# With -p: Full test with packet generation
./vhost_switch_test -n 4 -p -d 5        ✓ Traffic flows!
```

