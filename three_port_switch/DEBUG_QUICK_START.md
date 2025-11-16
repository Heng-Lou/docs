# Three-Port Switch GDB Debugging - Quick Start

## TL;DR - Start Debugging Now

```bash
cd /home/heng/workdir/doca/three_port_switch

# Option 1: Debug simulation (no hardware needed)
make -f Makefile.debug test        # Run tests
make -f Makefile.debug debug       # Debug with GDB

# Option 2: Setup debugging for actual switch
./setup_debug.sh                   # Setup debug environment
./build/start_debug.sh            # Start debugging
```

---

## What You Can Debug

### 1. Simulation (Available Now - No Hardware) ✅

Test and debug switch logic on your host machine:

```bash
# Build simulation
make -f Makefile.debug

# Run tests
./switch_sim test

# Debug with GDB
gdb ./switch_sim
(gdb) break process_packet
(gdb) run test
(gdb) step
(gdb) print pkt->port_id
```

**What you can debug:**
- ✅ Packet forwarding logic
- ✅ Port statistics tracking
- ✅ Error handling
- ✅ MTU validation
- ✅ Port enable/disable

### 2. Actual Three-Port Switch (When Ready)

Debug the real DOCA application:

```bash
# Setup debug environment
./setup_debug.sh

# Start debugging
./build/start_debug.sh

# Choose option 1 for host-side debugging
```

---

## Quick Examples

### Example 1: Debug a Single Packet

```bash
# Start GDB
gdb ./switch_sim

# GDB commands:
(gdb) break process_packet
(gdb) run test
(gdb) print pkt->port_id        # See which port
(gdb) print pkt->size          # See packet size
(gdb) step                     # Step through code
(gdb) print out_port           # See forwarding decision
(gdb) continue                 # Continue to next packet
```

### Example 2: Watch Statistics Changes

```bash
gdb ./switch_sim

# GDB commands:
(gdb) watch port_stats[0].rx_packets
(gdb) run test
# GDB will stop whenever rx_packets changes
(gdb) print port_stats[0]
(gdb) backtrace              # See what caused the change
(gdb) continue
```

### Example 3: Debug Specific Test

```bash
gdb ./switch_sim

# GDB commands:
(gdb) break run_tests
(gdb) run test
(gdb) step                   # Step through each test
(gdb) print port_stats       # Examine state
(gdb) next                   # Next line
```

### Example 4: Interactive Debugging

```bash
# Run in one terminal
./switch_sim interactive

# Commands you can use:
> send 0 64              # Send packet from port 0, size 64
> stats                  # Show statistics
> disable 1              # Disable port 1
> send 0 64              # Try sending again
> enable 1               # Re-enable port 1
> reset                  # Reset statistics
> quit                   # Exit
```

---

## GDB Command Reference

### Essential Commands

| Command | Description | Example |
|---------|-------------|---------|
| `break` | Set breakpoint | `break process_packet` |
| `run` | Start program | `run test` |
| `step` | Step into function | `step` |
| `next` | Step over function | `next` |
| `continue` | Continue execution | `continue` |
| `print` | Print variable | `print pkt->port_id` |
| `watch` | Watch variable | `watch counter` |
| `backtrace` | Show call stack | `bt` |
| `quit` | Exit GDB | `quit` |

### Useful Shortcuts

| Shortcut | Full Command |
|----------|-------------|
| `b` | `break` |
| `r` | `run` |
| `s` | `step` |
| `n` | `next` |
| `c` | `continue` |
| `p` | `print` |
| `bt` | `backtrace` |
| `q` | `quit` |

### Advanced Commands

```gdb
# Examine memory
(gdb) x/16x pkt->data          # Show 16 bytes in hex

# Print array
(gdb) print port_stats[0]@3    # Print all 3 port stats

# Conditional breakpoint
(gdb) break process_packet if pkt->port_id == 2

# Display variable after each step
(gdb) display pkt->port_id

# Set variable value
(gdb) set var pkt->size = 100

# Save breakpoints
(gdb) save breakpoints bp.txt

# Load breakpoints
(gdb) source bp.txt
```

---

## Debugging Workflows

### Workflow 1: Find Bug in Forwarding

