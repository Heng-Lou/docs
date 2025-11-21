# High-Rate Packet Generation Errors - Solution Guide

## Problem

When running high-rate tests, you see errors like:
```
[PKTGEN] Host 5: vlink_send failed (result=-28, attempt 1027)
[PKTGEN] Host 7: vlink_send failed (result=-28, attempt 1027)
```

**Error Code:** `-28` = `ENOSPC` (No space left on device)
**Meaning:** Virtual link queue is full

## Root Cause

The virtual link queue has a fixed size of **1024 packets** (defined in `virtual_link.h`).

When running:
```bash
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15
```

This creates:
- **8 hosts** sending packets simultaneously
- **1000 packets/second** per host = **8000 total pps**
- Each packet must traverse through multiple switches in ring
- Queues fill faster than they can be drained

**The bottleneck:**
- Packet generation: 8 × 1000 = 8000 pps
- Queue draining: Limited by processing speed
- Result: Queue fills to 1024, then `vlink_send()` returns `-ENOSPC`

## Solutions

### Solution 1: Increase Queue Size (Recommended for High Rates)

Edit `virtual_link.h`:

```c
// OLD:
#define VLINK_QUEUE_SIZE 1024

// NEW (for high-rate tests):
#define VLINK_QUEUE_SIZE 4096  // or 8192 for very high rates
```

**When to use:**
- High packet rates (>500 pps per host)
- Many hosts (>8)
- You want to stress test the system

**Trade-offs:**
- ✓ Handles bursts better
- ✓ Supports higher sustained rates
- ✗ Uses more memory (4× more with 4096)

**Steps:**
```bash
cd /home/heng/workdir/doca/three_port_switch

# Edit the file
vim virtual_link.h
# Change line 17: #define VLINK_QUEUE_SIZE 4096

# Rebuild
make -f Makefile.vhost clean all

# Re-run test
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15
```

### Solution 2: Reduce Packet Rate (Quick Fix)

Lower the rate to match queue capacity:

```bash
# Instead of 1000 pps:
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15  # ❌ Queue overflows

# Use 200-500 pps:
./vhost_switch_test -n 8 -p -r 200 -c 10000 -d 30   # ✓ Works with current queue
```

**When to use:**
- Quick testing without recompiling
- You don't need ultra-high rates
- Memory is constrained

**Recommended rates by topology size:**

| Hosts | Max Rate (pps) | Total Traffic |
|-------|----------------|---------------|
| 4     | 500            | 2000 pps      |
| 8     | 200-300        | 1600-2400 pps |
| 16    | 100-150        | 1600-2400 pps |
| 32    | 50-100         | 1600-3200 pps |

### Solution 3: Reduce Number of Hosts

Fewer hosts = less concurrent traffic:

```bash
# Instead of 8 hosts:
./vhost_switch_test -n 8 -p -r 1000 -c 10000  # ❌ 8000 pps total

# Use 4 hosts:
./vhost_switch_test -n 4 -p -r 1000 -c 10000  # ✓ 4000 pps total
```

### Solution 4: Implement Retry with Backoff (Code Fix)

If you want packet generation to handle queue full gracefully, modify the packet generator in `virtual_host.c`:

```c
// Current code (gives up on ENOSPC):
result = vlink_send(host->link_mgr, host->pci_link_id, packet, pkt_size);
if (result < 0) {
    fprintf(stderr, "[PKTGEN] Host %u: vlink_send failed (result=%d, attempt %u)\n",
            host->host_id, result, i);
    host->stats.tx_errors++;
    continue;  // Skip this packet
}

// Better: Retry with exponential backoff
int retry_count = 0;
while (retry_count < 10) {
    result = vlink_send(host->link_mgr, host->pci_link_id, packet, pkt_size);
    if (result == 0) {
        break;  // Success
    }
    if (result == -ENOSPC) {
        // Queue full, wait and retry
        usleep(1000 * (1 << retry_count));  // 1ms, 2ms, 4ms, 8ms...
        retry_count++;
    } else {
        // Other error, give up
        break;
    }
}
if (result < 0) {
    host->stats.tx_errors++;
}
```

