# Virtual Link Infrastructure for Switch Simulation

## Overview

The virtual link infrastructure provides a software-based network simulation environment for testing multi-switch topologies without requiring physical hardware. This allows you to:

- Connect multiple switch instances together
- Simulate network characteristics (bandwidth, latency, packet loss)
- Test various network topologies (ring, line, mesh)
- Debug switch forwarding logic
- Validate multi-hop packet forwarding

## Architecture

### Components

1. **Virtual Link Manager** (`vlink_manager_t`)
   - Manages all virtual network links
   - Coordinates packet forwarding between links
   - Tracks statistics and configuration

2. **Virtual Link Endpoint** (`vlink_endpoint_t`)
   - Represents one end of a network connection
   - Has TX and RX queues for packet buffering
   - Supports both polling and callback modes
   - Simulates network characteristics

3. **Packet Queues** (`vlink_queue_t`)
   - Thread-safe ring buffers
   - Support blocking and non-blocking operations
   - Configurable size (default: 256 packets)

### Features

- **Thread-safe**: All operations use proper locking
- **Callback mode**: Asynchronous packet delivery via callbacks
- **Polling mode**: Explicit receive calls for synchronous operation
- **Network simulation**:
  - Bandwidth limiting (Mbps)
  - Latency simulation (microseconds)
  - Packet loss simulation (probability)
- **Statistics tracking**: TX/RX packets, bytes, drops, errors
- **Flexible topology**: Ring, line, mesh, or custom

## Building

### Build Virtual Link Simulator

```bash
cd /home/heng/workdir/doca/three_port_switch
make -f Makefile.vlink
```

### Build and Run Tests

```bash
make -f Makefile.vlink test
```

## Usage

### Basic Usage

Run a 4-switch ring topology with test traffic:

```bash
./vlink_switch_sim -n 4 -t ring -s
```

### Command Line Options

- `-n NUM`: Number of switches (2-16, default: 4)
- `-t TOPO`: Topology type (ring, line, mesh, default: ring)
- `-s`: Send test traffic periodically
- `-h`: Show help

### Examples

**Ring topology (4 switches):**
```bash
./vlink_switch_sim -n 4 -t ring -s
```

**Line topology (8 switches):**
```bash
./vlink_switch_sim -n 8 -t line -s
```

**Mesh topology (6 switches):**
```bash
./vlink_switch_sim -n 6 -t mesh -s
```

## Topology Types

### Ring Topology

Switches connected in a ring:
- Switch 0 ↔ Switch 1 ↔ Switch 2 ↔ ... ↔ Switch N-1 ↔ Switch 0
- Each packet can travel full circle
- Good for testing loop handling

```
     SW0 ---- SW1
      |        |
     SW3 ---- SW2
```

### Line Topology

Switches connected in a line:
- Switch 0 ↔ Switch 1 ↔ Switch 2 ↔ ... ↔ Switch N-1
- End-to-end communication
- Good for testing multi-hop forwarding

```
SW0 ---- SW1 ---- SW2 ---- SW3
```

### Mesh Topology

Partial mesh (limited by 3-port switch constraint):
- Each switch connected to multiple others
- Better redundancy
- Good for testing path selection

```
    SW0 ─┬─ SW1
     │   │   │
    SW3 ─┴─ SW2
```

## API Reference

### Manager Operations

```c
/* Initialize manager */
int vlink_manager_init(vlink_manager_t *mgr);

/* Cleanup manager */
void vlink_manager_cleanup(vlink_manager_t *mgr);
```

### Link Operations

```c
/* Create a virtual link */
int vlink_create(vlink_manager_t *mgr, const char *name,
                 uint32_t bandwidth_mbps, uint32_t latency_us,
                 float loss_rate, uint32_t *link_id);

/* Connect two links bidirectionally */
int vlink_connect(vlink_manager_t *mgr, uint32_t link_id1, uint32_t link_id2);

/* Start/stop link */
int vlink_start(vlink_manager_t *mgr, uint32_t link_id);
int vlink_stop(vlink_manager_t *mgr, uint32_t link_id);
```

### Data Operations

```c
/* Send packet */
int vlink_send(vlink_manager_t *mgr, uint32_t link_id,
               const uint8_t *data, uint16_t size);

/* Receive packet (polling mode) */
int vlink_recv(vlink_manager_t *mgr, uint32_t link_id,
               uint8_t *data, uint16_t *size, uint16_t max_size);

/* Set RX callback (async mode) */
int vlink_set_rx_callback(vlink_manager_t *mgr, uint32_t link_id,
                          void (*callback)(void *ctx, const uint8_t *data, uint16_t size),
                          void *ctx);
```

### Statistics

