# DPA GDB Debugging Guide for Three-Port Switch

## Overview
This guide shows how to debug DPA programs (like your three-port switch) using GDB in the DevEmu simulation environment without physical hardware.

---

## Prerequisites ✅

All tools are already installed:
- ✅ `dpa-gdbserver` - DPA GDB debugging server
- ✅ `dpa-gdb` - GDB for DPA programs
- ✅ DevEmu framework for device emulation
- ✅ DOCA SDK with debugging symbols

---

## Quick Start: Debug Your Three-Port Switch

### Step 1: Build with Debug Symbols

```bash
cd /home/heng/workdir/doca/three_port_switch

# Clean previous build
rm -rf build

# Build with debug symbols
meson setup build -Dbuildtype=debug
ninja -C build
```

**What `-Dbuildtype=debug` does:**
- Adds `-g` flag for debug symbols
- Disables optimizations (`-O0`)
- Enables assertions
- Makes source stepping accurate

---

### Step 2: Start DevEmu Environment

The DevEmu emulator provides a simulated environment for DPA debugging.

**Option A: Use existing mock simulator**
```bash
cd /home/heng/workdir/doca/multi_switch_topology
./mock_simulator.sh 1 standalone
```

**Option B: Create standalone debug environment**
```bash
# Create debug directory
mkdir -p /home/heng/workdir/doca/dpa_debug
cd /home/heng/workdir/doca/dpa_debug
```

---

### Step 3: Launch DPA GDB Server

The GDB server connects your debugger to the DPA execution environment.

**Terminal 1: Start GDB Server**
```bash
# Check if dpa-gdbserver is available
which dpa-gdbserver

# Start GDB server (will listen for connections)
# Note: May need emulated device or mock device
sudo dpa-gdbserver -s 1981 -T debug_session

# Alternative: If specific device needed
sudo dpa-gdbserver mlx5_0 -s 1981 -T debug_session
```

**Parameters:**
- `-s 1981` - Listen on port 1981
- `-T debug_session` - Debug session token/name

**Expected Output:**
```
DPA GDB Server started
Listening on port 1981
Waiting for GDB connection...
```

---

### Step 4: Launch Your Three-Port Switch

**Terminal 2: Run your application**
```bash
cd /home/heng/workdir/doca/three_port_switch

# Run with environment variables for debugging
sudo ./build/doca_three_port_switch \
  -a 0000:03:00.0,dv_flow_en=2 \
  -l dpdk-port-hairpin.json

# Or run in background
sudo ./build/doca_three_port_switch \
  -a 0000:03:00.0,dv_flow_en=2 \
  -l dpdk-port-hairpin.json &
```

**Note:** The application will pause at DPA kernel entry points if breakpoints are set.

---

### Step 5: Connect GDB and Debug

**Terminal 3: Start GDB session**
```bash
cd /home/heng/workdir/doca/three_port_switch

# Start DPA GDB with your program binary
dpa-gdb build/doca_three_port_switch

# Inside GDB:
```

**GDB Commands:**
```gdb
# Connect to GDB server
(gdb) target remote localhost:1981

# Load symbol file (if needed)
(gdb) file build/doca_three_port_switch

# Set breakpoints in DPA kernel code
# Look at three_port_switch.c for function names
(gdb) break main
(gdb) break process_packet
(gdb) break forward_packet

# List source code
(gdb) list

# Run the program
(gdb) continue

# When breakpoint hits:
# Step through code
(gdb) step

# Next line (skip function calls)
(gdb) next

# Print variable
(gdb) print packet_count
(gdb) print port_stats

# Examine memory
(gdb) x/16x $sp
(gdb) x/s string_variable

# View registers
(gdb) info registers

# Backtrace
(gdb) backtrace

# Continue execution
(gdb) continue

# Set watchpoint (break when variable changes)
(gdb) watch packet_count

# Conditional breakpoint
(gdb) break forward_packet if port_id == 2

# Quit
(gdb) quit
```

---

## Debugging Scenarios

### Scenario 1: Debug Packet Processing

```gdb
# Start GDB
dpa-gdb build/doca_three_port_switch
(gdb) target remote localhost:1981

# Set breakpoint in packet handler
(gdb) break process_packet
(gdb) continue

# When breakpoint hits, examine packet
(gdb) print pkt->size
(gdb) print pkt->port_id
(gdb) x/64x pkt->data

# Step through processing
(gdb) step
(gdb) step

# Check forwarding decision
(gdb) print forward_port
```

### Scenario 2: Debug Port Statistics

```gdb
# Set breakpoint where stats are updated
(gdb) break update_port_stats
(gdb) continue

# When hit:
(gdb) print port_stats[0]
(gdb) print port_stats[1]
(gdb) print port_stats[2]

# Watch for changes
(gdb) watch port_stats[0].rx_packets
(gdb) continue
```

