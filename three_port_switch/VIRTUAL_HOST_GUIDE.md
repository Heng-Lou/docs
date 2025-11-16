# Virtual Host with PCI Port Simulation

## Overview

This implementation provides a complete virtual host infrastructure that connects to BlueField DPU switches via simulated PCI ports. It enables testing multi-switch topologies with virtual hosts that can send and receive packets.

## Architecture

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│  Virtual Host 0 │         │  Virtual Host 1 │         │  Virtual Host 2 │
│  192.168.1.10   │         │  192.168.1.11   │         │  192.168.1.12   │
└────────┬────────┘         └────────┬────────┘         └────────┬────────┘
         │ PCI                       │ PCI                       │ PCI
         │                           │                           │
┌────────▼────────┐         ┌────────▼────────┐         ┌────────▼────────┐
│   Switch 0      │         │   Switch 1      │         │   Switch 2      │
│  ┌───┬───┬───┐ │         │  ┌───┬───┬───┐ │         │  ┌───┬───┬───┐ │
│  │PCI│E0 │E1 │ │         │  │PCI│E0 │E1 │ │         │  │PCI│E0 │E1 │ │
│  └─┬─┴─┬─┴─┬─┘ │         │  └─┬─┴─┬─┴─┬─┘ │         │  └─┬─┴─┬─┴─┬─┘ │
└────┼───┼───┼───┘         └────┼───┼───┼───┘         └────┼───┼───┼───┘
     │   │   └──────Ethernet─────┘   │                      │   │
     │   └──────────────Ethernet──────┴──────────────────────┘   │
     └────────────────────────────────────Ethernet───────────────┘
                              (Ring Topology)
```

## Features

### Virtual Host Capabilities

1. **PCI Connection**: Each virtual host connects to a switch's PCI port via virtual link
2. **Packet Generation**: Built-in packet generator with configurable rate and patterns
3. **Packet Reception**: Receives packets from switch with custom packet handlers
4. **Network Stack**: Basic Ethernet, IP, and UDP packet building
5. **Statistics**: Comprehensive TX/RX statistics per host

### Packet Generator

- Configurable packet rate (packets per second)
- Configurable packet count (finite or infinite)
- Configurable destination MAC/IP addresses
- UDP packet generation with checksums
- Rate limiting with nanosecond precision

### Switch Features

- **Three-port design**: PCI + 2 Ethernet ports
- **TTL handling**: Decrements TTL on IP packets to prevent loops
- **Ring topology support**: Connects switches in a ring with loop prevention
- **Per-port statistics**: Tracks RX/TX packets, bytes, and drops

## Building

```bash
cd three_port_switch
make -f Makefile.vhost
```

## Usage

### Basic Test (4 hosts + 4 switches)

```bash
./vhost_switch_test -n 4 -d 10
```

### With Packet Generation

```bash
./vhost_switch_test -n 4 -p -r 100 -c 100 -d 15
```

### Parameters

- `-n NUM`: Number of hosts/switches (default: 4, max: 32)
- `-p`: Enable packet generation
- `-r RATE`: Packet generation rate in pps (default: 100)
- `-c COUNT`: Number of packets to send (default: 100, 0=infinite)
- `-d DURATION`: Run duration in seconds (default: 10)
- `-h`: Show help

### Make Targets

```bash
make -f Makefile.vhost test-basic    # Basic 4 host/switch test
make -f Makefile.vhost test-pktgen   # Packet generation test
make -f Makefile.vhost test-ring     # 8 host/switch ring test
make -f Makefile.vhost test-stress   # High rate stress test
```

## Example Output

```
========================================
Virtual Host + Switch Simulation
========================================
Switches/Hosts: 4
Topology: Ring
Packet Gen: Enabled
  Rate: 100 pps
  Count: 100 packets
Duration: 10 seconds

Creating switches...
  Created Switch 0
  Created Switch 1
  Created Switch 2
  Created Switch 3

Connecting switches in ring topology...
  Switch 0 (eth1) <-> Switch 1 (eth0)
  Switch 1 (eth1) <-> Switch 2 (eth0)
  Switch 2 (eth1) <-> Switch 3 (eth0)
  Switch 3 (eth1) <-> Switch 0 (eth0)

Creating 4 virtual hosts...
  Created and connected Host 0 to Switch 0
  Created and connected Host 1 to Switch 1
  Created and connected Host 2 to Switch 2
  Created and connected Host 3 to Switch 3

