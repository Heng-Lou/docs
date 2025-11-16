# QoS Features in Three-Port Switch

## Overview

The three-port switch now includes Quality of Service (QoS) support with IP DSCP and VLAN PCP classification capabilities.

## Features Added

### 1. QoS Queue Infrastructure

- **8 Priority Queues per Port**: Each port has 8 distinct priority levels (0-7)
- **Queue 7**: Highest priority (Expedited Forwarding)
- **Queue 0**: Lowest priority (Best Effort)

### 2. IP DSCP Classification

The switch classifies packets based on IP Differentiated Services Code Point (DSCP) values:

| Priority | DSCP Value | Traffic Class | Description |
|----------|------------|---------------|-------------|
| 7 | 46 (EF) | Expedited Forwarding | Real-time, low-latency traffic (VoIP) |
| 6 | 32-38 (AF4x) | Assured Forwarding 4 | Premium traffic |
| 5 | 24-30 (AF3x) | Assured Forwarding 3 | High priority data |
| 4 | 16-22 (AF2x) | Assured Forwarding 2 | Medium priority data |
| 3 | 8-14 (AF1x) | Assured Forwarding 1 | Standard data |
| 2 | 8 (CS1) | Class Selector 1 | Scavenger traffic |
| 1 | N/A | Low priority | Low priority traffic |
| 0 | 0 | Best Effort | Default traffic |

### 3. VLAN PCP Support

The switch can also classify based on VLAN Priority Code Point (PCP):

- **3-bit field** in VLAN tag (802.1Q)
- **Values 0-7** map directly to queue priorities
- PCP 7 → Queue 7 (highest)
- PCP 0 → Queue 0 (lowest)

## Architecture

### Data Structures

```c
/* QoS queue entry */
struct qos_queue_entry {
    uint8_t priority;      /* 0-7, where 7 is highest */
    uint16_t port_id;
    uint64_t enqueued;     /* Packets enqueued */
    uint64_t dequeued;     /* Packets dequeued */
    uint64_t dropped;      /* Packets dropped */
};

/* Per-port configuration with QoS stats */
struct port_config {
    uint16_t port_id;
    char port_name[64];
    enum { PORT_TYPE_PCI_EMU, PORT_TYPE_ETHERNET } type;
    uint64_t queue_stats[NB_QOS_QUEUES];  /* Per-queue counters */
};
```

### Classification Functions

```c
/* Map VLAN PCP (0-7) to queue priority */
uint8_t vlan_pcp_to_queue(uint8_t pcp);

/* Map IP DSCP (0-63) to queue priority */
uint8_t ip_dscp_to_queue(uint8_t dscp);
```

## Statistics and Monitoring

The switch tracks QoS statistics per port and per queue:

- **Packets enqueued**: Total packets assigned to each queue
- **Packets dequeued**: Packets successfully transmitted
- **Packets dropped**: Packets dropped due to congestion
- **Total QoS classified**: Overall count of QoS-classified packets

Statistics are displayed every 5 seconds showing:
```
===================================
      Switch Statistics
===================================
Packets forwarded:      12345
Packets dropped:        0
Packets QoS classified: 12345

Port 0 (PCI_EMU) QoS Queue Statistics:
  Q7 [EF (Highest)]: enq=100 deq=100 drop=0
  Q6 [AF4x]: enq=200 deq=200 drop=0
  Q4 [AF2x]: enq=500 deq=500 drop=0
  Q0 [Best Effort]: enq=11545 deq=11545 drop=0
===================================
```

## Implementation Notes

### Current Implementation

The current version implements:
1. **Data structures** for tracking QoS queues and statistics
2. **Classification functions** for DSCP and VLAN PCP mapping
3. **Statistics tracking** per queue and per port
4. **Enhanced logging** showing QoS classification information

### Hardware Offload Considerations

For full hardware offload of QoS in DOCA Flow:
- Use DOCA Flow's RSS (Receive Side Scaling) to direct traffic to specific queues
- Configure hairpin queues for hardware-based QoS
- Use meter actions for rate limiting per priority class
- Leverage DOCA Flow's priority field in pipe entries

### Future Enhancements

Potential improvements for production use:
1. **Hardware-accelerated QoS**: Use DOCA Flow RSS with queue selection
2. **Traffic shaping**: Add rate limiting per queue
3. **Weighted Fair Queuing**: Implement WFQ scheduling
4. **Congestion management**: Add RED/WRED for active queue management
5. **Policy-based QoS**: Configuration file for custom DSCP/PCP mappings

## Testing QoS

To test QoS functionality:

### 1. Generate traffic with different DSCP values:
```bash
# High priority traffic (EF)
iperf3 -c <dest> --dscp 46

# Medium priority traffic (AF41)
iperf3 -c <dest> --dscp 34

# Best effort traffic
iperf3 -c <dest> --dscp 0
```

### 2. Monitor QoS statistics:
```bash
# Watch the switch output for QoS statistics
# Statistics are displayed every 5 seconds
```

### 3. Test VLAN PCP:
```bash
# Use tools like scapy to send packets with VLAN tags
from scapy.all import *
pkt = Ether()/Dot1Q(prio=7)/IP(dst="10.0.0.1")/ICMP()
sendp(pkt, iface="eth0")
```

## Configuration

### Queue Depth
```c
#define NB_QOS_QUEUES 8        /* 8 priority queues */
#define MAX_QUEUE_DEPTH 1024   /* Maximum queue depth */
```

### DSCP to Priority Mapping

Modify `ip_dscp_to_queue()` function to customize DSCP mappings according to your network policy.

### VLAN PCP Mapping

Modify `vlan_pcp_to_queue()` function for custom PCP-to-queue mappings.

## References

- **RFC 2474**: Definition of the Differentiated Services Field (DS Field)
- **RFC 3246**: Expedited Forwarding PHB
- **IEEE 802.1Q**: VLAN tagging standard with PCP field
- **NVIDIA DOCA Flow**: Hardware offload programming guide
- **DOCA QoS API**: Queue management and scheduling

## Building

Build the switch with QoS support:
```bash
cd /home/heng/workdir/doca/three_port_switch
./build.sh
```

## Running

Run the switch (requires root):
```bash
sudo ./build/doca_three_port_switch -a <PCI_0> -a <PCI_1> -a <PCI_2> --
```

The switch will display QoS statistics every 5 seconds showing traffic classification and queue utilization.
