# DPA Testing and Simulation Tools - Complete Guide

## Question: Is there a simulation tool for testing DPA code?

## Answer: YES! Multiple Tools Available ✅

NVIDIA provides several tools for testing, debugging, and simulating DPA code without requiring physical BlueField hardware.

---

## Available Tools

### 1. **DPA GDB Server** ✅
**Location**: `/opt/mellanox/doca/tools/dpa-gdbserver`  
**Size**: 1.1 MB  
**Purpose**: Debug DPA kernels using GDB

**What It Does:**
- Connects GDB to running DPA code
- Set breakpoints in DPA kernels
- Step through device code
- Inspect DPA variables and memory
- Debug on actual hardware or emulation

**Usage:**
```bash
# Start GDB server for DPA debugging
sudo dpa-gdbserver <devname> -s <port> -T <token>

# Example
sudo dpa-gdbserver mlx5_0 -s 1981 -T mytoken

# Then connect with GDB
dpa-gdb
(gdb) target remote localhost:1981
(gdb) break hello_world
(gdb) continue
```

**Features:**
- Remote debugging over network
- Full GDB capabilities for DPA code
- Examine DPA execution on BlueField
- Debug host-device interaction

---

### 2. **DPA Emulation Manager (dpaeumgmt)** ✅
**Location**: `/opt/mellanox/doca/tools/dpaeumgmt`  
**Size**: 168 KB  
**Purpose**: Manage DPA emulation environment

**What It Does:**
- Manages DPA emulation units (EUs)
- Controls DPA execution environment
- Configures emulation parameters
- Monitors DPA resource usage

**Usage:**
```bash
# Must run with root privileges
sudo dpaeumgmt [options]
```

---

### 3. **Device Emulation (DevEmu) Framework** ✅
**Location**: `/opt/mellanox/doca/samples/doca_devemu/`  
**Purpose**: Emulate PCI devices and test device-host interaction

**What It Provides:**

#### a) PCI Device Emulation
Create virtual PCI devices without hardware:
- Emulate BlueField devices on host
- Test device drivers
- Simulate hot-plug/unplug
- Test DMA operations

#### b) Stateful Region Testing
Test memory-mapped I/O:
- Simulate BAR regions
- Test host-device communication
- Validate state transitions
- Debug device interactions

#### c) Available Samples
```
/opt/mellanox/doca/samples/doca_devemu/
├── devemu_pci_device_list          # List emulated devices
├── devemu_pci_device_hotplug       # Hot-plug simulation
├── devemu_pci_device_stateful_region # Memory region testing
├── devemu_pci_device_dma           # DMA testing
├── devemu_pci_device_db            # Doorbell testing
├── devemu_pci_device_msix          # MSI-X interrupt testing
└── devemu_vfs_*                    # VF emulation
```

---

### 4. **DPA Debug Tools** ✅

#### dpa-ps
Monitor DPA processes and threads
```bash
/opt/mellanox/doca/tools/dpa-ps
```

#### dpa-objdump
Examine DPA binary objects
```bash
/opt/mellanox/doca/tools/dpa-objdump [options] file.a
```

#### dpa-nm
List symbols in DPA binaries
```bash
/opt/mellanox/doca/tools/dpa-nm dpa_kernel_launch_program.a
```

---

## Testing Approaches

### Approach 1: Device Emulation (No Hardware Required)

**Best for:**
- Initial development
- Host-device protocol testing
- Driver development
- CI/CD pipelines

**Example Workflow:**
```bash
# 1. Build DevEmu sample
cd /home/heng/workdir/doca
cp -r /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list .
cd devemu_pci_device_list

# 2. Adapt build files (like we did for DPA samples)
# 3. Build and run
./build.sh
sudo ./build/doca_devemu_pci_device_list

# 4. Test your code against emulated device
```

---

### Approach 2: DPA Kernel Debugging with GDB

**Best for:**
- Debugging DPA kernel logic
- Finding bugs in device code
- Performance analysis
- Understanding execution flow

**Setup:**
```bash
# 1. Build DPA code with debug symbols
cd /home/heng/workdir/doca/dpa_kernel_launch
meson setup build -Dbuildtype=debug

# 2. Run application with GDB server
sudo dpa-gdbserver mlx5_0 -s 1981 -T debug_session &

# 3. Run your DPA application
sudo ./build/doca_dpa_kernel_launch &

# 4. Connect with GDB
dpa-gdb build/dpa_kernel_launch/device/build_dpacc/dpa_kernel_launch_program.a
(gdb) target remote localhost:1981
(gdb) break hello_world
(gdb) continue
(gdb) step
(gdb) print variable_name
```

