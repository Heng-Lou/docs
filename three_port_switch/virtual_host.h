/*
 * Virtual Host Infrastructure
 * 
 * Simulates host systems that connect to switch PCI ports.
 * Provides packet generation, reception, and basic networking stack.
 */

#ifndef VIRTUAL_HOST_H
#define VIRTUAL_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "virtual_link.h"

#define MAX_VHOSTS 64
#define VHOST_MAC_LEN 6
#define VHOST_IP_LEN 4

/* Virtual host statistics */
typedef struct {
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t tx_errors;
    uint64_t rx_errors;
    uint64_t rx_drops;
} vhost_stats_t;

/* Virtual host configuration */
typedef struct {
    char name[64];
    uint8_t mac_addr[VHOST_MAC_LEN];
    uint8_t ip_addr[VHOST_IP_LEN];
    uint16_t mtu;
    bool enabled;
} vhost_config_t;

/* Packet generator configuration */
typedef struct {
    bool enabled;
    uint32_t pkt_size;
    uint32_t pps;           /* Packets per second */
    uint32_t count;         /* Number of packets to send (0 = infinite) */
    uint8_t dst_mac[VHOST_MAC_LEN];
    uint8_t dst_ip[VHOST_IP_LEN];
    uint16_t dst_port;
} vhost_pktgen_config_t;

/* Virtual host instance */
typedef struct {
    uint32_t host_id;
    vhost_config_t config;
    vhost_stats_t stats;
    
    /* Connection to switch PCI port */
    uint32_t pci_link_id;
    vlink_manager_t *link_mgr;
    
    /* Packet generator */
    vhost_pktgen_config_t pktgen;
    pthread_t pktgen_thread;
    
    /* Receive handler */
    pthread_t rx_thread;
    
    /* Control */
    bool running;
    pthread_mutex_t lock;
    
    /* Custom packet handler callback */
    void (*pkt_handler)(void *ctx, const uint8_t *data, uint16_t size);
    void *pkt_handler_ctx;
} vhost_instance_t;

/* Virtual host manager */
typedef struct {
    vhost_instance_t hosts[MAX_VHOSTS];
    uint32_t num_hosts;
    vlink_manager_t *link_mgr;
    pthread_mutex_t mgr_lock;
} vhost_manager_t;

/*
 * Initialize virtual host manager
 */
int vhost_manager_init(vhost_manager_t *mgr, vlink_manager_t *link_mgr);

/*
 * Cleanup virtual host manager
 */
void vhost_manager_cleanup(vhost_manager_t *mgr);

/*
 * Create a virtual host
 */
int vhost_create(vhost_manager_t *mgr, const char *name,
                 const uint8_t *mac_addr, const uint8_t *ip_addr,
                 uint32_t *host_id);

/*
 * Connect virtual host to a switch PCI link
 */
int vhost_connect_to_switch(vhost_manager_t *mgr, uint32_t host_id,
                            uint32_t switch_pci_link_id);

/*
 * Start virtual host
 */
int vhost_start(vhost_manager_t *mgr, uint32_t host_id);

/*
 * Stop virtual host
 */
int vhost_stop(vhost_manager_t *mgr, uint32_t host_id);

/*
 * Send packet from virtual host
 */
int vhost_send_packet(vhost_manager_t *mgr, uint32_t host_id,
                      const uint8_t *data, uint16_t size);

/*
 * Configure packet generator
 */
int vhost_configure_pktgen(vhost_manager_t *mgr, uint32_t host_id,
                           const vhost_pktgen_config_t *config);

/*
 * Start packet generator
 */
int vhost_start_pktgen(vhost_manager_t *mgr, uint32_t host_id);

/*
 * Stop packet generator
 */
int vhost_stop_pktgen(vhost_manager_t *mgr, uint32_t host_id);

/*
 * Set custom packet handler callback
 */
int vhost_set_packet_handler(vhost_manager_t *mgr, uint32_t host_id,
                             void (*handler)(void *ctx, const uint8_t *data, uint16_t size),
                             void *ctx);

/*
 * Get host statistics
 */
int vhost_get_stats(vhost_manager_t *mgr, uint32_t host_id, vhost_stats_t *stats);

/*
 * Reset host statistics
 */
int vhost_reset_stats(vhost_manager_t *mgr, uint32_t host_id);

/*
 * Get host configuration
 */
int vhost_get_config(vhost_manager_t *mgr, uint32_t host_id, vhost_config_t *config);

/*
 * Print all host statistics
 */
void vhost_print_stats(vhost_manager_t *mgr);

/*
 * Helper: Generate Ethernet frame
 */
uint16_t vhost_build_eth_frame(uint8_t *frame, uint16_t max_size,
                               const uint8_t *dst_mac, const uint8_t *src_mac,
                               uint16_t eth_type, const uint8_t *payload, uint16_t payload_len);

/*
 * Helper: Generate UDP/IP packet
 */
uint16_t vhost_build_udp_packet(uint8_t *packet, uint16_t max_size,
                                const uint8_t *dst_mac, const uint8_t *src_mac,
                                const uint8_t *dst_ip, const uint8_t *src_ip,
                                uint16_t dst_port, uint16_t src_port,
                                const uint8_t *payload, uint16_t payload_len);

#endif /* VIRTUAL_HOST_H */
