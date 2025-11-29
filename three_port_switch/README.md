# Three-Port Switch with DevEmu

A software switch implementation using DOCA Flow with three ports:
- **Port 0**: PCI device (can be emulated with DevEmu)
- **Port 1**: Ethernet port 0
- **Port 2**: Ethernet port 1

## Architecture

```
                     ┌──────────────────────────┐
                     │  Three-Port Switch       │
                     │  (DOCA Flow Engine)      │
                     │                          │
                     │  ┌────────────────────┐  │
                     │  │  MAC Learning      │  │
                     │  │  Table (256 ent.)  │  │
                     │  └────────────────────┘  │
                     │           │              │
      ┌──────────────┼───────────┼──────────────┼──────────────┐
      │              │           │              │              │
  ┌───▼───┐      ┌───▼───┐   ┌───▼───┐     ┌───▼───┐      ┌───▼───┐
  │Port 0 │◄────►│ Fwd   │◄─►│ Fwd   │◄───►│Port 1 │◄────►│Port 2 │
  │ PCI   │      │Engine │   │Engine │     │ETH 0  │      │ETH 1  │
  │ (Emu) │      └───────┘   └───────┘     │       │      │       │
  └───────┘                                 └───────┘      └───────┘
      │                                          │              │
      │ Virtual PCI                              │              │
      │ (DevEmu)                                 │              │
      │                                          │              │
   [Virtual]                                [Physical]     [Physical]
   Device                                   Ethernet       Ethernet
```

## Features

### 1. **MAC Learning**
- 256-entry learning table
- Simple hash-based lookup
- Automatic MAC address learning
- Age-out support

### 2. **Port Forwarding**
- Bidirectional forwarding between all ports
- Hardware-accelerated with DOCA Flow
- Control pipe for flexible forwarding
- Low latency switching

### 3. **Port Types**
- **PCI Port (Port 0)**: Can be emulated with DevEmu or real PCI device
- **Ethernet Ports (1-2)**: Physical or virtual Ethernet interfaces
- Mixed mode operation

### 4. **QoS and Advanced Features**
- **8 Priority Queues** per port with IP DSCP and VLAN PCP mapping
- **RSS (Receive Side Scaling)**: 4 queues for load distribution
- **Hairpin Queues**: 2 queues for hardware-to-hardware forwarding
- Per-queue packet statistics and monitoring

### 5. **TTL/Hop Limit Loop Prevention**
- **IPv4 TTL Decrement**: Automatic TTL decrement on every hop
- **IPv6 Hop Limit Decrement**: Hop limit handling for IPv6
- **Loop Prevention**: Packets dropped when TTL/hop limit expires
- **Ring Topology Support**: Prevents infinite loops in ring configurations
- See [TTL_LOOP_PREVENTION.md](TTL_LOOP_PREVENTION.md) and [TTL_QUICK_REFERENCE.md](TTL_QUICK_REFERENCE.md)

### 6. **Statistics**
- Packets forwarded counter
- Packets dropped counter
- Packets TTL expired counter
- QoS, RSS, and hairpin statistics
- Real-time statistics display

## Building

```bash
cd /home/heng/workdir/doca/three_port_switch
./build.sh
```

## Running

### With Real Hardware

```bash
sudo ./build/doca_three_port_switch \
  -a 03:00.0,representor=pf0vf0 \
  -a 03:00.1,representor=pf1vf0 \
  -a 03:00.2,representor=pf2vf0 -- 
```

### With Emulated PCI Device

1. **Create emulated PCI device** (using DevEmu):
   ```bash
   # In separate terminal - create emulated device
   sudo doca_devemu_pci_device_create ...
   ```

2. **Run switch**:
   ```bash
   sudo ./build/doca_three_port_switch \
     -a <emulated_pci_addr> \
     -a <eth_port_1> \
     -a <eth_port_2> --
   ```

## Configuration

### Port Configuration

The switch automatically configures three ports:

```c
Port 0: PCI_EMU  (PCI device - emulated or real)
Port 1: ETH0     (Ethernet)
Port 2: ETH1     (Ethernet)
```

### Forwarding Logic

```
Port 0 ◄──► Port 1
Port 0 ◄──► Port 2
Port 1 ◄──► Port 2
```

All ports can forward to all other ports (full mesh).

## Operation

### MAC Learning

When a packet arrives:
1. Learn source MAC on ingress port
2. Lookup destination MAC in table
3. Forward to destination port
4. If MAC unknown, flood to all ports (except ingress)

### Example Flow

```
1. Packet arrives on Port 1
   - Source MAC: 00:11:22:33:44:55
   - Dest MAC: AA:BB:CC:DD:EE:FF

2. Learn 00:11:22:33:44:55 on Port 1

3. Lookup AA:BB:CC:DD:EE:FF
   - Found on Port 2
   - Forward to Port 2

4. If not found:
   - Flood to Port 0 and Port 2
```

## Use Cases

### 1. **DevEmu Testing**
Test PCI device emulation:
- Emulate a network device
- Test driver interaction
- Validate device protocols

### 2. **Network Bridge**
Bridge PCI and Ethernet:
- Connect virtual and physical networks
- VM networking
- Container networking

### 3. **Development Platform**
Test switching logic:
- MAC learning algorithms
- Forwarding performance
- Flow offload testing

### 4. **Multi-Interface Switching**
Software switch for:
- Multi-homed hosts
- Network aggregation
- Link redundancy

## Technical Details

### Flow Offload

