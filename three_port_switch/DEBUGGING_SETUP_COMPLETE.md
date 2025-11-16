# GDB Debugging Setup Complete! ✅

## What's Been Created

Your three-port switch now has a complete GDB debugging environment:

### 1. Simulation for Testing (No Hardware Needed) ✅

**File: `three_port_switch_sim.c`**
- Simulates three-port switch logic on host
- Can be debugged with standard GDB
- Tests packet forwarding, statistics, error handling
- Interactive mode for manual testing

### 2. Debugging Tools ✅

**Files created:**
```
three_port_switch/
├── three_port_switch_sim.c     # Host simulation
├── Makefile.debug              # Build with debug symbols
├── gdb_commands.gdb            # GDB command file
├── demo_gdb.sh                 # Interactive demos
├── setup_debug.sh              # Setup for actual switch
├── DEBUG_QUICK_START.md        # Quick reference
└── DPA_GDB_DEBUG_GUIDE.md      # Complete guide (in parent dir)
```

---

## Quick Start (Do This Now!)

### Test 1: Run Simulation

```bash
cd /home/heng/workdir/doca/three_port_switch

# Run tests
./switch_sim test
```

**Expected output:** All tests pass ✓

### Test 2: Interactive Debugging Demo

```bash
# Run automated demo
./demo_gdb.sh

# Select option 2 (automated demo)
# This shows breakpoints, stepping, examining variables
```

### Test 3: Manual GDB Session

```bash
# Start GDB with custom commands
gdb -x gdb_commands.gdb ./switch_sim

# In GDB, try:
(gdb) run test              # Run the tests
(gdb) show_all_stats        # Show port statistics
(gdb) break process_packet  # Set breakpoint
(gdb) run test              # Run again
(gdb) print pkt->port_id    # Examine packet
(gdb) step                  # Step through code
(gdb) continue              # Continue execution
(gdb) quit                  # Exit GDB
```

---

## Learning Path

### Phase 1: Master Simulation (Now) ✅

Learn GDB with the simulation:

1. **Run tests:** `./switch_sim test`
2. **Try interactive mode:** `./switch_sim interactive`
3. **Debug with GDB:** `./demo_gdb.sh`
4. **Manual debugging:** `gdb -x gdb_commands.gdb ./switch_sim`

### Phase 2: Prepare for Real Switch

When you're ready to debug the actual DOCA application:

```bash
# This won't work yet (needs DPDK), but shows the process:
# ./setup_debug.sh
# ./build/start_debug.sh
```

### Phase 3: DPA Debugging (When Hardware Available)

```bash
# Terminal 1: Start GDB server
./build/start_gdbserver.sh

# Terminal 2: Debug with dpa-gdb
./build/start_debug.sh
```

---

## Key Features

### Custom GDB Commands ✅

The `gdb_commands.gdb` file provides:

**`show_packet <pkt>`** - Display packet details:
```gdb
(gdb) show_packet pkt
=== Packet Details ===
Port ID:   0
Size:      64 bytes
Timestamp: 0
Data[0-7]: 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07
```

**`show_all_stats`** - Display all port statistics:
```gdb
(gdb) show_all_stats
=== Port Statistics ===

Port 0 (pci_port):
  Enabled: Yes
  RX: 1 pkts, 64 bytes, 0 errors
  TX: 0 pkts, 0 bytes, 0 errors
...
```

### Demo Modes ✅

The `demo_gdb.sh` script provides 4 demo modes:

1. **Interactive GDB** - Full GDB session with pre-loaded commands
2. **Automated Demo** - Shows breakpoints, stepping, variable examination
3. **Watch Demo** - Demonstrates watchpoints for tracking changes
4. **Conditional Breakpoint** - Shows how to break on specific conditions

---

## Debugging Techniques

### Technique 1: Breakpoint Debugging

```gdb
# Set breakpoint
(gdb) break process_packet

# Run to breakpoint
(gdb) run test

# Examine state
(gdb) print pkt
(gdb) print port_stats

# Step through
(gdb) step
(gdb) next
```

### Technique 2: Watchpoint Debugging

```gdb
# Watch for changes to variable
(gdb) watch port_stats[0].rx_packets

# Run - stops when variable changes
(gdb) run test

# See what changed it
(gdb) backtrace
```

### Technique 3: Conditional Debugging

```gdb
# Only break for specific conditions
(gdb) break process_packet if pkt->port_id == 2

# Or add condition to existing breakpoint
(gdb) break process_packet
(gdb) condition 1 pkt->size > 1000
```

---

## What Each File Does

### `three_port_switch_sim.c`
Simulates your three-port switch logic on the host. This lets you test and debug the forwarding logic, statistics tracking, and error handling without hardware.

**Features:**
- Packet processing simulation
- Port statistics tracking
- Error handling (invalid port, MTU, disabled port)
- Automated tests
- Interactive mode

### `Makefile.debug`
Builds the simulation with debug symbols (`-g -O0`).

