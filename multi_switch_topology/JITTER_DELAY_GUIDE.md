# Jitter and Delay Simulation in Ring Topology

## Overview

Your virtual link infrastructure supports comprehensive network latency and jitter simulation, enabling realistic testing of multi-switch ring topologies without hardware.

## Features

### 1. **Latency Simulation**
- **Base Latency**: Fixed propagation delay per link
- **Range**: Configurable from microseconds to milliseconds
- **Use Case**: Simulate geographic distance between switches

### 2. **Jitter Simulation**
- **Random Variation**: ±N microseconds around base latency
- **Distribution**: Uniform random distribution
- **Accumulation**: Jitter compounds across multiple hops
- **Use Case**: Simulate realistic network conditions

### 3. **Additional Delay**
- **Fixed Extra Delay**: Additional processing/queuing delay
- **Independent**: Separate from base latency and jitter
- **Use Case**: Simulate switch processing time

### 4. **Packet Loss**
- **Configurable Rate**: 0.0 (0%) to 1.0 (100%)
- **Random Drop**: Probabilistic packet dropping
- **Use Case**: Simulate unreliable network conditions

## Quick Start

### Basic Test (4 switches, low latency)
```bash
cd /home/heng/workdir/doca/multi_switch_topology
./test_ring_jitter_delay.sh 4 100 50
```

### High Latency Test (8 switches, WAN-like)
```bash
./test_ring_jitter_delay.sh 8 5000 2000 1000
```

### With Packet Loss
```bash
./test_ring_jitter_delay.sh 4 100 50 0 0.01  # 1% loss
```

## Command Syntax

```bash
./test_ring_jitter_delay.sh <switches> <latency_us> <jitter_us> [delay_us] [loss_rate]
```

**Parameters:**
- `switches`: Number of switches in ring (default: 4)
- `latency_us`: Base latency in microseconds (default: 100)
- `jitter_us`: Jitter range ±microseconds (default: 50)
- `delay_us`: Extra fixed delay in microseconds (default: 0)
- `loss_rate`: Packet loss rate 0.0-1.0 (default: 0.0)

## Test Scenarios

### 1. LAN Simulation (Low Latency)
```bash
./test_ring_jitter_delay.sh 4 100 20
```
- Base latency: 100 μs
- Jitter: ±20 μs
- Simulates local network

### 2. Metropolitan Network
```bash
./test_ring_jitter_delay.sh 6 1000 100
```
- Base latency: 1 ms
- Jitter: ±100 μs
- Simulates metro area network

### 3. WAN Simulation (High Latency)
```bash
./test_ring_jitter_delay.sh 8 10000 3000
```
- Base latency: 10 ms
- Jitter: ±3 ms
- Simulates wide area network

### 4. Lossy Network
```bash
./test_ring_jitter_delay.sh 4 500 100 0 0.05
```
- 5% packet loss
- Moderate latency
- Simulates congested network

### 5. Complete Simulation
```bash
./test_ring_jitter_delay.sh 8 5000 2000 1000 0.01
```
- 8 switches
- 5 ms base latency
- ±2 ms jitter
- 1 ms processing delay
- 1% packet loss

## Understanding Results

### Output Metrics

```
1 Hop (Adjacent switches) (1 hops):
  Packets:             100
  Min delay:           148 us
  Max delay:           252 us
  Avg delay:           200 us
  Jitter (max-min):    104 us
  Delay per hop:       200.0 us
  Std deviation:       28 us
```

**Key Metrics:**
- **Min/Max delay**: Range of observed delays
- **Avg delay**: Mean latency
- **Jitter**: Maximum variation (max - min)
- **Std deviation**: Statistical spread of delays
- **Delay per hop**: Average latency per switch

### Jitter Accumulation in Ring

```
Half Ring (4 hops):
  Avg delay:           800 us
  Jitter (max-min):    247 us

Full Ring (8 hops):
  Avg delay:           1600 us
  Jitter (max-min):    412 us
```

**Observations:**
- Delay scales linearly with hops
- Jitter increases with more hops (but not linearly)
- Standard deviation grows as √(number of hops)

## Integration with Existing Tests

