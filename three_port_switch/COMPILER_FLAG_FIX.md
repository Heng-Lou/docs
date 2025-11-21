# Large Queue Size Compiler Error Fix

## The Problem

When increasing `VLINK_QUEUE_SIZE` to 16384 or larger, you get a linker error:

```
vhost_switch_test.o: in function `create_switch':
/home/heng/workdir/doca/.../vhost_switch_test.c:200:(.text.startup+0x287): 
relocation truncated to fit: R_X86_64_PC32 against `.bss'
```

## What This Means

This is a **relocation error** caused by the `.bss` section (where global variables live) growing too large:

- **Queue size 1024**: ~9 MB per queue, ~300 MB total → Works fine
- **Queue size 16384**: ~144 MB per queue, ~4.6 GB total → **Exceeds 32-bit offset limit**

The default x86_64 code model uses 32-bit offsets for data, which limits the total size of global/static data to about 2 GB.

## The Solution

Add `-mcmodel=medium` compiler flag to allow larger data sections:

```bash
# Edit Makefile.vhost, line 3:
CFLAGS = -Wall -Wextra -O2 -g -pthread -mcmodel=medium
```

### What `-mcmodel=medium` Does

| Code Model | Data Size Limit | Performance | Use Case |
|------------|----------------|-------------|----------|
| `small` (default) | 2 GB | Fastest | Normal programs |
| `medium` | 2 GB code, unlimited data | Slightly slower | Large data arrays |
| `large` | Unlimited | Slowest | Very large programs |

For our use case (large queue arrays), `medium` is perfect.

## How to Fix

### Option 1: Already Fixed in Makefile.vhost

The Makefile has been updated with the flag. Just rebuild:

```bash
make -f Makefile.vhost clean all
```

### Option 2: Manual Fix

If you need to fix it yourself:

```bash
# Edit Makefile.vhost
vim Makefile.vhost

# Change line 3 from:
CFLAGS = -Wall -Wextra -O2 -g -pthread

# To:
CFLAGS = -Wall -Wextra -O2 -g -pthread -mcmodel=medium

# Rebuild
make -f Makefile.vhost clean all
```

### Option 3: Use fix_high_rate.sh

The script now automatically adds the flag when needed:

```bash
./fix_high_rate.sh
# Choose option 2 or 3 (large queue sizes)
# Script will add -mcmodel=medium and rebuild
```

## When You Need This Flag

You need `-mcmodel=medium` when:

| Queue Size | Total .bss Size | Need Flag? |
|------------|-----------------|------------|
| 1024       | ~300 MB         | No         |
| 4096       | ~1.2 GB         | No         |
| 8192       | ~2.4 GB         | **Yes**    |
| 16384      | ~4.8 GB         | **Yes**    |
| 32768      | ~9.6 GB         | **Yes**    |

**Rule of thumb:** Queue size > 4096 requires `-mcmodel=medium`

## Performance Impact

The `-mcmodel=medium` flag has **minimal performance impact**:

- Code section uses same addressing (fast)
- Only data section uses 64-bit addresses
- Slight increase in code size (~1-2%)
- Negligible runtime overhead for our use case

**For network simulation, this is completely acceptable.**

## Verification

After rebuilding with the flag, check that it worked:

```bash
# Should compile without errors
make -f Makefile.vhost clean all

# Run your test
./vhost_switch_test -n 4 -p -r 500 -c 5000 -d 10

# Expected output:
# ✓ Each host sends all 5000 packets
# ✓ No errors: 0
# ✓ No -28 errors
```

## Alternative Solutions (Not Recommended)

### 1. Dynamic Allocation (Complex)
Instead of static arrays, use malloc():
- More complex code
- Requires careful memory management
- Not worth the effort for simulation

### 2. Reduce Queue Size (Defeats Purpose)
Keep queue at 4096 or lower:
- Avoids compiler flag
- But then can't run high-packet tests
- Not a real solution

### 3. Use -mcmodel=large (Overkill)
Allows unlimited code and data:
- Slower performance
- Unnecessary for our needs
- `medium` is sufficient

## Summary

**Problem:** Large queue sizes cause "relocation truncated" linker error

**Cause:** Default code model limits static data to ~2 GB

**Solution:** Add `-mcmodel=medium` to CFLAGS

**Already fixed in:** Makefile.vhost (line 3)

**Just rebuild:** `make -f Makefile.vhost clean all`

**Performance:** Negligible impact, totally acceptable

## Test Results

After fix with queue size 16384:

```
Host 0:
  TX: 5000 pkts / 265000 bytes (errors: 0)  ✓ All packets sent!
  RX: 5001 pkts / 265042 bytes (errors: 0, drops: 0)

Host 1:
  TX: 5000 pkts / 265000 bytes (errors: 0)  ✓ All packets sent!
  RX: 5001 pkts / 265042 bytes (errors: 0, drops: 0)
  
[... same for all hosts ...]
```

**Perfect! No more -28 errors!**