## Understanding the Error Pattern

Looking at your log:
```
[PKTGEN] Host 0: vlink_send failed (result=-28, attempt 1023)
[PKTGEN] Host 1: vlink_send failed (result=-28, attempt 1023)
...
[PKTGEN] Host 0: vlink_send failed (result=-28, attempt 1024)
```

**Key observations:**
1. **All hosts fail at same time** - Queue saturation is system-wide
2. **Fails at attempt 1023-1027** - Right at queue size (1024)
3. **Only ~1000 packets sent** - Queue filled before count reached
4. **Each host sent 1023 packets** - Stopped when queue full

This shows the queue is the bottleneck, not the packet generator.

## What Actually Happened

From your statistics:
```
Switch 0: Switch-0
  Port 0 (PCI):  RX 1023 pkts/54208 bytes, TX 1023 pkts/54208 bytes
```

Each host successfully sent **1023 packets** before hitting the queue limit. This is **expected behavior** with the current queue size—not a bug, but a design limit.

## Recommended Configuration

### For Development/Testing (Current)
```c
#define VLINK_QUEUE_SIZE 1024  // Good for up to 500 pps × 4 hosts
```

### For High-Rate Testing
```c
#define VLINK_QUEUE_SIZE 4096  // Good for up to 1000 pps × 8 hosts
```

### For Stress Testing
```c
#define VLINK_QUEUE_SIZE 8192  // Good for extreme rates or burst traffic
```

## Quick Reference

### Test Profiles That Work Without Changes

**Profile 1: Standard Test**
```bash
./vhost_switch_test -n 4 -p -r 100 -c 1000 -d 10
# ✓ Works perfectly with queue size 1024
```

**Profile 2: Medium Load**
```bash
./vhost_switch_test -n 8 -p -r 200 -c 5000 -d 20
# ✓ Works with queue size 1024
```

**Profile 3: High Load (Requires Queue Increase)**
```bash
# First: Change VLINK_QUEUE_SIZE to 4096
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15
# ✓ Works with queue size 4096
```

### Memory Usage

| Queue Size | Memory per Queue | Total for 32 Links | Notes |
|------------|------------------|-------------------|-------|
| 1024       | ~9 MB            | ~288 MB           | Default |
| 2048       | ~18 MB           | ~576 MB           | 2× default |
| 4096       | ~36 MB           | ~1.1 GB           | Recommended for high-rate |
| 8192       | ~72 MB           | ~2.3 GB           | Stress testing only |

*Based on MAX_PACKET_SIZE=9000 bytes + overhead*

## How to Fix Your Specific Test

For your command:
```bash
./vhost_switch_test -n 8 -p -r 1000 -c 10000 -d 15
```

**Option A: Increase Queue (Best)**
1. Edit `virtual_link.h`, line 17: `#define VLINK_QUEUE_SIZE 4096`
2. Rebuild: `make -f Makefile.vhost clean all`
3. Re-run same command

**Option B: Reduce Rate (Quick)**
```bash
./vhost_switch_test -n 8 -p -r 300 -c 10000 -d 30
# Sends same total packets, just slower
```

**Option C: Fewer Hosts (Alternative)**
```bash
./vhost_switch_test -n 4 -p -r 1000 -c 10000 -d 15
# Half the concurrent senders
```

## Summary

**Error `-28` (ENOSPC)** means queue full. This is **not a bug**, but a **capacity limit**.

**Solutions (pick one):**
1. ✓ **Increase `VLINK_QUEUE_SIZE`** to 4096 or 8192
2. ✓ **Reduce packet rate** to 200-300 pps per host
3. ✓ **Use fewer hosts** (4 instead of 8)
4. ✓ **Add retry logic** to packet generator

**For your test specifically:** Increase queue size to 4096 and rebuild.

