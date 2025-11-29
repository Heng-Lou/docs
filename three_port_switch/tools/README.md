# QoS Monitoring and Testing Tools

This directory contains tools for testing and monitoring the DPDK switch QoS implementation.

## Tools Overview

### 1. monitor_qos.sh
Real-time QoS statistics monitor with auto-refresh.

```bash
./tools/monitor_qos.sh
```

**Features:**
- Live statistics from all switches
- 5-second auto-refresh
- Shows per-queue enqueue/dequeue/drop counters
- Queue depth visualization

### 2. quick_qos_test.sh
Quick validation test for QoS functionality.

```bash
./tools/quick_qos_test.sh
```

**What it tests:**
- DSCP classification into correct queues
- Basic weighted round-robin scheduling
- Per-queue counters

**Expected output:**
```
Priority Queue | DSCP | Packets
---------------|------|--------
EF (Highest)   | 46   | 20
AF4x           | 34   | 20
AF1x           | 10   | 20
Best Effort    | 0    | 20
```

### 3. test_qos_differentiation.sh
Comprehensive QoS differentiation and stress test.

```bash
./tools/test_qos_differentiation.sh
```

**Test scenarios:**
1. **Normal Load**: Verifies all priorities work under normal conditions
2. **High Load**: Demonstrates queue overflow and drop behavior
3. **Starvation Test**: Proves high priority traffic is protected during congestion

**Use cases:**
- Performance validation
- Queue tuning
- Demonstrating QoS benefits

## QoS Queue Mapping

| Queue | Priority    | DSCP Values | Use Case              |
|-------|-------------|-------------|-----------------------|
| Q7    | EF/Highest  | 46          | Voice, critical apps  |
| Q6    | AF4x        | 32-34       | Video streaming       |
| Q5    | AF3x        | 24-26       | High-priority data    |
| Q4    | AF2x        | 16-18       | Medium-priority data  |
| Q3    | AF1x        | 8-10        | Bulk data             |
| Q0-2  | Best Effort | 0           | Default traffic       |

## DSCP Marking Examples

```bash
# Best Effort (Queue 0)
ping 10.0.3.2

# AF1x (Queue 3) - DSCP 10 = 0x28
ping -Q 0x28 10.0.3.2

# AF4x (Queue 6) - DSCP 34 = 0x88
ping -Q 0x88 10.0.3.2

# EF (Queue 7) - DSCP 46 = 0xb8
ping -Q 0xb8 10.0.3.2
```

## Weighted Round Robin Weights

The scheduler uses these weights for fair scheduling:

```c
Queue 0: Weight 1   (1/255 = 0.39%)
Queue 1: Weight 2   (2/255 = 0.78%)
Queue 2: Weight 4   (4/255 = 1.57%)
Queue 3: Weight 8   (8/255 = 3.14%)
Queue 4: Weight 16  (16/255 = 6.27%)
Queue 5: Weight 32  (32/255 = 12.55%)
Queue 6: Weight 64  (64/255 = 25.10%)
Queue 7: Weight 128 (128/255 = 50.20%)
```

High priority queues get exponentially more scheduling opportunities.

## Monitoring Tips

### Watch for Queue Drops
```bash
watch -n 1 'tail -20 logs/sw2.log | grep -A 10 "QoS Queues"'
```

### Check Total Throughput
```bash
watch -n 1 'tail -5 logs/sw*.log | grep "RX="'
```

### Monitor Specific Queue
```bash
watch -n 1 'tail -30 logs/sw2.log | grep "Q7"'
```

## Troubleshooting

**No QoS statistics showing:**
- Check if switches are running: `pgrep -f three_port_switch`
- Verify logs are being written: `ls -lh logs/`
- Wait for 10-second stats interval

**All packets in Queue 0:**
- Check DSCP marking: `sudo tcpdump -i veth_s1_h1 -n -vv icmp`
- Verify ToS field is set correctly

**High drop rates:**
- Increase `QOS_QUEUE_SIZE` in source code
- Reduce traffic load
- Check for MAC table misconfigurations

## Integration with CI/CD

Example test in CI pipeline:

```bash
#!/bin/bash
# Run QoS validation
./tools/quick_qos_test.sh

# Check for packet drops
DROPS=$(tail -20 logs/sw2.log | grep "Drop=" | tail -1 | sed 's/.*Drop=//' | cut -d' ' -f1)

if [ "$DROPS" -gt 10 ]; then
    echo "❌ Too many drops: $DROPS"
    exit 1
fi

echo "✓ QoS test passed"
```

## Future Enhancements

Potential additions to the QoS system:
1. Dynamic queue size adjustment
2. Congestion notification (ECN)
3. Traffic shaping/policing
4. Per-flow QoS
5. Priority-based flow control
