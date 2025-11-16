/*
 * Unit tests for Virtual Link infrastructure
 */

#include "virtual_link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

/* Helper for timing */
static uint64_t get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/* Test packet data */
static uint8_t test_data[] = "Hello Virtual Link!";
static volatile int callback_count = 0;

/* RX callback for testing */
static void test_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    (void)ctx;
    printf("  RX Callback: Received %u bytes: %s\n", size, data);
    callback_count++;
}

/* Test 1: Basic manager initialization */
static void test_manager_init(void)
{
    printf("\nTest 1: Manager Initialization\n");
    printf("--------------------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    
    assert(vlink_manager_init(mgr) == 0);
    assert(mgr->num_links == 0);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 2: Create virtual links */
static void test_link_creation(void)
{
    printf("\nTest 2: Link Creation\n");
    printf("---------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1, link2;
    
    assert(vlink_manager_init(mgr) == 0);
    
    assert(vlink_create(mgr, "test_link1", 1000, 10, 0.0, &link1) == 0);
    assert(vlink_create(mgr, "test_link2", 10000, 1, 0.01, &link2) == 0);
    assert(mgr->num_links == 2);
    
    vlink_config_t config;
    assert(vlink_get_config(mgr, link1, &config) == 0);
    assert(strcmp(config.name, "test_link1") == 0);
    assert(config.bandwidth_mbps == 1000);
    assert(config.latency_us == 10);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 3: Send and receive */
static void test_send_recv(void)
{
    printf("\nTest 3: Send and Receive\n");
    printf("------------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1, link2;
    
    assert(vlink_manager_init(mgr) == 0);
    assert(vlink_create(mgr, "link1", 1000, 0, 0.0, &link1) == 0);
    assert(vlink_create(mgr, "link2", 1000, 0, 0.0, &link2) == 0);
    assert(vlink_connect(mgr, link1, link2) == 0);
    
    /* Send on link1 */
    assert(vlink_send(mgr, link1, test_data, sizeof(test_data)) == 0);
    
    /* Receive on link2 */
    uint8_t recv_buf[256];
    uint16_t recv_size;
    
    usleep(10000);  /* Give time for delivery */
    
    assert(vlink_recv(mgr, link2, recv_buf, &recv_size, sizeof(recv_buf)) == 0);
    assert(recv_size == sizeof(test_data));
    assert(memcmp(recv_buf, test_data, sizeof(test_data)) == 0);
    
    printf("  Sent: %s\n", test_data);
    printf("  Received: %s\n", recv_buf);
    
    vlink_stats_t stats;
    assert(vlink_get_stats(mgr, link1, &stats) == 0);
    assert(stats.tx_packets == 1);
    assert(stats.tx_bytes == sizeof(test_data));
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 4: Callback mode */
static void test_callback_mode(void)
{
    printf("\nTest 4: Callback Mode\n");
    printf("---------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1, link2;
    
    callback_count = 0;
    
    assert(vlink_manager_init(mgr) == 0);
    assert(vlink_create(mgr, "link1", 1000, 0, 0.0, &link1) == 0);
    assert(vlink_create(mgr, "link2", 1000, 0, 0.0, &link2) == 0);
    assert(vlink_connect(mgr, link1, link2) == 0);
    
    /* Set callback and start link2 */
    assert(vlink_set_rx_callback(mgr, link2, test_rx_callback, NULL) == 0);
    assert(vlink_start(mgr, link2) == 0);
    
    /* Send some packets */
    for (int i = 0; i < 5; i++) {
        assert(vlink_send(mgr, link1, test_data, sizeof(test_data)) == 0);
        usleep(50000);  /* 50ms between sends */
    }
    
    /* Wait for callbacks */
    sleep(1);
    
    assert(callback_count == 5);
    printf("  Received %d callbacks\n", callback_count);
    
    vlink_stop(mgr, link2);
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 5: Statistics */
static void test_statistics(void)
{
    printf("\nTest 5: Statistics\n");
    printf("------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1, link2;
    
    assert(vlink_manager_init(mgr) == 0);
    assert(vlink_create(mgr, "link1", 1000, 0, 0.0, &link1) == 0);
    assert(vlink_create(mgr, "link2", 1000, 0, 0.0, &link2) == 0);
    assert(vlink_connect(mgr, link1, link2) == 0);
    
    /* Send multiple packets */
    for (int i = 0; i < 10; i++) {
        vlink_send(mgr, link1, test_data, sizeof(test_data));
    }
    
    vlink_stats_t stats;
    assert(vlink_get_stats(mgr, link1, &stats) == 0);
    printf("  TX packets: %lu\n", stats.tx_packets);
    printf("  TX bytes: %lu\n", stats.tx_bytes);
    assert(stats.tx_packets == 10);
    assert(stats.tx_bytes == sizeof(test_data) * 10);
    
    /* Reset and verify */
    assert(vlink_reset_stats(mgr, link1) == 0);
    assert(vlink_get_stats(mgr, link1, &stats) == 0);
    assert(stats.tx_packets == 0);
    assert(stats.tx_bytes == 0);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 6: Packet loss simulation */
static void test_packet_loss(void)
{
    printf("\nTest 6: Packet Loss Simulation\n");
    printf("-------------------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1;
    
    assert(vlink_manager_init(mgr) == 0);
    
    /* Create link with 50% packet loss */
    assert(vlink_create(mgr, "lossy_link", 1000, 0, 0.5, &link1) == 0);
    
    /* Send many packets */
    int sent = 100;
    for (int i = 0; i < sent; i++) {
        vlink_send(mgr, link1, test_data, sizeof(test_data));
    }
    
    vlink_stats_t stats;
    assert(vlink_get_stats(mgr, link1, &stats) == 0);
    
    printf("  Sent: %d packets\n", sent);
    printf("  Delivered: %lu packets\n", stats.tx_packets);
    printf("  Dropped: %lu packets\n", stats.drops);
    printf("  Loss rate: %.1f%%\n", (float)stats.drops / sent * 100);
    
    /* Should have some drops (not exact due to randomness) */
    assert(stats.drops > 0);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

/* Test 7: Latency simulation */
static void test_latency(void)
{
    printf("\nTest 7: Latency Simulation\n");
    printf("--------------------------\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    assert(mgr != NULL);
    uint32_t link1;
    
    assert(vlink_manager_init(mgr) == 0);
    
    /* Create link with 100ms latency */
    assert(vlink_create(mgr, "slow_link", 1000, 100000, 0.0, &link1) == 0);
    
    uint64_t start = get_time_us();
    vlink_send(mgr, link1, test_data, sizeof(test_data));
    uint64_t end = get_time_us();
    
    uint64_t elapsed = end - start;
    printf("  Configured latency: 100 ms\n");
    printf("  Measured time: %.1f ms\n", elapsed / 1000.0);
    
    /* Should take at least 100ms */
    assert(elapsed >= 100000);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("✓ Test passed\n");
}

int main(void)
{
    printf("========================================\n");
    printf("Virtual Link Unit Tests\n");
    printf("========================================\n");
    
    test_manager_init();
    test_link_creation();
    test_send_recv();
    test_callback_mode();
    test_statistics();
    test_packet_loss();
    test_latency();
    
    printf("\n========================================\n");
    printf("All Tests Passed! ✓\n");
    printf("========================================\n");
    
    return 0;
}
