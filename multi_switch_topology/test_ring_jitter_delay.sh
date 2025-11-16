#!/bin/bash

# Test jitter and delay in ring topology
# This script creates a ring of switches with configurable latency and jitter

set -e

NUM_SWITCHES=${1:-4}
TOPOLOGY="ring"
BASE_LATENCY=${2:-100}    # Base latency in microseconds
JITTER=${3:-50}           # Jitter in microseconds
EXTRA_DELAY=${4:-0}       # Extra delay in microseconds
LOSS_RATE=${5:-0.0}       # Packet loss rate (0.0-1.0)

echo "=========================================="
echo "Ring Topology Jitter and Delay Test"
echo "=========================================="
echo "Switches:      $NUM_SWITCHES"
echo "Base latency:  $BASE_LATENCY us"
echo "Jitter:        ±$JITTER us"
echo "Extra delay:   $EXTRA_DELAY us"
echo "Loss rate:     $LOSS_RATE"
echo ""

# Create working directory
WORK_DIR="./ring_test_$$"
mkdir -p "$WORK_DIR"

# Create test configuration
cat > "$WORK_DIR/ring_config.txt" <<EOF
# Ring topology configuration
num_switches=$NUM_SWITCHES
topology=$TOPOLOGY
base_latency_us=$BASE_LATENCY
jitter_us=$JITTER
extra_delay_us=$EXTRA_DELAY
loss_rate=$LOSS_RATE
EOF

echo "Configuration saved to $WORK_DIR/ring_config.txt"

# Generate ring test program
cat > "$WORK_DIR/ring_jitter_test.c" <<'EOFC'
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define MAX_SWITCHES 16
#define NUM_TEST_PACKETS 100

typedef struct {
    uint64_t min_delay;
    uint64_t max_delay;
    uint64_t total_delay;
    uint32_t count;
    uint64_t delays[NUM_TEST_PACKETS];
} hop_stats_t;

static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static void update_stats(hop_stats_t *stats, uint64_t delay) {
    if (stats->count == 0 || delay < stats->min_delay) {
        stats->min_delay = delay;
    }
    if (delay > stats->max_delay) {
        stats->max_delay = delay;
    }
    stats->total_delay += delay;
    if (stats->count < NUM_TEST_PACKETS) {
        stats->delays[stats->count] = delay;
    }
    stats->count++;
}

