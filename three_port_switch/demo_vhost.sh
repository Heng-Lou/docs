#!/bin/bash
#
# Quick Start Script for Virtual Host Testing
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ $1${NC}"
}

# Build if needed
if [ ! -f vhost_switch_test ]; then
    print_info "Building virtual host test program..."
    make -f Makefile.vhost clean
    make -f Makefile.vhost
    print_success "Build complete"
    echo ""
fi

# Parse arguments
DEMO_TYPE=${1:-basic}

case "$DEMO_TYPE" in
    basic)
        print_header "Basic Virtual Host Demo"
        echo "Configuration:"
        echo "  - 4 virtual hosts"
        echo "  - 4 switches in ring topology"
        echo "  - No packet generation"
        echo "  - 10 second duration"
        echo ""
        ./vhost_switch_test -n 4 -d 10
        ;;
    
    pktgen)
        print_header "Packet Generation Demo"
        echo "Configuration:"
        echo "  - 4 virtual hosts"
        echo "  - 4 switches in ring topology"
        echo "  - Packet generation: 100 pps"
        echo "  - 100 packets per host"
        echo "  - 15 second duration"
        echo ""
        ./vhost_switch_test -n 4 -p -r 100 -c 100 -d 15
        ;;
    
    ring)
        print_header "Ring Topology Demo"
        echo "Configuration:"
        echo "  - 8 virtual hosts"
        echo "  - 8 switches in ring topology"
        echo "  - Packet generation: 50 pps"
        echo "  - 200 packets per host"
        echo "  - 20 second duration"
        echo ""
        ./vhost_switch_test -n 8 -p -r 50 -c 200 -d 20
        ;;
    
    stress)
        print_header "Stress Test Demo"
        echo "Configuration:"
        echo "  - 8 virtual hosts"
        echo "  - 8 switches in ring topology"
        echo "  - Packet generation: 1000 pps (high rate)"
        echo "  - 5000 packets per host"
        echo "  - 30 second duration"
        echo ""
        print_info "This test generates high packet rates"
        ./vhost_switch_test -n 8 -p -r 1000 -c 5000 -d 30
        ;;
    
    continuous)
        print_header "Continuous Traffic Demo"
        echo "Configuration:"
        echo "  - 4 virtual hosts"
        echo "  - 4 switches in ring topology"
        echo "  - Packet generation: 100 pps"
        echo "  - Continuous (infinite packets)"
        echo "  - 60 second duration"
        echo ""
        print_info "Press Ctrl+C to stop early"
        ./vhost_switch_test -n 4 -p -r 100 -c 0 -d 60
        ;;
    
    custom)
        print_header "Custom Configuration"
        echo "Usage: $0 custom <num_hosts> <pps> <packet_count> <duration>"
        echo ""
        
        NUM_HOSTS=${2:-4}
        PPS=${3:-100}
        PKT_COUNT=${4:-100}
        DURATION=${5:-15}
        
        echo "Configuration:"
        echo "  - Hosts: $NUM_HOSTS"
        echo "  - Packet rate: $PPS pps"
        echo "  - Packet count: $PKT_COUNT"
        echo "  - Duration: $DURATION seconds"
        echo ""
        
        ./vhost_switch_test -n $NUM_HOSTS -p -r $PPS -c $PKT_COUNT -d $DURATION
        ;;
    
    help|--help|-h)
        print_header "Virtual Host Demo Script"
        echo ""
        echo "Usage: $0 [demo_type]"
        echo ""
        echo "Demo Types:"
        echo "  basic       - Basic demo with 4 hosts/switches, no traffic"
        echo "  pktgen      - Packet generation with 4 hosts (default)"
        echo "  ring        - Ring topology with 8 hosts/switches"
        echo "  stress      - Stress test with high packet rates"
        echo "  continuous  - Continuous traffic generation"
        echo "  custom      - Custom configuration (specify parameters)"
        echo "  help        - Show this help"
        echo ""
        echo "Examples:"
        echo "  $0 basic"
        echo "  $0 pktgen"
        echo "  $0 ring"
        echo "  $0 custom 6 200 500 30"
        echo ""
        echo "Custom Parameters:"
        echo "  $0 custom <num_hosts> <pps> <packet_count> <duration>"
        ;;
    
    *)
        print_error "Unknown demo type: $DEMO_TYPE"
        echo "Run '$0 help' for usage information"
        exit 1
        ;;
esac
