# ARP and UDP Traffic Analysis

## Current Situation

The virtual host implementation is **NOT sending ARP packets before UDP packets**.

### What's Happening Now

1. **Direct MAC Address Configuration** (vhost_switch_test.c:326-327)
   ```c
   config.dst_mac[0] = 0x02;
   config.dst_mac[5] = dst_host;
   ```
   - MAC addresses are pre-configured
   - No ARP discovery process

2. **UDP Packets Sent Immediately** (virtual_host.c:45-51)
   ```c
   uint16_t pkt_size = vhost_build_udp_packet(
       packet, sizeof(packet),
       host->pktgen.dst_mac,  // Uses pre-configured MAC
       host->config.mac_addr,
       host->pktgen.dst_ip,
       host->config.ip_addr,
       host->pktgen.dst_port, 12345,
       (uint8_t *)"Test packet", 11
   );
   ```

### Why This Matters for DOCA Switch

In a real network:
- Hosts don't know MAC addresses of other hosts initially
- ARP protocol is used to resolve IP → MAC address mapping
- Switch must learn MAC addresses and forward ARP broadcasts
- Only after ARP resolution can UDP/TCP traffic flow

## Recommendations

### Option 1: Add ARP Support (Most Realistic)

Implement proper ARP protocol:
1. Send ARP request (broadcast)
2. Switch floods ARP request to all ports
3. Target host sends ARP reply
4. Switch learns MAC addresses
5. Source host caches MAC and sends UDP

### Option 2: Pre-populate ARP Cache (Simpler)

Simulate having ARP cache already populated:
- Keep current pre-configured MAC addresses
- Document this as a simplification
- Note that real DOCA implementation would need full ARP support

### Option 3: Broadcast Initial Packets (Middle Ground)

Send first few packets to broadcast MAC (ff:ff:ff:ff:ff:ff):
- Switch learns source MAC from first packet
- Eventually packets reach destination
- More realistic than pre-configuration

## Impact on Queue Filling Issue

The lack of ARP might be contributing to the queue filling issue because:

1. **No MAC Learning Phase**: Switch doesn't have time to learn MAC addresses
2. **Immediate Flood of Traffic**: UDP packets start immediately
3. **Potential Broadcast Storm**: If switch doesn't know destinations, it broadcasts everything

## Recommendation

For the current stress test issue, I recommend **Option 2** (document the simplification) for now, but add proper ARP support in future iterations if you plan to test with real DOCA switch hardware.

The queue filling issue is more likely related to:
- Queue size (already investigated - 1024 helped)
- Packet rate (1000 pps × 8 hosts = 8000 pps total)
- Switch processing speed
- Virtual link implementation

Would you like me to add ARP support, or should we focus on the queue/performance issues first?