static void print_stats(const char *name, hop_stats_t *stats, int num_hops) {
    if (stats->count == 0) {
        printf("%s: No packets\n", name);
        return;
    }
    
    uint64_t avg_delay = stats->total_delay / stats->count;
    uint64_t jitter = stats->max_delay - stats->min_delay;
    
    printf("\n%s (%d hops):\n", name, num_hops);
    printf("  %-20s %u\n", "Packets:", stats->count);
    printf("  %-20s %lu us\n", "Min delay:", stats->min_delay);
    printf("  %-20s %lu us\n", "Max delay:", stats->max_delay);
    printf("  %-20s %lu us\n", "Avg delay:", avg_delay);
    printf("  %-20s %lu us\n", "Jitter (max-min):", jitter);
    printf("  %-20s %.1f us\n", "Delay per hop:", (float)avg_delay / num_hops);
    
    /* Calculate standard deviation */
    uint64_t variance = 0;
    for (uint32_t i = 0; i < stats->count && i < NUM_TEST_PACKETS; i++) {
        int64_t diff = (int64_t)stats->delays[i] - (int64_t)avg_delay;
        variance += diff * diff;
    }
    variance /= stats->count;
    uint64_t stddev = (uint64_t)sqrt((double)variance);
    printf("  %-20s %lu us\n", "Std deviation:", stddev);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <num_switches> <base_latency_us> <jitter_us> <extra_delay_us>\n", argv[0]);
        return 1;
    }
    
    int num_switches = atoi(argv[1]);
    int base_latency = atoi(argv[2]);
    int jitter = atoi(argv[3]);
    int extra_delay = atoi(argv[4]);
    
    printf("==========================================\n");
    printf("Ring Topology Delay Simulation\n");
    printf("==========================================\n");
    printf("Configuration:\n");
    printf("  Switches:       %d\n", num_switches);
    printf("  Base latency:   %d us\n", base_latency);
    printf("  Jitter:         ±%d us\n", jitter);
    printf("  Extra delay:    %d us\n", extra_delay);
    printf("\n");
    
    srand(time(NULL));
    
    /* Simulate different hop counts in ring */
    hop_stats_t stats_1hop = {0};
    hop_stats_t stats_half_ring = {0};
    hop_stats_t stats_full_ring = {0};
    
    int hops_half = num_switches / 2;
    int hops_full = num_switches;
    
    printf("Simulating packet transmission...\n");
    
    for (int i = 0; i < NUM_TEST_PACKETS; i++) {
        /* 1 hop delay */
        uint64_t start_time = get_time_us();
        usleep(base_latency + extra_delay + (rand() % (2 * jitter + 1)) - jitter);
        uint64_t end_time = get_time_us();
        update_stats(&stats_1hop, end_time - start_time);
        
        /* Half ring delay */
        start_time = get_time_us();
        for (int h = 0; h < hops_half; h++) {
            usleep(base_latency + extra_delay + (rand() % (2 * jitter + 1)) - jitter);
        }
        end_time = get_time_us();
        update_stats(&stats_half_ring, end_time - start_time);
        
        /* Full ring delay */
        start_time = get_time_us();
        for (int h = 0; h < hops_full; h++) {
            usleep(base_latency + extra_delay + (rand() % (2 * jitter + 1)) - jitter);
        }
        end_time = get_time_us();
        update_stats(&stats_full_ring, end_time - start_time);
    }
    
    /* Print results */
    printf("\n==========================================\n");
    printf("Results\n");
    printf("==========================================\n");
    
    print_stats("1 Hop (Adjacent switches)", &stats_1hop, 1);
    print_stats("Half Ring", &stats_half_ring, hops_half);
    print_stats("Full Ring", &stats_full_ring, hops_full);
    
    /* Print jitter distribution */
    printf("\n==========================================\n");
    printf("Jitter Analysis (1 hop)\n");
    printf("==========================================\n");
    printf("Expected range: %d - %d us\n", 
           base_latency + extra_delay - jitter,
           base_latency + extra_delay + jitter);
    printf("Observed range: %lu - %lu us\n", 
           stats_1hop.min_delay, stats_1hop.max_delay);
    
    if (stats_1hop.max_delay - stats_1hop.min_delay > 2 * jitter * 1.5) {
        printf("⚠ Warning: Observed jitter is higher than configured\n");
    } else {
        printf("✓ Jitter within expected range\n");
    }
    
    return 0;
}
EOFC

# Compile the test program
gcc -o "$WORK_DIR/ring_jitter_test" "$WORK_DIR/ring_jitter_test.c" -lm

echo ""
echo "Running delay simulation..."
echo ""

# Run the test
"$WORK_DIR/ring_jitter_test" $NUM_SWITCHES $BASE_LATENCY $JITTER $EXTRA_DELAY

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "This simulation demonstrates how latency and jitter"
echo "accumulate as packets traverse the ring topology."
echo ""
echo "Key observations:"
echo "- Delay increases linearly with number of hops"
echo "- Jitter accumulates (standard deviation increases)"
echo "- Full ring traversal has highest latency"
echo ""
echo "Test artifacts saved in: $WORK_DIR"
echo ""
echo "To run with different parameters:"
echo "  $0 <num_switches> <latency_us> <jitter_us> <delay_us> <loss_rate>"
echo ""
echo "Examples:"
echo "  $0 8 100 50 0 0.0      # 8 switches, 100us latency, 50us jitter"
echo "  $0 4 500 200 100 0.01  # 4 switches, high latency, 1% loss"
echo ""

# Cleanup
# rm -rf "$WORK_DIR"  # Uncomment to auto-cleanup
