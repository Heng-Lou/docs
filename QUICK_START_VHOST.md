# Quick Start: Virtual Host Testing

Test BlueField DPU switch topologies without hardware using virtual hosts and switches!

## What You Get

- **Virtual Hosts**: Simulated servers with MAC/IP addresses
- **Virtual Switches**: Three-port switches (PCI + 2 Ethernet)
- **PCI Connectivity**: Each host connects to a switch via virtual PCI
- **Ring Topology**: Switches connected in a ring via Ethernet
- **Packet Generation**: Built-in traffic generator
- **TTL Loop Prevention**: Prevents infinite loops in ring
- **Full Statistics**: Track all packets through the system

## 30-Second Test

```bash
cd three_port_switch
make -f Makefile.vhost
./demo_vhost.sh pktgen
```

That's it! You'll see:
- 4 virtual hosts created
- 4 switches in ring topology
- Hosts connected via PCI to switches
- Packet generation and forwarding
- Complete statistics

## Demo Options

### Basic Demo (No Traffic)
```bash
./demo_vhost.sh basic
```
Creates hosts and switches, no packet generation.

### Packet Generation Demo
```bash
./demo_vhost.sh pktgen
```
4 hosts, 100 pps, 100 packets each.

### Ring Topology Demo
```bash
./demo_vhost.sh ring
```
8 hosts/switches in larger ring.

### Stress Test
```bash
./demo_vhost.sh stress
```
High packet rate stress test.

### Custom Configuration
```bash
./demo_vhost.sh custom <hosts> <pps> <packets> <duration>
```

Example:
```bash
./demo_vhost.sh custom 6 500 1000 30
```
6 hosts, 500 pps, 1000 packets, 30 seconds.

## Manual Control

For full control, use the program directly:

```bash
./vhost_switch_test -n 8 -p -r 200 -c 500 -d 20
```

Options:
- `-n NUM`: Number of hosts/switches (2-32)
- `-p`: Enable packet generation
- `-r RATE`: Packets per second
- `-c COUNT`: Total packets (0=infinite)
- `-d DURATION`: Run duration in seconds

## What Happens

### Topology Created
```
Host0 ─PCI─ Switch0 ─Eth─┐
                         ├─ Ring
Host1 ─PCI─ Switch1 ─Eth─┤
                         ├─ Ring
Host2 ─PCI─ Switch2 ─Eth─┤
                         ├─ Ring
Host3 ─PCI─ Switch3 ─Eth─┘
```

### Traffic Flow
1. Host 0 sends packet destined for Host 1
2. Packet goes via PCI to Switch 0
3. Switch 0 forwards to Eth0 → Switch 1
4. At each switch, TTL is decremented
5. Eventually reaches Switch 1
6. Switch 1 delivers via PCI to Host 1

### Loop Prevention
- Each switch decrements IP TTL
- Packets with TTL=0 are dropped
- Prevents infinite loops in ring

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

Configuring packet generators...
  Host 0 -> Host 1 (100 pps, 100 packets)
  Host 1 -> Host 2 (100 pps, 100 packets)
  Host 2 -> Host 3 (100 pps, 100 packets)
  Host 3 -> Host 0 (100 pps, 100 packets)

✓ All components running!

[Statistics shown after test completes]
```

## Next Steps

1. **Read the guide**: See `three_port_switch/VIRTUAL_HOST_GUIDE.md`
2. **Try different topologies**: Modify the code for line or mesh
3. **Add features**: Implement QoS, VLAN tagging, RSS
4. **Measure performance**: Add latency tracking
5. **Export traffic**: Add pcap support for Wireshark

## Why Use This?

✅ **No hardware needed** - Test before hardware arrives  
✅ **Fast iteration** - Develop and test switch logic quickly  
✅ **Complex scenarios** - Test 32 switches in ring/mesh  
✅ **Traffic generation** - Built-in packet generator  
✅ **Statistics** - Track every packet  
✅ **Debugging** - Easy to add instrumentation  

## Files

- `virtual_host.h/c` - Virtual host implementation
- `vhost_switch_test.c` - Integration test program
- `Makefile.vhost` - Build system
- `demo_vhost.sh` - Demo script
- `VIRTUAL_HOST_GUIDE.md` - Detailed documentation

## Troubleshooting

**No packets forwarded?**
- Check that packet generator is enabled (`-p` flag)
- Verify duration is long enough for packets to be sent
- Look for drops in statistics

**Build errors?**
- Ensure you have gcc and pthread development libraries
- Run `make -f Makefile.vhost clean` first

**Low packet rates?**
- This is software simulation, not hardware speeds
- Higher rates may cause drops due to CPU scheduling
- Use lower rates (<1000 pps) for reliable results

## Have Fun!

This is a complete simulation environment for BlueField DPU development. Experiment, break things, add features, and learn!

For questions or issues, see the documentation in:
- `VIRTUAL_HOST_SUMMARY.md` - Complete implementation summary
- `three_port_switch/VIRTUAL_HOST_GUIDE.md` - User guide
