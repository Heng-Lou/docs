/*
 * Test program for virtual link jitter and delay
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "virtual_link.h"

#define NUM_PACKETS 20

/* Get current time in microseconds */
static uint64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

/* Statistics for delay measurements */
typedef struct {
    uint64_t min_delay;
    uint64_t max_delay;
    uint64_t total_delay;
    uint32_t count;
} delay_stats_t;

static void update_delay_stats(delay_stats_t *stats, uint64_t delay)
{
    if (stats->count == 0 || delay < stats->min_delay) {
        stats->min_delay = delay;
    }
    if (delay > stats->max_delay) {
        stats->max_delay = delay;
    }
    stats->total_delay += delay;
    stats->count++;
}

static void print_delay_stats(const char *name, delay_stats_t *stats)
{
    if (stats->count == 0) {
        printf("%s: No packets\n", name);
        return;
    }
    
    uint64_t avg_delay = stats->total_delay / stats->count;
    printf("%s:\n", name);
    printf("  Packets:    %u\n", stats->count);
    printf("  Min delay:  %lu us\n", stats->min_delay);
    printf("  Max delay:  %lu us\n", stats->max_delay);
    printf("  Avg delay:  %lu us\n", avg_delay);
    printf("  Jitter:     %lu us (max-min)\n", stats->max_delay - stats->min_delay);
}

/* Test scenario */
typedef struct {
    const char *name;
    uint32_t latency_us;
    uint32_t jitter_us;
    uint32_t delay_us;
    float loss_rate;
} test_scenario_t;

static void run_test(vlink_manager_t *mgr, test_scenario_t *scenario)
{
    printf("\n==========================================\n");
    printf("Test: %s\n", scenario->name);
    printf("==========================================\n");
    printf("Config:\n");
    printf("  Base latency: %u us\n", scenario->latency_us);
    printf("  Jitter:       %u us\n", scenario->jitter_us);
    printf("  Extra delay:  %u us\n", scenario->delay_us);
    printf("  Loss rate:    %.1f%%\n", scenario->loss_rate * 100);
    printf("\n");
    
    /* Create link pair */
    uint32_t link_id1, link_id2;
    int ret = vlink_create_ex(mgr, "link1", 1000, 
                              scenario->latency_us, scenario->jitter_us, 
                              scenario->delay_us, scenario->loss_rate, &link_id1);
    if (ret != 0) {
        printf("Failed to create link1\n");
        return;
    }
    
    ret = vlink_create_ex(mgr, "link2", 1000, 
                          scenario->latency_us, scenario->jitter_us,
                          scenario->delay_us, scenario->loss_rate, &link_id2);
    if (ret != 0) {
        printf("Failed to create link2\n");
        return;
    }
    
    vlink_connect(mgr, link_id1, link_id2);
    vlink_start(mgr, link_id1);
    vlink_start(mgr, link_id2);
    
    /* Send test packets and measure delays */
    delay_stats_t stats = {0};
    uint8_t tx_data[64];
    uint8_t rx_data[64];
    uint16_t rx_size;
    
    printf("Sending %d packets...\n\n", NUM_PACKETS);
    
    for (int i = 0; i < NUM_PACKETS; i++) {
        sprintf((char*)tx_data, "Packet %d", i);
        
        uint64_t send_time = get_time_us();
        ret = vlink_send(mgr, link_id1, tx_data, strlen((char*)tx_data) + 1);
        uint64_t recv_time = get_time_us();
        
        if (ret == 0) {
            /* Try to receive (with timeout) */
            ret = vlink_recv(mgr, link_id2, rx_data, &rx_size, sizeof(rx_data));
            if (ret == 0) {
                uint64_t delay = recv_time - send_time;
                update_delay_stats(&stats, delay);
                printf("Packet %2d: delay = %6lu us\n", i, delay);
            } else {
                printf("Packet %2d: dropped/timeout\n", i);
            }
        } else {
            printf("Packet %2d: send failed\n", i);
        }
    }
    
    printf("\n");
    print_delay_stats("Results", &stats);
    
    /* Print link statistics */
    printf("\n");
    vlink_stats_t link_stats;
    vlink_get_stats(mgr, link_id1, &link_stats);
    printf("Link stats:\n");
    printf("  TX packets: %lu\n", link_stats.tx_packets);
    printf("  RX packets: %lu\n", link_stats.rx_packets);
    printf("  Drops:      %lu\n", link_stats.drops);
    
    vlink_stop(mgr, link_id1);
    vlink_stop(mgr, link_id2);
}

int main(void)
{
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    if (!mgr) {
        printf("Failed to allocate manager\n");
        return 1;
    }
    
    printf("==========================================\n");
    printf("Virtual Link Jitter and Delay Test\n");
    printf("==========================================\n");
    
    if (vlink_manager_init(mgr) != 0) {
        printf("Failed to initialize vlink manager\n");
        free(mgr);
        return 1;
    }
    
    /* Test scenarios */
    test_scenario_t scenarios[] = {
        {
            .name = "Low latency, no jitter",
            .latency_us = 100,
            .jitter_us = 0,
            .delay_us = 0,
            .loss_rate = 0.0,
        },
        {
            .name = "Low latency with jitter",
            .latency_us = 100,
            .jitter_us = 50,
            .delay_us = 0,
            .loss_rate = 0.0,
        },
        {
            .name = "High latency with high jitter",
            .latency_us = 500,
            .jitter_us = 200,
            .delay_us = 0,
            .loss_rate = 0.0,
        },
        {
            .name = "With additional delay",
            .latency_us = 100,
            .jitter_us = 50,
            .delay_us = 300,
            .loss_rate = 0.0,
        },
        {
            .name = "With packet loss",
            .latency_us = 100,
            .jitter_us = 50,
            .delay_us = 0,
            .loss_rate = 0.2,  /* 20% loss */
        },
        {
            .name = "Real-world WAN simulation",
            .latency_us = 5000,  /* 5ms base latency */
            .jitter_us = 2000,   /* +/- 2ms jitter */
            .delay_us = 1000,    /* 1ms extra delay */
            .loss_rate = 0.01,   /* 1% loss */
        },
    };
    
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++) {
        run_test(mgr, &scenarios[i]);
    }
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("\n==========================================\n");
    printf("All tests completed\n");
    printf("==========================================\n");
    
    return 0;
}