---

### Approach 3: Software Simulation (Host-Only Testing)

**Best for:**
- Algorithm validation
- Unit testing
- Logic verification
- Quick iteration

**Method:**
Create host-side versions of DPA kernels for testing:

```c
// Original DPA kernel
__dpa_global__ void packet_processor(packet_t *pkt)
{
    // Process packet
}

// Host simulation version for testing
void packet_processor_sim(packet_t *pkt)
{
    // Same logic, runs on host for testing
    // Validate algorithm before deploying to DPA
}
```

**Benefits:**
- Fast iteration
- No hardware needed
- Use standard debugging tools
- Easy unit testing

---

### Approach 4: Hybrid Testing

**Best for:**
- Complete validation
- Production readiness
- Performance testing

**Workflow:**
1. **Unit test** on host (simulation)
2. **Integration test** with DevEmu (emulation)
3. **System test** with DPA GDB (on hardware/emulation)
4. **Performance test** on actual BlueField

---

## Building a DevEmu Sample

Let me show you how to build and use device emulation:

### Step 1: Copy DevEmu Sample
```bash
cd /home/heng/workdir/doca
cp -r /opt/mellanox/doca/samples/doca_devemu/devemu_pci_device_list .
cd devemu_pci_device_list
```

### Step 2: Adapt Build Files
Similar to what we did for DPA samples:
- Update `meson.build` (remove parent directory references)
- Copy common files
- Create `build.sh`

### Step 3: Build and Run
```bash
./build.sh
sudo ./build/doca_devemu_pci_device_list
```

This will show emulated PCI devices without needing hardware!

---

## Comparison: Testing Methods

| Method | Hardware Needed | Speed | Accuracy | Best For |
|--------|----------------|-------|----------|----------|
| **DevEmu** | ✗ No | Fast | High | Protocol testing |
| **DPA GDB** | ✓ Yes or Emu | Medium | Very High | Debugging |
| **Host Sim** | ✗ No | Very Fast | Medium | Algorithm testing |
| **Real HW** | ✓ Yes | Slow | Perfect | Final validation |

---

## Complete Testing Pipeline

### Recommended Development Flow

```
┌─────────────────────────────────────┐
│ 1. Write DPA Kernel (C code)       │
└───────────────┬─────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ 2. Host Simulation Testing          │
│    - Unit tests                     │
│    - Algorithm validation           │
│    - Quick iteration                │
└───────────────┬─────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ 3. Device Emulation (DevEmu)        │
│    - Host-device protocol           │
│    - Memory mapping                 │
│    - State transitions              │
└───────────────┬─────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ 4. DPA GDB Debugging                │
│    - Breakpoints                    │
│    - Step through code              │
│    - Inspect variables              │
└───────────────┬─────────────────────┘
                ↓
┌─────────────────────────────────────┐
│ 5. BlueField Hardware Testing       │
│    - Real traffic                   │
│    - Performance measurement        │
│    - Final validation               │
└─────────────────────────────────────┘
```

---

## DPA Testing Example

### Example 1: Basic DPA Kernel Test

**File: `test_dpa_kernel.c`**
```c
#include <stdio.h>
#include <assert.h>

// DPA kernel (runs on device)
#ifdef DPA_BUILD
__dpa_global__ void process_data(int *data, int size)
{
    for (int i = 0; i < size; i++) {
        data[i] = data[i] * 2;
    }
}
#else
// Host simulation for testing
void process_data(int *data, int size)
{
    for (int i = 0; i < size; i++) {
        data[i] = data[i] * 2;
    }
}

// Unit test (runs on host)
void test_process_data()
{
    int data[] = {1, 2, 3, 4, 5};
    int expected[] = {2, 4, 6, 8, 10};
    
    process_data(data, 5);
    
    for (int i = 0; i < 5; i++) {
        assert(data[i] == expected[i]);
    }
    
    printf("✓ Test passed!\n");
}

int main()
{
    test_process_data();
    return 0;
}
#endif
```

**Build and Test:**
```bash
# Test on host
gcc -o test test_dpa_kernel.c
./test

# Build for DPA
dpacc -DDPA_BUILD -c test_dpa_kernel.c -o test.o
```

---

### Example 2: DevEmu Testing

**Test host-device communication without hardware:**

```c
// Host side
#include <doca_devemu.h>

int main()
{
    // Create emulated device
    doca_devemu_pci_device *emu_dev;
    doca_devemu_pci_device_create(&emu_dev);
    
    // Write to device memory
    uint32_t value = 0x12345678;
    doca_devemu_write_bar(emu_dev, 0, 0, &value, sizeof(value));
    
    // Read back
    uint32_t read_value;
    doca_devemu_read_bar(emu_dev, 0, 0, &read_value, sizeof(read_value));
    
    assert(read_value == value);
    printf("✓ DevEmu test passed!\n");
    
    return 0;
}
```

