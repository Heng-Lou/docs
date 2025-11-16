/*
 * Virtual Host and Switch Integration Test
 * 
 * Demonstrates connecting N virtual hosts to N switches in a ring topology.
 * Each host can send/receive packets through its switch's PCI port.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "virtual_link.h"
#include "virtual_host.h"

/* Switch instance */
typedef struct {
    uint32_t switch_id;
    char name[64];
    
    /* Virtual links for each port */
    uint32_t pci_link_id;      /* Port 0 - PCI to host */
    uint32_t eth0_link_id;     /* Port 1 - Ethernet 0 */
    uint32_t eth1_link_id;     /* Port 2 - Ethernet 1 */
    
    /* Port statistics */
    struct {
        uint64_t rx_packets;
        uint64_t tx_packets;
        uint64_t rx_bytes;
        uint64_t tx_bytes;
        uint64_t drops;
    } port_stats[3];
    
    vlink_manager_t *link_mgr;
    bool running;
    uint8_t ttl;  /* TTL for ring topology */
} switch_instance_t;

#define MAX_SWITCHES 32
static switch_instance_t switches[MAX_SWITCHES];
static uint32_t num_switches = 0;
static vlink_manager_t global_link_mgr;
static vhost_manager_t global_host_mgr;
static volatile bool keep_running = true;

/* Signal handler */
void signal_handler(int sig)
{
    (void)sig;
    keep_running = false;
    printf("\nShutdown requested...\n");
}

/* Decrement TTL and check for loop prevention */
static bool check_and_decrement_ttl(uint8_t *packet, uint16_t size)
{
    /* Check if this is an IP packet */
    if (size < 34) {  /* Min Ethernet + IP header */
        return true;
    }
    
    /* Check EtherType for IPv4 (0x0800) */
    if (packet[12] != 0x08 || packet[13] != 0x00) {
        return true;  /* Not IP, allow */
    }
    
    /* Get TTL from IP header (byte 8 of IP header) */
    uint8_t *ttl_ptr = &packet[14 + 8];
    
    if (*ttl_ptr == 0) {
        return false;  /* Drop packet, TTL expired */
    }
    
    (*ttl_ptr)--;
    
    /* Recalculate IP checksum */
    uint8_t *ip_hdr = &packet[14];
    ip_hdr[10] = 0;
    ip_hdr[11] = 0;
    
    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2) {
        sum += (ip_hdr[i] << 8) | ip_hdr[i + 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    uint16_t checksum = ~sum;
    ip_hdr[10] = (checksum >> 8) & 0xFF;
    ip_hdr[11] = checksum & 0xFF;
    
    return *ttl_ptr > 0;
}

/* Forward packet based on three-port switch logic */
static uint8_t get_forward_port(uint8_t input_port)
{
    switch (input_port) {
        case 0: return 1;  /* PCI -> Eth0 */
        case 1: return 2;  /* Eth0 -> Eth1 */
        case 2: return 0;  /* Eth1 -> PCI */
        default: return 0;
    }
}

/* RX callback for PCI port */
static void pci_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    switch_instance_t *sw = (switch_instance_t *)ctx;
    uint8_t packet[9000];
    
    memcpy(packet, data, size);
    
    sw->port_stats[0].rx_packets++;
    sw->port_stats[0].rx_bytes += size;
    
    /* Check TTL */
    if (!check_and_decrement_ttl(packet, size)) {
        sw->port_stats[0].drops++;
        return;
    }
    
    /* Forward to Eth0 (port 1) */
    uint8_t out_port = get_forward_port(0);
    uint32_t out_link = (out_port == 1) ? sw->eth0_link_id : sw->eth1_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, packet, size) == 0) {
        sw->port_stats[out_port].tx_packets++;
        sw->port_stats[out_port].tx_bytes += size;
    } else {
        sw->port_stats[out_port].drops++;
    }
}

