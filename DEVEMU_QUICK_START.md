# DevEmu Quick Start - What Can You Do?

## TL;DR - DevEmu in One Minute

**DevEmu = Virtual BlueField Hardware for Development**

Instead of buying expensive BlueField DPU hardware, you can:
- ✅ Emulate PCI devices on your laptop
- ✅ Test host-device communication
- ✅ Validate DMA operations
- ✅ Debug driver code
- ✅ Run DPA code with virtual hardware

**Think of it as:** Docker for BlueField hardware!

---

## What You Can Do Right Now

### 1. List Virtual Devices (30 seconds)

```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list
meson build && ninja -C build
sudo ./build/doca_devemu_pci_device_list
```

### 2. Create Virtual PCI Device (1 minute)

```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
meson build && ninja -C build
sudo ./build/doca_devemu_pci_device_hotplug

# Verify on host
lspci | grep -i mellanox
# You'll see a virtual Mellanox device!
```

### 3. Test Host-Device Communication (2 minutes)

```bash
# Build both sides
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_stateful_region
cd dpu && meson build && ninja -C build && cd ..
cd host && meson build && ninja -C build && cd ..

# Terminal 1: DPU monitors BAR writes
sudo ./dpu/build/doca_devemu_pci_device_stateful_region_dpu -v <VUID>

# Terminal 2: Host writes to BAR
sudo ./host/build/doca_devemu_pci_device_stateful_region_host \
    -p <PCI_ADDR> -g <VFIO_GROUP> -d 0x12345678

# DPU will log: "Received write: 0x12345678"
```

---

## Core Features at a Glance

| Feature | What It Does | Build Time | Complexity |
|---------|--------------|------------|------------|
| **Device List** | Show virtual devices | 30s | Easy ✅ |
| **Hot-Plug** | Add/remove devices | 1min | Easy ✅ |
| **Stateful Region** | BAR read/write | 2min | Medium |
| **Doorbell** | Event notifications | 3min | Medium |
| **MSI-X** | Interrupt testing | 3min | Medium |
| **DMA** | Memory transfers | 3min | Medium |

---

## Real-World Use Cases

### Use Case 1: Three-Port Switch Port 0

**Problem:** Need PCI port for switch, don't have hardware.

**Solution:**
```bash
# Create virtual PCI device
sudo /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug/build/doca_devemu_pci_device_hotplug

# Use in switch
sudo ./three_port_switch/build/doca_three_port_switch \
    -a <DevEmu_PCI> \    # Virtual Port 0
    -a enp0s1 \          # Real Port 1
    -a enp0s2 --         # Real Port 2
```

### Use Case 2: Host Driver Development

**Problem:** Writing VFIO driver, need to test without DPU.

**Solution:**
```bash
# 1. Create virtual device
sudo ./devemu_pci_device_hotplug/build/doca_devemu_pci_device_hotplug

# 2. Develop driver with VFIO
# Your driver sees a real PCI device!

# 3. Test DMA, interrupts, BAR access
# All works like real hardware!
```

### Use Case 3: DPA Kernel Testing

**Problem:** Testing DPA doorbell handler.

**Solution:**
```bash
# Terminal 1: Start DPA handler (on "DPU")
sudo ./devemu_pci_device_db/dpu/build/doca_devemu_pci_device_db_dpu -v <VUID>

# Terminal 2: Ring doorbell (from "host")
sudo ./devemu_pci_device_db/host/build/doca_devemu_pci_device_db_host \
    -p <PCI> -g <GROUP> -d 0xABCD

# DPA handler receives and processes doorbell!
```

---

## Architecture Overview

### Without DevEmu (Need Hardware)

```
┌──────────┐           ┌─────────────────┐
│   Host   │◄─ PCIe ──►│  BlueField DPU  │
│  (x86)   │           │   (Hardware)    │
└──────────┘           └─────────────────┘
     $$$$                    $$$$
   Expensive!            Expensive!
```

### With DevEmu (No Hardware)

```
┌────────────────────────────────────┐
│         Your Laptop/Server         │
│                                    │
│  ┌──────────┐    ┌──────────────┐ │
│  │   Host   │    │ BlueField    │ │
│  │  (VFIO)  │◄──►│ (Emulated)   │ │
│  └──────────┘    └──────────────┘ │
│         Virtual PCIe               │
└────────────────────────────────────┘
         $0 - FREE!
```

---

## What You CAN Test ✅

### Hardware Features
- ✅ **PCI device enumeration** - lspci shows device
- ✅ **BAR regions** - Memory-mapped I/O
- ✅ **Doorbells** - Event notifications
- ✅ **MSI-X interrupts** - Interrupt delivery
- ✅ **DMA operations** - Host-device transfers
- ✅ **Hot-plug/unplug** - Dynamic devices
- ✅ **VFIO drivers** - Linux driver interface

### Software Features
- ✅ **DPA kernels** - Device code execution
- ✅ **RPC mechanisms** - Host-DPA communication
- ✅ **Event handling** - Asynchronous events
- ✅ **Memory management** - DMA buffers
- ✅ **Error handling** - FLR, recovery

---

## What You CANNOT Test ❌

### Performance
- ❌ **Line-rate forwarding** - Needs hardware acceleration
- ❌ **Real packet processing** - Software emulation only
- ❌ **DPDK performance** - Virtual devices are slow
- ❌ **Network benchmarks** - Not representative

### Production
- ❌ **Deploy to production** - Development only
- ❌ **Real network traffic** - Simulated only
- ❌ **Hardware offload** - Software path