### Scenario 3: Debug Control Flow

```gdb
# Set multiple breakpoints
(gdb) break main
(gdb) break init_ports
(gdb) break setup_forwarding
(gdb) break cleanup

# Run and examine call stack
(gdb) continue
(gdb) backtrace
(gdb) frame 0
(gdb) info locals
```

---

## Advanced Debugging Techniques

### 1. Pretty Printing Structures

```gdb
# Define custom print function for packets
(gdb) define print_packet
  print "Packet size: %d", $arg0->size
  print "Port ID: %d", $arg0->port_id
  x/16x $arg0->data
end

# Use it
(gdb) print_packet pkt
```

### 2. Logging GDB Session

```bash
# Start GDB with logging
dpa-gdb -ex "set logging on" -ex "set logging file debug.log" build/doca_three_port_switch
```

### 3. Automated Debugging Script

Create `debug_script.gdb`:
```gdb
# Connect to server
target remote localhost:1981

# Set breakpoints
break main
break process_packet

# Define custom commands
define show_packet
  print "=== Packet Info ==="
  print "Size:", $arg0->size
  print "Port:", $arg0->port_id
  x/16x $arg0->data
end

# Run
continue
```

**Use it:**
```bash
dpa-gdb -x debug_script.gdb build/doca_three_port_switch
```

---

## Debugging Without Hardware: DevEmu Method

Since you don't have hardware yet, use DevEmu for DPA debugging simulation.

### Method 1: Mock DPA Execution

Create a host-side simulation that mimics DPA behavior:

**File: `three_port_switch_sim.c`**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simulate packet structure
typedef struct {
    uint8_t data[1500];
    uint16_t size;
    uint8_t port_id;
} packet_t;

// Simulate port statistics
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} port_stats_t;

port_stats_t port_stats[3] = {0};

// Simulate packet processing (same logic as DPA kernel)
void process_packet(packet_t *pkt)
{
    printf("Processing packet: size=%d, port=%d\n", pkt->size, pkt->port_id);
    
    // Update stats
    port_stats[pkt->port_id].rx_packets++;
    port_stats[pkt->port_id].rx_bytes += pkt->size;
    
    // Forwarding logic (simplified 3-port switch)
    uint8_t out_port = (pkt->port_id + 1) % 3;
    
    printf("Forwarding to port %d\n", out_port);
    port_stats[out_port].tx_packets++;
    port_stats[out_port].tx_bytes += pkt->size;
}

int main()
{
    // Create test packets
    packet_t pkt1 = { .size = 64, .port_id = 0 };
    packet_t pkt2 = { .size = 128, .port_id = 1 };
    packet_t pkt3 = { .size = 256, .port_id = 2 };
    
    // Process packets
    process_packet(&pkt1);
    process_packet(&pkt2);
    process_packet(&pkt3);
    
    // Print statistics
    for (int i = 0; i < 3; i++) {
        printf("\nPort %d stats:\n", i);
        printf("  RX: %lu packets, %lu bytes\n", 
               port_stats[i].rx_packets, port_stats[i].rx_bytes);
        printf("  TX: %lu packets, %lu bytes\n",
               port_stats[i].tx_packets, port_stats[i].tx_bytes);
    }
    
    return 0;
}
```

**Build and Debug:**
```bash
# Compile with debug symbols
gcc -g -O0 -o three_port_switch_sim three_port_switch_sim.c

# Debug with standard GDB
gdb ./three_port_switch_sim
(gdb) break process_packet
(gdb) run
(gdb) step
(gdb) print pkt->port_id
(gdb) continue
```

---

### Method 2: DPA Code with Host Fallback

Modify your DPA code to support both DPA and host compilation:

```c
#ifdef __DPA__
// DPA version (runs on device)
__dpa_global__ void process_packet(packet_t *pkt)
{
    // DPA-specific code
}
#else
// Host version (for debugging)
void process_packet(packet_t *pkt)
{
    // Same logic, but runs on host
    // Can use printf, assert, etc.
}
#endif
```

**Build for host debugging:**
```bash
gcc -g -O0 -o debug_host three_port_switch.c
gdb ./debug_host
```

---

## Troubleshooting

### Issue 1: GDB Server Not Starting

**Error:** `Cannot find DPA device`

**Solution:**
```bash
# Check available devices
ls /sys/class/infiniband/

# Use DevEmu to create virtual device
# Or run in simulation mode
```

### Issue 2: Cannot Set Breakpoints

**Error:** `No symbol table is loaded`

**Solution:**
```gdb
# Ensure debug symbols
(gdb) file build/doca_three_port_switch
(gdb) info sources

