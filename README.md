# NVIDIA DOCA Development Project

A comprehensive NVIDIA DOCA development project featuring BlueField DPU examples, DPA programming, P4 compiler integration, and a complete virtual simulation infrastructure for testing multi-switch topologies without hardware.

## 🎯 Project Overview

This repository represents a complete journey from basic DOCA concepts to advanced network simulation, including:
- **DOCA Flow**: Packet processing and flow management
- **DPA Programming**: Data Path Accelerator kernel development with GDB debugging
- **P4 Integration**: P4 to DPA compilation with nvp4c
- **DevEmu Simulation**: Virtual PCI device emulation
- **Multi-Switch Simulation**: Complete network topologies with virtual hosts
- **Advanced Features**: TTL/hop limits, QoS queues, RSS, hairpin, jitter/delay simulation

## 📚 Documentation

### Getting Started
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)** - Overall project status and capabilities
- **[BLUEFIELD_APPS.md](BLUEFIELD_APPS.md)** - BlueField application examples
- **[README_SAMPLES.md](README_SAMPLES.md)** - Sample code overview

### Simulation & Testing
- **[VIRTUAL_HOST_SUMMARY.md](VIRTUAL_HOST_SUMMARY.md)** - Virtual host simulation (⭐ recommended)
- **[THREE_PORT_SWITCH_SUMMARY.md](THREE_PORT_SWITCH_SUMMARY.md)** - Three-port switch architecture
- **[TTL_FEATURE_COMPLETE.md](TTL_FEATURE_COMPLETE.md)** - TTL/hop limit implementation
- **[JITTER_DELAY_SIMULATION.md](JITTER_DELAY_SIMULATION.md)** - Network impairment simulation

### DPA Programming
- **[DPA_PROGRAMMING.md](DPA_PROGRAMMING.md)** - DPA kernel development guide
- **[DPA_GDB_DEBUG_GUIDE.md](DPA_GDB_DEBUG_GUIDE.md)** - GDB debugging for DPA
- **[README_DPA_GDB_DEBUGGING.md](README_DPA_GDB_DEBUGGING.md)** - Debugging setup
- **[DPA_DEBUGGING_SETUP_SUMMARY.md](DPA_DEBUGGING_SETUP_SUMMARY.md)** - Complete debugging reference
- **[DPA_TESTING_TOOLS.md](DPA_TESTING_TOOLS.md)** - Testing tools and methods

### P4 & Compilation
- **[P4_DPA_SUMMARY.md](P4_DPA_SUMMARY.md)** - P4 to DPA compilation overview
- **[P4_COMPILER_STATUS.md](P4_COMPILER_STATUS.md)** - Compiler availability and status

### DevEmu
- **[DEVEMU_CAPABILITIES.md](DEVEMU_CAPABILITIES.md)** - DevEmu features and capabilities
- **[DEVEMU_QUICK_START.md](DEVEMU_QUICK_START.md)** - Getting started with DevEmu
- **[DEVEMU_SAMPLES.md](DEVEMU_SAMPLES.md)** - DevEmu sample programs

### Hardware
- **[HARDWARE_COMPATIBILITY.md](HARDWARE_COMPATIBILITY.md)** - Hardware requirements and compatibility
- **[QUICK_START_VHOST.md](QUICK_START_VHOST.md)** - Virtual host quick reference

## 🚀 Quick Start Guide

### 1. Virtual Host Simulation (No Hardware Required!)

Test complete network topologies with virtual hosts and switches:

```bash
cd three_port_switch

# Build the virtual host test infrastructure
make -f Makefile.vhost

# Run a demo with packet generation (4 hosts, 4 switches in ring)
./demo_vhost.sh pktgen

# Or customize: 8 hosts, ping test, 100 packets/sec, 200 total, 15s duration
./vhost_switch_test -n 8 -p -r 100 -c 200 -d 15
```

### 2. Multi-Switch Topologies

Deploy multiple switches with different topologies:

```bash
cd multi_switch_topology

# Mock simulation (no DPDK required)
./mock_simulator.sh 4 ring    # 4 switches in ring
./mock_simulator.sh 6 line    # 6 switches in line
./mock_simulator.sh 8 mesh    # 8 switches in mesh

# Monitor all switches
./monitor_switches.sh
```

### 3. Build Basic DOCA Flow Example

```bash
# Using meson
meson build && ninja -C build

# Or using make
make
```

### 4. Build DPA Samples

```bash
cd dpa_kernel_launch
./build.sh
```

