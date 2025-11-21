/*
 * Virtual Link Infrastructure for Switch Simulation
 * 
 * Provides virtual network links to connect multiple switch instances
 * together for testing multi-switch topologies without hardware.
 */

#ifndef VIRTUAL_LINK_H
#define VIRTUAL_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_VLINKS 32
#define MAX_PACKET_SIZE 9000
#define VLINK_QUEUE_SIZE 16384  /* High-rate testing: <5000 pkts/host */

/* Virtual link statistics */
typedef struct {
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t drops;
    uint64_t errors;
} vlink_stats_t;

/* Packet in virtual link queue */
typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t size;
    uint64_t timestamp;
    uint32_t seq_num;
} vlink_packet_t;

/* Virtual link queue (ring buffer) */
typedef struct {
    vlink_packet_t packets[VLINK_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} vlink_queue_t;

/* Virtual link configuration */
typedef struct {
    char name[64];
    uint32_t bandwidth_mbps;  /* Simulated bandwidth */
    uint32_t latency_us;      /* Base simulated latency */
    uint32_t jitter_us;       /* Latency jitter (+/- random variation) */
    uint32_t delay_us;        /* Additional fixed delay */
    float loss_rate;          /* Packet loss probability (0.0-1.0) */
    bool enabled;
} vlink_config_t;

/* Virtual link endpoint */
typedef struct {
    uint32_t link_id;
    uint32_t peer_id;         /* Connected peer link ID (or UINT32_MAX if not connected) */
    vlink_config_t config;
    vlink_queue_t tx_queue;
    vlink_queue_t rx_queue;
    vlink_stats_t stats;
    pthread_t rx_thread;
    bool running;
    
    /* Callback for received packets */
    void (*rx_callback)(void *ctx, const uint8_t *data, uint16_t size);
    void *rx_callback_ctx;
} vlink_endpoint_t;

/* Virtual link manager */
typedef struct {
    vlink_endpoint_t links[MAX_VLINKS];
    uint32_t num_links;
    pthread_mutex_t mgr_lock;
} vlink_manager_t;

/*
 * Initialize virtual link manager
 */
int vlink_manager_init(vlink_manager_t *mgr);

/*
 * Cleanup virtual link manager
 */
void vlink_manager_cleanup(vlink_manager_t *mgr);

/*
 * Create a virtual link with jitter and delay support
 */
int vlink_create(vlink_manager_t *mgr, const char *name, 
                 uint32_t bandwidth_mbps, uint32_t latency_us,
                 float loss_rate, uint32_t *link_id);

/*
 * Create a virtual link with full configuration including jitter and delay
 */
int vlink_create_ex(vlink_manager_t *mgr, const char *name, 
                    uint32_t bandwidth_mbps, uint32_t latency_us,
                    uint32_t jitter_us, uint32_t delay_us,
                    float loss_rate, uint32_t *link_id);

/*
 * Connect two virtual links (bidirectional)
 */
int vlink_connect(vlink_manager_t *mgr, uint32_t link_id1, uint32_t link_id2);

/*
 * Set RX callback for link
 */
int vlink_set_rx_callback(vlink_manager_t *mgr, uint32_t link_id,
                          void (*callback)(void *ctx, const uint8_t *data, uint16_t size),
                          void *ctx);

/*
 * Send packet on virtual link
 */
int vlink_send(vlink_manager_t *mgr, uint32_t link_id, 
               const uint8_t *data, uint16_t size);

/*
 * Receive packet from virtual link (polling mode)
 */
int vlink_recv(vlink_manager_t *mgr, uint32_t link_id,
               uint8_t *data, uint16_t *size, uint16_t max_size);

/*
 * Start virtual link (enables RX thread if callback is set)
 */
int vlink_start(vlink_manager_t *mgr, uint32_t link_id);

/*
 * Stop virtual link
 */
int vlink_stop(vlink_manager_t *mgr, uint32_t link_id);

/*
 * Get link statistics
 */
int vlink_get_stats(vlink_manager_t *mgr, uint32_t link_id, vlink_stats_t *stats);

/*
 * Reset link statistics
 */
int vlink_reset_stats(vlink_manager_t *mgr, uint32_t link_id);

/*
 * Get link configuration
 */
int vlink_get_config(vlink_manager_t *mgr, uint32_t link_id, vlink_config_t *config);

/*
 * Update link configuration
 */
int vlink_set_config(vlink_manager_t *mgr, uint32_t link_id, const vlink_config_t *config);

/*
 * Print all link statistics
 */
void vlink_print_stats(vlink_manager_t *mgr);

#endif /* VIRTUAL_LINK_H */
