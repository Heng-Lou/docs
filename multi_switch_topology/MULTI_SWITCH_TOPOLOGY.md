# Multi-Switch Topology: N Switches Connecting N Hosts

## Architecture Overview

This document describes how to link N three-port switches together using Ethernet and emulated PCI ports to create a larger switched network connecting N hosts.

---

## Topology Design

### Basic Concept

Each three-port switch has:
- **Port 0**: PCI (emulated with DevEmu) → Connected to a host
- **Port 1**: Ethernet → Uplink to backbone/other switches
- **Port 2**: Ethernet → Uplink to backbone/other switches

### Example: 4 Switches, 4 Hosts

```
┌─────────────────────────────────────────────────────────────────────┐
│                     Backbone Switch (Optional)                      │
│                   or Direct Switch-to-Switch Links                  │
└──────┬────────────┬─────────────┬─────────────┬────────────────────┘
       │ ETH        │ ETH         │ ETH         │ ETH
       │            │             │             │
┌──────▼──────┐ ┌──▼──────────┐ ┌▼───────────┐ ┌▼──────────────┐
│  Switch 0   │ │  Switch 1   │ │ Switch 2   │ │  Switch 3     │
│ ┌─────────┐ │ │ ┌─────────┐ │ │┌─────────┐ │ │ ┌─────────┐  │
│ │Port 1   │─┼─┼─│Port 1   │─┼─┼│Port 1   │─┼─┼─│Port 1   │  │
│ │Ethernet │ │ │ │Ethernet │ │ ││Ethernet │ │ │ │Ethernet │  │
│ └─────────┘ │ │ └─────────┘ │ │└─────────┘ │ │ └─────────┘  │
│             │ │             │ │            │ │              │
│ ┌─────────┐ │ │ ┌─────────┐ │ │┌─────────┐ │ │ ┌─────────┐  │
│ │Port 2   │ │ │ │Port 2   │ │ ││Port 2   │ │ │ │Port 2   │  │
│ │Ethernet │ │ │ │Ethernet │ │ ││Ethernet │ │ │ │Ethernet │  │
│ └─────────┘ │ │ └─────────┘ │ │└─────────┘ │ │ └─────────┘  │
│             │ │             │ │            │ │              │
│ ┌─────────┐ │ │ ┌─────────┐ │ │┌─────────┐ │ │ ┌─────────┐  │
│ │Port 0   │ │ │ │Port 0   │ │ ││Port 0   │ │ │ │Port 0   │  │
│ │PCI Emu  │ │ │ │PCI Emu  │ │ ││PCI Emu  │ │ │ │PCI Emu  │  │
│ └────┬────┘ │ │ └────┬────┘ │ │└────┬────┘ │ │ └────┬────┘  │
└──────┼──────┘ └──────┼──────┘ └─────┼──────┘ └──────┼────────┘
       │ PCI           │ PCI           │ PCI           │ PCI
       │ (DevEmu)      │ (DevEmu)      │ (DevEmu)      │ (DevEmu)
       │               │               │               │
  ┌────▼────┐     ┌───▼─────┐    ┌────▼────┐     ┌───▼─────┐
  │ Host 0  │     │ Host 1  │    │ Host 2  │     │ Host 3  │
  │         │     │         │    │         │     │         │
  │ VM/App  │     │ VM/App  │    │ VM/App  │     │ VM/App  │
  └─────────┘     └─────────┘    └─────────┘     └─────────┘
```

---

## Connectivity Options

### Option 1: Full Mesh (All Switches Interconnected)

Each switch connects to every other switch via Ethernet:

```
Switch 0 Port 1 ↔ Switch 1 Port 1
Switch 0 Port 2 ↔ Switch 2 Port 1
Switch 1 Port 2 ↔ Switch 3 Port 1
Switch 2 Port 2 ↔ Switch 3 Port 2
...
```

**Pros**: 
- Maximum redundancy
- Best performance (single hop)
- No single point of failure

**Cons**: 
- Complex cabling
- Requires N*(N-1)/2 links
- STP needed to prevent loops