See documentation below for detailed guides on each component.

## 🎁 Key Features

### Virtual Simulation Infrastructure ⭐
- **Complete simulation without hardware** - Test everything in software
- **Virtual hosts** with MAC/IP addressing and PCI connectivity
- **Multi-switch topologies**: ring, line, mesh with configurable sizes
- **Packet generation**: Configurable rates, patterns, and protocols
- **Traffic testing**: Ping, packet generation, flow verification
- **Loop prevention**: TTL/hop limit with comprehensive testing
- **Network impairments**: Jitter and delay simulation
- **Link testing**: Link down scenarios and recovery
- **Monitoring**: Real-time statistics and health monitoring

### DOCA Flow
- Flow initialization and management
- Control pipe creation
- Bidirectional port forwarding
- RSS (Receive Side Scaling)
- Hairpin queues
- IP and VLAN QoS queues
- DPDK integration

### DPA Programming
- Data Path Accelerator kernel development
- GDB debugging with simulator support
- DevEmu virtual PCI device emulation
- Unit testing framework
- Code coverage tools

### P4 Integration
- P4 to DPA compilation with dpacc
- nvp4c compiler support (when available)
- P4 sample programs

## 📁 Project Structure

```
.
├── README.md                           # This file
├── VERSION                             # Project version
├── Makefile                            # Top-level build
├── meson.build                         # Meson build configuration
├── setup_environment.sh                # Environment setup script
├── check_p4_availability.sh            # Check P4 compiler availability
│
├── Documentation/                      # All documentation (see above)
│   ├── PROJECT_STATUS.md
│   ├── BLUEFIELD_APPS.md
│   └── ... (20+ documentation files)
│
├── doca_flow_simple.c                  # Basic DOCA Flow example
│
├── three_port_switch/                  ⭐ Main simulation project
│   ├── three_port_switch.c             # Core switch implementation
│   ├── three_port_switch_sim.c         # Simulator for GDB debugging
│   ├── vhost_switch_test.c             # Virtual host test infrastructure
│   ├── virtual_link.c/h                # Virtual link implementation
│   ├── Makefile.sim                    # Simulator build
│   ├── Makefile.vhost                  # Virtual host build
│   ├── Makefile.vlink                  # Virtual link tests
│   ├── demo_vhost.sh                   # Quick demo script
│   ├── test_ttl.sh                     # TTL testing
│   ├── test_link_down.sh               # Link failure testing
│   └── test_coverage.sh                # Code coverage analysis
│
├── multi_switch_topology/              # Multi-switch deployments
│   ├── mock_simulator.sh               # Mock switch simulator
│   ├── deploy_multi_switch.sh          # Deploy N switches
│   ├── monitor_switches.sh             # Monitor tool
│   ├── run_simulator.sh                # DPDK-based simulator
│   └── network_simulator.c             # Network topology simulator
│
├── dpa_kernel_launch/                  # DPA programming samples
│   ├── dpa_kernel_launch_sample.c
│   ├── device/dpa_kernel_launch_kernels_dev.c
│   └── build.sh
│
├── flow_control_pipe/                  # Flow control examples
│   ├── flow_control_pipe_sample.c
│   └── meson.build
│
├── simple_fwd_vnf/                     # Simple forwarding VNF
│   └── ...
│
└── devemu_sample/                      # DevEmu PCI samples
    └── doca_devemu/
        ├── devemu_pci_device_dma/
        ├── devemu_pci_device_msix/
        └── ...
```

## 💻 Requirements

### For Virtual Simulation (No Hardware!)
- Linux system (Ubuntu 20.04+ recommended)
- GCC compiler
- Make
- Standard C libraries

### For Real Hardware
- NVIDIA BlueField DPU (BlueField-2 or BlueField-3)
- DOCA SDK 2.0 or later
- DPDK 22.11 or later
- Mellanox OFED drivers

## 🔧 Building

### Virtual Simulation (Recommended for Testing)

```bash
cd three_port_switch

# Build simulator for GDB debugging
make -f Makefile.sim

# Build virtual host infrastructure
make -f Makefile.vhost

# Build virtual link tests
make -f Makefile.vlink

# Run all tests with coverage
./test_coverage.sh
```

### DOCA Flow Examples

```bash
# Using Meson (Recommended)
meson build
ninja -C build

# Using Makefile
make
```

### DPA Samples

```bash
cd dpa_kernel_launch
./build.sh
```

### Multi-Switch Deployment

