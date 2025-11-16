# TTL/Hop Limit Loop Prevention

## Overview

The three-port switch now includes TTL (Time To Live) and hop limit handling to prevent infinite loops in ring topologies. This is critical for multi-switch ring configurations where packets could circulate indefinitely without proper loop prevention.

## Features

### IPv4 TTL Handling
- **Decrement TTL**: Each time an IPv4 packet is forwarded, its TTL is decremented by 1
- **Drop on Expiration**: Packets with TTL ≤ 1 are dropped before forwarding
- **Checksum Update**: IPv4 header checksum is updated using RFC 1624 incremental update method
- **Statistics**: TTL-expired packets are tracked in `packets_ttl_expired` counter

### IPv6 Hop Limit Handling
- **Decrement Hop Limit**: Each time an IPv6 packet is forwarded, its hop limit is decremented by 1
- **Drop on Expiration**: Packets with hop limit ≤ 1 are dropped before forwarding
- **No Checksum**: IPv6 has no header checksum, so only the hop limit field is updated
- **Statistics**: Hop limit-expired packets are tracked in the same `packets_ttl_expired` counter

### Non-IP Traffic
- **Passthrough**: Non-IP packets (ARP, raw Ethernet, etc.) are forwarded without TTL checks
- **No Loop Prevention**: Layer 2 protocols need their own loop prevention (e.g., STP)

## Ring Topology Example

In a ring topology with N switches:

```
Switch 0 <--> Switch 1 <--> Switch 2 <--> ... <--> Switch N-1 <--> Switch 0
    |                                                                  ^
    |__________________________________________________________________|
                          Ring Connection
```

Without TTL handling, a packet could circulate infinitely. With TTL:

1. **Host sends packet with TTL=64**
2. **Switch 0**: TTL=64 → 63, forward
3. **Switch 1**: TTL=63 → 62, forward
4. **Switch 2**: TTL=62 → 61, forward
5. ... continues ...
6. **After 63 hops**: TTL=1 → drop

The packet is guaranteed to be dropped within 63 hops, preventing infinite loops.

## Implementation Details

### Main Switch Code (`three_port_switch.c`)

```c
/* Check and decrement IPv4 TTL */
static inline int check_and_decrement_ipv4_ttl(struct rte_ipv4_hdr *ipv4_hdr);

/* Check and decrement IPv6 hop limit */
static inline int check_and_decrement_ipv6_hop_limit(struct rte_ipv6_hdr *ipv6_hdr);

/* Process packet TTL/hop limit (main entry point) */
static inline int process_packet_ttl(struct rte_mbuf *mbuf);
```

The `process_packet_ttl()` function should be called for each packet before forwarding:

```c
/* In packet processing loop */
if (!process_packet_ttl(mbuf)) {
    /* Drop packet - TTL/hop limit expired */
    rte_pktmbuf_free(mbuf);
    continue;
}
/* Forward packet normally */
```

### Simulation Code (`three_port_switch_sim.c`)

The simulation includes TTL handling with these packet fields:
- `uint8_t ttl`: TTL (IPv4) or hop limit (IPv6)
- `uint8_t is_ipv4`: 1 for IPv4, 0 for IPv6 or non-IP

Create IP packets with TTL:
```c
packet_t pkt = create_ip_packet(port_id, size, ttl, is_ipv4);
```

### Statistics

Track TTL-expired packets:
```c
struct switch_state {
    ...
    uint64_t packets_ttl_expired;  /* IPv4 TTL or IPv6 hop limit expired */
};
```

View statistics:
```
Packets TTL expired:    42
```

## Testing

### Automated Tests

The simulation includes comprehensive TTL tests:

1. **Test 6**: IPv4 TTL Expiration
   - Verifies packets with TTL ≤ 1 are dropped
   
2. **Test 7**: IPv4 TTL Decrement
   - Verifies TTL is decremented on forwarding
   
