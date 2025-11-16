# TTL/Hop Limit Quick Reference

## Testing TTL in Simulation

### Run All Tests (Including TTL)
```bash
cd three_port_switch
./switch_sim test
```

### Interactive TTL Testing

```bash
./switch_sim
```

**Send IPv4 packet with various TTL values:**
```
> sendip 0 64 64     # TTL=64, will forward and decrement to 63
> sendip 0 64 2      # TTL=2, will forward and decrement to 1
> sendip 0 64 1      # TTL=1, will be DROPPED
> sendip 0 64 0      # TTL=0, will be DROPPED
```

**Send IPv6 packet with various hop limits:**
```
> sendip6 0 64 128   # Hop=128, will forward and decrement to 127
> sendip6 0 64 1     # Hop=1, will be DROPPED
```

**Simulate ring topology (4 hops before drop):**
```
> sendip 0 64 4      # Port 0, TTL=4 -> 3
> sendip 1 64 3      # Port 1, TTL=3 -> 2
> sendip 2 64 2      # Port 2, TTL=2 -> 1
> sendip 0 64 1      # Port 0, TTL=1 -> DROPPED
> stats              # View TTL expired count
```

## Expected Output

### Successful Forward
```
> sendip 0 64 64
Processing packet: size=64, port=0, TTL=64, is_ipv4=1
  TTL decremented to 63
  Forwarding: port 0 -> port 1
  SUCCESS: Packet forwarded
```

### TTL Expiration
```
> sendip 0 64 1
Processing packet: size=64, port=0, TTL=1, is_ipv4=1
  DROPPED: TTL/hop limit expired (TTL=1)
```

### Statistics
```
> stats

Port 0 (pci_port):
  Enabled: Yes
  MTU: 1500
  RX: 5 packets, 320 bytes, 0 errors
  TX: 3 packets, 192 bytes, 0 errors
  TTL expired: 2 packets    <-- TTL-expired packets
```

## Ring Topology Behavior

### Without TTL (would loop forever)
```
Packet -> Switch 0 -> Switch 1 -> Switch 2 -> Switch 0 -> Switch 1 -> ...
                                                          ↑__________|
                                                          (INFINITE LOOP)
```

### With TTL (terminates after N hops)
```
TTL=64: Switch 0 -> (TTL=63) Switch 1 -> (TTL=62) Switch 2 -> ...
TTL=2:  Switch 0 -> (TTL=1) Switch 1 -> (DROP)
```

## Integration with Multi-Switch

When running multi-switch ring topology:

```bash
cd multi_switch_topology
./deploy_multi_switch.sh 8 ring
```

Each switch automatically:
1. Decrements TTL on every IP packet
2. Drops packets when TTL ≤ 1
3. Tracks TTL-expired packets in statistics
4. Prevents infinite loops in the ring

## Key Takeaways

✅ **Loop Prevention**: TTL prevents infinite packet circulation in rings  
✅ **Automatic**: No configuration needed, works out of the box  
✅ **Layer 3**: Only IP packets (IPv4/IPv6) are protected  
✅ **Statistics**: TTL expirations are tracked per port  
✅ **Performance**: Minimal overhead (~10 cycles per IPv4 packet)  

⚠️ **Limitations**:
- Non-IP traffic (ARP, etc.) not protected by TTL
- No ICMP Time Exceeded messages sent
- Requires hosts to set reasonable initial TTL (typically 64 or 128)
