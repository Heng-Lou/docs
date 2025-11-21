# Quick Update Checklist

## When You Change three_port_switch.c

Use this checklist to know what else to update.

---

## Step 1: Identify Your Change Type

Check one:
- [ ] **Forwarding Logic** (which port forwards to which)
- [ ] **Data Structures** (added/modified struct fields)
- [ ] **MAC Learning** (learning algorithm changes)
- [ ] **QoS/RSS/Hairpin** (queue configuration)
- [ ] **TTL/Hop Limit** (packet lifetime handling)
- [ ] **Statistics** (counters and tracking)
- [ ] **New Feature** (entirely new functionality)
- [ ] **DOCA API Only** (no logic change)

---

## Step 2: Update Required Files

### If you changed: Forwarding Logic
- [x] **three_port_switch_sim.c** - Update `get_forward_port()` function
- [ ] test_three_port_switch.c - Usually no change needed
- [ ] vhost_switch_test.c - Check if expectations changed

### If you changed: Data Structures
- [ ] three_port_switch_sim.c - Maybe (if used in debugging)
- [x] **test_three_port_switch.c** - Update mock structures (~line 115)
- [ ] vhost_switch_test.c - Check if structure is used

### If you changed: MAC Learning
- [x] **three_port_switch_sim.c** - Update learning logic
- [x] **test_three_port_switch.c** - Update test_mac_learning(), test_mac_lookup()
- [ ] vhost_switch_test.c - Usually no change needed

### If you changed: QoS/RSS/Hairpin
- [ ] three_port_switch_sim.c - Usually no change (DOCA-specific)
- [x] **test_three_port_switch.c** - Update QoS/RSS/Hairpin tests
- [ ] vhost_switch_test.c - Check if queue behavior changed

### If you changed: TTL/Hop Limit
- [x] **three_port_switch_sim.c** - Update TTL decrement logic
- [ ] test_three_port_switch.c - Usually no change
- [x] **vhost_switch_test.c** - Update TTL tests (used in ring topology)

### If you changed: Statistics
- [ ] three_port_switch_sim.c - Update port_stats if debugging
- [x] **test_three_port_switch.c** - Update test_port_statistics()
- [ ] vhost_switch_test.c - Check if stats display needs update

### If you added: New Feature
- [x] **three_port_switch_sim.c** - Add if needed for debugging
- [x] **test_three_port_switch.c** - Add new test function
- [ ] vhost_switch_test.c - Add if integration test needed
- [x] **Documentation** - Create/update .md files

### If you changed: DOCA API Only
- [ ] three_port_switch_sim.c - No change needed! ✓
- [ ] test_three_port_switch.c - No change needed! ✓
- [ ] vhost_switch_test.c - No change needed! ✓

---

## Step 3: Build & Test Everything

```bash
# 1. Build simulation
make -f Makefile.sim clean all

# 2. Build unit tests
make -f Makefile.test clean all

# 3. Build integration tests
make -f Makefile.vhost clean all

# 4. Run unit tests
make -f Makefile.test test

# 5. Run integration test
./vhost_switch_test -n 4 -d 5
```

---

## Step 4: Verify Tests Pass

### Unit Tests (Expected: 17/17)
```bash
make -f Makefile.test test
# Look for: "17 tests passed, 0 failed"
```

### Integration Test (Expected: No errors)
```bash
./vhost_switch_test -n 4 -d 5
# Look for: All switches running, packets forwarding
```

### Simulation (Expected: Compiles)
```bash
make -f Makefile.sim
# Look for: No compilation errors
```

---

## Step 5: Update Documentation

If you added features:
- [ ] Update README.md
- [ ] Create FEATURE_NAME.md if significant
- [ ] Update TEST_DOCUMENTATION.md with new tests
- [ ] Update this checklist if needed

If you fixed bugs:
- [ ] Add to DEVELOPMENT_HISTORY.md
- [ ] Update relevant .md files

---

## Quick Reference: File Purposes

| File | Purpose | Update When |
|------|---------|-------------|
| `three_port_switch.c` | Main DOCA implementation | Always (your changes) |
| `three_port_switch_sim.c` | GDB debugging simulation | Logic changes |
| `test_three_port_switch.c` | Unit tests | Structure/feature changes |
| `vhost_switch_test.c` | Integration tests | Behavior changes |

---

## Common Mistakes to Avoid

❌ **Don't**: Change three_port_switch.c and forget to update tests
✅ **Do**: Update tests in the same commit

❌ **Don't**: Only update unit tests, ignore simulation
✅ **Do**: Keep simulation in sync for GDB debugging

❌ **Don't**: Change structures without updating mocks
✅ **Do**: Update mock structures in test file

❌ **Don't**: Skip running tests after changes
✅ **Do**: Always run `make -f Makefile.test test`

---

## Need Help?

See detailed guides:
- **SYNC_GUIDE.md** - Complete synchronization guide
- **HOW_TO_SYNC.md** - Manual sync process details
- **UPDATE_SIMULATION_GUIDE.md** - Simulation update guide
- **TEST_DOCUMENTATION.md** - Test suite documentation

---

## Example Workflow

1. **Make changes** to `three_port_switch.c`
2. **Identify type** using Step 1 above
3. **Update files** from Step 2 checklist
4. **Build all** using Step 3 commands
5. **Verify tests** using Step 4
6. **Update docs** using Step 5
7. **Commit** with clear message

Done! ✓

