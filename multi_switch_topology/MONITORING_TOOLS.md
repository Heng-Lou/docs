# Monitoring Tools for Multi-Switch Network

## Important Note

The command `doca_flow_query` mentioned in earlier documentation **does not exist** as a standalone DOCA tool. This document provides the **actual tools** available for monitoring your three-port switches.

---

## Available Monitoring Tools

### 1. Custom Scripts (Recommended) ✅

We've created three monitoring scripts that work with standard Linux utilities:

#### check_status.sh
Quick health check of switches and network interfaces.

```bash
cd /home/heng/workdir/doca/multi_switch_topology
./check_status.sh
```

**Output:**
- Running switch processes
- Network interface status
- Packet statistics
- System resources
- Error detection

#### monitor_switch.sh
Real-time continuous monitoring (like `watch`).

```bash
./monitor_switch.sh [switch_name] [interval_seconds]

# Examples
./monitor_switch.sh                          # Default: 5s refresh
./monitor_switch.sh doca_three_port_switch 2 # 2s refresh
```

**Displays:**
- Process CPU/memory usage
- Network interface status
- Per-port packet counts
- Error rates
- Real-time updates

#### collect_stats.sh
Background statistics collection to log file.

```bash
./collect_stats.sh &

# Check collected data
tail -f switch_stats_*.log
```

**Logs:**
- Timestamped statistics
- Process resource usage
- Network traffic summary
- Continuous recording

---

### 2. Standard Linux Tools ✅

#### ethtool - Interface Statistics

```bash
# Get detailed stats for an interface
ethtool -S eth0

# Check link status
ethtool eth1

# Get driver info
ethtool -i eth0
```

#### ip - Network Configuration

```bash
# Show all interfaces
ip link show

# Show statistics
ip -s link show eth0

# Show routing table
ip route show
```

#### ifconfig - Classic Interface Tool

```bash
# Show interface stats
ifconfig eth0

# All interfaces
ifconfig -a
```

#### netstat/ss - Connection Statistics

```bash
# Network statistics
netstat -i          # Interface stats
netstat -s          # Protocol stats

# Modern alternative
ss -i               # Interface info
ss -s               # Statistics
```

---

### 3. Process Monitoring ✅

#### ps - Process Status

```bash
# Find switch processes
ps aux | grep doca_three_port_switch

# Detailed resource usage
ps -p <PID> -o pid,ppid,%cpu,%mem,vsz,rss,cmd

# Real-time updates
watch -n 1 "ps aux | grep doca_three_port_switch"
```

#### top/htop - System Monitor

```bash
# Interactive process monitor
top -p $(pgrep -d, doca_three_port_switch)

# Better interface (if installed)
htop -p $(pgrep doca_three_port_switch)
```

#### perf - Performance Analysis

```bash
# CPU profiling
perf top -p $(pgrep doca_three_port_switch)

# Record and analyze
perf record -p $(pgrep doca_three_port_switch) sleep 10
perf report
```

---

### 4. DOCA Tools ✅

#### doca_telemetry_utils

Get DOCA-specific counter information (requires sudo and device access).

```bash
# List available counters
sudo /opt/mellanox/doca/tools/doca_telemetry_utils get-counters

# Get specific counter by name
sudo /opt/mellanox/doca/tools/doca_telemetry_utils [device_pci] <counter_name>

# Example
sudo /opt/mellanox/doca/tools/doca_telemetry_utils 03:00.0 rx_packets
```

#### doca_flow_tune

Flow offload tuning tool.

```bash
# Note: Requires proper setup and permissions
sudo /opt/mellanox/doca/tools/doca_flow_tune [options]
```

#### dpa-ps

Monitor DPA processes (if using DPA).

```bash
/opt/mellanox/doca/tools/dpa-ps
```

---

### 5. System Statistics ✅

#### /sys/class/net Statistics

Direct access to kernel network statistics:

```bash
# Interface statistics
cat /sys/class/net/eth0/statistics/rx_packets
cat /sys/class/net/eth0/statistics/tx_packets
cat /sys/class/net/eth0/statistics/rx_bytes
cat /sys/class/net/eth0/statistics/tx_bytes
cat /sys/class/net/eth0/statistics/rx_errors
cat /sys/class/net/eth0/statistics/tx_errors
cat /sys/class/net/eth0/statistics/rx_dropped
cat /sys/class/net/eth0/statistics/tx_dropped

# Link state
cat /sys/class/net/eth0/operstate
cat /sys/class/net/eth0/speed
```

#### Script to Monitor All Interfaces

```bash
#!/bin/bash
for iface in /sys/class/net/*; do
    name=$(basename $iface)
    if [ "$name" != "lo" ]; then
        echo "$name:"
        echo "  RX: $(cat $iface/statistics/rx_packets) packets"
        echo "  TX: $(cat $iface/statistics/tx_packets) packets"
        echo "  State: $(cat $iface/operstate)"
    fi
done
```

---

## Practical Monitoring Examples

### Example 1: Monitor Switch in Real-Time

```bash
# Using our custom script
./monitor_switch.sh doca_three_port_switch 1
```

Output every 1 second:
```
==========================================
Three-Port Switch Monitor
Time: 2024-11-10 12:25:34
==========================================

Process Statistics:
  PID: 12345
  Memory: 128 MB
  CPU: 5.2%
  Threads: 8

Network Interfaces:
  eth0: UP (10000 Mbps)
  eth1: UP (10000 Mbps)
  eth2: DOWN

Port Statistics:
----------------------------------------
Port eth0:
  RX: 1234567 pkts, 512 MB, 0 errors, 0 dropped
  TX: 2345678 pkts, 1024 MB, 0 errors, 0 dropped

==========================================
Refreshing in 1s... (Ctrl+C to stop)
```

