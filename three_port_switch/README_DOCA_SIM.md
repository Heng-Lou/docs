# DPDK Virtual Switch Network with veth Topology

A scalable DPDK-based Layer 2 switch implementation supporting multi-switch topologies using veth pairs and Linux network namespaces.

## Features

- ✅ **Multiple Topology Support**: Line and Ring topologies
- ✅ **Scalable**: Tested with up to 8 switches
- ✅ **Static MAC Forwarding**: Deterministic packet forwarding with no broadcast flooding
- ✅ **DPDK AF_PACKET PMD**: Direct packet I/O on veth interfaces
- ✅ **Isolated Namespaces**: Each "host" runs in its own network namespace
- ✅ **Zero-copy Forwarding**: DPDK-based packet processing

## Architecture

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│    ns1      │         │    ns2      │         │    ns3      │
│  10.0.1.2   │         │  10.0.2.2   │         │  10.0.3.2   │
└──────┬──────┘         └──────┬──────┘         └──────┬──────┘
       │ veth_h1_s1            │ veth_h2_s2            │ veth_h3_s3
       │                       │                       │
┌──────┴──────┐         ┌──────┴──────┐         ┌──────┴──────┐
│  Switch 1   │─────────│  Switch 2   │─────────│  Switch 3   │
│   (DPDK)    │         │   (DPDK)    │         │   (DPDK)    │
└─────────────┘         └─────────────┘         └─────────────┘
  veth_s1_s2 ←─────────→ veth_s2_s1
                          veth_s2_s3 ←─────────→ veth_s3_s2
```

## Quick Start

### 1. Setup Topology (e.g., 3-switch line)

```bash
./cleanup.sh
./setup_veth_topology.sh 3 line
./disable_ipv6.sh 3
./setup_static_arp.sh
./generate_mac_tables.sh 3 line
```

### 2. Build and Deploy Switches

```bash
./build_veth.sh
./deploy_switches.sh 3 line
```

### 3. Test Connectivity

```bash
./test_Nswitch.sh 3

# Or test manually
sudo ip netns exec ns1 ping 10.0.2.2
sudo ip netns exec ns1 ping 10.0.3.2
```

## Tested Configurations

| Switches | Topology | Status |
|----------|----------|--------|
| 3        | Line     | ✅ Pass |
| 3        | Ring     | ✅ Pass |
| 8        | Line     | ✅ Pass |
| 8        | Ring     | ✅ Pass |

## Key Implementation Details

### veth Interface Naming Convention

Each switch binds to veth interfaces using the pattern: `veth_s<MY_ID>_s<PEER_ID>`

For a link between Switch 2 and Switch 3:
- Switch 2 binds to: `veth_s2_s3`
- Switch 3 binds to: `veth_s3_s2`

These are the two ends of the same veth pair.

### Static MAC Forwarding

Each switch loads a static MAC table that maps destination MAC addresses to output ports:

```
# mac_tables/switch_2_line.txt
82:9b:a5:fc:de:57 0 ns2_local      # Local host
a6:63:43:4e:c3:63 1 ns1_via_left   # ns1 via left link
32:cd:74:69:0a:07 2 ns3_via_right  # ns3 via right link
```

This eliminates broadcast flooding and provides deterministic forwarding.

### DPDK Configuration

- **PMD**: AF_PACKET (for veth interface support)
- **Memory**: `--no-huge` (uses regular RAM, no hugepages needed)
- **Process Type**: Primary (each switch is independent)
- **Core Assignment**: Switch N runs on CPU core N-1

## File Structure

```
.
├── three_port_switch_veth.c    # Main DPDK switch implementation
├── build_veth.sh               # Build script
├── deploy_switches.sh          # Start all switches
├── setup_veth_topology.sh      # Create veth pairs and namespaces
├── setup_static_arp.sh         # Populate ARP tables
├── generate_mac_tables.sh      # Generate static MAC forwarding tables
├── disable_ipv6.sh             # Disable IPv6 (optional)
├── test_Nswitch.sh            # Connectivity test suite
├── test_without_switches.sh    # Debug script using Linux bridge
├── cleanup.sh                  # Clean up everything
└── mac_tables/                 # Generated MAC forwarding tables
    ├── switch_1_line.txt
    ├── switch_2_line.txt
    └── ...
```

## Troubleshooting

### Check if switches are running
```bash
ps aux | grep three_port_switch_veth
```

### View switch logs
```bash
tail -f logs/switch_1.log
```

### Verify veth interfaces exist
```bash
ip link show | grep veth_s
```

### Check ARP tables
```bash
for i in {1..3}; do
    echo "=== ns$i ==="
    sudo ip netns exec ns$i arp -a
done
```

### Test without DPDK switches (using Linux bridge)
```bash
./test_without_switches.sh
```

## Performance Tips

1. **Pin switches to different CPU cores** - Already done (switch N uses core N-1)
2. **Use hugepages** - Modify EAL args to remove `--no-huge` and configure hugepages
3. **Enable RSS** - For multi-queue support (requires NIC support)
4. **Batch processing** - Already using `MAX_PKT_BURST=32`

## Known Limitations

- No spanning tree protocol (STP) - loops in ring topology can cause packet storms
- No VLAN support yet
- No QoS/traffic shaping
- Fixed 1500 MTU

## Future Enhancements

- [ ] Implement STP for loop prevention
- [ ] Add VLAN tagging support
- [ ] Port mirroring for debugging
- [ ] Real-time statistics dashboard
- [ ] Support for jumbo frames
- [ ] Multi-queue support

## Requirements

- DPDK 20.11 or later
- Linux kernel with veth support
- Root privileges (for namespace and interface creation)
- gcc compiler

## License

[Your license here]