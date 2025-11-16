#!/bin/bash
#
# Comprehensive test runner for Three-Port Switch
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Three-Port Switch Test Suite"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if test binary exists
if [ ! -f "test_three_port_switch" ]; then
    echo "Building test suite..."
    make -f Makefile.test clean
    make -f Makefile.test all
    echo ""
fi

# Run basic tests
echo "Running Basic Tests..."
echo "=========================================="
if ./test_three_port_switch; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    TEST_RESULT=0
else
    echo -e "${RED}✗ Some tests failed!${NC}"
    TEST_RESULT=1
fi

echo ""
echo "=========================================="
echo "Running Coverage Analysis..."
echo "=========================================="

# Build with coverage
make -f Makefile.test coverage

# Run tests with coverage
./test_three_port_switch > /dev/null

# Generate coverage report
if command -v gcov &> /dev/null; then
    gcov test_three_port_switch.c > /dev/null 2>&1
    
    echo ""
    echo "Coverage Summary:"
    echo "------------------"
    
    if [ -f "test_three_port_switch.c.gcov" ]; then
        # Extract coverage percentage
        LINES=$(grep -c ":" test_three_port_switch.c.gcov || true)
        EXECUTED=$(grep -c ":" test_three_port_switch.c.gcov | grep -v "####" | grep -v "-" || true)
        
        echo "Test file coverage:"
        grep "Lines executed" test_three_port_switch.c.gcov || echo "Coverage data available in test_three_port_switch.c.gcov"
    fi
else
    echo -e "${YELLOW}gcov not found. Install gcc to get coverage reports.${NC}"
fi

# Try to generate HTML report if lcov is available
if command -v lcov &> /dev/null; then
    echo ""
    echo "Generating HTML coverage report..."
    make -f Makefile.test html-coverage > /dev/null 2>&1 || true
    
    if [ -d "coverage_html" ]; then
        echo -e "${GREEN}✓ HTML coverage report generated in coverage_html/index.html${NC}"
    fi
else
    echo ""
    echo -e "${YELLOW}Note: Install lcov for HTML coverage reports:${NC}"
    echo "  sudo apt-get install lcov"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="

if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}Status: PASSED${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Review coverage report in coverage_html/index.html"
    echo "  2. Build the actual switch: make -f Makefile.debug"
    echo "  3. Run integration tests with DevEmu"
else
    echo -e "${RED}Status: FAILED${NC}"
    echo ""
    echo "Please review test output above for details."
fi

echo ""

exit $TEST_RESULT