---

## Available Tools Summary

### Installed and Ready ✅

| Tool | Location | Purpose | Hardware Needed |
|------|----------|---------|----------------|
| **dpa-gdbserver** | `/opt/mellanox/doca/tools/` | Debug DPA kernels | Yes or Emu |
| **dpaeumgmt** | `/opt/mellanox/doca/tools/` | Manage emulation | No |
| **DevEmu samples** | `/opt/mellanox/doca/samples/doca_devemu/` | Device emulation | No |
| **dpa-ps** | `/opt/mellanox/doca/tools/` | Monitor DPA | Yes or Emu |
| **dpa-objdump** | `/opt/mellanox/doca/tools/` | Examine binaries | No |
| **dpa-nm** | `/opt/mellanox/doca/tools/` | List symbols | No |

---

## Building DevEmu Sample Now

Want me to build a device emulation sample for you? We can:

1. **Build DevEmu PCI Device List**
   - Lists emulated devices
   - No hardware required
   - See how emulation works

2. **Build DevEmu Stateful Region**
   - Test memory-mapped I/O
   - Host-device communication
   - Practical testing example

3. **Build DevEmu Hot-Plug**
   - Device lifecycle testing
   - Hot-plug/unplug simulation
   - Driver development aid

---

## Debugging DPA Code

### With GDB (On Hardware or Emulation)

```gdb
# Start GDB session
dpa-gdb build/dpa_kernel_launch_program.a

# Connect to DPA
(gdb) target remote localhost:1981

# Set breakpoint in DPA kernel
(gdb) break hello_world

# Run until breakpoint
(gdb) continue

# Examine DPA memory
(gdb) x/10x $dpa_mem_addr

# Step through DPA code
(gdb) step

# Print DPA variable
(gdb) print my_variable

# Backtrace on DPA
(gdb) backtrace

# Inspect DPA registers
(gdb) info registers
```

---

## Practical Testing Strategy

### For Your DPA Kernel Launch Sample

**Phase 1: Host Simulation** (Done ✅)
- Compiled successfully
- Basic validation complete

**Phase 2: Static Analysis**
```bash
# Examine compiled DPA binary
dpa-objdump -d build/dpa_kernel_launch/device/build_dpacc/dpa_kernel_launch_program.a

# Check symbols
dpa-nm build/dpa_kernel_launch/device/build_dpacc/dpa_kernel_launch_program.a
```

**Phase 3: DevEmu Testing** (Can do now!)
```bash
# Build DevEmu sample
# Test device creation
# Validate host-device protocol
```

**Phase 4: Hardware/Emu Debugging** (When available)
```bash
# Use dpa-gdbserver
# Debug with GDB
# Validate on BlueField
```

---

## Next Steps

### Option 1: Build DevEmu Sample (Recommended)
```bash
cd /home/heng/workdir/doca
# I can build a device emulation sample for you
```

### Option 2: Explore DPA Debug Tools
```bash
# Examine your DPA binary
dpa-objdump -d dpa_kernel_launch/build/dpa_kernel_launch/device/build_dpacc/dpa_kernel_launch_program.a

# List symbols
dpa-nm dpa_kernel_launch/build/dpa_kernel_launch/device/build_dpacc/dpa_kernel_launch_program.a
```

### Option 3: Create Host Simulation Tests
```bash
# Write unit tests for DPA logic
# Test algorithms on host before DPA deployment
```

---

## Conclusion

### Testing Tools Available: YES! ✅

You have access to:
1. ✅ **DPA GDB Server** - Full debugging capabilities
2. ✅ **DevEmu Framework** - Device emulation without hardware
3. ✅ **DPA Debug Tools** - Binary inspection and monitoring
4. ✅ **Emulation Manager** - Control emulation environment

### Testing Strategy

**Without Hardware:**
- ✅ Host simulation (unit tests)
- ✅ DevEmu (integration tests)
- ✅ Static analysis (binary inspection)

**With Hardware (or later):**
- ✅ DPA GDB (interactive debugging)
- ✅ Real-world testing
- ✅ Performance validation

### Recommendation

1. **Start with DevEmu** - Build device emulation sample
2. **Use DPA tools** - Inspect compiled binaries
3. **Prepare for hardware** - When you get BlueField, use GDB server

---

**You have a complete testing and simulation toolkit!** ✅

Would you like me to build a DevEmu sample so you can see device emulation in action?
