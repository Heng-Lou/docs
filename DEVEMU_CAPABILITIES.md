# What Can You Do With DevEmu? - Complete Guide

## DevEmu Overview

**DevEmu (Device Emulation)** is NVIDIA's framework for emulating BlueField hardware without requiring physical devices. It allows you to test host-device interactions, DMA operations, interrupts, and PCI communication on a development machine.

---

## Core Capabilities

### 1. **PCI Device Emulation** ✅

Create virtual PCI devices that behave like real BlueField hardware:

```bash
# Emulated devices appear as real PCI devices
lspci
# Shows: 00:05.0 Processing accelerators: Mellanox Technologies MT43244 BlueField-3...

# Host can interact with them using VFIO
ls /dev/vfio/
```

**What you can emulate:**
- ✅ BlueField DPU as PCI device
- ✅ Multiple virtual functions (VFs)
- ✅ PCI configuration space
- ✅ BAR regions (memory-mapped I/O)
- ✅ Hot-plug/unplug events
- ✅ Device capabilities

---

### 2. **Host-Device Communication** ✅

Test bidirectional communication between host and BlueField:

#### a) Stateful Memory Regions
```bash
# Host writes to BAR → BlueField reads
# BlueField writes to BAR → Host reads
```

**Use cases:**
- Control registers
- Status reporting
- Configuration exchange
- Shared memory communication

#### b) Doorbell (DB) Mechanisms
```bash
# Host rings doorbell → DPA thread handles it
# Fast notification path for events
```

**Use cases:**
- Queue notifications
- Event signaling
- Work submission
- Completion notifications

---

### 3. **DMA Operations** ✅

Test Direct Memory Access between host and device:

```bash
# BlueField → Host (device writes to host memory)
# Host → BlueField (device reads from host memory)
```

**What you can test:**
- ✅ Memory mapping (IOMMU)
- ✅ DMA transfers
- ✅ Scatter-gather operations
- ✅ Memory barriers
- ✅ Cache coherency

**Perfect for:**
- Packet DMA testing
- Bulk data transfers
- Ring buffer operations
- Zero-copy networking

---

### 4. **MSI-X Interrupts** ✅

Test interrupt delivery from device to host:

```bash
# BlueField raises MSI-X → Host receives interrupt
# Multiple vectors supported
```

**What you can test:**
- ✅ Interrupt generation
- ✅ Vector mapping
- ✅ Interrupt coalescing
- ✅ Multi-queue notifications

**Use cases:**
- Packet arrival notifications
- DMA completion signaling
- Error reporting
- Queue management

---

### 5. **Hot-Plug/Unplug** ✅

Test dynamic device management:

```bash
# Add device at runtime → PCI rescan → Driver loads
# Remove device → Driver unloads → Device disappears
```

**What you can test:**
- ✅ Device discovery
- ✅ Driver initialization
- ✅ Resource allocation
- ✅ Graceful shutdown
- ✅ Error recovery

---

### 6. **DPA Integration** ✅

Run DPA code with emulated devices:

```bash
# DPA threads handle doorbells
# DPA kernels process interrupts
# DPA code accesses emulated hardware
```

**What you can test:**
- ✅ DPA kernel execution
- ✅ DPA-host interaction
- ✅ RPC mechanisms
- ✅ Event handling
- ✅ DPA debugging

---

## Available DevEmu Samples

### Located in: `/opt/mellanox/doca/samples/doca_devemu/`

| Sample | Purpose | DPA Needed | Complexity |
|--------|---------|------------|------------|
| **devemu_pci_device_list** | List emulated devices | No | Simple |
| **devemu_pci_device_hotplug** | Hot-plug/unplug testing | No | Simple |
| **devemu_pci_device_stateful_region** | BAR region read/write | No | Medium |
| **devemu_pci_device_db** | Doorbell + DPA handler | **Yes** | Medium |
| **devemu_pci_device_msix** | MSI-X interrupts + DPA | **Yes** | Medium |
| **devemu_pci_device_dma** | DMA transfers | No | Medium |
| **devemu_vfs_list_devices** | List VF devices | No | Simple |
| **devemu_vfs_device_hotplug_unplug** | VF hot-plug | No | Simple |

---

## Detailed Sample Descriptions

### 1. PCI Device List ✅
**File:** `devemu_pci_device_list/`

**What it does:**
- Lists all emulated PCI devices
- Shows VUID (Virtual Unique ID)
- Displays PCI addresses (BDF)