/* RX callback for Eth0 port */
static void eth0_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    switch_instance_t *sw = (switch_instance_t *)ctx;
    uint8_t packet[9000];
    
    memcpy(packet, data, size);
    
    sw->port_stats[1].rx_packets++;
    sw->port_stats[1].rx_bytes += size;
    
    /* Check TTL */
    if (!check_and_decrement_ttl(packet, size)) {
        sw->port_stats[1].drops++;
        return;
    }
    
    /* Forward to Eth1 (port 2) */
    uint8_t out_port = get_forward_port(1);
    uint32_t out_link = (out_port == 2) ? sw->eth1_link_id : sw->pci_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, packet, size) == 0) {
        sw->port_stats[out_port].tx_packets++;
        sw->port_stats[out_port].tx_bytes += size;
    } else {
        sw->port_stats[out_port].drops++;
    }
}

/* RX callback for Eth1 port */
static void eth1_rx_callback(void *ctx, const uint8_t *data, uint16_t size)
{
    switch_instance_t *sw = (switch_instance_t *)ctx;
    uint8_t packet[9000];
    
    memcpy(packet, data, size);
    
    sw->port_stats[2].rx_packets++;
    sw->port_stats[2].rx_bytes += size;
    
    /* Check TTL */
    if (!check_and_decrement_ttl(packet, size)) {
        sw->port_stats[2].drops++;
        return;
    }
    
    /* Forward to PCI (port 0) */
    uint8_t out_port = get_forward_port(2);
    uint32_t out_link = (out_port == 0) ? sw->pci_link_id : sw->eth0_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, packet, size) == 0) {
        sw->port_stats[out_port].tx_packets++;
        sw->port_stats[out_port].tx_bytes += size;
    } else {
        sw->port_stats[out_port].drops++;
    }
}

/* Create a switch instance */
static int create_switch(uint32_t switch_id, const char *name)
{
    if (num_switches >= MAX_SWITCHES) {
        return -1;
    }
    
    switch_instance_t *sw = &switches[num_switches++];
    memset(sw, 0, sizeof(*sw));
    
    sw->switch_id = switch_id;
    snprintf(sw->name, sizeof(sw->name), "%s", name);
    sw->link_mgr = &global_link_mgr;
    sw->running = true;
    sw->ttl = 64;
    
    /* Create virtual links for each port */
    char link_name[64];
    
    snprintf(link_name, sizeof(link_name), "sw%u_pci", switch_id);
    vlink_create(&global_link_mgr, link_name, 100000, 1, 0.0, &sw->pci_link_id);
    
    snprintf(link_name, sizeof(link_name), "sw%u_eth0", switch_id);
    vlink_create(&global_link_mgr, link_name, 10000, 10, 0.0, &sw->eth0_link_id);
    
    snprintf(link_name, sizeof(link_name), "sw%u_eth1", switch_id);
    vlink_create(&global_link_mgr, link_name, 10000, 10, 0.0, &sw->eth1_link_id);
    
    /* Set RX callbacks */
    vlink_set_rx_callback(&global_link_mgr, sw->pci_link_id, pci_rx_callback, sw);
    vlink_set_rx_callback(&global_link_mgr, sw->eth0_link_id, eth0_rx_callback, sw);
    vlink_set_rx_callback(&global_link_mgr, sw->eth1_link_id, eth1_rx_callback, sw);
    
    /* Start links */
    vlink_start(&global_link_mgr, sw->pci_link_id);
    vlink_start(&global_link_mgr, sw->eth0_link_id);
    vlink_start(&global_link_mgr, sw->eth1_link_id);
    
    return 0;
}

/* Connect switches in ring topology */
static void connect_ring_topology(void)
{
    printf("\nConnecting switches in ring topology...\n");
    
    for (uint32_t i = 0; i < num_switches; i++) {
        uint32_t next = (i + 1) % num_switches;
        
        /* Connect switch[i].eth1 to switch[next].eth0 */
        vlink_connect(&global_link_mgr, 
                     switches[i].eth1_link_id,
                     switches[next].eth0_link_id);
        
        printf("  Switch %u (eth1) <-> Switch %u (eth0)\n", i, next);
    }
}