✓ All components running!
```

## Traffic Patterns

### Ring Forwarding

In ring topology, traffic flows:
- Host 0 → Switch 0 (PCI) → Switch 0 (Eth0) → Switch 1 (Eth0) → Switch 1 (Eth1) → ... → Host 1

### Three-Port Switch Logic

Each switch implements:
- **Port 0 (PCI)** → Forward to Port 1 (Eth0)
- **Port 1 (Eth0)** → Forward to Port 2 (Eth1)
- **Port 2 (Eth1)** → Forward to Port 0 (PCI)

### Loop Prevention

The switches implement TTL (Time To Live) decrement on IP packets:
- Each switch decrements the TTL field in IP header
- Packets with TTL=0 are dropped
- This prevents infinite loops in ring topologies

## Virtual Host API

### Creating Hosts

```c
vhost_manager_t host_mgr;
vhost_manager_init(&host_mgr, &link_mgr);

uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
uint8_t ip[4] = {192, 168, 1, 10};
uint32_t host_id;

vhost_create(&host_mgr, "Host-1", mac, ip, &host_id);
```

### Connecting to Switch

```c
// Connect host to switch's PCI link
vhost_connect_to_switch(&host_mgr, host_id, switch_pci_link_id);
vhost_start(&host_mgr, host_id);
```

### Packet Generation

```c
vhost_pktgen_config_t config = {
    .enabled = true,
    .pkt_size = 128,
    .pps = 1000,
    .count = 10000,
    .dst_port = 5000
};

// Set destination
memcpy(config.dst_mac, dst_mac, 6);
memcpy(config.dst_ip, dst_ip, 4);

vhost_configure_pktgen(&host_mgr, host_id, &config);
vhost_start_pktgen(&host_mgr, host_id);
```

### Custom Packet Handler

```c
void my_packet_handler(void *ctx, const uint8_t *data, uint16_t size) {
    printf("Received packet: %u bytes\n", size);
    // Process packet...
}

vhost_set_packet_handler(&host_mgr, host_id, my_packet_handler, context);
```

### Sending Packets

```c
uint8_t packet[128];
uint16_t size = build_my_packet(packet);
vhost_send_packet(&host_mgr, host_id, packet, size);
```

## Performance

### Tested Configurations

- **Small scale**: 4 hosts/switches at 1000 pps each (tested ✓)
- **Medium scale**: 8 hosts/switches at 500 pps each (tested ✓)
- **Large scale**: 32 hosts/switches (build tested)

### Limitations

- Packet rate limited by CPU and thread scheduling
- Higher rates (>10000 pps per host) may cause packet drops
- Total system throughput depends on CPU cores available

## Integration with Hardware

This virtual host simulation can be used to:

1. **Develop and test switch logic** before hardware is available
2. **Validate multi-switch topologies** with complex routing
3. **Test packet forwarding behavior** including TTL, QoS, VLAN
4. **Prototype new features** like RSS, hairpin queues
5. **Regression testing** without requiring hardware setup

When hardware becomes available, the same three-port switch logic can be deployed on actual BlueField DPUs with minimal changes.

## Statistics and Monitoring

Each virtual host tracks:
- TX packets/bytes
- RX packets/bytes
- TX errors
- RX errors and drops

Example output:
```
Host 0: Host-0
  MAC: 02:00:00:00:00:00
  IP: 192.168.1.10
  TX: 10000 pkts / 1280000 bytes (errors: 0)
  RX: 10000 pkts / 1280000 bytes (errors: 0, drops: 0)
```

## Files

- `virtual_host.h` - Virtual host API header
- `virtual_host.c` - Virtual host implementation
- `vhost_switch_test.c` - Integration test program
- `Makefile.vhost` - Build and test makefile
- `VIRTUAL_HOST_GUIDE.md` - This documentation

## Next Steps

Potential enhancements:

1. **TCP support** - Add TCP packet building and state tracking
2. **ARP simulation** - Implement ARP request/response
3. **Performance monitoring** - Add latency measurement
4. **Traffic shaping** - Add bandwidth limiting per host
5. **Packet capture** - Add pcap export for Wireshark analysis
6. **Multi-threaded hosts** - Support parallel TX/RX per host

## Related Documentation

- [Virtual Link Infrastructure](VIRTUAL_LINK_GUIDE.md)
- [Ring Topology Testing](RING_TOPOLOGY_GUIDE.md)
- [TTL and Loop Prevention](TTL_FEATURE_COMPLETE.md)
- [Jitter and Delay Simulation](JITTER_DELAY_SIMULATION.md)
