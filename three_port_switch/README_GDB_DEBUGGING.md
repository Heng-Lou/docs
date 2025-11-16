# DPA Program GDB Debugging - Setup Complete ✅

## Overview

You now have a **complete GDB debugging environment** for your three-port switch DPA program, even **without hardware**!

---

## What You Have

### ✅ Working Simulation
A host-side simulation (`switch_sim`) that mimics your three-port switch logic. You can debug it with standard GDB right now.

### ✅ Debugging Tools
- Custom GDB commands for examining packets and statistics
- Automated debugging demos
- Pre-configured breakpoints and watchpoints
- Setup scripts for future DPA debugging

### ✅ Documentation
- Quick start guide (`DEBUG_QUICK_START.md`)
- Complete debugging guide (`DPA_GDB_DEBUG_GUIDE.md`)
- Setup completion summary (`DEBUGGING_SETUP_COMPLETE.md`)

---

## Quick Start (3 Steps)

### Step 1: Test the Simulation

```bash
cd /home/heng/workdir/doca/three_port_switch
./switch_sim test
```

**Expected:** All 5 tests pass ✓

### Step 2: Try Interactive Demo

```bash
./demo_gdb.sh
# Select option 2 (automated demo)
```

**You'll see:** GDB breaking at packet processing, showing variables, stepping through code.

### Step 3: Manual GDB Debugging

```bash
gdb -x gdb_commands.gdb ./switch_sim
```

**In GDB:**
```gdb
(gdb) run test              # Run the tests
(gdb) show_all_stats        # Custom command - shows port stats
(gdb) print pkt->port_id    # Examine packet
(gdb) step                  # Step through code
(gdb) quit                  # Exit
```

---

## Why This Matters

### Without Hardware, You Can:

1. **Test Switch Logic** - Verify packet forwarding works correctly
2. **Learn GDB** - Practice debugging techniques you'll use with DPA
3. **Find Bugs Early** - Catch logic errors before deploying to hardware
4. **Rapid Iteration** - Compile and test in seconds, not minutes

### Same Techniques Apply to DPA

The debugging techniques you learn with the simulation work the same way with DPA:
- Setting breakpoints in kernel functions
- Examining packet structures
- Watching statistics change
- Stepping through forwarding logic

**The only difference:** Use `dpa-gdb` instead of `gdb` when debugging on hardware.

---

## Files Created

```
/home/heng/workdir/doca/three_port_switch/
├── three_port_switch_sim.c          # Simulation source
├── switch_sim                       # Compiled simulation
├── Makefile.debug                   # Build with debug symbols
├── gdb_commands.gdb                 # Custom GDB commands
├── demo_gdb.sh                      # Interactive demos
├── setup_debug.sh                   # Setup for actual switch
├── DEBUG_QUICK_START.md             # Quick reference
├── DEBUGGING_SETUP_COMPLETE.md      # Setup summary
└── README_GDB_DEBUGGING.md          # This file

/home/heng/workdir/doca/
└── DPA_GDB_DEBUG_GUIDE.md           # Complete debugging guide
```

---

## Demo Modes

Run `./demo_gdb.sh` and select:

**Option 1: Interactive GDB Session**
- Full GDB environment with custom commands loaded
- You control everything
- Best for learning GDB interactively

**Option 2: Automated Demo**
- Shows breakpoints, stepping, variable examination
- Runs automatically
- Best for seeing GDB in action

**Option 3: Watch Demo**
- Demonstrates watchpoints
- Shows how to track variable changes
- Best for understanding when/why statistics change

**Option 4: Conditional Breakpoint Demo**
- Shows conditional breakpoints
- Only breaks for specific packets
- Best for debugging specific scenarios

---

## Learning Path

### Phase 1: Master the Simulation ✅ (Start Here)

```bash
# Run tests
./switch_sim test

# Try interactive mode
./switch_sim interactive
> send 0 64        # Send packet from port 0
> stats            # Show statistics
> disable 1        # Disable port 1
> send 0 64        # Try again (will error)
> enable 1         # Re-enable
> quit

# Debug with GDB
./demo_gdb.sh      # Select option 2
```

### Phase 2: Learn GDB Commands

```bash
gdb -x gdb_commands.gdb ./switch_sim

# Try these commands:
(gdb) break process_packet                    # Set breakpoint
(gdb) run test                                # Run tests
(gdb) print pkt->port_id                      # Examine variable
(gdb) show_packet pkt                         # Custom command
(gdb) step                                    # Step into function
(gdb) next                                    # Step over
(gdb) watch port_stats[0].rx_packets          # Watch variable
(gdb) break process_packet if pkt->size > 100 # Conditional break
(gdb) continue                                # Continue execution
```

### Phase 3: Apply to DPA (When Hardware Available)

