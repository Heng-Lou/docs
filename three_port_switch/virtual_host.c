/*
 * Virtual Host Implementation
 */

#include "virtual_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

/* RX callback from virtual link */
static void vhost_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    vhost_instance_t *host = (vhost_instance_t *)ctx;
    
    pthread_mutex_lock(&host->lock);
    host->stats.rx_packets++;
    host->stats.rx_bytes += size;
    pthread_mutex_unlock(&host->lock);
    
    /* Call custom handler if set */
    if (host->pkt_handler) {
        host->pkt_handler(host->pkt_handler_ctx, data, size);
    }
}

/* Packet generator thread */
static void *pktgen_thread_func(void *arg)
{
    vhost_instance_t *host = (vhost_instance_t *)arg;
    uint8_t packet[9000];
    uint32_t sent = 0;
    uint64_t interval_ns = 1000000000ULL / host->pktgen.pps;
    struct timespec next_time, now;
    bool arp_sent = false;
    
    printf("[PKTGEN] Thread started for host %u (running=%d, enabled=%d, pps=%u)\n",
           host->host_id, host->running, host->pktgen.enabled, host->pktgen.pps);
    
    clock_gettime(CLOCK_MONOTONIC, &next_time);
    
    /* Send ARP request first to establish MAC-IP mapping */
    if (!arp_sent) {
        uint16_t arp_size = vhost_build_arp_request(
            packet, sizeof(packet),
            host->config.mac_addr,
            host->config.ip_addr,
            host->pktgen.dst_ip
        );
        
        if (arp_size > 0) {
            int arp_result = vlink_send(host->link_mgr, host->pci_link_id, packet, arp_size);
            if (arp_result == 0) {
                printf("[PKTGEN] Host %u: Sent ARP request for %u.%u.%u.%u\n",
                       host->host_id,
                       host->pktgen.dst_ip[0], host->pktgen.dst_ip[1],
                       host->pktgen.dst_ip[2], host->pktgen.dst_ip[3]);
                arp_sent = true;
                
                /* Wait a bit for ARP reply */
                usleep(100000);  /* 100ms */
            } else {
                printf("[PKTGEN] Host %u: Failed to send ARP request\n", host->host_id);
            }
        }
    }
    
    while (host->running && host->pktgen.enabled) {
        /* Build packet */
        uint16_t pkt_size = vhost_build_udp_packet(
            packet, sizeof(packet),
            host->pktgen.dst_mac, host->config.mac_addr,
            host->pktgen.dst_ip, host->config.ip_addr,
            host->pktgen.dst_port, 12345,
            (uint8_t *)"Test packet", 11
        );
        
        if (pkt_size == 0) {
            printf("[PKTGEN] Host %u: build_udp_packet failed\n", host->host_id);
            host->stats.tx_errors++;
            break;
        }
        
        /* Send packet */
        int send_result = vlink_send(host->link_mgr, host->pci_link_id, packet, pkt_size);
        if (send_result == 0) {
            pthread_mutex_lock(&host->lock);
            host->stats.tx_packets++;
            host->stats.tx_bytes += pkt_size;
            pthread_mutex_unlock(&host->lock);
            sent++;
            
            /* Check if we've sent enough */
            if (host->pktgen.count > 0 && sent >= host->pktgen.count) {
                printf("[PKTGEN] Host %u: reached count limit %u\n", host->host_id, sent);
                break;
            }
        } else {
            static int error_count[MAX_VHOSTS] = {0};
            if (error_count[host->host_id] < 5) {  /* Only log first 5 errors per host */
                printf("[PKTGEN] Host %u: vlink_send failed (result=%d, attempt %u)\n", 
                       host->host_id, send_result, sent + error_count[host->host_id] + 1);
                error_count[host->host_id]++;
            }
            pthread_mutex_lock(&host->lock);
            host->stats.tx_errors++;
            pthread_mutex_unlock(&host->lock);
        }
        
        /* Rate limiting */
        next_time.tv_nsec += interval_ns;
        if (next_time.tv_nsec >= 1000000000) {
            next_time.tv_sec++;
            next_time.tv_nsec -= 1000000000;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec < next_time.tv_sec ||
            (now.tv_sec == next_time.tv_sec && now.tv_nsec < next_time.tv_nsec)) {
            struct timespec sleep_time;
            sleep_time.tv_sec = next_time.tv_sec - now.tv_sec;
            sleep_time.tv_nsec = next_time.tv_nsec - now.tv_nsec;
            if (sleep_time.tv_nsec < 0) {
                sleep_time.tv_sec--;
                sleep_time.tv_nsec += 1000000000;
            }
            nanosleep(&sleep_time, NULL);
        }
    }
    
    if (!host->running) {
        printf("[PKTGEN] Host %u: host->running became false\n", host->host_id);
    }
    if (!host->pktgen.enabled) {
        printf("[PKTGEN] Host %u: pktgen.enabled became false\n", host->host_id);
    }
    
    printf("[PKTGEN] Thread ending for host %u (sent %u UDP packets)\n", host->host_id, sent);
    host->pktgen.enabled = false;
    return NULL;
}

