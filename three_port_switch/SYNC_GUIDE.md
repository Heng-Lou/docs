# Sync Guide: What to Update After Changing three_port_switch.c

## Quick Answer

When you update **three_port_switch.c** (the main DOCA implementation), you need to update:

1. ✅ **three_port_switch_sim.c** - For GDB debugging (if logic changes)
2. ✅ **test_three_port_switch.c** - For unit tests (if structures/features change)
3. ✅ **vhost_switch_test.c** - For integration tests (if behavior changes)
4. ✅ **Documentation** - Update relevant .md files

## File Dependency Map

```
three_port_switch.c (853 lines - MAIN IMPLEMENTATION)
         │
         ├─────────────────────────────────────────────┐
         │                                             │
         ↓                                             ↓
three_port_switch_sim.c                    test_three_port_switch.c
(479 lines - GDB DEBUGGING)                (634 lines - UNIT TESTS)
         │                                             │
         │                                             │
         ↓                                             ↓
   For interactive                              For automated
   debugging with GDB                           testing & coverage
         │                                             │
         │                                             │
         └──────────────┬──────────────────────────────┘
                        │
                        ↓
              vhost_switch_test.c
              (INTEGRATION TESTS)
                        │
                        ↓
            Full multi-switch topology
```

## What Each File Does

### 1. three_port_switch.c (Main Implementation)
- **Purpose**: Production DOCA code for BlueField DPU
- **Dependencies**: DOCA SDK, DPDK, real hardware
- **Size**: 853 lines
- **Contains**:
  - Real DOCA Flow API calls
  - DPDK packet processing
  - MAC learning table
  - QoS queues (8 priorities)
  - RSS configuration (4 queues)
  - Hairpin queues (2 queues)
  - TTL/hop limit handling

### 2. three_port_switch_sim.c (GDB Simulation)
- **Purpose**: Standalone simulation for debugging
- **Dependencies**: None (pure C)
- **Size**: 479 lines
- **Contains**:
  - Same forwarding LOGIC as main file
  - Simplified packet structures
  - No DOCA/DPDK dependencies
  - Can debug with standard GDB

**Used by**: `Makefile.sim`, `Makefile.debug`, `demo_gdb.sh`

### 3. test_three_port_switch.c (Unit Tests)
- **Purpose**: Automated unit testing
- **Dependencies**: None (mock structures)
- **Size**: 634 lines
- **Contains**:
  - 17 comprehensive test cases
  - Mock structures matching main implementation
  - Test framework (RUN_TEST, ASSERT_EQ)
  - Coverage analysis support

**Used by**: `Makefile.test`

### 4. vhost_switch_test.c (Integration Tests)
- **Purpose**: End-to-end multi-switch testing
- **Dependencies**: virtual_link.c, virtual_host.c
- **Contains**:
  - Full topology simulation
  - Virtual hosts and switches
  - Packet generation
  - TTL loop prevention

**Used by**: `Makefile.vhost`, `demo_vhost.sh`

## Update Decision Tree

```
Did you change three_port_switch.c?
│
├─ YES → What did you change?
│         │
│         ├─ Forwarding logic (which port → which port)
│         │   → Update: sim.c, test.c, vhost_test.c
│         │
│         ├─ Data structures (added fields to struct)
│         │   → Update: test.c (mock structures)
│         │   → Consider: sim.c (if used in debugging)
│         │
│         ├─ MAC learning algorithm
│         │   → Update: sim.c, test.c (test_mac_learning)
│         │
│         ├─ QoS/RSS/Hairpin configuration
│         │   → Update: test.c (QoS/RSS tests)
│         │
│         ├─ TTL/hop limit handling
│         │   → Update: sim.c, vhost_test.c (TTL tests)
│         │
│         ├─ Port statistics
│         │   → Update: test.c (test_port_statistics)
│         │
│         └─ DOCA API calls only (no logic change)
│             → No update needed!
│
└─ NO → No updates needed

```

## Detailed Update Instructions

### Change Type 1: Forwarding Logic

**Example**: Change port 0 to forward to port 2 instead of port 1

**1. Update three_port_switch_sim.c**

Find `get_forward_port()` function:
```c
// Line ~62 in three_port_switch_sim.c
uint8_t get_forward_port(uint8_t input_port)
{
    switch (input_port) {
        case 0:
            return 2;  // CHANGED from 1
        case 1:
            return 2;
        case 2:
            return 0;
        default:
            return 0;
    }
}
```

**2. Update test_three_port_switch.c**

No changes needed if just changing port numbers.
If logic becomes complex, add new test case.

**3. Update vhost_switch_test.c**

Check if integration test expectations need updating.

**4. Test all files**:
```bash
# Test simulation
make -f Makefile.sim clean all
./switch_sim

# Test unit tests
make -f Makefile.test test

# Test integration
make -f Makefile.vhost
./vhost_switch_test -n 4
```

### Change Type 2: Adding New Structure Fields

**Example**: Add a `max_packets` field to `switch_state`

**1. In three_port_switch.c**:
```c
struct switch_state {
    struct doca_flow_port *ports[NB_PORTS];
    // ... existing fields ...
    uint64_t max_packets;  // NEW
};
```

**2. Update test_three_port_switch.c**:
```c
// Line ~115 - Update mock structure
struct switch_state {
    struct mac_entry mac_table[MAC_TABLE_SIZE];
    struct port_stats port_stats[NB_PORTS];
    struct qos_config qos[NB_PORTS];
    struct rss_config rss[NB_PORTS];
    struct hairpin_config hairpin[NB_PORTS];
    uint32_t learning_enabled;
    uint64_t max_packets;  // ADD THIS
};
```