3. **Test 8**: IPv6 Hop Limit Expiration
   - Verifies packets with hop limit ≤ 1 are dropped
   
4. **Test 9**: Ring Topology Loop Prevention
   - Simulates packet going around ring multiple times
   - Verifies packet is dropped when TTL expires

Run tests:
```bash
cd three_port_switch
gcc -g -O0 -o switch_sim three_port_switch_sim.c
./switch_sim test
```

### Interactive Testing

Test TTL handling interactively:

```bash
./switch_sim
```

Commands:
```
# Send IPv4 packet with TTL=64
> sendip 0 64 64

# Send IPv4 packet with TTL=1 (will be dropped)
> sendip 0 64 1

# Send IPv6 packet with hop limit=128
> sendip6 0 64 128

# Send IPv6 packet with hop limit=1 (will be dropped)
> sendip6 0 64 1

# View statistics
> stats
```

### Manual Ring Test

Simulate a ring topology manually:

```bash
# Create packet with TTL=4
> sendip 0 64 4
  TTL decremented to 3
  Forwarding: port 0 -> port 1

# Simulate packet arriving at port 1 with TTL=3
> sendip 1 64 3
  TTL decremented to 2
  Forwarding: port 1 -> port 2

# Simulate packet arriving at port 2 with TTL=2
> sendip 2 64 2
  TTL decremented to 1
  Forwarding: port 2 -> port 0

# Simulate packet arriving at port 0 with TTL=1 (will drop)
> sendip 0 64 1
  DROPPED: TTL/hop limit expired (TTL=1)

# Check statistics
> stats
Port 0 (pci_port):
  TTL expired: 1 packets
```

## Integration with Multi-Switch Topology

For the multi-switch ring topology (`multi_switch_topology/`):

1. **Each switch** processes packets with TTL handling
2. **Initial TTL**: Hosts typically send packets with TTL=64 or TTL=128
3. **Maximum Ring Size**: With TTL=64, supports rings up to 63 switches
4. **Guaranteed Termination**: Packets will be dropped within N hops (N = initial TTL)

### Deployment

When deploying switches in a ring:

```bash
cd multi_switch_topology
./deploy_multi_switch.sh 8 ring
```

The ring topology will automatically benefit from TTL handling:
- Broadcast packets won't loop infinitely
- Unknown unicast traffic will be dropped after traversing all switches
- Layer 3 routing will work correctly with TTL decrements

## Performance Considerations

### Overhead

TTL handling adds minimal overhead:
- **IPv4**: ~10 CPU cycles (TTL check + decrement + checksum update)
- **IPv6**: ~5 CPU cycles (TTL check + decrement, no checksum)
- **Non-IP**: 0 cycles (passthrough)

### Optimization

The implementation uses:
- **Inline functions**: Reduces function call overhead
- **Early exit**: Checks TTL before other processing
- **Incremental checksum**: Avoids full recalculation (RFC 1624)
- **Branch prediction**: Common case (TTL > 1) is optimized

## Limitations

1. **Layer 2 Loops**: Non-IP traffic (ARP, STP BPDUs, etc.) is not protected by TTL
2. **Initial TTL**: Relies on hosts setting appropriate initial TTL values
3. **No ICMP**: Does not send ICMP Time Exceeded messages when TTL expires

## Future Enhancements

1. **ICMP Time Exceeded**: Send ICMP messages for expired packets
2. **Configurable TTL**: Allow setting minimum TTL threshold
3. **STP Integration**: Add Spanning Tree Protocol for Layer 2 loop prevention
4. **TTL Monitoring**: Add alerts when excessive TTL expirations occur

## References

- **RFC 791**: Internet Protocol (IPv4)
- **RFC 2460**: Internet Protocol Version 6 (IPv6)
- **RFC 1624**: Incremental Internet Checksum
- **DPDK rte_ip.h**: IP header structures and utilities