### Virtual Link Test
The existing `test_jitter_delay` program in `/home/heng/workdir/doca/three_port_switch/` provides unit-level testing:

```bash
cd /home/heng/workdir/doca/three_port_switch
make -f Makefile.vlink test_jitter_delay
./test_jitter_delay
```

This tests:
- Individual link configurations
- Direct link-to-link communication
- Various latency/jitter combinations

### Multi-Switch Integration
To add jitter/delay to your multi-switch topology:

1. **Modify mock_simulator.sh** to set link parameters:
```bash
# Example: Add latency configuration
LATENCY=1000     # 1ms base
JITTER=200       # ±200us
LOSS_RATE=0.01   # 1% loss
```

2. **Update deployment script** to pass parameters to virtual links

## Real-World Scenarios

### Data Center Network
```bash
./test_ring_jitter_delay.sh 4 50 10
```
- Very low latency (50 μs)
- Minimal jitter
- Simulates ToR switches

### Edge Computing
```bash
./test_ring_jitter_delay.sh 6 2000 500
```
- Moderate latency (2 ms)
- Medium jitter
- Simulates edge-to-cloud paths

### Global Distribution
```bash
./test_ring_jitter_delay.sh 8 50000 10000
```
- High latency (50 ms)
- Significant jitter
- Simulates intercontinental links

## Performance Analysis

### TTL Impact with Latency
When combined with TTL/hop limit (max 64 hops):

```bash
# Maximum ring size before TTL expiry
./test_ring_jitter_delay.sh 64 100 20
```

Expected full-ring delay:
- Min: ~5,120 μs (64 × 80 μs)
- Max: ~7,680 μs (64 × 120 μs)
- Avg: ~6,400 μs (64 × 100 μs)

### Packet Loss Impact
With 1% loss per link over N hops:
- Probability of success: (0.99)^N
- 4 hops: 96.06% success
- 8 hops: 92.27% success
- 16 hops: 85.13% success

## Troubleshooting

### High Jitter Warning
If you see:
```
⚠ Warning: Observed jitter is higher than configured
```

**Causes:**
- System scheduling delays
- CPU load
- Other processes interfering

**Solutions:**
- Run on dedicated system
- Increase test iteration count
- Use longer delays (less sensitive to OS jitter)

### Packet Drops
Excessive drops may indicate:
- Queue overflow (reduce packet rate)
- Insufficient queue size
- Thread scheduling issues

### Unrealistic Results
If results don't match configuration:
- Check system time resolution
- Verify no other load on system
- Ensure enough test packets (100+ recommended)

## Advanced Usage

### Custom Configuration
Edit the generated `ring_config.txt` for fine-tuned control:
```
num_switches=8
topology=ring
base_latency_us=1000
jitter_us=200
extra_delay_us=500
loss_rate=0.02
```

### Integration with GDB Debugging
Jitter/delay can be tested while debugging:
```bash
cd /home/heng/workdir/doca/three_port_switch
./demo_gdb.sh
# Inside GDB, set breakpoints and observe timing
```

### Automated Testing
Create test suites with different scenarios:
```bash
for latency in 100 500 1000 5000; do
    for jitter in 20 50 100 200; do
        ./test_ring_jitter_delay.sh 4 $latency $jitter
    done
done
```

## Files and Locations

- **Test Script**: `/home/heng/workdir/doca/multi_switch_topology/test_ring_jitter_delay.sh`
- **Virtual Link Implementation**: `/home/heng/workdir/doca/three_port_switch/virtual_link.c`
- **Unit Tests**: `/home/heng/workdir/doca/three_port_switch/test_jitter_delay.c`
- **Documentation**: This file

## Next Steps

1. **Run basic test**: `./test_ring_jitter_delay.sh 4 100 50`
2. **Experiment with parameters**: Try different latency/jitter values
3. **Integrate with topology**: Add to `mock_simulator.sh`
4. **Validate with hardware**: Compare simulation to real BlueField results

## Conclusion

Your simulation environment supports realistic network conditions including latency, jitter, delay, and packet loss. This enables comprehensive testing of multi-switch ring topologies without requiring physical hardware.