**When to use:**
- Verify device creation
- Check device visibility
- Debug device enumeration

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list
meson build
ninja -C build
sudo ./build/doca_devemu_pci_device_list
```

---

### 2. Hot-Plug/Unplug ✅
**File:** `devemu_pci_device_hotplug/`

**What it does:**
- Creates virtual PCI device
- Hot-plugs it to host
- Host discovers device
- Hot-unplugs device
- Device disappears from host

**When to use:**
- Test driver load/unload
- Verify PCI rescan
- Test error recovery
- Validate device lifecycle

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
meson build
ninja -C build

# Hot-plug new device
sudo ./build/doca_devemu_pci_device_hotplug

# Hot-unplug existing device (use VUID from previous run)
sudo ./build/doca_devemu_pci_device_hotplug -v <VUID>
```

---

### 3. Stateful Region ✅
**File:** `devemu_pci_device_stateful_region/`

**What it does:**
- **DPU side:** Monitors BAR writes from host
- **Host side:** Writes values to device BAR
- BlueField logs values written by host

**When to use:**
- Test control registers
- Validate host-device messaging
- Debug BAR access
- Test memory-mapped I/O

**Architecture:**
```
┌─────────────────┐         ┌─────────────────┐
│   Host Driver   │         │  BlueField DPU  │
│                 │         │                 │
│ Write to BAR ───┼────────>│ Read event      │
│  (0x12345678)   │  PCIe   │ Log: 0x12345678 │
└─────────────────┘         └─────────────────┘
```

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_stateful_region

# Build DPU side
cd dpu
meson build
ninja -C build

# Build host side
cd ../host
meson build
ninja -C build

# Terminal 1: Run DPU side (monitors BAR)
sudo ./dpu/build/doca_devemu_pci_device_stateful_region_dpu -v <VUID>

# Terminal 2: Run host side (writes to BAR)
sudo ./host/build/doca_devemu_pci_device_stateful_region_host \
    -p <PCI_ADDR> -g <VFIO_GROUP> -d 0x12345678
```

---

### 4. Doorbell (DB) ✅ ⚠️ Needs DPA
**File:** `devemu_pci_device_db/`

**What it does:**
- **Host:** Rings doorbell by writing to BAR
- **BlueField ARM:** Manages DPA thread
- **BlueField DPA:** Handles doorbell events

**When to use:**
- Test fast notification path
- Validate queue signaling
- Debug DPA event handling
- Test work submission

**Architecture:**
```
┌──────────┐      ┌──────────────┐      ┌─────────────┐
│   Host   │      │  BF ARM      │      │   BF DPA    │
│          │      │              │      │             │
│ Ring DB ─┼─────>│ Forward to  ─┼─────>│ Handle DB   │
│  (value) │ PCIe │  DPA thread  │ RPC  │ Get value   │
└──────────┘      └──────────────┘      └─────────────┘
```

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_db

# Build DPU side (ARM + DPA)
cd dpu
meson build
ninja -C build

# Build host side
cd ../host
meson build
ninja -C build

# Terminal 1: Run DPU side (creates DPA handler)
sudo ./dpu/build/doca_devemu_pci_device_db_dpu -v <VUID>

# Terminal 2: Run host side (rings doorbell)
sudo ./host/build/doca_devemu_pci_device_db_host \
    -p <PCI_ADDR> -g <VFIO_GROUP> -d 0xABCD -o 0
```

---

### 5. MSI-X Interrupts ✅ ⚠️ Needs DPA
**File:** `devemu_pci_device_msix/`

**What it does:**
- **BlueField DPA:** Raises MSI-X vector
- **BlueField ARM:** Manages DPA and MSI-X
- **Host:** Receives interrupt via eventfd

**When to use:**
- Test interrupt delivery
- Validate interrupt handlers
- Debug multi-queue systems
- Test completion notifications

**Architecture:**
```
┌─────────────┐      ┌──────────────┐      ┌──────────┐
│   BF DPA    │      │   BF ARM     │      │   Host   │
│             │      │              │      │          │
│ Raise MSI-X ┼─────>│ Forward to  ─┼─────>│ Read FD  │
│  (vector 0) │ RPC  │  PCI         │ PCIe │ (event!) │
└─────────────┘      └──────────────┘      └──────────┘
```

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_msix

# Build DPU side
cd dpu
meson build
ninja -C build

# Build host side
cd ../host
meson build
ninja -C build

# Terminal 1: Run host side (wait for interrupt)
sudo ./host/build/doca_devemu_pci_device_msix_host \
    -p <PCI_ADDR> -g <VFIO_GROUP>