/* Host packet handler - prints received packets */
static void host_packet_handler(void *ctx, const uint8_t *data, uint16_t size)
{
    uint32_t host_id = *(uint32_t *)ctx;
    
    printf("Host %u: Received packet (%u bytes)\n", host_id, size);
    
    /* Print first few bytes */
    printf("  Data: ");
    for (int i = 0; i < (size < 16 ? size : 16); i++) {
        printf("%02x ", data[i]);
    }
    if (size > 16) {
        printf("...");
    }
    printf("\n");
}

/* Create virtual hosts */
static void create_hosts(uint32_t count)
{
    printf("\nCreating %u virtual hosts...\n", count);
    
    for (uint32_t i = 0; i < count; i++) {
        char name[64];
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, (uint8_t)i};
        uint8_t ip[4] = {192, 168, 1, (uint8_t)(10 + i)};
        uint32_t host_id;
        
        snprintf(name, sizeof(name), "Host-%u", i);
        
        if (vhost_create(&global_host_mgr, name, mac, ip, &host_id) != 0) {
            fprintf(stderr, "Failed to create host %u\n", i);
            continue;
        }
        
        /* Connect to switch PCI port */
        if (vhost_connect_to_switch(&global_host_mgr, host_id, switches[i].pci_link_id) != 0) {
            fprintf(stderr, "Failed to connect host %u to switch %u\n", i, i);
            continue;
        }
        
        /* Set packet handler */
        static uint32_t host_ids[MAX_SWITCHES];
        host_ids[i] = i;
        vhost_set_packet_handler(&global_host_mgr, host_id, host_packet_handler, &host_ids[i]);
        
        /* Start host */
        vhost_start(&global_host_mgr, host_id);
        
        printf("  Created and connected Host %u to Switch %u\n", i, i);
    }
}

/* Configure packet generators on hosts */
static void configure_pktgen(bool enabled, uint32_t pps, uint32_t count)
{
    if (!enabled) {
        return;
    }
    
    printf("\nConfiguring packet generators...\n");
    
    for (uint32_t i = 0; i < num_switches; i++) {
        vhost_pktgen_config_t config = {0};
        config.enabled = true;
        config.pkt_size = 128;
        config.pps = pps;
        config.count = count;
        
        /* Send to next host in ring */
        uint32_t dst_host = (i + 1) % num_switches;
        config.dst_mac[0] = 0x02;
        config.dst_mac[5] = dst_host;
        config.dst_ip[0] = 192;
        config.dst_ip[1] = 168;
        config.dst_ip[2] = 1;
        config.dst_ip[3] = 10 + dst_host;
        config.dst_port = 5000;
        
        vhost_configure_pktgen(&global_host_mgr, i, &config);
        
        printf("  Host %u -> Host %u (%u pps, %u packets)\n", 
               i, dst_host, pps, count);
    }
}

/* Start all packet generators */
static void start_all_pktgen(void)
{
    printf("\nStarting packet generators...\n");
    
    for (uint32_t i = 0; i < num_switches; i++) {
        vhost_start_pktgen(&global_host_mgr, i);
    }
}

/* Print all statistics */
static void print_all_stats(void)
{
    printf("\n========================================\n");
    printf("Switch Statistics\n");
    printf("========================================\n");
    
    for (uint32_t i = 0; i < num_switches; i++) {
        switch_instance_t *sw = &switches[i];
        
        printf("\nSwitch %u: %s\n", sw->switch_id, sw->name);
        printf("  Port 0 (PCI):  RX %lu pkts/%lu bytes, TX %lu pkts/%lu bytes, Drops %lu\n",
               sw->port_stats[0].rx_packets, sw->port_stats[0].rx_bytes,
               sw->port_stats[0].tx_packets, sw->port_stats[0].tx_bytes,
               sw->port_stats[0].drops);
        printf("  Port 1 (Eth0): RX %lu pkts/%lu bytes, TX %lu pkts/%lu bytes, Drops %lu\n",
               sw->port_stats[1].rx_packets, sw->port_stats[1].rx_bytes,
               sw->port_stats[1].tx_packets, sw->port_stats[1].tx_bytes,
               sw->port_stats[1].drops);
        printf("  Port 2 (Eth1): RX %lu pkts/%lu bytes, TX %lu pkts/%lu bytes, Drops %lu\n",
               sw->port_stats[2].rx_packets, sw->port_stats[2].rx_bytes,
               sw->port_stats[2].tx_packets, sw->port_stats[2].tx_bytes,
               sw->port_stats[2].drops);
    }
    
    vhost_print_stats(&global_host_mgr);
}

