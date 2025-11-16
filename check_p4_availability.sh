#!/bin/bash
echo "=== Checking for P4 Compiler Availability ==="
echo ""

echo "1. Searching for P4 compilers..."
for compiler in nvp4c p4c p4c-dpdk; do
    if which $compiler 2>/dev/null; then
        echo "  ✓ Found: $compiler"
        $compiler --version 2>&1 | head -5
    else
        echo "  ✗ Not found: $compiler"
    fi
done

echo ""
echo "2. Checking installed packages..."
dpkg -l 2>/dev/null | grep -iE "p4.*compiler|p4.*devel|nvp4" | head -10

echo ""
echo "3. Checking DOCA documentation..."
find /opt/mellanox/doca -name "*.md" -o -name "README*" 2>/dev/null | xargs grep -l "P4\|p4c" 2>/dev/null | head -5

echo ""
echo "4. Available DOCA packages for P4..."
apt-cache search doca 2>/dev/null | grep -i p4

echo ""
echo "5. Checking online for NVIDIA P4 support..."
echo "   NVIDIA BlueField P4 is typically available as:"
echo "   - Part of NVIDIA DOCA SDK (may need separate download)"
echo "   - NVIDIA P4 Studio (separate product)"
echo "   - Open source p4c with NVIDIA backend"