```c
/* Get statistics */
int vlink_get_stats(vlink_manager_t *mgr, uint32_t link_id, vlink_stats_t *stats);

/* Reset statistics */
int vlink_reset_stats(vlink_manager_t *mgr, uint32_t link_id);

/* Print all statistics */
void vlink_print_stats(vlink_manager_t *mgr);
```

## Integration with Three-Port Switch

Each switch instance has three virtual links:

1. **Port 0 (PCI)**: Host communication (high bandwidth, low latency)
   - 100 Gbps bandwidth
   - 1 μs latency
   
2. **Port 1 (Eth0)**: First Ethernet port (network link)
   - 10 Gbps bandwidth
   - 10 μs latency
   
3. **Port 2 (Eth1)**: Second Ethernet port (network link)
   - 10 Gbps bandwidth
   - 10 μs latency

### Forwarding Logic

The three-port switch implements circular forwarding:
- Port 0 (PCI) → Port 1 (Eth0)
- Port 1 (Eth0) → Port 2 (Eth1)
- Port 2 (Eth1) → Port 0 (PCI)

### Multi-Switch Example

In a 4-switch ring topology:

1. Packet injected on Switch 0, Port 0 (PCI)
2. Forwarded to Switch 0, Port 1 (Eth0)
3. Sent over virtual link to Switch 1, Port 0 (Eth0)
4. Switch 1 forwards to Port 2 (Eth1)
5. Sent over virtual link to Switch 2, Port 0 (Eth0)
6. And so on...

## Testing

### Unit Tests

Run comprehensive unit tests:

```bash
make -f Makefile.vlink test
```

Tests cover:
- Manager initialization
- Link creation
- Send/receive operations
- Callback mode
- Statistics tracking
- Packet loss simulation
- Latency simulation

### Quick Tests

```bash
# Ring topology
make -f Makefile.vlink test-ring

# Line topology  
make -f Makefile.vlink test-line

# Mesh topology
make -f Makefile.vlink test-mesh
```

### Manual Testing

1. **Start simulator:**
   ```bash
   ./vlink_switch_sim -n 4 -t ring
   ```

2. **In another terminal, monitor process:**
   ```bash
   ps aux | grep vlink_switch_sim
   ```

3. **Stop with Ctrl+C** to see final statistics

## Performance Characteristics

### Packet Queue
- Size: 256 packets per queue
- Thread-safe with mutex locking
- Condition variables for blocking operations

### Latency
- Configurable per-link latency (microseconds)
- Uses `usleep()` for simulation
- Minimum: 0 μs, typical: 1-1000 μs

### Throughput
- Limited by queue size and packet processing rate
- Typical: ~10K packets/sec per link in callback mode
- Polling mode can be faster but requires active CPU

### Packet Loss
- Probabilistic: 0.0 (no loss) to 1.0 (100% loss)
- Uses random number generator
- Good for testing error handling

## Debugging

### Enable Verbose Output

Add debug prints in callback functions:

```c
static void eth0_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    printf("DEBUG: Received %u bytes on eth0\n", size);
    // ... rest of callback
}
```

### Check Statistics

Press Ctrl+C to stop the simulator and view statistics:
- Per-port TX/RX counters
- Drop counters
- Per-link statistics

### GDB Debugging

```bash
gdb ./vlink_switch_sim
(gdb) break eth0_rx_callback
(gdb) run -n 4 -t ring -s
```

## Limitations

1. **Software simulation**: Not real-time, timing is approximate
2. **Three ports only**: Switch has only 3 ports (1 PCI + 2 Ethernet)
3. **Simple forwarding**: No MAC learning, just fixed forwarding
4. **No QoS**: All packets treated equally
5. **No flow control**: Can overflow queues if not drained

## Future Enhancements

Possible improvements:

- [ ] Dynamic topology reconfiguration
- [ ] Packet filtering and inspection
- [ ] Traffic generation patterns
- [ ] Performance profiling
- [ ] PCAP capture support
- [ ] Multiple switch types
- [ ] QoS queue support
- [ ] Flow control mechanisms

## Troubleshooting

### "Queue full" errors
- Increase `VLINK_QUEUE_SIZE` in `virtual_link.h`
- Reduce packet injection rate
- Check if RX callbacks are processing packets

### High CPU usage
- Normal in callback mode (threads actively polling)
- Use polling mode for lower CPU usage
- Add sleep in RX threads

### Packets not delivered
- Verify links are connected with `vlink_connect()`
- Check if links are started with `vlink_start()`
- Verify callbacks are set correctly
- Check statistics for drops

### Deadlock
- Ensure proper mutex usage
- Don't call vlink functions from within callbacks
- Use separate threads for TX and RX

## Examples

See `test_virtual_link.c` for comprehensive examples of:
- Creating links
- Connecting topologies
- Sending/receiving packets
- Using callbacks
- Reading statistics

## License

This virtual link infrastructure is part of the DOCA three-port switch project.

## Contact

For questions or issues, refer to the main project documentation.
