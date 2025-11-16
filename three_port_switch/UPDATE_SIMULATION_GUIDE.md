# How to Update Simulation from Main Code

## Overview

The files serve different purposes:
- **three_port_switch.c** - Production DPA/DOCA code that runs on BlueField
- **three_port_switch_sim.c** - Host simulation for GDB debugging without hardware

## Key Differences

The simulation file (`three_port_switch_sim.c`) is NOT auto-generated. It's a **simplified standalone version** that:
1. Removes DOCA/DPDK/DPA dependencies
2. Uses simple C structures instead of DOCA APIs
3. Implements the same forwarding logic for testing
4. Can be debugged with standard GDB on any host

## When to Update the Simulation

Update `three_port_switch_sim.c` when you change:
1. **Forwarding logic** - the core switch behavior
2. **MAC learning algorithm** - how addresses are learned
3. **Port mapping** - which ports forward to which
4. **Packet validation** - checks before forwarding

## How to Update

### Manual Sync Process

Since the simulation uses different APIs, you must **manually port** logic changes:

1. **Identify the changed logic** in `three_port_switch.c`
   ```bash
   git diff three_port_switch.c  # See what changed
   ```

2. **Find equivalent section** in `three_port_switch_sim.c`
   - Look for function names or comments
   - The logic should be similar, just simpler

3. **Port the changes** by hand
   - Replace DOCA calls with simulation equivalents
   - Keep the algorithm the same

### Example: Changing Forward Logic

**In three_port_switch.c** (DOCA version):
```c
// Change forwarding: Port 0 -> Port 2 instead of Port 1
static int forward_packet(struct rte_mbuf *pkt, uint16_t src_port) {
    uint16_t dst_port = (src_port == 0) ? 2 : 0;  // Changed from 1
    return rte_eth_tx_burst(dst_port, 0, &pkt, 1);
}
```

**In three_port_switch_sim.c** (Simulation):
```c
// Port the same logic change
uint8_t get_forward_port(uint8_t input_port) {
    switch (input_port) {
        case 0:
            return 2;  // Changed from 1 - MATCHES MAIN CODE
        case 1:
            return 2;
        case 2:
            return 0;
        default:
            return 0;
    }
}
```

## Common Mapping Patterns

| three_port_switch.c (DOCA) | three_port_switch_sim.c (Simulation) |
|----------------------------|--------------------------------------|
| `rte_mbuf *pkt` | `packet_t *pkt` |
| `rte_eth_tx_burst()` | `port_stats[].tx_packets++` |
| `doca_flow_pipe_add_entry()` | `// Simulated in forwarding logic` |
| `struct mac_entry` | `typedef struct mac_entry` |
| `DOCA_LOG_ERR()` | `printf("ERROR: ...")` |

## Testing Your Changes

1. **Build the simulation**:
   ```bash
   cd /home/heng/workdir/doca/three_port_switch
   gcc -g -O0 -o switch_sim three_port_switch_sim.c
   ```

2. **Test with GDB**:
   ```bash
   gdb ./switch_sim
   (gdb) break process_packet
   (gdb) run
   (gdb) step
   ```

3. **Verify logic matches**:
   - Set breakpoints in both versions
   - Step through packet processing
   - Confirm same decisions are made

## Automated Approach (Future)

You could create a script to help identify changes:

```bash
#!/bin/bash
# compare_logic.sh - Shows functions that changed

echo "Functions changed in three_port_switch.c:"
git diff HEAD~1 three_port_switch.c | grep "^[-+].*{" | grep -v "^[-+][\t ]*$"

echo ""
echo "Review these functions in three_port_switch_sim.c and update manually"
```

## Current State

The simulation file was **manually created** based on the initial version of `three_port_switch.c`. It contains:

- Simplified 3-port forwarding logic
- Port statistics tracking  
- Basic packet validation
- Standalone C code (no DOCA dependencies)

**No automatic sync exists** - you maintain both files separately with the same logical behavior.

## Best Practice

1. **Document your changes** in both files with comments
2. **Test simulation first** with GDB before hardware
3. **Keep commits in sync** - update both files in same commit
4. **Use git commit messages** to note what logic changed

Example commit message:
```
Changed port 0 forwarding from port 1 to port 2

Updated both three_port_switch.c (DOCA) and 
three_port_switch_sim.c (simulation) to forward
PCI port traffic to Ethernet port 2 instead of port 1.
```