```bash
# 1. Build with debug symbols
make -f Makefile.debug

# 2. Start GDB
gdb ./switch_sim

# 3. Set breakpoint at forwarding logic
(gdb) break get_forward_port

# 4. Run tests
(gdb) run test

# 5. Examine input and output
(gdb) print input_port
(gdb) step
(gdb) print forward_port

# 6. Fix issue in code, rebuild, repeat
```

### Workflow 2: Debug Statistics Tracking

```bash
# 1. Start GDB with watchpoint
gdb ./switch_sim

# 2. Watch specific statistic
(gdb) watch port_stats[0].rx_packets

# 3. Run
(gdb) run test

# 4. When it stops, examine why
(gdb) backtrace
(gdb) print pkt
(gdb) print port_stats[0]

# 5. Continue to next change
(gdb) continue
```

### Workflow 3: Debug Error Handling

```bash
gdb ./switch_sim

# Break on error path
(gdb) break process_packet
(gdb) condition 1 pkt->port_id >= 3    # Only break on invalid port

(gdb) run test
# Stops only when error condition occurs
(gdb) backtrace
(gdb) print pkt
```

---

## Files Created

After running `setup_debug.sh`, you'll have:

```
three_port_switch/
├── setup_debug.sh              # Setup script
├── three_port_switch_sim.c     # Simulation code
├── Makefile.debug              # Debug Makefile
├── DEBUG_QUICK_START.md        # This file
└── build/
    ├── debug_commands.gdb      # GDB commands
    ├── start_debug.sh          # Debug launcher
    └── start_gdbserver.sh      # GDB server (for hardware)
```

---

## Common Issues

### Issue 1: "No symbol table is loaded"

**Solution:**
```bash
# Rebuild with debug symbols
make -f Makefile.debug clean
make -f Makefile.debug
```

### Issue 2: "Source file not found"

**Solution:**
```gdb
(gdb) directory /home/heng/workdir/doca/three_port_switch
(gdb) list
```

### Issue 3: Optimizations hiding variables

**Solution:**
```bash
# Ensure -O0 flag (already in Makefile.debug)
gcc -g -O0 -o switch_sim three_port_switch_sim.c
```

---

## Next Steps

### Step 1: Learn Simulation ✅ (Do This First)

```bash
cd /home/heng/workdir/doca/three_port_switch

# Build and test
make -f Makefile.debug test

# Debug it
make -f Makefile.debug debug
```

### Step 2: Prepare Real Switch Debugging

```bash
# Setup debug environment
./setup_debug.sh

# Review GDB commands
cat build/debug_commands.gdb
```

### Step 3: When You Get Hardware

```bash
# Terminal 1: Start GDB server
./build/start_gdbserver.sh

# Terminal 2: Debug application
./build/start_debug.sh
# Choose option 2 (DPA debugging)
```

---

## Learning Resources

### Practice These GDB Skills

1. **Setting breakpoints**
   ```gdb
   (gdb) break process_packet
   (gdb) break three_port_switch_sim.c:100
   ```

2. **Stepping through code**
   ```gdb
   (gdb) step    # Step into functions
   (gdb) next    # Step over functions
   (gdb) finish  # Run until function returns
   ```

3. **Examining variables**
   ```gdb
   (gdb) print pkt
   (gdb) print *pkt
   (gdb) print pkt->data[0]
   ```

4. **Watching for changes**
   ```gdb
   (gdb) watch port_stats[0].rx_packets
   ```

5. **Conditional breakpoints**
   ```gdb
   (gdb) break process_packet if pkt->size > 1000
   ```

---

## Summary

### You Have Two Debugging Environments:

#### 1. Simulation (No Hardware) ✅ **START HERE**

```bash
make -f Makefile.debug debug
```

**Perfect for:**
- Learning GDB
- Testing logic
- Finding bugs
- Quick iteration

#### 2. Real Switch (Needs Hardware)

```bash
./setup_debug.sh
./build/start_debug.sh
```

**Perfect for:**
- Hardware-specific issues
- DPA kernel debugging
- Performance analysis
- Final validation

---

## Get Started Now

```bash
cd /home/heng/workdir/doca/three_port_switch

# Quick test
make -f Makefile.debug test

# Start debugging
make -f Makefile.debug debug

# In GDB:
(gdb) break process_packet
(gdb) run test
(gdb) step
(gdb) print pkt->port_id
```

**Happy Debugging!** 🐛🔍