# Terminal 2: Run DPU side (raise interrupt)
sudo ./dpu/build/doca_devemu_pci_device_msix_dpu -v <VUID>
```

---

### 6. DMA Operations ✅
**File:** `devemu_pci_device_dma/`

**What it does:**
- Host allocates DMA-able memory
- BlueField reads from host memory (Host→BF)
- BlueField writes to host memory (BF→Host)

**When to use:**
- Test DMA setup
- Validate IOMMU mapping
- Debug packet DMA
- Test bulk transfers

**Architecture:**
```
┌─────────────────┐         ┌─────────────────┐
│      Host       │         │  BlueField DPU  │
│                 │         │                 │
│ Alloc DMA buf   │         │                 │
│ Map to IOMMU    │         │                 │
│ Write "Hello"   │         │                 │
│                 │<────────┤ DMA Read        │
│                 │  PCIe   │ (Host→BF)       │
│                 │         │ Process data    │
│                 │         │ DMA Write       │
│                 │────────>│ (BF→Host)       │
│ Read result     │  PCIe   │                 │
└─────────────────┘         └─────────────────┘
```

**Build & Run:**
```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_dma

# Build DPU side
cd dpu
meson build
ninja -C build

# Build host side
cd ../host
meson build
ninja -C build

# Terminal 1: Run DPU side (performs DMA)
sudo ./dpu/build/doca_devemu_pci_device_dma_dpu -v <VUID>

# Terminal 2: Run host side (provides DMA buffer)
sudo ./host/build/doca_devemu_pci_device_dma_host \
    -p <PCI_ADDR> -g <VFIO_GROUP> -s "Hello from host!"
```

---

## What DevEmu Can Do for Three-Port Switch

### Use Case: Virtual PCI Port

Your three-port switch needs:
- 1 PCI port (Port 0)
- 2 Ethernet ports (Port 1, 2)

**With DevEmu:**
```bash
# Create virtual PCI device for Port 0
# Use real Ethernet for Port 1, 2
# Test switch logic without dedicated PCI hardware!
```

### Integration Architecture

```
┌────────────────────────────────────────┐
│         Host Machine (x86)             │
│                                        │
│  ┌──────────────────────────────────┐  │
│  │   DevEmu PCI Device (Port 0)    │  │
│  │   - Virtual BAR regions         │  │
│  │   - Doorbell support            │  │
│  │   - DMA capable                 │  │
│  └──────────┬───────────────────────┘  │
└─────────────┼──────────────────────────┘
              │ Emulated PCIe
              ▼
┌─────────────────────────────────────────┐
│      BlueField DPU (Emulated)           │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │    Three-Port Switch            │    │
│  │                                 │    │
│  │  Port 0 ◄─── DevEmu PCI        │    │
│  │  Port 1 ◄─── Ethernet (real)   │    │
│  │  Port 2 ◄─── Ethernet (real)   │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

### Benefits

**1. Development Without Hardware** ✅
- No dedicated PCI device needed
- Test on laptop/workstation
- Debug host-device interaction

**2. Rapid Iteration** ✅
- Fast compile-test cycle
- No hardware setup
- Easy debugging with GDB

**3. Realistic Testing** ✅
- Actual PCI communication
- Real DMA operations
- True interrupt handling

**4. Host Driver Development** ✅
- Write VFIO driver for Port 0
- Test driver without hardware
- Debug driver issues easily

---

## DevEmu Limitations

### What DevEmu CANNOT Do ❌

| Feature | DevEmu | Real Hardware |
|---------|--------|---------------|
| **Packet forwarding** | ❌ Emulated | ✅ Real |
| **Hardware offload** | ❌ Software | ✅ Hardware |
| **Line-rate performance** | ❌ Slow | ✅ Fast |
| **Real network traffic** | ❌ Simulated | ✅ Real |
| **DPDK on PCI port** | ❌ Limited | ✅ Full |
| **Production deployment** | ❌ Dev only | ✅ Yes |

### What DevEmu IS FOR ✅

- ✅ **Development** - Write code without hardware
- ✅ **Testing** - Validate host-device interaction
- ✅ **Debugging** - Find issues before hardware testing
- ✅ **CI/CD** - Automated testing in pipelines
- ✅ **Learning** - Understand BlueField architecture

### What DevEmu is NOT FOR ❌

- ❌ **Performance testing** - Use real hardware
- ❌ **Production** - Deploy to real DPU
- ❌ **Network benchmarks** - Need real NICs
- ❌ **Line-rate forwarding** - Hardware acceleration required