/* Initialize virtual host manager */
int vhost_manager_init(vhost_manager_t *mgr, vlink_manager_t *link_mgr)
{
    if (!mgr || !link_mgr) {
        return -1;
    }
    
    memset(mgr, 0, sizeof(*mgr));
    mgr->link_mgr = link_mgr;
    pthread_mutex_init(&mgr->mgr_lock, NULL);
    
    return 0;
}

/* Cleanup virtual host manager */
void vhost_manager_cleanup(vhost_manager_t *mgr)
{
    if (!mgr) {
        return;
    }
    
    /* Stop all hosts */
    for (uint32_t i = 0; i < mgr->num_hosts; i++) {
        vhost_stop(mgr, i);
    }
    
    pthread_mutex_destroy(&mgr->mgr_lock);
}

/* Create a virtual host */
int vhost_create(vhost_manager_t *mgr, const char *name,
                 const uint8_t *mac_addr, const uint8_t *ip_addr,
                 uint32_t *host_id)
{
    if (!mgr || !name || !mac_addr || !ip_addr || !host_id) {
        return -1;
    }
    
    pthread_mutex_lock(&mgr->mgr_lock);
    
    if (mgr->num_hosts >= MAX_VHOSTS) {
        pthread_mutex_unlock(&mgr->mgr_lock);
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[mgr->num_hosts];
    memset(host, 0, sizeof(*host));
    
    host->host_id = mgr->num_hosts;
    snprintf(host->config.name, sizeof(host->config.name), "%s", name);
    memcpy(host->config.mac_addr, mac_addr, VHOST_MAC_LEN);
    memcpy(host->config.ip_addr, ip_addr, VHOST_IP_LEN);
    host->config.mtu = 1500;
    host->config.enabled = true;
    host->link_mgr = mgr->link_mgr;
    
    pthread_mutex_init(&host->lock, NULL);
    
    *host_id = mgr->num_hosts;
    mgr->num_hosts++;
    
    pthread_mutex_unlock(&mgr->mgr_lock);
    
    return 0;
}

/* Connect virtual host to a switch PCI link */
int vhost_connect_to_switch(vhost_manager_t *mgr, uint32_t host_id,
                            uint32_t switch_pci_link_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    /* Create our side of the PCI link */
    char link_name[64];
    snprintf(link_name, sizeof(link_name), "host%u_pci", host_id);
    
    if (vlink_create(mgr->link_mgr, link_name, 100000, 1, 0.0, &host->pci_link_id) != 0) {
        return -1;
    }
    
    /* Connect to switch PCI link (bidirectional) */
    if (vlink_connect(mgr->link_mgr, host->pci_link_id, switch_pci_link_id) != 0) {
        return -1;
    }
    
    /* Set RX callback */
    vlink_set_rx_callback(mgr->link_mgr, host->pci_link_id, vhost_rx_callback, host);
    
    return 0;
}

/* Start virtual host */
int vhost_start(vhost_manager_t *mgr, uint32_t host_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    if (host->running) {
        return 0;  /* Already running */
    }
    
    host->running = true;
    
    /* Start virtual link */
    vlink_start(mgr->link_mgr, host->pci_link_id);
    
    return 0;
}

/* Stop virtual host */
int vhost_stop(vhost_manager_t *mgr, uint32_t host_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    if (!host->running) {
        return 0;  /* Already stopped */
    }
    
    printf("[VHOST_STOP] Stopping host %u\n", host_id);
    host->running = false;
    
    /* Stop packet generator if running */
    if (host->pktgen.enabled) {
        vhost_stop_pktgen(mgr, host_id);
    }
    
    /* Stop virtual link */
    vlink_stop(mgr->link_mgr, host->pci_link_id);
    
    return 0;
}

/* Send packet from virtual host */
int vhost_send_packet(vhost_manager_t *mgr, uint32_t host_id,
                      const uint8_t *data, uint16_t size)
{
    if (!mgr || host_id >= mgr->num_hosts || !data || size == 0) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    if (!host->running) {
        return -1;
    }
    
    if (vlink_send(mgr->link_mgr, host->pci_link_id, data, size) != 0) {
        pthread_mutex_lock(&host->lock);
        host->stats.tx_errors++;
        pthread_mutex_unlock(&host->lock);
        return -1;
    }
    
    pthread_mutex_lock(&host->lock);
    host->stats.tx_packets++;
    host->stats.tx_bytes += size;
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Configure packet generator */
int vhost_configure_pktgen(vhost_manager_t *mgr, uint32_t host_id,
                           const vhost_pktgen_config_t *config)
{
    if (!mgr || host_id >= mgr->num_hosts || !config) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    pthread_mutex_lock(&host->lock);
    memcpy(&host->pktgen, config, sizeof(host->pktgen));
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Start packet generator */
int vhost_start_pktgen(vhost_manager_t *mgr, uint32_t host_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        printf("[PKTGEN_START] Invalid mgr or host_id=%u >= %u\n", host_id, mgr ? mgr->num_hosts : 0);
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    if (host->pktgen.enabled) {
        printf("[PKTGEN_START] Host %u already running\n", host_id);
        return 0;  /* Already running */
    }
    
    if (host->pktgen.pps == 0) {
        printf("[PKTGEN_START] Host %u not configured (pps=0)\n", host_id);
        return -1;  /* Not configured */
    }
    
    printf("[PKTGEN_START] Starting pktgen for host %u (running=%d, pps=%u)\n", 
           host_id, host->running, host->pktgen.pps);
    
    host->pktgen.enabled = true;
    
    if (pthread_create(&host->pktgen_thread, NULL, pktgen_thread_func, host) != 0) {
        printf("[PKTGEN_START] pthread_create failed for host %u\n", host_id);
        host->pktgen.enabled = false;
        return -1;
    }
    
    printf("[PKTGEN_START] pthread created successfully for host %u\n", host_id);
    return 0;
}

/* Stop packet generator */
int vhost_stop_pktgen(vhost_manager_t *mgr, uint32_t host_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    if (!host->pktgen.enabled) {
        return 0;  /* Already stopped */
    }
    
    host->pktgen.enabled = false;
    pthread_join(host->pktgen_thread, NULL);
    
    return 0;
}

/* Set custom packet handler callback */
int vhost_set_packet_handler(vhost_manager_t *mgr, uint32_t host_id,
                             void (*handler)(void *ctx, const uint8_t *data, uint16_t size),
                             void *ctx)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    pthread_mutex_lock(&host->lock);
    host->pkt_handler = handler;
    host->pkt_handler_ctx = ctx;
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Get host statistics */
int vhost_get_stats(vhost_manager_t *mgr, uint32_t host_id, vhost_stats_t *stats)
{
    if (!mgr || host_id >= mgr->num_hosts || !stats) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    pthread_mutex_lock(&host->lock);
    memcpy(stats, &host->stats, sizeof(*stats));
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Reset host statistics */
int vhost_reset_stats(vhost_manager_t *mgr, uint32_t host_id)
{
    if (!mgr || host_id >= mgr->num_hosts) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    pthread_mutex_lock(&host->lock);
    memset(&host->stats, 0, sizeof(host->stats));
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Get host configuration */
int vhost_get_config(vhost_manager_t *mgr, uint32_t host_id, vhost_config_t *config)
{
    if (!mgr || host_id >= mgr->num_hosts || !config) {
        return -1;
    }
    
    vhost_instance_t *host = &mgr->hosts[host_id];
    
    pthread_mutex_lock(&host->lock);
    memcpy(config, &host->config, sizeof(*config));
    pthread_mutex_unlock(&host->lock);
    
    return 0;
}

/* Print all host statistics */
void vhost_print_stats(vhost_manager_t *mgr)
{
    if (!mgr) {
        return;
    }
    
    printf("\n========================================\n");
    printf("Virtual Host Statistics\n");
    printf("========================================\n");
    
    for (uint32_t i = 0; i < mgr->num_hosts; i++) {
        vhost_instance_t *host = &mgr->hosts[i];
        
        printf("\nHost %u: %s\n", host->host_id, host->config.name);
        printf("  MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               host->config.mac_addr[0], host->config.mac_addr[1],
               host->config.mac_addr[2], host->config.mac_addr[3],
               host->config.mac_addr[4], host->config.mac_addr[5]);
        printf("  IP: %u.%u.%u.%u\n",
               host->config.ip_addr[0], host->config.ip_addr[1],
               host->config.ip_addr[2], host->config.ip_addr[3]);
        printf("  TX: %lu pkts / %lu bytes (errors: %lu)\n",
               host->stats.tx_packets, host->stats.tx_bytes, host->stats.tx_errors);
        printf("  RX: %lu pkts / %lu bytes (errors: %lu, drops: %lu)\n",
               host->stats.rx_packets, host->stats.rx_bytes,
               host->stats.rx_errors, host->stats.rx_drops);
    }
}

/* Helper: Calculate checksum */
static uint16_t calculate_checksum(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 0;
    
    for (uint16_t i = 0; i < len; i += 2) {
        if (i + 1 < len) {
            sum += (data[i] << 8) | data[i + 1];
        } else {
            sum += data[i] << 8;
        }
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/* Helper: Generate Ethernet frame */
uint16_t vhost_build_eth_frame(uint8_t *frame, uint16_t max_size,
                               const uint8_t *dst_mac, const uint8_t *src_mac,
                               uint16_t eth_type, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t frame_len = 14 + payload_len;
    
    if (frame_len > max_size) {
        return 0;
    }
    
    /* Destination MAC */
    memcpy(frame, dst_mac, 6);
    /* Source MAC */
    memcpy(frame + 6, src_mac, 6);
    /* EtherType */
    frame[12] = (eth_type >> 8) & 0xFF;
    frame[13] = eth_type & 0xFF;
    /* Payload */
    memcpy(frame + 14, payload, payload_len);
    
    return frame_len;
}

/* Helper: Generate UDP/IP packet */
uint16_t vhost_build_udp_packet(uint8_t *packet, uint16_t max_size,
                                const uint8_t *dst_mac, const uint8_t *src_mac,
                                const uint8_t *dst_ip, const uint8_t *src_ip,
                                uint16_t dst_port, uint16_t src_port,
                                const uint8_t *payload, uint16_t payload_len)
{
    uint16_t ip_len = 20 + 8 + payload_len;
    uint16_t total_len = 14 + ip_len;
    
    if (total_len > max_size) {
        return 0;
    }
    
    /* Ethernet header */
    memcpy(packet, dst_mac, 6);
    memcpy(packet + 6, src_mac, 6);
    packet[12] = 0x08;  /* EtherType: IPv4 */
    packet[13] = 0x00;
    
    /* IP header */
    uint8_t *ip = packet + 14;
    ip[0] = 0x45;  /* Version 4, IHL 5 */
    ip[1] = 0x00;  /* DSCP/ECN */
    ip[2] = (ip_len >> 8) & 0xFF;
    ip[3] = ip_len & 0xFF;
    ip[4] = ip[5] = 0;  /* ID */
    ip[6] = ip[7] = 0;  /* Flags/Fragment */
    ip[8] = 64;    /* TTL */
    ip[9] = 17;    /* Protocol: UDP */
    ip[10] = ip[11] = 0;  /* Checksum (calculate later) */
    memcpy(ip + 12, src_ip, 4);
    memcpy(ip + 16, dst_ip, 4);
    
    uint16_t ip_checksum = calculate_checksum(ip, 20);
    ip[10] = (ip_checksum >> 8) & 0xFF;
    ip[11] = ip_checksum & 0xFF;
    
    /* UDP header */
    uint8_t *udp = ip + 20;
    uint16_t udp_len = 8 + payload_len;
    udp[0] = (src_port >> 8) & 0xFF;
    udp[1] = src_port & 0xFF;
    udp[2] = (dst_port >> 8) & 0xFF;
    udp[3] = dst_port & 0xFF;
    udp[4] = (udp_len >> 8) & 0xFF;
    udp[5] = udp_len & 0xFF;
    udp[6] = udp[7] = 0;  /* Checksum (optional for UDP) */
    
    /* Payload */
    memcpy(udp + 8, payload, payload_len);
    
    return total_len;
}

/* Build ARP request packet */
uint16_t vhost_build_arp_request(uint8_t *packet, uint16_t max_size,
                                 const uint8_t *src_mac, const uint8_t *src_ip,
                                 const uint8_t *target_ip)
{
    uint16_t total_len = 14 + 28;  /* Ethernet header + ARP packet */
    
    if (total_len > max_size) {
        return 0;
    }
    
    /* Ethernet header - broadcast */
    memset(packet, 0xFF, 6);  /* Destination: broadcast */
    memcpy(packet + 6, src_mac, 6);  /* Source MAC */
    packet[12] = 0x08;  /* EtherType: ARP */
    packet[13] = 0x06;
    
    /* ARP packet */
    uint8_t *arp = packet + 14;
    
    /* Hardware type: Ethernet (1) */
    arp[0] = 0x00;
    arp[1] = 0x01;
    
    /* Protocol type: IPv4 (0x0800) */
    arp[2] = 0x08;
    arp[3] = 0x00;
    
    /* Hardware address length: 6 */
    arp[4] = 0x06;
    
    /* Protocol address length: 4 */
    arp[5] = 0x04;
    
    /* Operation: Request (1) */
    arp[6] = 0x00;
    arp[7] = 0x01;
    
    /* Sender hardware address (MAC) */
    memcpy(arp + 8, src_mac, 6);
    
    /* Sender protocol address (IP) */
    memcpy(arp + 14, src_ip, 4);
    
    /* Target hardware address (unknown, set to 0) */
    memset(arp + 18, 0x00, 6);
    
    /* Target protocol address (IP) */
    memcpy(arp + 24, target_ip, 4);
    
    return total_len;
}

/* Build ARP reply packet */
uint16_t vhost_build_arp_reply(uint8_t *packet, uint16_t max_size,
                               const uint8_t *src_mac, const uint8_t *src_ip,
                               const uint8_t *dst_mac, const uint8_t *dst_ip)
{
    uint16_t total_len = 14 + 28;  /* Ethernet header + ARP packet */
    
    if (total_len > max_size) {
        return 0;
    }
    
    /* Ethernet header - unicast to requester */
    memcpy(packet, dst_mac, 6);  /* Destination MAC */
    memcpy(packet + 6, src_mac, 6);  /* Source MAC */
    packet[12] = 0x08;  /* EtherType: ARP */
    packet[13] = 0x06;
    
    /* ARP packet */
    uint8_t *arp = packet + 14;
    
    /* Hardware type: Ethernet (1) */
    arp[0] = 0x00;
    arp[1] = 0x01;
    
    /* Protocol type: IPv4 (0x0800) */
    arp[2] = 0x08;
    arp[3] = 0x00;
    
    /* Hardware address length: 6 */
    arp[4] = 0x06;
    
    /* Protocol address length: 4 */
    arp[5] = 0x04;
    
    /* Operation: Reply (2) */
    arp[6] = 0x00;
    arp[7] = 0x02;
    
    /* Sender hardware address (MAC) */
    memcpy(arp + 8, src_mac, 6);
    
    /* Sender protocol address (IP) */
    memcpy(arp + 14, src_ip, 4);
    
    /* Target hardware address (MAC) */
    memcpy(arp + 18, dst_mac, 6);
    
    /* Target protocol address (IP) */
    memcpy(arp + 24, dst_ip, 4);
    
    return total_len;
}