**Commands:**
- `make -f Makefile.debug` - Build simulation
- `make -f Makefile.debug test` - Run tests
- `make -f Makefile.debug debug` - Start GDB
- `make -f Makefile.debug clean` - Clean build

### `gdb_commands.gdb`
GDB command file with:
- Custom commands for examining packets and statistics
- Pre-set breakpoints
- Pretty printing enabled
- Logging enabled

### `demo_gdb.sh`
Interactive demonstration script showing different GDB debugging techniques.

### `setup_debug.sh`
Sets up debugging for the actual three-port switch DOCA application (needs DPDK to work).

---

## Testing Your Knowledge

Try these debugging challenges:

### Challenge 1: Find the Forwarding Path
```gdb
(gdb) break process_packet
(gdb) run test
# Question: Which port does a packet from port 0 go to?
(gdb) step   # Step until you reach get_forward_port
(gdb) print input_port
(gdb) finish # Run until function returns
(gdb) print $retval  # See return value
```

### Challenge 2: Watch Statistics Change
```gdb
(gdb) watch port_stats[1].tx_packets
(gdb) run test
# When it stops, figure out which packet caused it
(gdb) backtrace
(gdb) print pkt
```

### Challenge 3: Debug Error Handling
```gdb
# Set breakpoint that only triggers on errors
(gdb) break process_packet if pkt->port_id >= 3
(gdb) run test
# Examine the invalid packet
(gdb) print *pkt
```

---

## Comparison: Simulation vs Real Debugging

| Aspect | Simulation (Now) | Real Switch (Later) |
|--------|------------------|---------------------|
| **Hardware** | Not needed ✅ | Needs BlueField or DevEmu |
| **Debugger** | Standard GDB ✅ | dpa-gdb |
| **Build** | gcc ✅ | meson + dpacc |
| **Speed** | Fast ✅ | Slower |
| **Accuracy** | Logic only | Full hardware behavior |
| **Best For** | Learning, testing logic | Final validation |

---

## Next Steps

### Immediate (Do Now!)

1. **Run the simulation:**
   ```bash
   cd /home/heng/workdir/doca/three_port_switch
   ./switch_sim test
   ```

2. **Try the demo:**
   ```bash
   ./demo_gdb.sh
   # Select option 2
   ```

3. **Manual GDB session:**
   ```bash
   gdb -x gdb_commands.gdb ./switch_sim
   (gdb) run test
   (gdb) show_all_stats
   ```

### Short Term

4. **Read the guides:**
   - `DEBUG_QUICK_START.md` - Quick reference
   - `/home/heng/workdir/doca/DPA_GDB_DEBUG_GUIDE.md` - Complete guide

5. **Practice GDB commands:**
   - Breakpoints, watchpoints, conditional breaks
   - Stepping, examining variables
   - Custom commands

### When Hardware Arrives

6. **Set up DPA debugging:**
   ```bash
   ./setup_debug.sh
   ./build/start_gdbserver.sh
   ./build/start_debug.sh
   ```

---

## Troubleshooting

### Issue: "switch_sim not found"

**Solution:**
```bash
make -f Makefile.debug
```

### Issue: GDB shows optimized out variables

**Solution:** Already handled - built with `-O0`

### Issue: Can't find source files

**Solution:**
```gdb
(gdb) directory /home/heng/workdir/doca/three_port_switch
```

---

## Summary

✅ **Simulation built** - Test switch logic without hardware  
✅ **GDB configured** - Custom commands and breakpoints ready  
✅ **Demos available** - Learn GDB interactively  
✅ **Guides written** - Quick start and complete reference  
✅ **Future ready** - Setup scripts for DPA debugging when hardware arrives

### You Can Now:

1. ✅ Test three-port switch logic with simulation
2. ✅ Debug with GDB (breakpoints, watchpoints, stepping)
3. ✅ Examine packets and statistics in real-time
4. ✅ Practice debugging techniques
5. ✅ Prepare for DPA debugging on hardware

---

## Get Started

```bash
cd /home/heng/workdir/doca/three_port_switch

# Quick test
./switch_sim test

# Interactive debugging
./demo_gdb.sh
```

**Happy Debugging!** 🐛🔍

---

## Files Reference

All debugging files are in: `/home/heng/workdir/doca/three_port_switch/`

```
three_port_switch/
├── three_port_switch_sim.c     ← Simulation code
├── switch_sim                  ← Compiled simulation (after make)
├── Makefile.debug              ← Build with debug symbols
├── gdb_commands.gdb            ← GDB commands
├── demo_gdb.sh                 ← Interactive demos
├── setup_debug.sh              ← Setup for real switch
├── DEBUG_QUICK_START.md        ← Quick reference
└── DEBUGGING_SETUP_COMPLETE.md ← This file
```

Parent directory docs:
```
/home/heng/workdir/doca/
├── DPA_GDB_DEBUG_GUIDE.md      ← Complete debugging guide
└── DPA_TESTING_TOOLS.md        ← Overview of testing tools
```