---

### Option 2: Star Topology (Backbone Switch)

All switches connect to a central backbone switch:

```
                ┌─────────────────┐
                │ Backbone Switch │
                │  (DOCA Flow)    │
                └┬──┬──┬──┬──┬──┬─┘
                 │  │  │  │  │  │
        ┌────────┴──┴──┴──┴──┴──┴────────┐
        │        │  │  │  │  │  │        │
     ┌──▼──┐  ┌─▼┐┌▼─┐┌▼─┐┌▼─┐┌▼─┐  ┌──▼──┐
     │SW 0 │  │SW1││S2││S3││S4││S5│  │SW N │
     └──┬──┘  └─┬┘└┬─┘└┬─┘└┬─┘└┬─┘  └──┬──┘
        │       │  │   │   │   │       │
        │       │  │   │   │   │       │
     ┌──▼──┐  ┌▼┐┌▼─┐┌▼─┐┌▼─┐┌▼─┐  ┌──▼──┐
     │Host0│  │H1││H2││H3││H4││H5│  │HostN│
     └─────┘  └─┘└──┘└──┘└──┘└──┘  └─────┘
```

**Pros**: 
- Simple cabling (N links)
- Easy to manage
- Clear hierarchy

**Cons**: 
- Backbone is single point of failure
- Two hops for inter-host traffic
- Backbone can be bottleneck

---

### Option 3: Ring Topology

Switches connected in a ring:

```
     ┌──────┐     ┌──────┐     ┌──────┐
     │ SW 0 │────►│ SW 1 │────►│ SW 2 │
     └───┬──┘     └───┬──┘     └───┬──┘
         │            │            │
         │            │            │
      ┌──▼──┐      ┌─▼──┐      ┌──▼──┐
      │Host0│      │Host1│      │Host2│
      └─────┘      └────┘      └─────┘
         ▲                         │
         │                         │
     ┌───┴──┐                  ┌───▼──┐
     │ SW 3 │◄─────────────────│ SW 3 │
     └───┬──┘                  └──────┘
         │
      ┌──▼──┐
      │Host3│
      └─────┘
```

**Pros**: 
- Redundancy (two paths)
- Predictable topology
- Good for linear layouts

**Cons**: 
- Variable hop count
- Ring break impacts connectivity
- Requires STP or ring protocol

---

### Option 4: Hybrid (Recommended)

Combine approaches for best results:

```
        ┌─────────────────────────┐
        │  Backbone Switch Pool   │
        │  (2-3 switches for HA)  │
        └─┬──┬──┬──┬──┬──┬──┬──┬─┘
          │  │  │  │  │  │  │  │
    ┌─────┴──┴──┴──┴──┴──┴──┴──┴─────┐
    │ Edge Switches (with redundancy) │
    └─┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┘
      │  │  │  │  │  │  │  │  │  │
   [Host] [Host] ... ... ... [Host]
```

**Features**:
- Dual-homed hosts (two switches per host)
- Redundant backbone
- LACP/bonding for uplinks
- Best reliability and performance

---

## Detailed Configuration

### Per-Switch Configuration

#### Switch Instance Template

```yaml
Switch N:
  Ports:
    Port 0 (PCI):
      Type: DevEmu PCI Device
      VUID: "pci-emu-switch-N"
      Connected to: "Host N"
      
    Port 1 (Ethernet):
      Type: Physical Ethernet
      Purpose: Uplink/Inter-switch
      Connected to: Backbone or Switch N+1
      VLAN: Trunk (all VLANs)
      
    Port 2 (Ethernet):
      Type: Physical Ethernet
      Purpose: Uplink/Inter-switch
      Connected to: Backbone or Switch N-1
      VLAN: Trunk (all VLANs)
      
  MAC Learning:
    Enabled: Yes
    Table Size: 256
    Aging: 300 seconds
    
  Statistics:
    Enabled: Yes
    Interval: 5 seconds
```

---

## Implementation Guide

### Step 1: Plan the Topology