**3. Maybe update three_port_switch_sim.c**:

Only if the field affects debugging logic:
```c
// Add as global variable if needed
uint64_t max_packets = 1000000;
```

### Change Type 3: Adding New Feature

**Example**: Add VLAN filtering

**Full update sequence**:

1. **Implement in three_port_switch.c**
   - Add VLAN structures
   - Add DOCA Flow VLAN matching
   - Add configuration functions

2. **Add to test_three_port_switch.c**
   - Create new test function `test_vlan_filtering()`
   - Add mock VLAN structures
   - Add to main() with `RUN_TEST(test_vlan_filtering)`

3. **Add to three_port_switch_sim.c** (optional)
   - Only if you want to debug VLAN logic with GDB
   - Add simplified VLAN handling to `process_packet()`

4. **Update vhost_switch_test.c** (if needed)
   - Add VLAN tags to test packets
   - Verify VLAN filtering in integration test

5. **Update documentation**
   - Add VLAN section to README.md
   - Create VLAN_IMPLEMENTATION.md
   - Update TEST_DOCUMENTATION.md

## Mapping Table: Main → Simulation → Test

| Feature | three_port_switch.c | three_port_switch_sim.c | test_three_port_switch.c |
|---------|---------------------|------------------------|-------------------------|
| **Structures** | `struct switch_state` | `port_stats_t[]`, `port_config_t[]` | `struct switch_state` (mock) |
| **Packets** | `struct rte_mbuf *` | `packet_t *` | Mock packet data |
| **Forwarding** | DOCA Flow pipes | `get_forward_port()` | Logic tests |
| **MAC Learning** | `sw_state.mac_table[]` | Global `mac_table[]` | `test_mac_learning()` |
| **Statistics** | `sw_state.packets_*` | `port_stats[].tx_packets` | `test_port_statistics()` |
| **QoS** | DOCA Flow QoS queues | Basic priority in comments | `test_vlan_pcp_mapping()` |
| **RSS** | `doca_flow_pipe_rss()` | Comment only | `test_rss_config()` |
| **TTL** | `process_packet_ttl()` | Same function name | No unit test (in vhost) |

## Quick Commands

### Build All Variants
```bash
# Main DOCA version (requires hardware)
meson build && ninja -C build

# Simulation for GDB
make -f Makefile.sim

# Unit tests
make -f Makefile.test

# Integration tests
make -f Makefile.vhost
```

### Test All Variants
```bash
# Unit tests
make -f Makefile.test test

# Unit tests with coverage
make -f Makefile.test run-coverage

# Integration test (4 hosts/switches)
./vhost_switch_test -n 4

# Stress test
./vhost_switch_test -n 8 -r 1000 -c 10000
```

### Debug Simulation
```bash
# Build with debug symbols
make -f Makefile.sim clean all

# Run in GDB
gdb ./switch_sim

# Or use demo script
./demo_gdb.sh
```

## Common Update Scenarios

### Scenario 1: Bug Fix in Forwarding

```bash
# 1. Fix in main file
vim three_port_switch.c  # Fix the bug

# 2. Port to simulation
vim three_port_switch_sim.c  # Apply same fix

# 3. Test simulation
make -f Makefile.sim && ./switch_sim

# 4. Build DOCA version
ninja -C build
```

### Scenario 2: Add New QoS Feature

```bash
# 1. Implement in main
vim three_port_switch.c  # Add QoS feature

# 2. Add unit test
vim test_three_port_switch.c  # Add test_new_qos_feature()

# 3. Run tests
make -f Makefile.test test

# 4. Update docs
vim QOS_FEATURES.md  # Document new feature
```

### Scenario 3: Change Data Structure

```bash
# 1. Change in main
vim three_port_switch.c  # Modify struct switch_state

# 2. Update mock in test
vim test_three_port_switch.c  # Update struct switch_state mock

# 3. Verify compilation
make -f Makefile.test clean all

# 4. Run tests
make -f Makefile.test test
```

## Checklist After Updating three_port_switch.c

- [ ] Identified what changed (logic, structure, feature)
- [ ] Updated three_port_switch_sim.c (if logic changed)
- [ ] Updated test_three_port_switch.c (if structure/feature changed)
- [ ] Updated vhost_switch_test.c (if integration behavior changed)
- [ ] Built all variants without errors
- [ ] Ran unit tests (all pass)
- [ ] Ran integration tests (work as expected)
- [ ] Updated documentation
- [ ] Committed changes with clear message

## Example Commit Message

```
Add VLAN filtering support

Changes:
- three_port_switch.c: Implemented DOCA Flow VLAN matching
- test_three_port_switch.c: Added test_vlan_filtering() test case
- three_port_switch_sim.c: Added VLAN tag checking in process_packet()
- VLAN_FEATURES.md: New documentation for VLAN filtering

Tested:
- Unit tests: 18/18 passed (added 1 new test)
- Integration: VLAN packets correctly filtered
- Simulation: GDB debugging verified VLAN logic
```

## Summary

### Files to Always Check:
1. **three_port_switch_sim.c** - If forwarding logic changes
2. **test_three_port_switch.c** - If structures or features change

### Files to Sometimes Check:
3. **vhost_switch_test.c** - If integration behavior changes
4. **Documentation** - Always update for new features

### The Rule:
**Same logic in all files, different APIs**
- Main file: DOCA/DPDK APIs
- Simulation: Pure C
- Tests: Mock structures
- All three: Same forwarding decisions