---

## Sample Comparison

### By Complexity

**Beginner** (Start Here!)
1. `devemu_pci_device_list` - Just list devices
2. `devemu_pci_device_hotplug` - Create/destroy devices

**Intermediate**
3. `devemu_pci_device_stateful_region` - BAR read/write
4. `devemu_pci_device_dma` - Memory transfers

**Advanced** (Needs DPA)
5. `devemu_pci_device_db` - Doorbell + DPA handler
6. `devemu_pci_device_msix` - Interrupts + DPA

### By Use Case

**Device Management**
- `devemu_pci_device_list` - List devices
- `devemu_pci_device_hotplug` - Hot-plug/unplug
- `devemu_vfs_list_devices` - VF management

**Host-Device Communication**
- `devemu_pci_device_stateful_region` - BAR access
- `devemu_pci_device_db` - Doorbells
- `devemu_pci_device_dma` - DMA transfers

**Interrupt Testing**
- `devemu_pci_device_msix` - MSI-X interrupts

---

## Quick Commands Reference

### Build Sample

```bash
cd /opt/mellanox/doca/samples/doca_devemu/<SAMPLE_NAME>
meson build
ninja -C build
```

### Run Sample (DPU side)

```bash
sudo ./build/doca_devemu_<sample>_dpu -v <VUID>
```

### Run Sample (Host side)

```bash
sudo ./build/doca_devemu_<sample>_host -p <PCI_ADDR> -g <VFIO_GROUP>
```

### Get VUID

```bash
sudo /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list/build/doca_devemu_pci_device_list | grep VUID | awk '{print $2}'
```

### Get PCI Address

```bash
lspci | grep -i mellanox | awk '{print $1}'
```

### Get VFIO Group

```bash
ls /dev/vfio/ | grep -v vfio
```

---

## Integration Paths

### Path 1: Standalone Testing

```bash
# Just test DevEmu samples
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
meson build && ninja -C build
sudo ./build/doca_devemu_pci_device_hotplug
```

**Good for:** Learning DevEmu, understanding PCI emulation

### Path 2: Three-Port Switch Integration

```bash
# Use DevEmu for Port 0
# Create virtual PCI device
sudo ./devemu_pci_device_hotplug/build/doca_devemu_pci_device_hotplug

# Run switch with DevEmu port
cd /home/heng/workdir/doca/three_port_switch
sudo ./build/doca_three_port_switch -a <DevEmu_PCI> -a net_null0 -a net_null1 --
```

**Good for:** Testing switch logic, host communication

### Path 3: Custom Application

```bash
# Your app uses DevEmu PCI device
# Link against DOCA DevEmu libraries
# Create custom PCI type
# Build host + DPU sides
```

**Good for:** Custom drivers, new applications

---

## Troubleshooting

### Problem: No devices listed

```bash
sudo /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list/build/doca_devemu_pci_device_list
# Shows: No devices found
```

**Solution:** Create a device first!
```bash
sudo ./devemu_pci_device_hotplug/build/doca_devemu_pci_device_hotplug
```

### Problem: lspci doesn't show device

**Check:**
```bash
# 1. Device created?
sudo ./devemu_pci_device_list/build/doca_devemu_pci_device_list

# 2. PCI rescan?
sudo sh -c 'echo 1 > /sys/bus/pci/rescan'

# 3. Check dmesg
dmesg | tail -20
```

### Problem: VFIO permission denied

```bash
# Need root or vfio group permissions
sudo chown $USER /dev/vfio/<group>

# Or just use sudo
sudo ./build/doca_devemu_<sample>_host ...
```

---

## Learning Path

### Week 1: Basics
1. Build and run `devemu_pci_device_list`
2. Build and run `devemu_pci_device_hotplug`
3. Verify with `lspci`

### Week 2: Communication
4. Build `devemu_pci_device_stateful_region`
5. Test host writes → DPU reads
6. Understand BAR regions

### Week 3: DMA
7. Build `devemu_pci_device_dma`
8. Test host-DPU memory transfers
9. Understand IOMMU mapping

### Week 4: Advanced
10. Build DPA samples (`db`, `msix`)
11. Test DPA event handling
12. Integrate with three-port switch

---

## Key Takeaways

### DevEmu = Development Tool ✅

**Use DevEmu for:**
- Rapid development without hardware
- Testing host-device interaction
- Debugging drivers and applications
- CI/CD automated testing
- Learning BlueField architecture

### DevEmu ≠ Production ❌

**Don't use DevEmu for:**
- Performance testing
- Production deployment
- Network benchmarks
- Hardware validation

---

## Next Steps

### Option 1: Quick Test (5 minutes)

```bash
# Build and run device list
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list
meson build && ninja -C build && sudo ./build/doca_devemu_pci_device_list
```

### Option 2: Full Testing (30 minutes)

```bash
# Build all samples
cd /opt/mellanox/doca/samples/doca_devemu
./build_dpacc_samples.sh  # Builds DPA samples
```

### Option 3: Switch Integration (1 hour)

```bash
# Integrate DevEmu with three-port switch
# See DEVEMU_CAPABILITIES.md for details
```

---

## Documentation

- **Full guide:** `DEVEMU_CAPABILITIES.md`
- **DPA testing:** `DPA_TESTING_TOOLS.md`
- **Switch integration:** `THREE_PORT_SWITCH_SUMMARY.md`
- **Official docs:** `/opt/mellanox/doca/samples/doca_devemu/README.md`

---

**DevEmu lets you develop BlueField applications on a laptop! Start experimenting!** ✅