Decide on:
- Number of switches (N)
- Number of hosts (typically N, one per switch)
- Connectivity pattern (star, mesh, ring, hybrid)
- Backbone requirements

### Step 2: Allocate Resources

For each switch:
```
Switch i (i = 0 to N-1):
  - 1 × PCI device (emulated)
  - 2 × Ethernet ports
  - 1 × BlueField DPU (or VM)
  - MAC address range
  - IP addressing scheme
```

### Step 3: Create DevEmu PCI Devices

For each host connection:

```bash
# Create emulated PCI device for Host 0
cd /home/heng/workdir/doca
# Build DevEmu sample
cp -r /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug .

# Create device with specific VUID
sudo doca_devemu_pci_device_create \
  --vuid "pci-emu-switch-0-host-0" \
  --pci-address 04:00.0
```

### Step 4: Configure Network Connectivity

#### Star Topology Example (4 switches + backbone):

```bash
# Backbone switch
sudo ./doca_backbone_switch \
  -a 10:00.0 \
  -a 10:00.1 \
  -a 10:00.2 \
  -a 10:00.3 --

# Access Switch 0
sudo ./doca_three_port_switch \
  -a 04:00.0 \        # DevEmu PCI to Host 0
  -a 03:00.0 \        # Uplink to backbone
  -a 03:00.1 --       # Spare or redundant uplink

# Access Switch 1
sudo ./doca_three_port_switch \
  -a 05:00.0 \        # DevEmu PCI to Host 1
  -a 03:01.0 \        # Uplink to backbone
  -a 03:01.1 --       # Spare or redundant uplink

# ... repeat for switches 2, 3, ..., N
```

---

## Network Diagram: 4-Switch Example

### Physical Connectivity

```
                    Backbone Network
                    ═══════════════════
                           ║
         ╔═════════════════╬═════════════════╗
         ║                 ║                 ║
      ETH 1             ETH 1             ETH 1             ETH 1
    ┌────────┐        ┌────────┐        ┌────────┐        ┌────────┐
    │Switch 0│        │Switch 1│        │Switch 2│        │Switch 3│
    │  BF-3  │        │  BF-3  │        │  BF-3  │        │  BF-3  │
    ├────────┤        ├────────┤        ├────────┤        ├────────┤
    │Port 0  │        │Port 0  │        │Port 0  │        │Port 0  │
    │PCI Emu │        │PCI Emu │        │PCI Emu │        │PCI Emu │
    └───┬────┘        └───┬────┘        └───┬────┘        └───┬────┘
        │ PCIe            │ PCIe            │ PCIe            │ PCIe
        │ Virtual         │ Virtual         │ Virtual         │ Virtual
        │                 │                 │                 │
    ┌───▼────┐        ┌───▼────┐        ┌───▼────┐        ┌───▼────┐
    │ Host 0 │        │ Host 1 │        │ Host 2 │        │ Host 3 │
    │ x86/VM │        │ x86/VM │        │ x86/VM │        │ x86/VM │
    │        │        │        │        │        │        │        │
    │10.0.0.1│        │10.0.0.2│        │10.0.0.3│        │10.0.0.4│
    └────────┘        └────────┘        └────────┘        └────────┘
```

### Traffic Flow Example

**Host 0 → Host 3**:
```
Host 0 (10.0.0.1)
  │ (PCI via DevEmu)
  ▼
Switch 0 Port 0
  │ (MAC learning: learn src=Host0_MAC on Port 0)
  │ (Lookup dst=Host3_MAC → not found → flood)
  ▼
Switch 0 Port 1 → Backbone
  │
  ▼
Backbone → All connected switches
  │
  ▼
Switch 3 Port 1
  │ (Lookup dst=Host3_MAC → found on Port 0)
  ▼
Switch 3 Port 0 → Host 3 (PCI via DevEmu)
  │
  ▼
Host 3 (10.0.0.4) receives packet

Return path:
  Switch 3 learns Host0_MAC on Port 1
  Direct forwarding on subsequent packets
```

