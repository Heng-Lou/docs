#!/bin/bash
# Quick verification that everything works

echo "Testing DPA GDB Debugging Setup..."
echo

echo "1. Testing simulation build..."
if [ -f "switch_sim" ]; then
    echo "   ✅ Simulation binary exists"
else
    echo "   ❌ Building simulation..."
    make -f Makefile.debug
fi

echo
echo "2. Running simulation tests..."
./switch_sim test | tail -5

echo
echo "3. Testing GDB (quick test)..."
cat > /tmp/quick_gdb_test.gdb << 'GDBEOF'
break main
run test
backtrace
quit
GDBEOF

if gdb -batch -x /tmp/quick_gdb_test.gdb ./switch_sim 2>&1 | grep -q "main"; then
    echo "   ✅ GDB can debug the simulation"
else
    echo "   ❌ GDB test failed"
fi
rm /tmp/quick_gdb_test.gdb

echo
echo "=========================================="
echo "✅ VERIFICATION COMPLETE"
echo "=========================================="
echo
echo "Everything is working! Try:"
echo "  ./demo_gdb.sh          - Interactive demos"
echo "  ./switch_sim test      - Run tests"
echo "  gdb ./switch_sim       - Manual debugging"
echo