---

## Quick Start: Build Your First DevEmu Sample

### Step 1: List Devices (Simplest)

```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list
meson build
ninja -C build
sudo ./build/doca_devemu_pci_device_list
```

**Expected output:**
```
Emulated device #0:
  VUID: 00000000-0000-0000-0000-000000000001
  PCI Address: 0000:00:05.0
```

### Step 2: Test Hot-Plug

```bash
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
meson build
ninja -C build

# Create and hot-plug device
sudo ./build/doca_devemu_pci_device_hotplug

# On host, verify:
lspci | grep -i mellanox
```

### Step 3: Test Stateful Region (Host-Device Communication)

```bash
# Terminal 1: DPU side (monitor BAR writes)
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_stateful_region/dpu
meson build
ninja -C build
VUID=$(sudo ../../../devemu_pci_device_list/build/doca_devemu_pci_device_list | grep VUID | awk '{print $2}')
sudo ./build/doca_devemu_pci_device_stateful_region_dpu -v $VUID

# Terminal 2: Host side (write to BAR)
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_stateful_region/host
meson build
ninja -C build
PCI_ADDR=$(lspci | grep -i mellanox | awk '{print $1}')
VFIO_GROUP=$(ls /dev/vfio/ | grep -v vfio)
sudo ./build/doca_devemu_pci_device_stateful_region_host \
    -p $PCI_ADDR -g $VFIO_GROUP -d 0x12345678
```

---

## Integration with Three-Port Switch

### Scenario: Use DevEmu for Port 0

```bash
# 1. Create DevEmu PCI device for Port 0
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
sudo ./build/doca_devemu_pci_device_hotplug

# 2. Get PCI address
PCI_ADDR=$(lspci | grep -i mellanox | head -1 | awk '{print $1}')

# 3. Use in three-port switch
cd /home/heng/workdir/doca/three_port_switch
sudo ./build/doca_three_port_switch \
    -a $PCI_ADDR \          # Port 0: DevEmu PCI
    -a enp0s1 \             # Port 1: Real Ethernet
    -a enp0s2 --            # Port 2: Real Ethernet
```

### Benefits for Your Switch

**1. Test without PCI hardware** ✅
- Port 0 = DevEmu (virtual PCI)
- Port 1 = Ethernet (real/DPDK null)
- Port 2 = Ethernet (real/DPDK null)

**2. Host communication testing** ✅
- Send packets from host via Port 0
- Switch forwards to Port 1/2
- Verify MAC learning, forwarding

**3. DMA testing** ✅
- Test packet reception via DMA
- Validate descriptor rings
- Debug host driver

---

## Summary: DevEmu Capabilities

### ✅ What You CAN Do

1. **Emulate PCI devices** - Virtual BlueField hardware
2. **Test host-device communication** - BAR access, doorbells
3. **Validate DMA operations** - Memory transfers
4. **Test interrupts** - MSI-X delivery
5. **Hot-plug/unplug** - Dynamic device management
6. **Run DPA code** - With emulated hardware
7. **Debug without hardware** - GDB, logging
8. **Automated testing** - CI/CD integration
9. **Driver development** - VFIO driver testing
10. **Three-port switch Port 0** - Virtual PCI port

### ❌ What You CANNOT Do

1. ❌ Real packet forwarding at line rate
2. ❌ Hardware acceleration testing
3. ❌ Production deployment
4. ❌ Network performance benchmarks
5. ❌ Replace real BlueField hardware

---

## Next Steps

### For Three-Port Switch Integration

```bash
# 1. Build DevEmu samples
cd /home/heng/workdir/doca
./build_devemu_samples.sh

# 2. Create integration script
cat > integrate_devemu.sh << 'SCRIPT'
#!/bin/bash
# Start DevEmu + Three-Port Switch

# Start DevEmu PCI device
cd /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_hotplug
sudo ./build/doca_devemu_pci_device_hotplug &

sleep 2

# Get PCI address
PCI_ADDR=$(lspci | grep -i mellanox | head -1 | awk '{print $1}')

# Start switch
cd /home/heng/workdir/doca/three_port_switch
sudo ./build/doca_three_port_switch \
    -a $PCI_ADDR \
    -a net_null0 \
    -a net_null1 --
SCRIPT

chmod +x integrate_devemu.sh
```

---

**DevEmu is your development environment for BlueField without needing physical hardware!** ✅

Use it for testing, debugging, and rapid iteration, then deploy to real hardware for production!