```bash
# Setup DPA debugging
./setup_debug.sh

# Start GDB server (Terminal 1)
./build/start_gdbserver.sh

# Debug DPA program (Terminal 2)
./build/start_debug.sh
# Select option 2 (DPA debugging)
```

---

## Custom GDB Commands

Loaded automatically from `gdb_commands.gdb`:

### `show_packet <pkt>`
Display detailed packet information:
```gdb
(gdb) show_packet pkt
=== Packet Details ===
Port ID:   0
Size:      64 bytes
Timestamp: 0
Data[0-7]: 0x00 0x01 0x02 0x03...
```

### `show_all_stats`
Display statistics for all ports:
```gdb
(gdb) show_all_stats
=== Port Statistics ===

Port 0 (pci_port):
  Enabled: Yes
  RX: 1 pkts, 64 bytes, 0 errors
  TX: 0 pkts, 0 bytes, 0 errors
...
```

---

## Common GDB Workflows

### Workflow 1: Debug Packet Forwarding

```gdb
(gdb) break get_forward_port
(gdb) run test
# Stopped at get_forward_port
(gdb) print input_port           # See input port
(gdb) finish                     # Run until return
(gdb) print $retval              # See which port it returns
```

### Workflow 2: Track Statistics

```gdb
(gdb) watch port_stats[0].rx_packets
(gdb) run test
# Stops when rx_packets changes
(gdb) backtrace                  # See what changed it
(gdb) print pkt                  # See the packet
```

### Workflow 3: Debug Errors

```gdb
(gdb) break process_packet if pkt->port_id >= 3
(gdb) run test
# Only stops for invalid ports
(gdb) print *pkt                 # Examine bad packet
(gdb) step                       # See error handling
```

---

## Debugging Cheat Sheet

| Task | GDB Command |
|------|-------------|
| Set breakpoint | `break function_name` |
| Set breakpoint at line | `break file.c:123` |
| Run program | `run [args]` |
| Continue execution | `continue` or `c` |
| Step into function | `step` or `s` |
| Step over function | `next` or `n` |
| Print variable | `print var` or `p var` |
| Watch variable | `watch var` |
| Show call stack | `backtrace` or `bt` |
| Show local variables | `info locals` |
| Examine memory | `x/16x address` |
| List source code | `list` |
| Quit GDB | `quit` or `q` |

---

## What's Next?

### Immediate Actions

1. **Run the simulation tests:**
   ```bash
   ./switch_sim test
   ```

2. **Try the automated demo:**
   ```bash
   ./demo_gdb.sh
   # Select option 2
   ```

3. **Practice manual debugging:**
   ```bash
   gdb -x gdb_commands.gdb ./switch_sim
   ```

### Optional Enhancements

4. **Modify the simulation:**
   - Edit `three_port_switch_sim.c`
   - Add your own tests
   - Try different forwarding logic

5. **Create custom GDB commands:**
   - Edit `gdb_commands.gdb`
   - Add your own debugging helpers

### When Hardware Arrives

6. **Debug actual DPA program:**
   - Run `./setup_debug.sh`
   - Use same techniques with `dpa-gdb`
   - Debug on real BlueField hardware

---

## Troubleshooting

### "switch_sim not found"
```bash
make -f Makefile.debug
```

### "GDB not found"
```bash
sudo apt install gdb
```

### "Can't set breakpoint"
Ensure you built with debug symbols:
```bash
make -f Makefile.debug clean
make -f Makefile.debug
```

---

## Summary

### ✅ What Works Now (No Hardware Needed)

- ✅ Three-port switch simulation compiles and runs
- ✅ All 5 automated tests pass
- ✅ Interactive mode works
- ✅ GDB debugging fully functional
- ✅ Custom GDB commands loaded
- ✅ Automated demos available

### 🔧 What's Prepared (For When Hardware Arrives)

- 🔧 DPA debugging setup script
- 🔧 GDB server launcher
- 🔧 Debug environment configuration
- 🔧 Complete documentation

### 📚 What You Can Learn Now

- 📚 GDB debugging techniques
- 📚 Packet processing logic
- 📚 Statistics tracking
- 📚 Error handling
- 📚 Testing strategies

---

## Get Started

```bash
cd /home/heng/workdir/doca/three_port_switch

# Quick test
./switch_sim test

# Interactive debugging demo
./demo_gdb.sh
```

---

## Documentation

- **Quick Start:** `DEBUG_QUICK_START.md`
- **Complete Guide:** `DPA_GDB_DEBUG_GUIDE.md` (in parent directory)
- **Setup Summary:** `DEBUGGING_SETUP_COMPLETE.md`
- **This File:** `README_GDB_DEBUGGING.md`

---

**Your DPA debugging environment is ready!** 🎉

You can now debug DPA-style programs with GDB, even without BlueField hardware. The same techniques you learn here will apply when you debug on actual hardware with `dpa-gdb`.

**Happy Debugging!** 🐛🔍
