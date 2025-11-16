# DOCA DevEmu Samples - Built Successfully

## What is DevEmu?

DOCA DevEmu (Device Emulation) is a framework for creating and managing emulated PCIe devices on NVIDIA BlueField DPUs. It allows the DPU to present virtual PCIe devices to the host system, enabling custom hardware emulation and advanced host-DPU communication.

## Built Sample

**DevEmu PCI Device List** - Successfully built at:
- Location: `/home/heng/workdir/doca/devemu_sample/doca_devemu/devemu_pci_device_list/build/doca_devemu_pci_device_list`
- Purpose: Lists all emulated PCIe devices of a configured generic type
- Usage: `./build/doca_devemu_pci_device_list -p <PCI_ADDRESS>`

## DevEmu Capabilities

### 1. **PCI Device List**
- Enumerate all emulated devices with generic type
- Query device VUID (Virtual Unique ID)
- Get PCIe address as seen by host

### 2. **PCI Device Hot-Plug/Unplug**
- Create new emulated devices dynamically
- Hot-plug devices to host
- Hot-unplug devices from host
- Event-driven state change notifications

### 3. **PCI Device Stateful Regions**
- Host can write to stateful memory regions
- DPU Arm cores handle write operations
- Real-time monitoring of host-to-device communication

### 4. **PCI Device DMA**
- Emulated device can perform DMA operations
- Access host memory from DPU
- Bi-directional data transfer

### 5. **PCI Device MSIX**
- Configure MSI-X interrupt vectors
- Send interrupts from emulated device to host
- Event-driven interrupt handling

### 6. **PCI Device Doorbell**
- Host writes to doorbell registers
- DPU receives notifications
- Low-latency signaling mechanism

### 7. **VFS Device Management**
- Create Virtual Function Sets (VFS)
- Manage multiple emulated devices
- Dynamic device hotplug/unplug

## Use Cases

1. **Custom Hardware Acceleration**
   - Emulate specialized accelerators visible to host
   - Present custom PCIe devices with specific capabilities

2. **Host-DPU Communication**
   - Low-latency signaling via doorbells
   - Shared memory regions for data exchange
   - Interrupt-driven notifications

3. **Multi-Tenant Scenarios**
   - Create isolated virtual devices per tenant
   - Dynamic provisioning and deprovisioning
   - Resource isolation

4. **Testing and Development**
   - Test PCIe device drivers without physical hardware
   - Simulate various device behaviors
   - Debug host-device interactions

## Sample Architecture

```
Host System                          BlueField DPU
-----------                          -------------
   |                                      |
   | Sees emulated PCIe device           |
   | (appears as real hardware)          |
   |                                      |
   +----> Writes to registers ---------> DevEmu handles writes
   +----> Initiates DMA -------------->  DevEmu processes DMA
   <----- Receives MSI-X interrupts <--- DevEmu sends interrupts
   <----- DMA transfers <--------------- DevEmu initiates DMA
```

## Running DevEmu Samples

### Requirements
- NVIDIA BlueField DPU (physical hardware or emulator)
- DOCA SDK installed
- Root/sudo privileges for device access

### Basic Usage
```bash
# List emulated devices
sudo ./doca_devemu_pci_device_list -p <DPU_PCI_ADDRESS>

# Hot-plug a new device
sudo ./doca_devemu_pci_device_hotplug -p <DPU_PCI_ADDRESS>

# Hot-unplug an existing device
sudo ./doca_devemu_pci_device_hotplug -p <DPU_PCI_ADDRESS> -u <VUID>
```

## Available Sample Applications

All samples are located in `/opt/mellanox/doca/samples/doca_devemu/`:

1. **devemu_pci_device_list** - ✅ Built
2. **devemu_pci_device_hotplug** - Available to build
3. **devemu_pci_device_stateful_region** - DPU + Host pair
4. **devemu_pci_device_dma** - DPU + Host pair
5. **devemu_pci_device_msix** - DPU + Host pair
6. **devemu_pci_device_db** - DPU + Host pair
7. **devemu_vfs_list_devices** - Available to build
8. **devemu_vfs_device_hotplug_unplug** - Available to build

## DevEmu vs DevEmu Emulator

- **DevEmu**: Runs on actual BlueField DPU hardware, creates real emulated PCIe devices
- **DevEmu Emulator**: Software simulator for testing DPA code without physical hardware

## Notes

- Most DevEmu samples require actual BlueField hardware
- Some samples have DPU and host components that must run together
- The emulator mentioned in DEVEMU_QUICK_START.md is for DPA testing, not full DevEmu simulation
- DevEmu primarily focuses on PCIe device emulation and host-DPU interaction

## Build Instructions

To build other DevEmu samples in your workspace:

```bash
cd /home/heng/workdir/doca/devemu_sample/doca_devemu/<sample_name>
meson setup build
ninja -C build
```

Make sure to copy common.c and common.h from `/opt/mellanox/doca/samples/` to the parent directory.
