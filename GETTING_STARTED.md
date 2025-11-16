# Getting Started with NVIDIA DOCA Development

This guide walks you through setting up and using this DOCA development project from scratch.

## Prerequisites

### Minimal (For Virtual Simulation)
- Linux system (Ubuntu 20.04+ recommended)
- GCC: `sudo apt install build-essential`
- Make: `sudo apt install make`
- Git: `sudo apt install git`

### For Real Hardware
- NVIDIA BlueField DPU (BlueField-2 or BlueField-3)
- DOCA SDK 2.0+: Download from [NVIDIA DOCA Downloads](https://developer.nvidia.com/networking/doca)
- DPDK 22.11+: Included with DOCA SDK
- Mellanox OFED drivers

## Quick Start (5 Minutes)

### Step 1: Clone the Repository

```bash
git clone <your-repo-url>
cd doca
```

### Step 2: Run Virtual Host Demo

No hardware required!

```bash
cd three_port_switch
make -f Makefile.vhost
./demo_vhost.sh pktgen
```

You should see:
- 4 virtual hosts created
- 4 switches in ring topology
- Packet generation and forwarding
- Real-time statistics

Press Ctrl+C to stop.

### Step 3: Explore Other Tests

```bash
# Test TTL/hop limit on ring
./test_ttl.sh

# Test link failure scenarios
./test_link_down.sh

# Run code coverage analysis
./test_coverage.sh
```

## Detailed Setup

### 1. Environment Setup

```bash
# Source the environment script
source ./setup_environment.sh

# Verify DOCA installation (if you have hardware)
./check_p4_availability.sh
```

### 2. Build All Components

#### Virtual Simulation (No Hardware)

```bash
cd three_port_switch

# Build simulator for GDB debugging
make -f Makefile.sim

# Build virtual host tests
make -f Makefile.vhost

# Build virtual link unit tests
make -f Makefile.vlink
```

#### DOCA Flow Examples (Requires DOCA SDK)

```bash
# From project root
meson build
ninja -C build

# Or use Makefile
make
```

#### DPA Samples (Requires DOCA SDK)

```bash
cd dpa_kernel_launch
./build.sh
```

### 3. Run Tests

#### Virtual Host Tests

```bash
cd three_port_switch

# Basic test: 4 hosts, packet generation
./vhost_switch_test -n 4 -g -r 100 -c 1000 -d 10

# Ping test: 8 hosts
./vhost_switch_test -n 8 -p -c 100 -d 15

# Both ping and packet gen
./vhost_switch_test -n 6 -p -g -r 50 -c 500 -d 20
```

**Command line options:**
- `-n NUM`: Number of hosts/switches (2-16)
- `-p`: Enable ping test
- `-g`: Enable packet generation
- `-r RATE`: Packets per second (default: 10)
- `-c COUNT`: Total packets (default: 100)
- `-d DURATION`: Test duration in seconds (default: 10)
- `-v`: Verbose output

#### Multi-Switch Topologies

```bash
cd multi_switch_topology

# Ring: Each switch connects to next, last connects to first
./mock_simulator.sh 4 ring

# Line: Switches in a line
./mock_simulator.sh 6 line

# Mesh: Every switch connects to every other
./mock_simulator.sh 8 mesh

# Monitor (in another terminal)
./monitor_switches.sh
```

#### TTL Testing

```bash
cd three_port_switch
./test_ttl.sh
```

This tests:
- Packets loop in ring without TTL: SHOULD LOOP
- Packets with TTL=1: Dropped at first hop
- Packets with TTL=3: Forward 2 hops then drop
- TTL decrements correctly

#### Link Failure Testing

```bash
cd three_port_switch
./test_link_down.sh
```

Tests recovery from link failures in ring topology.

### 4. GDB Debugging (DPA Simulator)

```bash
cd three_port_switch

# Build simulator
make -f Makefile.sim

# Run under GDB
gdb ./three_port_switch_sim

# In GDB:
(gdb) break process_packet
(gdb) run
(gdb) print packet->src_mac
(gdb) continue
```

See [DPA_GDB_DEBUG_GUIDE.md](DPA_GDB_DEBUG_GUIDE.md) for advanced debugging.

## Understanding the Architecture

### Virtual Host Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Host 0    │     │   Host 1    │     │   Host 2    │
│ MAC: 00:00  │     │ MAC: 00:01  │     │ MAC: 00:02  │
│ IP: .0.10   │     │ IP: .0.11   │     │ IP: .0.12   │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │ PCI               │ PCI               │ PCI
       │                   │                   │
┌──────▼──────┐     ┌──────▼──────┐     ┌──────▼──────┐
│  Switch 0   │◄────┤  Switch 1   │◄────┤  Switch 2   │
│  PCI | ETH  │ ETH │  PCI | ETH  │ ETH │  PCI | ETH  │
└─────────┬───┘     └─────────┬───┘     └──────┬──────┘
          │                   │                 │
          └───────────────────┴─────────────────┘
                    Ring Topology
```

### Three-Port Switch

Each switch has:
- **Port 0 (PCI)**: Connects to virtual host
- **Port 1 (ETH)**: Ethernet connection to previous switch
- **Port 2 (ETH)**: Ethernet connection to next switch

Features:
- MAC learning and forwarding
- IP QoS queues (priority-based)
- VLAN QoS queues
- RSS (Receive Side Scaling)
- Hairpin queues
- TTL decrement for loop prevention

### Packet Flow

1. **Host generates packet** → Sends to PCI port
2. **Switch receives on PCI** → Lookup destination MAC
3. **Forward decision**:
   - If MAC learned: Forward to specific port
   - If unknown: Flood to all ports except source
4. **TTL check**: Decrement and drop if TTL=0
5. **Next switch** → Repeat from step 2

## Common Workflows

### Workflow 1: Test New Feature in Simulation

```bash
# 1. Edit source
vi three_port_switch/three_port_switch.c

# 2. Update simulator
vi three_port_switch/three_port_switch_sim.c

# 3. Rebuild
cd three_port_switch
make -f Makefile.sim clean
make -f Makefile.sim

# 4. Debug with GDB
gdb ./three_port_switch_sim
(gdb) break your_new_function
(gdb) run

# 5. Test in virtual network
make -f Makefile.vhost clean
make -f Makefile.vhost
./vhost_switch_test -n 4 -g -v
```

### Workflow 2: Test Multi-Switch Topology

```bash
# 1. Start switches
cd multi_switch_topology
./mock_simulator.sh 8 ring

# 2. Monitor (separate terminal)
./monitor_switches.sh

# 3. Check logs
tail -f sim_logs/switch_*.log

# 4. Stop
pkill -f mock_switch
```

### Workflow 3: Measure Code Coverage

```bash
cd three_port_switch
./test_coverage.sh

# View results
firefox coverage_html/index.html
```

## Next Steps

### Beginner
1. Read [THREE_PORT_SWITCH_SUMMARY.md](THREE_PORT_SWITCH_SUMMARY.md)
2. Run `demo_vhost.sh` and understand output
3. Modify packet generation parameters
4. Try different topologies (ring, line, mesh)

### Intermediate
1. Study [TTL_FEATURE_COMPLETE.md](TTL_FEATURE_COMPLETE.md)
2. Implement custom QoS policy
3. Add new packet processing features
4. Test with link failures

### Advanced
1. Read [DPA_PROGRAMMING.md](DPA_PROGRAMMING.md)
2. Build DPA kernels for real hardware
3. Integrate P4 programs ([P4_DPA_SUMMARY.md](P4_DPA_SUMMARY.md))
4. Explore DevEmu ([DEVEMU_CAPABILITIES.md](DEVEMU_CAPABILITIES.md))

## Troubleshooting

### Build Issues

**Error: `command not found`**
```bash
sudo apt install build-essential make gcc
```

**Error: `Permission denied`**
```bash
# Don't build in /opt/mellanox
# Build in home directory
cd ~/workdir/doca
```

### Runtime Issues

**"Switch process not running"**
```bash
# Check logs
cat sim_logs/switch_0.log

# Verify mock switch built correctly
cd multi_switch_topology
ls -l mock_switch
```

**"No packet forwarding"**
```bash
# Enable verbose mode
./vhost_switch_test -n 4 -g -v

# Check switch state
./monitor_switches.sh
```

**Segmentation fault**
```bash
# Run under GDB
gdb ./vhost_switch_test
(gdb) run -n 4
(gdb) backtrace
```

### Getting Help

1. Check relevant documentation in docs list
2. Review log files in `sim_logs/`
3. Enable verbose/debug output
4. Run under GDB for crashes

## What's Next?

- **Add jitter/delay**: See [JITTER_DELAY_SIMULATION.md](JITTER_DELAY_SIMULATION.md)
- **Real hardware**: See [HARDWARE_COMPATIBILITY.md](HARDWARE_COMPATIBILITY.md)
- **DPA debugging**: See [README_DPA_GDB_DEBUGGING.md](README_DPA_GDB_DEBUGGING.md)
- **P4 programming**: See [P4_COMPILER_STATUS.md](P4_COMPILER_STATUS.md)

Happy coding! 🚀