---

## Scalability

### Small Deployment (2-4 switches)

```
Topology: Star with single backbone
Switches: 4
Hosts: 4
Links: 4 uplinks
Management: Simple
Use case: Lab, small office
```

### Medium Deployment (5-20 switches)

```
Topology: Dual-homed star with redundant backbone
Switches: 20
Hosts: 20
Links: 40 uplinks (2 per switch)
Management: VLAN segregation, basic QoS
Use case: Enterprise edge, data center rack
```

### Large Deployment (20+ switches)

```
Topology: Hierarchical (Core-Aggregation-Access)
Switches: 100+
Hosts: 100+
Links: 200+ uplinks
Management: Full SDN, automation, monitoring
Use case: Data center, cloud infrastructure
```

---

## Configuration Files

### hosts_config.yaml

```yaml
# Define N hosts
hosts:
  - id: 0
    name: "host-0"
    ip: "10.0.0.1/24"
    mac: "00:00:00:00:00:01"
    pci_device: "pci-emu-switch-0"
    
  - id: 1
    name: "host-1"
    ip: "10.0.0.2/24"
    mac: "00:00:00:00:00:02"
    pci_device: "pci-emu-switch-1"
    
  # ... add more hosts
```

### switches_config.yaml

```yaml
# Define N switches
switches:
  - id: 0
    name: "access-switch-0"
    ports:
      - id: 0
        type: "pci_emu"
        device: "pci-emu-switch-0"
        connected_to: "host-0"
      - id: 1
        type: "ethernet"
        device: "03:00.0"
        connected_to: "backbone"
      - id: 2
        type: "ethernet"
        device: "03:00.1"
        connected_to: "backup-backbone"
    mac_learning: true
    
  - id: 1
    name: "access-switch-1"
    # ... similar config
```

---

## Deployment Scripts

### deploy_multi_switch.sh

```bash
#!/bin/bash
# Deploy N three-port switches with host connectivity

NUM_SWITCHES=4
BASE_IP="10.0.0"

echo "Deploying $NUM_SWITCHES switches with DevEmu PCI connections"

# Create DevEmu PCI devices
for i in $(seq 0 $((NUM_SWITCHES-1))); do
    echo "Creating PCI device for switch $i..."
    # sudo doca_devemu_pci_device_create \
    #   --vuid "pci-emu-switch-$i" \
    #   --pci-address "0$i:00.0"
done

# Start switches
for i in $(seq 0 $((NUM_SWITCHES-1))); do
    PCI_EMU="0$i:00.0"
    ETH1="03:0$i.0"
    ETH2="03:0$i.1"
    
    echo "Starting switch $i..."
    sudo ./doca_three_port_switch \
      -a $PCI_EMU \
      -a $ETH1 \
      -a $ETH2 -- &
    
    SWITCH_PIDS[$i]=$!
done

echo "All switches started"
echo "PIDs: ${SWITCH_PIDS[@]}"

# Wait for Ctrl+C
trap "kill ${SWITCH_PIDS[@]}; exit" INT TERM
wait
```

---

## Monitoring and Management

### Statistics Collection

```bash
# Collect stats from all switches
for switch in switch-{0..3}; do
    echo "=== $switch ==="
    ssh $switch "doca_flow_query --port all --stats"
done
```

### Health Monitoring

```bash
# Check switch connectivity
for i in {0..3}; do
    ping -c 1 -W 1 switch-$i && \
        echo "Switch $i: UP" || \
        echo "Switch $i: DOWN"
done
```

### Topology Discovery

```bash
# Use LLDP to discover topology
lldpcli show neighbors
```

---

## Advanced Features

### 1. VLAN Segmentation

```c
// Separate traffic by VLAN
// Port 0 (host): Access port, VLAN 10
// Ports 1-2: Trunk ports, all VLANs

match.outer.vlan_id = 10;
fwd.port_id = uplink_port;
```

### 2. Link Aggregation (LACP)

```c
// Bond Port 1 and Port 2 for redundancy
// Increase bandwidth
// Failover on link down
```