/* Usage */
static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("Virtual Host and Switch Integration Test\n\n");
    printf("Options:\n");
    printf("  -n NUM      Number of switches/hosts (default: 4, max: %d)\n", MAX_SWITCHES);
    printf("  -p          Enable packet generation\n");
    printf("  -r RATE     Packet generation rate in pps (default: 100)\n");
    printf("  -c COUNT    Number of packets to send (default: 100, 0=infinite)\n");
    printf("  -d DURATION Run duration in seconds (default: 10)\n");
    printf("  -h          Show this help\n");
}

int main(int argc, char *argv[])
{
    int opt;
    uint32_t num = 4;
    bool enable_pktgen = false;
    uint32_t pps = 100;
    uint32_t pkt_count = 100;
    uint32_t duration = 10;
    
    /* Parse arguments */
    while ((opt = getopt(argc, argv, "n:pr:c:d:h")) != -1) {
        switch (opt) {
            case 'n':
                num = atoi(optarg);
                if (num < 2 || num > MAX_SWITCHES) {
                    fprintf(stderr, "Invalid number (2-%d)\n", MAX_SWITCHES);
                    return 1;
                }
                break;
            case 'p':
                enable_pktgen = true;
                break;
            case 'r':
                pps = atoi(optarg);
                break;
            case 'c':
                pkt_count = atoi(optarg);
                break;
            case 'd':
                duration = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("========================================\n");
    printf("Virtual Host + Switch Simulation\n");
    printf("========================================\n");
    printf("Switches/Hosts: %u\n", num);
    printf("Topology: Ring\n");
    printf("Packet Gen: %s\n", enable_pktgen ? "Enabled" : "Disabled");
    if (enable_pktgen) {
        printf("  Rate: %u pps\n", pps);
        printf("  Count: %u packets\n", pkt_count);
    }
    printf("Duration: %u seconds\n", duration);
    printf("\n");
    
    /* Initialize managers */
    if (vlink_manager_init(&global_link_mgr) != 0) {
        fprintf(stderr, "Failed to initialize link manager\n");
        return 1;
    }
    
    if (vhost_manager_init(&global_host_mgr, &global_link_mgr) != 0) {
        fprintf(stderr, "Failed to initialize host manager\n");
        return 1;
    }
    
    /* Create switches */
    printf("Creating switches...\n");
    for (uint32_t i = 0; i < num; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Switch-%u", i);
        if (create_switch(i, name) != 0) {
            fprintf(stderr, "Failed to create switch %u\n", i);
            return 1;
        }
        printf("  Created Switch %u\n", i);
    }
    
    /* Connect ring topology */
    connect_ring_topology();
    
    /* Create and connect hosts */
    create_hosts(num);
    
    /* Configure packet generation */
    if (enable_pktgen) {
        configure_pktgen(true, pps, pkt_count);
        start_all_pktgen();
    }
    
    printf("\n✓ All components running!\n");
    printf("Press Ctrl+C to stop and show statistics\n\n");
    
    /* Run for specified duration or until interrupted */
    for (uint32_t i = 0; i < duration && keep_running; i++) {
        sleep(1);
        if (enable_pktgen && (i % 5 == 0)) {
            printf("Running... (%u/%u seconds)\n", i, duration);
        }
    }
    
    /* Print final statistics */
    print_all_stats();
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    vhost_manager_cleanup(&global_host_mgr);
    
    for (uint32_t i = 0; i < num_switches; i++) {
        vlink_stop(&global_link_mgr, switches[i].pci_link_id);
        vlink_stop(&global_link_mgr, switches[i].eth0_link_id);
        vlink_stop(&global_link_mgr, switches[i].eth1_link_id);
    }
    
    vlink_manager_cleanup(&global_link_mgr);
    
    printf("Done.\n");
    
    return 0;
}
