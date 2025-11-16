/*
 * Three-Port Switch with Virtual Links
 * 
 * Enhanced simulation that uses virtual network links to connect
 * multiple switch instances together.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include "virtual_link.h"

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
} switch_instance_t;

/* Global switch array for multi-switch topology */
#define MAX_SWITCHES 16
static switch_instance_t switches[MAX_SWITCHES];
static uint32_t num_switches = 0;
static vlink_manager_t global_link_mgr;
static volatile bool keep_running = true;

/* Signal handler */
void signal_handler(int sig)
{
    (void)sig;
    keep_running = false;
    printf("\nShutdown requested...\n");
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
    
    sw->port_stats[0].rx_packets++;
    sw->port_stats[0].rx_bytes += size;
    
    /* Forward to Eth0 (port 1) */
    uint8_t out_port = get_forward_port(0);
    uint32_t out_link = (out_port == 1) ? sw->eth0_link_id : sw->eth1_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, data, size) == 0) {
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
    
    sw->port_stats[1].rx_packets++;
    sw->port_stats[1].rx_bytes += size;
    
    /* Forward to Eth1 (port 2) */
    uint8_t out_port = get_forward_port(1);
    uint32_t out_link = (out_port == 2) ? sw->eth1_link_id : sw->pci_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, data, size) == 0) {
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
    
    sw->port_stats[2].rx_packets++;
    sw->port_stats[2].rx_bytes += size;
    
    /* Forward to PCI (port 0) */
    uint8_t out_port = get_forward_port(2);
    uint32_t out_link = (out_port == 0) ? sw->pci_link_id : sw->eth0_link_id;
    
    if (vlink_send(sw->link_mgr, out_link, data, size) == 0) {
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
    
    printf("Created switch %u: %s\n", switch_id, name);
    
    return 0;
}

/* Connect switches in a ring topology */
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

/* Connect switches in a line topology */
static void connect_line_topology(void)
{
    printf("\nConnecting switches in line topology...\n");
    
    for (uint32_t i = 0; i < num_switches - 1; i++) {
        /* Connect switch[i].eth1 to switch[i+1].eth0 */
        vlink_connect(&global_link_mgr,
                     switches[i].eth1_link_id,
                     switches[i+1].eth0_link_id);
        
        printf("  Switch %u (eth1) <-> Switch %u (eth0)\n", i, i + 1);
    }
}

/* Connect switches in a mesh topology */
static void connect_mesh_topology(void)
{
    printf("\nConnecting switches in mesh topology...\n");
    printf("Note: Limited to 2 Ethernet ports per switch\n");
    
    /* For simplicity with 3-port switches, create partial mesh */
    for (uint32_t i = 0; i < num_switches; i++) {
        uint32_t next1 = (i + 1) % num_switches;
        
        vlink_connect(&global_link_mgr,
                     switches[i].eth0_link_id,
                     switches[next1].eth1_link_id);
        
        printf("  Switch %u (eth0) <-> Switch %u (eth1)\n", i, next1);
    }
}

/* Print statistics for all switches */
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
    
    vlink_print_stats(&global_link_mgr);
}

/* Send test traffic */
static void send_test_traffic(void)
{
    printf("\nSending test traffic...\n");
    
    uint8_t test_packet[128];
    memset(test_packet, 0xAA, sizeof(test_packet));
    
    /* Inject packets into each switch's PCI port */
    for (uint32_t i = 0; i < num_switches; i++) {
        printf("  Switch %u: Injecting packet on PCI port\n", i);
        vlink_send(&global_link_mgr, switches[i].pci_link_id, test_packet, sizeof(test_packet));
        usleep(10000);  /* 10ms delay between packets */
    }
    
    /* Give time for packets to propagate */
    sleep(1);
}

/* Usage */
static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("Options:\n");
    printf("  -n NUM      Number of switches (default: 4)\n");
    printf("  -t TOPO     Topology: ring, line, mesh (default: ring)\n");
    printf("  -s          Send test traffic\n");
    printf("  -h          Show this help\n");
}

int main(int argc, char *argv[])
{
    int opt;
    uint32_t num_sw = 4;
    char *topology = "ring";
    bool send_traffic = false;
    
    /* Parse arguments */
    while ((opt = getopt(argc, argv, "n:t:sh")) != -1) {
        switch (opt) {
            case 'n':
                num_sw = atoi(optarg);
                if (num_sw < 2 || num_sw > MAX_SWITCHES) {
                    fprintf(stderr, "Invalid number of switches (2-%d)\n", MAX_SWITCHES);
                    return 1;
                }
                break;
            case 't':
                topology = optarg;
                break;
            case 's':
                send_traffic = true;
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
    printf("Virtual Link Switch Simulation\n");
    printf("========================================\n");
    printf("Switches: %u\n", num_sw);
    printf("Topology: %s\n", topology);
    printf("\n");
    
    /* Initialize virtual link manager */
    if (vlink_manager_init(&global_link_mgr) != 0) {
        fprintf(stderr, "Failed to initialize virtual link manager\n");
        return 1;
    }
    
    /* Create switches */
    for (uint32_t i = 0; i < num_sw; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Switch-%u", i);
        create_switch(i, name);
    }
    
    /* Connect topology */
    if (strcmp(topology, "ring") == 0) {
        connect_ring_topology();
    } else if (strcmp(topology, "line") == 0) {
        connect_line_topology();
    } else if (strcmp(topology, "mesh") == 0) {
        connect_mesh_topology();
    } else {
        fprintf(stderr, "Unknown topology: %s\n", topology);
        vlink_manager_cleanup(&global_link_mgr);
        return 1;
    }
    
    printf("\nAll switches connected and running!\n");
    printf("Press Ctrl+C to stop and show statistics\n\n");
    
    /* Send test traffic if requested */
    if (send_traffic) {
        sleep(1);
        send_test_traffic();
    }
    
    /* Main loop */
    uint32_t iteration = 0;
    while (keep_running) {
        sleep(5);
        iteration++;
        
        if (send_traffic && iteration % 6 == 0) {
            send_test_traffic();
        }
    }
    
    /* Print final statistics */
    print_all_stats();
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    for (uint32_t i = 0; i < num_switches; i++) {
        vlink_stop(&global_link_mgr, switches[i].pci_link_id);
        vlink_stop(&global_link_mgr, switches[i].eth0_link_id);
        vlink_stop(&global_link_mgr, switches[i].eth1_link_id);
    }
    
    vlink_manager_cleanup(&global_link_mgr);
    
    printf("Done.\n");
    
    return 0;
}
