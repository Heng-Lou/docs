# RSS and Hairpin Queue Features

## Overview
The three-port switch now includes advanced DOCA Flow features for improved performance and scalability:

1. **RSS (Receive Side Scaling)** - Multi-queue load distribution
2. **Hairpin Queues** - Hardware-to-hardware packet forwarding

---

## RSS (Receive Side Scaling)

### What is RSS?
RSS distributes incoming packets across multiple receive queues based on a hash of packet header fields (IP addresses, ports). This enables:
- **Multi-core packet processing**: Different CPU cores can process packets from different queues simultaneously
- **Load balancing**: Traffic is evenly distributed across available processing resources
- **Improved throughput**: Parallel processing increases overall packet processing capacity

### Configuration
- **Number of RSS queues**: 4 per port (configurable via `NB_RSS_QUEUES`)
- **Hash fields**: IP 5-tuple (src IP, dst IP, src port, dst port, protocol)
- **RSS key**: 40-byte Toeplitz hash key (Microsoft's recommended default)
- **Supported protocols**: IPv4, UDP, TCP

### RSS Queue Mapping
```
Queue 0: Hash result % 4 == 0
Queue 1: Hash result % 4 == 1
Queue 2: Hash result % 4 == 2
Queue 3: Hash result % 4 == 3
```

### Benefits
- **Scalability**: CPU utilization scales with number of cores
- **Flow affinity**: Packets from the same flow go to the same queue (preserves ordering)
- **Performance**: Reduces lock contention and cache misses

---

## Hairpin Queues

### What is Hairpin?
Hairpin queues enable direct hardware-to-hardware packet forwarding between ports, bypassing the CPU entirely. This provides:
- **Zero-copy forwarding**: Packets move directly from NIC to NIC
- **Ultra-low latency**: No CPU involvement in the forwarding path
- **Wire-speed performance**: Hardware handles forwarding at line rate
- **CPU offload**: Frees CPU cycles for control plane and slow-path processing

### Configuration
- **Number of hairpin queues**: 2 per port (configurable via `NB_HAIRPIN_QUEUES`)
- **Topology**: Ring topology (Port 0 -> 1 -> 2 -> 0)
- **Queue IDs**: Start after QoS queues (8, 9, 10...)

### Hairpin Forwarding Paths
```
Port 0 (PCI)  ----hairpin----> Port 1 (ETH0)
Port 1 (ETH0) ----hairpin----> Port 2 (ETH1)
Port 2 (ETH1) ----hairpin----> Port 0 (PCI)
```

### Use Cases
- **Fast switching**: Layer 2/3 switching at wire speed
- **Packet mirroring**: Copy packets between ports without CPU
- **Service chaining**: Chain network functions in hardware
- **Traffic steering**: Redirect traffic flows in hardware

---

## Integration with QoS

The RSS and Hairpin features work together with the existing QoS system:

### Queue Hierarchy
```
Per-port queue layout:
├── QoS Queues (0-7):     Priority-based packet classification
├── RSS Queues (0-3):     Load distribution across cores
└── Hairpin Queues (8-9): Hardware-to-hardware forwarding
```

### Traffic Flow
1. **Ingress**: Packet arrives at port
2. **Classification**: QoS engine classifies based on DSCP/VLAN PCP
3. **Distribution**: 
   - High-priority traffic -> Dedicated QoS queue
   - Bulk traffic -> RSS queues for parallel processing
   - Fast-path traffic -> Hairpin queues for hw forwarding
4. **Egress**: Packet forwarded to destination port

---

## Performance Characteristics

### RSS Performance
- **Throughput**: Scales linearly with number of cores (up to 4x with 4 queues)
- **Latency**: Minimal overhead (~1-2 μs for queue selection)
- **CPU efficiency**: Distributes load, prevents single-core bottlenecks

### Hairpin Performance
- **Throughput**: Wire-speed (100 Gbps on BlueField-2)
- **Latency**: Sub-microsecond (<500 ns typical)
- **CPU usage**: Zero CPU cycles for forwarded packets

### Combined Benefits
- **Smart offload**: CPU handles control/management, hardware handles data plane
- **Flexible processing**: Mix of software (RSS) and hardware (hairpin) forwarding
- **Optimal resource usage**: Right tool for each traffic type

---

## Statistics and Monitoring

### RSS Statistics
```c
sw_state.packets_rss_distributed     // Total packets distributed via RSS
port_config.rss.packets_distributed  // Per-port RSS packets
```

### Hairpin Statistics
```c
sw_state.packets_hairpinned                    // Total hairpinned packets
port_config.hairpin[i].packets_hairpinned      // Per-queue hairpin packets
```

### Viewing Statistics
The `display_stats()` function shows:
- Total RSS distributed packets
- Total hairpinned packets
- Per-port RSS distribution
- Per-hairpin-queue forwarding counts

---

## Configuration Options

### Compile-time Configuration
```c
#define NB_RSS_QUEUES 4        // Number of RSS queues (1-16)
#define NB_HAIRPIN_QUEUES 2    // Number of hairpin queues (1-8)
#define NB_QOS_QUEUES 8        // Number of QoS priority queues
```

### Runtime Configuration
- RSS can be enabled/disabled per port: `port_config.rss.enabled`
- Hairpin can be enabled/disabled per queue: `port_config.hairpin[i].enabled`
- RSS key can be customized: `port_config.rss.rss_key[]`

---

## Advanced Features

### RSS Key Customization
The RSS hash key determines how packets are distributed. The default key provides good distribution for most traffic patterns, but you can customize it:

```c
/* Custom RSS key for specific traffic patterns */
uint32_t custom_key[10] = {
    0x12345678, 0x9abcdef0, /* ... */
};
memcpy(port->rss.rss_key, custom_key, sizeof(custom_key));
```

### Hairpin Modes
- **Direct hairpin**: Port-to-port forwarding (current implementation)
- **Queued hairpin**: Buffered forwarding with QoS
- **Bidirectional hairpin**: Two-way hardware forwarding

---

## Testing and Validation

### RSS Testing
1. Generate diverse traffic with different 5-tuples
2. Monitor per-queue packet counts
3. Verify even distribution across RSS queues
4. Check CPU utilization across cores

### Hairpin Testing
1. Send packets between ports
2. Monitor hairpin queue statistics
3. Measure latency (should be sub-μs)
4. Verify zero CPU usage for hairpinned packets

### Integration Testing
1. Mixed traffic (QoS + RSS + Hairpin)
2. Verify correct classification and forwarding
3. Check statistics consistency
4. Validate under high load

---

## Troubleshooting

### RSS Issues
- **Uneven distribution**: Check RSS key and traffic patterns
- **Missing packets**: Verify queue configuration
- **Poor performance**: Check CPU affinity and NUMA placement

### Hairpin Issues
- **Packets not forwarded**: Verify port connectivity and configuration
- **High latency**: Check for hairpin queue overflow
- **Missing statistics**: Ensure hairpin pipes are created successfully

---

## References

- DOCA Flow Programming Guide
- RSS (Receive Side Scaling) Specification
- BlueField DPU Hardware Offload Capabilities
- NVIDIA DOCA SDK Documentation

---

## Future Enhancements

Potential improvements for RSS and Hairpin:

1. **Adaptive RSS**: Dynamic queue adjustment based on load
2. **Flow steering**: Per-flow hairpin path selection
3. **RSS + Hairpin fusion**: Combine for optimal performance
4. **Telemetry integration**: Export RSS/Hairpin metrics
5. **Advanced QoS**: Integrate RSS with weighted fair queuing