```bash
cd multi_switch_topology

# Mock simulation (no dependencies)
./mock_simulator.sh 4 ring
```

## 🧪 Testing

### Virtual Host Tests (Complete Network Simulation)

```bash
cd three_port_switch

# Quick demo
./demo_vhost.sh pktgen

# Custom test: 8 hosts, ping, 100pps, 200 packets, 15s
./vhost_switch_test -n 8 -p -r 100 -c 200 -d 15

# TTL/hop limit testing on ring topology
./test_ttl.sh

# Link failure testing
./test_link_down.sh

# Code coverage analysis
./test_coverage.sh
```

### Multi-Switch Tests

```bash
cd multi_switch_topology

# Start 4 switches in ring topology
./mock_simulator.sh 4 ring

# Monitor in another terminal
./monitor_switches.sh

# Check connectivity (if mock switch supports it)
./test_connectivity.sh
```

### Virtual Link Tests

```bash
cd three_port_switch
make -f Makefile.vlink test
```

### Jitter/Delay Simulation

```bash
cd three_port_switch

# Build with jitter support
make -f Makefile.vhost clean
make -f Makefile.vhost CFLAGS="-DENABLE_JITTER"

# Run with jitter configuration
./vhost_switch_test -n 4 -r 100 -d 10 --jitter-file jitter.conf
```

See [JITTER_DELAY_SIMULATION.md](JITTER_DELAY_SIMULATION.md) for details.

## 🔍 Development Journey

This project evolved through several phases:

### Phase 1: DOCA Flow Basics
- Built simple DOCA Flow example with bidirectional forwarding
- Learned DOCA initialization, port management, and control pipes

### Phase 2: DPA Programming
- Explored DPA kernel development
- Set up GDB debugging for DPA code
- Created simulator for testing DPA programs

### Phase 3: P4 Integration
- Investigated P4 to DPA compilation
- Tested dpacc compiler (available)
- Documented nvp4c status (not yet available in DOCA 2.9)

### Phase 4: DevEmu Exploration
- Studied virtual PCI device emulation
- Built DevEmu samples for DMA, MSI-X, stateful regions
- Learned DevEmu capabilities and limitations

### Phase 5: Three-Port Switch
- Designed three-port switch (1 PCI + 2 Ethernet)
- Implemented basic forwarding logic
- Added IP and VLAN QoS queues
- Integrated RSS and hairpin queues

### Phase 6: Virtual Simulation Infrastructure ⭐
- Created virtual link abstraction for testing
- Built virtual host with PCI connectivity
- Implemented packet generation and traffic patterns
- Added TTL/hop limit for loop prevention
- Created comprehensive test suite

### Phase 7: Multi-Switch Topologies
- Extended to N switches in ring/line/mesh
- Added monitoring and statistics
- Implemented link failure testing
- Added jitter and delay simulation

### Phase 8: Hardware Readiness
- Documented compatibility with real hardware
- Ensured simulation matches real behavior
- Created migration guides

## 🎓 Learning Resources

### Key Concepts Covered
1. **DOCA Flow**: Packet processing pipeline, flow tables, control vs. data plane
2. **DPA**: Hardware offload, kernel programming, debugging techniques
3. **DevEmu**: Virtual device emulation, PCI protocol, host-DPU interaction
4. **Network Simulation**: Virtual hosts, topology testing, traffic generation
5. **Testing**: Unit tests, integration tests, code coverage, failure scenarios

### External References
- [NVIDIA DOCA Documentation](https://docs.nvidia.com/doca/)
- [DPDK Documentation](https://doc.dpdk.org/)
- [P4 Language Specification](https://p4.org/specs/)

## 🛠️ Troubleshooting

### "Permission denied" when building in /opt/mellanox
Build in your home directory or use proper permissions.

### "Switch process not running" in monitor
Check that switches started successfully. Review log files in `sim_logs/`.

### "EAL: failed to parse device" with DPDK null PMDs
Check device naming in run script. Virtual devices should be named consistently.

### Simulation doesn't match expected behavior
Enable debug logging and check packet flow with monitor tools.

### Code coverage shows low coverage
Run all test scripts: `test_ttl.sh`, `test_link_down.sh`, and `vhost_switch_test`.

## 🤝 Contributing

This is a learning and development project. Feel free to:
- Add new features
- Improve documentation
- Report issues
- Share your DOCA experiences

## 📄 License

Copyright (c) 2024 - NVIDIA DOCA Development Project

This project is for educational and development purposes.
