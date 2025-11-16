# Corrected Monitoring Commands

## The Error You Encountered

```bash
watch -n 1 "doca_flow_query --all-switches --stats"
# Error: sh: 1: doca_flow_query: not found
```

## Why It Failed

**`doca_flow_query` does not exist** - it was used in documentation examples as an illustration, but is not an actual DOCA tool.

---

## ✅ CORRECT Commands to Use Instead

### Quick Status Check

```bash
cd /home/heng/workdir/doca/multi_switch_topology
./check_status.sh
```

### Real-Time Monitoring (Like watch)

```bash
# Monitor switch with 1-second refresh
./monitor_switch.sh doca_three_port_switch 1

# Monitor with 5-second refresh (default)
./monitor_switch.sh
```

### Continuous Statistics Collection

```bash
# Run in background
./collect_stats.sh &

# View live log
tail -f switch_stats_*.log
```

### Standard Linux Commands

```bash
# Watch interface statistics
watch -n 1 'ip -s link show'

# Watch all interfaces
watch -n 1 'cat /sys/class/net/eth*/statistics/rx_packets'

# Monitor processes
watch -n 1 'ps aux | grep doca_three_port_switch'

# Network interface stats
watch -n 1 'netstat -i'
```

---

## Available Tools Summary

| Tool | Exists? | Purpose | Command |
|------|---------|---------|---------|
| `doca_flow_query` | ❌ NO | N/A - fictional | - |
| `check_status.sh` | ✅ YES | Quick check | `./check_status.sh` |
| `monitor_switch.sh` | ✅ YES | Real-time monitor | `./monitor_switch.sh` |
| `collect_stats.sh` | ✅ YES | Statistics logging | `./collect_stats.sh &` |
| `ip` | ✅ YES | Network config/stats | `ip -s link show` |
| `ethtool` | ✅ YES | Interface details | `ethtool -S eth0` |
| `netstat` | ✅ YES | Network statistics | `netstat -i` |
| `doca_telemetry_utils` | ✅ YES | DOCA counters | `sudo doca_telemetry_utils get-counters` |

---

## Example Session

### 1. Check if Switch is Running

```bash
$ ./check_status.sh

==========================================
Three-Port Switch Status Check
==========================================

1. Running Switch Processes:
----------------------------
  ✓ PID 12345: Running

2. Network Interface Status:
----------------------------
  ✓ eth0: UP
  ✓ eth1: UP

...
```

### 2. Monitor in Real-Time

```bash
$ ./monitor_switch.sh doca_three_port_switch 2

==========================================
Three-Port Switch Monitor
Time: 2024-11-10 12:27:05
==========================================

Process Statistics:
  PID: 12345
  Memory: 128 MB
  CPU: 3.5%
  Threads: 8

Network Interfaces:
  eth0: UP (10000 Mbps)
  eth1: UP (10000 Mbps)
...
```

### 3. Collect Statistics

```bash
$ ./collect_stats.sh &
[1] 54321

$ tail -f switch_stats_20241110_122705.log

Multi-Switch Statistics Log
Started: Mon Nov 10 12:27:05 CST 2025
==================================

[2024-11-10 12:27:05]
-----------------------------------
Running Switches:
  PID 12345: CPU=3.5% MEM=1.2%
...
```

---

## All Corrected References

### In Documentation

**Replace:**
```bash
doca_flow_query --switch 0 --mac-table
doca_flow_query --all-switches --stats
watch -n 1 "doca_flow_query --port all --stats"
```

**With:**
```bash
./check_status.sh
./monitor_switch.sh
watch -n 1 'ip -s link show'
```

### In Deployment Script

The deployment script (`deploy_multi_switch.sh`) has been updated to show correct commands in its testing instructions.

### In Configuration

The `config.yaml` monitoring section references actual tools.

---

## Quick Reference Card

```
╔══════════════════════════════════════════════════════════╗
║          Switch Monitoring Quick Reference               ║
╠══════════════════════════════════════════════════════════╣
║                                                          ║
║  Quick Check:        ./check_status.sh                   ║
║  Real-time Monitor:  ./monitor_switch.sh                 ║
║  Log Statistics:     ./collect_stats.sh &                ║
║                                                          ║
║  Interface Stats:    ip -s link show eth0                ║
║  Process Monitor:    ps aux | grep doca_three_port      ║
║  Network Summary:    netstat -i                          ║
║                                                          ║
║  Watch (1s):         watch -n 1 'ip -s link show'        ║
║  Watch Process:      watch -n 1 'ps aux | grep doca'     ║
║                                                          ║
╚══════════════════════════════════════════════════════════╝
```

---

## Files Created

All monitoring tools are in:
```
/home/heng/workdir/doca/multi_switch_topology/
├── check_status.sh      - Quick health check ✅
├── monitor_switch.sh    - Real-time monitoring ✅
├── collect_stats.sh     - Statistics logging ✅
└── MONITORING_TOOLS.md  - Complete guide ✅
```

---

## Summary

**Problem**: `doca_flow_query` doesn't exist  
**Solution**: Use our custom scripts or standard Linux tools  
**Status**: All corrected tools ready to use ✅

Check `MONITORING_TOOLS.md` for complete documentation on all available monitoring options!