# Rebuild with debug
meson setup build -Dbuildtype=debug
```

### Issue 3: Source Not Found

**Error:** `Source file not found`

**Solution:**
```gdb
# Set source directory
(gdb) directory /home/heng/workdir/doca/three_port_switch
(gdb) list
```

---

## Complete Debugging Workflow

### Automated Setup Script

Create `start_debug.sh`:
```bash
#!/bin/bash

echo "Starting DPA Debug Environment..."

# Terminal 1: GDB Server (background)
echo "Starting GDB server..."
sudo dpa-gdbserver -s 1981 -T debug_session &
GDBSERVER_PID=$!
sleep 2

# Terminal 2: Application (background)
echo "Starting three-port switch..."
cd /home/heng/workdir/doca/three_port_switch
sudo ./build/doca_three_port_switch \
  -a 0000:03:00.0,dv_flow_en=2 \
  -l dpdk-port-hairpin.json &
APP_PID=$!
sleep 2

# Terminal 3: GDB
echo "Starting GDB..."
dpa-gdb -ex "target remote localhost:1981" \
        -ex "break main" \
        build/doca_three_port_switch

# Cleanup
echo "Cleaning up..."
kill $APP_PID 2>/dev/null
kill $GDBSERVER_PID 2>/dev/null
```

**Make executable and run:**
```bash
chmod +x start_debug.sh
./start_debug.sh
```

---

## Host-Side GDB Debugging (No Hardware Needed)

### Option 1: Debug Host Code with Standard GDB

Your three-port switch has host-side code that you can debug now:

```bash
cd /home/heng/workdir/doca/three_port_switch

# Build with debug symbols
meson setup build -Dbuildtype=debug
ninja -C build

# Debug with standard GDB
gdb build/doca_three_port_switch
(gdb) break main
(gdb) run -a 0000:03:00.0,dv_flow_en=2 -l dpdk-port-hairpin.json
(gdb) step
(gdb) print variable_name
```

This debugs the host-side logic (initialization, control flow, etc.).

---

### Option 2: Create Simulation Binary

Create a pure host simulation for testing logic:

**File: `three_port_switch_test.c`**
```c
// Test harness for three-port switch logic
#include <stdio.h>
#include <assert.h>

// Copy your switch logic here (host version)

int main()
{
    printf("Testing three-port switch logic...\n");
    
    // Test 1: Port forwarding
    assert(forward_port(0) == 1);
    assert(forward_port(1) == 2);
    assert(forward_port(2) == 0);
    printf("✓ Port forwarding test passed\n");
    
    // Test 2: Statistics
    // ...
    
    printf("All tests passed!\n");
    return 0;
}
```

**Debug:**
```bash
gcc -g -O0 -o test three_port_switch_test.c
gdb ./test
(gdb) break main
(gdb) run
```

---

## GDB Cheat Sheet for DPA Debugging

### Essential Commands

| Command | Purpose | Example |
|---------|---------|---------|
| `break` | Set breakpoint | `break process_packet` |
| `run` | Start program | `run` |
| `continue` | Continue execution | `continue` |
| `step` | Step into function | `step` |
| `next` | Step over function | `next` |
| `print` | Print variable | `print pkt->size` |
| `x` | Examine memory | `x/16x pkt->data` |
| `backtrace` | Show call stack | `backtrace` |
| `info locals` | Show local vars | `info locals` |
| `watch` | Watch variable | `watch counter` |
| `list` | Show source code | `list 20` |

### DPA-Specific Commands

```gdb
# Connect to DPA
target remote localhost:1981

# Load DPA symbols
file build/doca_three_port_switch

# Set DPA breakpoint
break __dpa_kernel_function

# Examine DPA memory region
x/64x $dpa_base_addr

# Print DPA register
info registers dpa
```

---

## Summary

### Current Status ✅

You can debug your three-port switch using:

1. **Standard GDB** (host-side code) - **Available Now** ✅
2. **Host simulation** (test logic) - **Available Now** ✅  
3. **DPA GDB** (device code) - **Needs hardware or full emulation**

### Recommended Approach

**Phase 1: Host Debugging (Now)**
```bash
# Debug host code
cd three_port_switch
gdb build/doca_three_port_switch
```

**Phase 2: Logic Testing (Now)**
```bash
# Create and debug simulation
gcc -g -O0 three_port_switch_sim.c
gdb ./a.out
```

**Phase 3: DPA Debugging (When hardware available)**
```bash
# Full DPA debugging with dpa-gdbserver
./start_debug.sh
```

---

## Next Steps

1. **Build host simulation** - Test switch logic on host
2. **Debug with standard GDB** - Debug host-side code
3. **Prepare DPA debug scripts** - Ready for hardware
4. **When you get hardware** - Use dpa-gdbserver for full DPA debugging

---

**You have everything ready for debugging!** ✅

Start with host-side debugging and simulation, then move to full DPA debugging when hardware arrives.