Uses DOCA Flow for hardware acceleration:
- Control pipes per port
- Hardware flow tables
- Wire-speed forwarding
- Minimal CPU usage

### MAC Table

Simple hash-based design:
```c
struct mac_entry {
    uint8_t mac[6];      // MAC address
    uint16_t port_id;    // Learned port
    time_t timestamp;    // Learning time
    uint8_t valid;       // Entry valid
};
```

Hash function: `mac[5] % TABLE_SIZE`

### Performance

Expected performance (with hardware):
- **Throughput**: Line rate (depends on ports)
- **Latency**: < 1 μs (hardware offload)
- **MAC Learning**: Instant
- **Table Size**: 256 entries

## DevEmu Integration

### Emulated PCI Port

Port 0 can be an emulated PCI device:

**Advantages:**
- Test without hardware
- Rapid prototyping
- Driver development
- CI/CD testing

**How It Works:**
```
┌─────────────────────┐
│ Switch Application  │
│ (Port 0 = PCI)     │
└──────────┬──────────┘
           │
    ┌──────▼──────┐
    │   DevEmu    │
    │ PCI Emulator│
    └──────┬──────┘
           │
    [Virtual PCI Device]
```

## Example Session

```bash
# Terminal 1: Build and run switch
cd /home/heng/workdir/doca/three_port_switch
./build.sh
sudo ./build/doca_three_port_switch -a <ports> --

# Output:
===========================================
 Three-Port Switch with DevEmu
===========================================
 Port 0: PCI (Emulated with DevEmu)
 Port 1: Ethernet 0
 Port 2: Ethernet 1
===========================================
MAC learning table initialized (256 entries)
Started port 0: PCI_EMU (PCI Emulated)
Started port 1: ETH0 (Ethernet)
Started port 2: ETH1 (Ethernet)
Creating switch forwarding flows...
Created forwarding rules for port 0 (PCI_EMU)
Created forwarding rules for port 1 (ETH0)
Created forwarding rules for port 2 (ETH1)
Switch is running - Press Ctrl+C to stop

=== Switch Statistics ===
Packets forwarded: 1234
Packets dropped:   56
=========================
```

## Limitations

### Current Implementation

- **Simplified MAC learning**: Basic hash table
- **No VLAN support**: Single broadcast domain
- **Static configuration**: Ports configured at startup
- **No STP**: No loop prevention

### Hardware Requirements

For full operation:
- BlueField DPU or compatible NIC
- At least 2 network ports
- DPDK-compatible devices

### Testing Without Hardware

For testing without actual hardware:
- Port 0 can use DevEmu (emulated)
- Ports 1-2 can use DPDK null PMD
- Functionality tested, not performance

## Extending the Switch

### Add Features

1. **VLAN Support**
   - Add VLAN ID to match criteria
   - Multiple broadcast domains
   - VLAN translation

2. **QoS**
   - Priority queues
   - Rate limiting
   - Traffic shaping

3. **ACLs**
   - Packet filtering
   - Security rules
   - Access control

4. **Advanced Learning**
   - Better hash function
   - Aging mechanism
   - Static entries

## Troubleshooting

### Build Fails

```bash
# Ensure environment is set
source /home/heng/workdir/doca/setup_environment.sh

# Clean and rebuild
rm -rf build
./build.sh
```

### Runtime Errors

```bash
# Check available ports
dpdk-devbind.py --status

# Verify hugepages
cat /proc/meminfo | grep Huge

# Run with debug logging
sudo ./build/doca_three_port_switch -a <ports> -- -l 60
```

## Files

```
three_port_switch/
├── three_port_switch.c     # Main implementation
├── meson.build            # Build configuration  
├── build.sh               # Build script
└── README.md              # This file
```

## Summary

This three-port switch demonstrates:
- ✅ DOCA Flow switching
- ✅ MAC learning table
- ✅ Multi-port forwarding
- ✅ DevEmu PCI emulation support
- ✅ Hardware offload
- ✅ Statistics tracking

**Ready to deploy on BlueField or test with DevEmu!**

---

**Note**: For production use, add error handling, VLAN support, proper aging, and comprehensive testing.

## QoS (Quality of Service)

The switch implements an 8-queue QoS system with:
- **DSCP-based classification** (IP ToS field)
- **Weighted Round Robin scheduling**
- **Per-queue statistics**

### Quick Test

```bash
# Start switches with QoS
./deploy_switches.sh 3 line

# Run quick validation
./tools/quick_qos_test.sh

# Monitor in real-time
./tools/monitor_qos.sh
```

### QoS Tools

See [tools/README.md](tools/README.md) for:
- Real-time monitoring (`monitor_qos.sh`)
- Quick validation (`quick_qos_test.sh`)
- Comprehensive testing (`test_qos_differentiation.sh`)

### Queue Priority Levels

| Priority | Queue | DSCP | Example `-Q` value |
|----------|-------|------|-------------------|
| Highest  | Q7    | 46   | `0xb8`           |
| High     | Q6    | 34   | `0x88`           |
| Medium   | Q3    | 10   | `0x28`           |
| Default  | Q0    | 0    | (no -Q flag)     |

Example with different priorities:
```bash
# High priority (EF)
sudo ip netns exec ns1 ping -Q 0xb8 10.0.3.2

# Medium priority (AF1x)
sudo ip netns exec ns1 ping -Q 0x28 10.0.3.2

# Best effort
sudo ip netns exec ns1 ping 10.0.3.2
```

