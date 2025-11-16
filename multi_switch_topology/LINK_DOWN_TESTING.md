# Link Down Testing for Ring Topology

## Overview

The ring topology has unique resilience characteristics that need specific testing. When links fail in a ring, the topology degrades gracefully but TTL/hop limit protection becomes critical to prevent infinite loops.

## Test Coverage

### `test_link_down.sh`

Tests switch behavior when network links fail in a ring topology:

1. **Single Link Down**: Kills one switch, verifies remaining switches continue operating
2. **Multiple Link Failures**: Tests cascading failures
3. **Survival Test**: Confirms remaining switches stay alive
4. **TTL Loop Prevention**: Verifies TTL mechanism prevents infinite packet loops

## Running Tests

```bash
cd multi_switch_topology
./test_link_down.sh
```

## Ring Topology Failure Modes

### Normal Operation
```
Switch 0 <-> Switch 1 <-> Switch 2 <-> Switch 3 <-> Switch 0
  TTL=3      TTL=2        TTL=1        TTL=0
```

### Single Link Failure
```
Switch 0 <-> Switch 2 <-> Switch 3 <-> Switch 0
          (Switch 1 down)
```
- Ring is broken at one point
- Other switches continue operating
- TTL prevents packets from looping indefinitely

### Multiple Link Failures
```
Switch 0    Switch 2
          (Switches 1, 3 down)
```
- Switches become isolated
- No loops possible
- Each switch operates independently

## Why TTL is Critical in Ring Topology

Without TTL/hop limit:
- A packet entering the ring could loop forever
- Each switch would forward to the next, creating an infinite cycle
- Network would be saturated with looping packets

With TTL (as implemented):
- Each hop decrements TTL
- When TTL reaches 0, packet is dropped
- Maximum loop count = initial TTL value
- Prevents network saturation

## Test Results Interpretation

**All Tests Pass**: 
- Switches survive link failures ✓
- No crashes from broken links ✓
- TTL protection working ✓
- Graceful degradation ✓

## Integration with Other Tests

This complements:
- `test_mock_infrastructure.sh` - Process management
- TTL implementation in `three_port_switch.c`
- Ring topology configuration in deployment scripts

## Real-World Scenarios

This test simulates:
- Cable disconnection
- Switch hardware failure
- Intermittent link issues
- Network maintenance (taking switches offline)

## Expected Behavior

1. Remaining switches continue operating independently
2. No cascading failures
3. No memory leaks or resource issues
4. Clean process state after failures
5. TTL prevents infinite loops even with broken ring