### Example 2: Quick Status Check

```bash
./check_status.sh
```

Output:
```
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
  ✗ eth2: down

3. Packet Statistics:
----------------------------
  eth0: RX=1234567 TX=2345678 packets
  eth1: RX=987654 TX=876543 packets

4. System Resources:
----------------------------
  CPU Load:  0.45, 0.52, 0.48
  Memory: 2.1G/15G
  Hugepages: HugePages_Total:    1024
             HugePages_Free:      512

5. Error Check:
----------------------------
  ✓ No errors detected

==========================================
Check complete: 2024-11-10 12:25:34
==========================================
```

### Example 3: Collect Statistics to File

```bash
# Start collection in background
./collect_stats.sh &
COLLECTOR_PID=$!

# Let it run for a while...
sleep 300  # 5 minutes

# Stop collection
kill $COLLECTOR_PID

# View results
cat switch_stats_*.log
```

### Example 4: Monitor with Standard Tools

```bash
# Watch interface statistics
watch -n 1 'cat /sys/class/net/eth0/statistics/rx_packets; \
            cat /sys/class/net/eth0/statistics/tx_packets'

# Or use ip command
watch -n 1 'ip -s link show eth0'

# Or netstat
watch -n 1 'netstat -i'
```

---

## Corrected Documentation Examples

### Replace This ❌

```bash
# WRONG - This command doesn't exist
watch -n 1 "doca_flow_query --all-switches --stats"
doca_flow_query --switch 0 --mac-table
```

### With This ✅

```bash
# RIGHT - Use our monitoring scripts
./monitor_switch.sh doca_three_port_switch 1

# Or use standard Linux tools
watch -n 1 'ip -s link show'
watch -n 1 'cat /sys/class/net/eth*/statistics/rx_packets'

# Quick status check
./check_status.sh

# Background collection
./collect_stats.sh &
```

---

## Advanced Monitoring

### 1. DPDK Statistics (If Using DPDK Ports)

If your switch uses DPDK ports, monitor with DPDK tools:

```bash
# DPDK proc info tool (if available)
dpdk-proc-info -- --stats

# Or use telemetry
dpdk-telemetry.py
```

### 2. Performance Counter Monitoring

```bash
# CPU performance counters
perf stat -p $(pgrep doca_three_port_switch)

# Cache misses
perf stat -e cache-misses,cache-references -p $(pgrep doca_three_port_switch)

# Branch prediction
perf stat -e branches,branch-misses -p $(pgrep doca_three_port_switch)
```

### 3. Network Performance Testing

```bash
# Bandwidth test with iperf3
# On one host (server)
iperf3 -s

# On another host (client)
iperf3 -c 10.0.0.1 -t 30

# Latency test with ping
ping -c 100 -i 0.01 10.0.0.2 | tail -1
```

### 4. Continuous Monitoring Dashboard

Create a simple dashboard with tmux:

```bash
#!/bin/bash
# dashboard.sh - Multi-pane monitoring

tmux new-session -d -s switch_monitor

# Pane 0: Process monitor
tmux send-keys -t switch_monitor "watch -n 1 'ps aux | grep doca_three_port_switch'" C-m

# Split and create pane 1: Network stats
tmux split-window -h -t switch_monitor
tmux send-keys -t switch_monitor "watch -n 1 'ip -s link show'" C-m

# Split and create pane 2: System resources
tmux split-window -v -t switch_monitor
tmux send-keys -t switch_monitor "htop -p \$(pgrep -d, doca_three_port_switch)" C-m

# Attach to session
tmux attach -t switch_monitor
```

---

## Monitoring Scripts Summary

| Script | Purpose | Usage | Output |
|--------|---------|-------|--------|
| **check_status.sh** | Quick health check | `./check_status.sh` | One-time report |
| **monitor_switch.sh** | Real-time monitor | `./monitor_switch.sh [interval]` | Continuous display |
| **collect_stats.sh** | Log statistics | `./collect_stats.sh &` | File logging |

All scripts work with **standard Linux utilities** and don't require special DOCA tools!

---

## Troubleshooting

### "Command not found" errors

If you see errors about missing commands:

1. **Use our custom scripts** - They work without special tools
2. **Check tool availability**: `which <command>`
3. **Install if needed**: `sudo apt-get install <package>`

### Permission errors

Some DOCA tools require sudo:

```bash
# If you get permission denied
sudo /opt/mellanox/doca/tools/doca_telemetry_utils get-counters
```

### No statistics showing

If network statistics are all zeros:

1. Check if switch is actually running
2. Verify traffic is flowing
3. Check correct interface names
4. Ensure ports are UP

---

## Summary

### Correct Monitoring Tools ✅

1. **check_status.sh** - Quick status check
2. **monitor_switch.sh** - Real-time monitoring
3. **collect_stats.sh** - Statistics logging
4. **Standard Linux tools** - ip, ethtool, netstat, etc.
5. **DOCA telemetry** - For advanced metrics (requires sudo)

### Don't Use ❌

- `doca_flow_query` - Doesn't exist
- Fictional monitoring commands from examples

### Best Practice

Use our provided scripts or standard Linux networking tools for reliable monitoring without dependencies on non-existent utilities.

---

**All monitoring scripts are ready to use in `/home/heng/workdir/doca/multi_switch_topology/`!**