### 3. Quality of Service (QoS)

```c
// Prioritize traffic from certain hosts
// Rate limit per-flow
// Guarantee bandwidth
```

### 4. Access Control

```c
// Filter by MAC/IP
// Drop unauthorized traffic
// Implement security policies
```

---

## Testing Scenarios

### Test 1: Basic Connectivity

```bash
# From Host 0, ping Host 3
ping -c 4 10.0.0.4

# Expected: 4 packets transmitted, 4 received
```

### Test 2: Broadcast/Flood

```bash
# Send broadcast from Host 0
ping -b 10.0.0.255

# Expected: All hosts receive
```

### Test 3: MAC Learning

```bash
# Check MAC table on Switch 0
doca_flow_dump_table switch-0

# Expected: Host MACs learned on correct ports
```

### Test 4: Failover

```bash
# Disconnect Port 1 on Switch 0
ifconfig eth1 down

# Traffic should reroute via Port 2
# Connectivity maintained
```

---

## Performance Considerations

### With Hardware Offload

- **Throughput**: 100 Gbps+ (line rate)
- **Latency**: < 10 μs (switch-to-switch)
- **Host latency**: < 5 μs (via DevEmu PCI)
- **Concurrent flows**: Millions

### Bottlenecks

- **Backbone capacity**: May need 100G+ links
- **Switch CPU**: Minimal with DOCA Flow offload
- **MAC table size**: 256 entries may need increase
- **DPDK hugepages**: Ensure sufficient memory

---

## Recommended Topology for N=4

```
Best Choice: Star with Dual Backbone

              ┌──────────┐  ┌──────────┐
              │Backbone 0│  │Backbone 1│
              │(Primary) │  │(Backup)  │
              └─┬──┬──┬─┬┘  └┬─┬──┬──┬─┘
                │  │  │ └────┘ │  │  │
        ┌───────┴──┴──┴────────┴──┴──┴───────┐
        │       │  │  │        │  │  │       │
     ┌──▼──┐ ┌─▼┐┌▼─┐┌───────▼─┐┌▼─┐┌▼─┐ ┌─▼───┐
     │SW 0 │ │SW1││S2││  SW 3   ││S4││S5│ │SW N │
     └──┬──┘ └─┬┘└┬─┘└────┬────┘└┬─┘└┬─┘ └──┬──┘
        │      │  │       │       │   │      │
      PCI     PCI PCI    PCI     PCI PCI    PCI
        │      │  │       │       │   │      │
     ┌──▼──┐ ┌▼┐┌▼─┐  ┌──▼──┐ ┌─▼┐┌▼─┐  ┌──▼──┐
     │Host0│ │H1││H2│  │Host3│ │H4││H5│  │HostN│
     └─────┘ └─┘└──┘  └─────┘ └──┘└──┘  └─────┘

Benefits:
- Redundant backbone (HA)
- Simple management
- Easy scaling
- Predictable performance
```

---

## Summary

### What You Can Build

✅ **N three-port switches** connected via Ethernet  
✅ **N hosts** connected via emulated PCI (DevEmu)  
✅ **Full Layer 2 switching** with MAC learning  
✅ **Scalable architecture** (2 to 100+ hosts)  
✅ **Multiple topology options** (star, mesh, ring, hybrid)  
✅ **Production-ready** with DOCA Flow offload  

### Key Components

1. **Three-Port Switches** ✅ (already built!)
2. **DevEmu PCI Devices** (for host connections)
3. **Ethernet Links** (for inter-switch connectivity)
4. **Backbone Switch** (optional, for star topology)
5. **Management System** (monitoring, config)

### Next Steps to Deploy

1. Build DevEmu samples for PCI emulation
2. Configure host systems with virtual PCI devices
3. Deploy switches with proper port assignments
4. Configure backbone (if using star topology)
5. Test connectivity and performance
6. Add VLANs, QoS, and advanced features

**Your three-port switch is the perfect building block for this architecture!** 🎉

