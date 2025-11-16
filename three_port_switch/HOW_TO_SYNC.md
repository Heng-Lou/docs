# How to Sync Changes Between DOCA and Simulation Files

## Quick Answer

**The simulation file is NOT auto-generated.** You manually maintain both files with the same logic but different APIs.

## File Purposes

### three_port_switch.c (Production)
- Real DOCA/DPDK code for BlueField DPU
- Uses `struct switch_state sw_state` for global state
- Requires DOCA SDK and hardware

### three_port_switch_sim.c (Debugging) 
- Standalone C simulation for GDB testing
- Uses `port_stats[]` and `port_config[]` for state
- No dependencies, runs on any Linux host

## State Structure Mapping

### In three_port_switch.c (lines 58-71):
```c
struct switch_state {
    struct doca_flow_port *ports[NB_PORTS];
    struct port_config port_configs[NB_PORTS];
    struct mac_entry mac_table[MAC_TABLE_SIZE];
    volatile sig_atomic_t keep_running;
    uint64_t packets_forwarded;
    uint64_t packets_dropped;
};

static struct switch_state sw_state = {
    .keep_running = 1,
    .packets_forwarded = 0,
    .packets_dropped = 0
};
```

### In three_port_switch_sim.c (lines 45-50):
```c
// Equivalent simulated state (simplified)
port_stats_t port_stats[3] = {0};
port_config_t port_config[3] = {
    {0, true, "pci_port", 1500},
    {1, true, "eth_port_1", 1500},
    {2, true, "eth_port_2", 1500}
};
```

## Update Workflow

### Example: Change Forwarding Logic

Let's say you want to change port 0 to forward to port 2 instead of port 1.

**Step 1: Edit three_port_switch.c**

Find the forwarding function and modify:
```c
// OLD:
static uint16_t get_output_port(uint16_t input_port) {
    if (input_port == 0) return 1;  // PCI -> Eth1
    // ...
}

// NEW:
static uint16_t get_output_port(uint16_t input_port) {
    if (input_port == 0) return 2;  // PCI -> Eth2 (CHANGED)
    // ...
}
```

**Step 2: Identify equivalent in three_port_switch_sim.c**

Find the same logic (around line 59):
```c
uint8_t get_forward_port(uint8_t input_port)
{
    switch (input_port) {
        case 0:
            return 1;  // PCI -> Eth1
        // ...
    }
}
```

**Step 3: Apply same change**
```c
uint8_t get_forward_port(uint8_t input_port)
{
    switch (input_port) {
        case 0:
            return 2;  // PCI -> Eth2 (CHANGED - matches DOCA version)
        // ...
    }
}
```

**Step 4: Test simulation**
```bash
cd /home/heng/workdir/doca/three_port_switch
gcc -g -O0 -o switch_sim three_port_switch_sim.c
gdb ./switch_sim

# In GDB:
(gdb) break get_forward_port
(gdb) run
(gdb) print input_port
(gdb) next
(gdb) print forward_port  # Should be 2, not 1
```

**Step 5: Build DOCA version**
```bash
meson setup build
ninja -C build
```

## What Needs Syncing?

### Always Sync:
- ✓ Forwarding logic (which port sends to which)
- ✓ MAC learning algorithm
- ✓ Packet validation rules
- ✓ Port mappings

### Never Sync:
- ✗ DOCA API calls (doca_flow_*, rte_*)
- ✗ Initialization code
- ✗ Hardware-specific config

### Maybe Sync:
- ? Statistics counters (if logic changes)
- ? Error handling (if validation changes)

## API Translation Table

| DOCA Code | Simulation Equivalent |
|-----------|----------------------|
| `sw_state.packets_forwarded++` | `port_stats[dst].tx_packets++` |
| `sw_state.packets_dropped++` | `port_stats[src].tx_errors++` |
| `rte_mbuf *pkt` | `packet_t *pkt` |
| `rte_pktmbuf_mtod(pkt, void*)` | `pkt->data` |
| `DOCA_LOG_ERR("error")` | `printf("ERROR: error\n")` |
| `lookup_mac(mac, &port)` | `// Same function, port logic` |

## Helper Commands

### Compare functions in both files:
```bash
cd /home/heng/workdir/doca/three_port_switch
./compare_files.sh
```

### See what you changed:
```bash
# If using git:
git diff three_port_switch.c

# Manual diff:
diff -u three_port_switch.c.backup three_port_switch.c
```

### Test simulation quickly:
```bash
gcc -g -O0 -o switch_sim three_port_switch_sim.c && ./switch_sim
```

### Debug simulation:
```bash
gdb ./switch_sim
(gdb) list process_packet
(gdb) break process_packet
(gdb) run
```

## Example: Adding a New Feature

Let's add a packet counter limit.

### 1. Add to DOCA version (three_port_switch.c):

Add to switch_state (line 63):
```c
struct switch_state {
    // ... existing fields ...
    uint64_t max_packets;  // NEW
};
```

Initialize (line 70):
```c
static struct switch_state sw_state = {
    .keep_running = 1,
    .packets_forwarded = 0,
    .packets_dropped = 0,
    .max_packets = 1000000  // NEW
};
```

Add check in forwarding:
```c
if (sw_state.packets_forwarded >= sw_state.max_packets) {
    DOCA_LOG_WARN("Max packets reached");
    sw_state.packets_dropped++;
    return;
}
```

### 2. Add to simulation (three_port_switch_sim.c):

Add global variable (after line 50):
```c
uint64_t max_packets = 1000000;  // NEW
uint64_t total_forwarded = 0;     // NEW
```

Add check in process_packet:
```c
void process_packet(packet_t *pkt) {
    if (total_forwarded >= max_packets) {  // NEW
        printf("ERROR: Max packets reached\n");
        return;
    }
    
    // ... existing code ...
    
    total_forwarded++;  // NEW
}
```

### 3. Test both:
```bash
# Test simulation
gcc -g -O0 -o switch_sim three_port_switch_sim.c
./switch_sim

# Test DOCA (when you have hardware)
ninja -C build
sudo ./build/doca_three_port_switch
```

## Best Practices

1. **Comment your changes** in both files:
   ```c
   // Changed: Forward PCI to Eth2 instead of Eth1 (2024-11-10)
   return 2;
   ```

2. **Test simulation first** before hardware:
   - Faster iteration
   - Easier debugging with GDB
   - No hardware needed

3. **Keep changes small**:
   - One logical change at a time
   - Easier to track and sync

4. **Document in commit**:
   ```
   git commit -m "Forward PCI traffic to Eth2
   
   - Updated three_port_switch.c forwarding logic
   - Synced change to three_port_switch_sim.c
   - Tested in simulation with GDB"
   ```

## Automation Possibility

Future enhancement - create a script to extract and compare logic:

```bash
#!/bin/bash
# extract_logic.sh - Extract forwarding logic from both files

echo "=== DOCA Version ==="
sed -n '/get_output_port/,/^}/p' three_port_switch.c

echo ""
echo "=== Simulation Version ==="
sed -n '/get_forward_port/,/^}/p' three_port_switch_sim.c
```

This helps visually compare the core logic.

## Summary

- **No automatic generation** - manual sync required
- **Same logic, different APIs** - translate DOCA calls to simulation equivalents  
- **Test simulation first** - GDB debugging without hardware
- **Keep in sync** - update both files when changing forwarding behavior
- **Use the guides** - UPDATE_SIMULATION_GUIDE.md and this file

