/*
 * Three Port Switch with veth/Namespace Integration - v2
 * Improved with better error handling and diagnostics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#define MAX_PKT_BURST 32
#define MBUF_CACHE_SIZE 250
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8192

#define MAC_TABLE_SIZE 1024
#define MAC_AGING_TIME 300

static volatile bool force_quit = false;

enum port_type {
    PORT_TYPE_HOST = 0,
    PORT_TYPE_SWITCH_LINK = 1,
};

struct port_config {
    char veth_name[32];
    uint16_t port_id;
    enum port_type type;
    char peer_switch[16];
    bool configured;
};

struct mac_entry {
    struct rte_ether_addr mac;
    uint16_t port_id;
    time_t timestamp;
    bool valid;
};

#define MAX_PORTS 11

struct switch_ctx {
    int switch_id;
    int num_switches;
    char topology[16];
    
    struct port_config ports[MAX_PORTS];
    int num_ports;
    
    struct mac_entry mac_table[MAC_TABLE_SIZE];
    
    struct rte_mempool *mbuf_pool;
    
    uint64_t rx_packets[MAX_PORTS];
    uint64_t tx_packets[MAX_PORTS];
    uint64_t dropped_packets;
    uint64_t mac_learned;
};

static struct switch_ctx ctx = {0};

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n", signum);
        force_quit = true;
    }
}

static uint32_t mac_hash(const struct rte_ether_addr *mac)
{
    uint32_t hash = 0;
    for (int i = 0; i < 6; i++) {
        hash = hash * 31 + mac->addr_bytes[i];
    }
    return hash % MAC_TABLE_SIZE;
}

static int mac_lookup(const struct rte_ether_addr *mac)
{
    uint32_t hash = mac_hash(mac);
    uint32_t idx = hash;
    time_t now = time(NULL);
    
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        idx = (hash + i) % MAC_TABLE_SIZE;
        
        if (!ctx.mac_table[idx].valid) {
            return -1;
        }
        
        if (rte_is_same_ether_addr(&ctx.mac_table[idx].mac, mac)) {
            if (now - ctx.mac_table[idx].timestamp > MAC_AGING_TIME) {
                ctx.mac_table[idx].valid = false;
                return -1;
            }
            return ctx.mac_table[idx].port_id;
        }
    }
    
    return -1;
}

static int load_mac_table_from_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Warning: Cannot open MAC table file %s\n", filename);
        return -1;
    }
    
    printf("Loading static MAC table from %s...\n", filename);
    fflush(stdout);
    
    char line[256];
    int entries_loaded = 0;
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        uint8_t mac[6];
        int port_index;
        char desc[64];
        
        int parsed = sscanf(line, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx %d %63s",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5],
                   &port_index, desc);
        
        if (parsed == 8) {
            // Validate port_index
            if (port_index < 0 || port_index >= ctx.num_ports) {
                printf("  Warning: Invalid port index %d on line %d (have %d ports)\n", 
                       port_index, line_num, ctx.num_ports);
                continue;
            }
            
            // Get the actual DPDK port_id from the port_index
            uint16_t dpdk_port_id = ctx.ports[port_index].port_id;
            
            // Add to MAC table using direct hash
            struct rte_ether_addr addr;
            memcpy(addr.addr_bytes, mac, 6);
            
            uint32_t hash = mac_hash(&addr);
            uint32_t idx = hash;
            bool added = false;
            
            for (int i = 0; i < MAC_TABLE_SIZE; i++) {
                idx = (hash + i) % MAC_TABLE_SIZE;
                
                if (!ctx.mac_table[idx].valid) {
                    rte_ether_addr_copy(&addr, &ctx.mac_table[idx].mac);
                    ctx.mac_table[idx].port_id = dpdk_port_id;
                    ctx.mac_table[idx].timestamp = time(NULL);
                    ctx.mac_table[idx].valid = true;
                    
                    printf("  [Line %d] Added: %02x:%02x:%02x:%02x:%02x:%02x -> port_idx=%d (port_id=%u, %s) [%s]\n",
                           line_num,
                           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                           port_index, dpdk_port_id, ctx.ports[port_index].veth_name, desc);
                    printf("           Hash slot: %u\n", idx);
                    fflush(stdout);
                    
                    entries_loaded++;
                    ctx.mac_learned++;
                    added = true;
                    break;
                }
            }
            
            if (!added) {
                printf("  ERROR: Could not add MAC entry on line %d (hash table full?)\n", line_num);
            }
        } else {
            if (line[0] != '#' && line[0] != '\n') {
                printf("  Warning: Could not parse line %d: %s", line_num, line);
            }
        }
    }
    
    fclose(fp);
    printf("Loaded %d static MAC entries from %s\n", entries_loaded, filename);
    fflush(stdout);
    
    // Verify by doing test lookups
    printf("\nVerifying MAC table entries:\n");
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (ctx.mac_table[i].valid) {
            int lookup_result = mac_lookup(&ctx.mac_table[i].mac);
            printf("  Slot %d: %02x:%02x:%02x:%02x:%02x:%02x -> port_id=%u (lookup returns: %d)\n",
                   i,
                   ctx.mac_table[i].mac.addr_bytes[0],
                   ctx.mac_table[i].mac.addr_bytes[1],
                   ctx.mac_table[i].mac.addr_bytes[2],
                   ctx.mac_table[i].mac.addr_bytes[3],
                   ctx.mac_table[i].mac.addr_bytes[4],
                   ctx.mac_table[i].mac.addr_bytes[5],
                   ctx.mac_table[i].port_id,
                   lookup_result);
        }
    }
    fflush(stdout);
    
    return entries_loaded;
}

static void forward_packet(struct rte_mbuf *m, uint16_t rx_port_id)
{
    struct rte_ether_hdr *eth_hdr;
    int dst_port_id;
    static uint64_t pkt_num = 0;
    
    eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
    
    pkt_num++;
    if (pkt_num <= 50) {
        printf("PKT#%lu: RX on port_id=%u, dst_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
               pkt_num, rx_port_id,
               eth_hdr->dst_addr.addr_bytes[0],
               eth_hdr->dst_addr.addr_bytes[1],
               eth_hdr->dst_addr.addr_bytes[2],
               eth_hdr->dst_addr.addr_bytes[3],
               eth_hdr->dst_addr.addr_bytes[4],
               eth_hdr->dst_addr.addr_bytes[5]);
        fflush(stdout);
    }
    
    // Don't learn MACs - use static table only
    
    // Lookup destination in static MAC table
    dst_port_id = mac_lookup(&eth_hdr->dst_addr);
    
    if (pkt_num <= 50) {
        printf("  dst_port lookup=%d (rx_port=%u)\n", dst_port_id, rx_port_id);
        fflush(stdout);
    }
    
    if (dst_port_id >= 0 && dst_port_id != rx_port_id) {
        // Known destination - forward directly
        if (pkt_num <= 50) {
            printf("  → Unicast forwarding to port_id %d\n", dst_port_id);
            fflush(stdout);
        }
        uint16_t sent = rte_eth_tx_burst(dst_port_id, 0, &m, 1);
        if (sent > 0) {
            ctx.tx_packets[dst_port_id]++;
        } else {
            rte_pktmbuf_free(m);
            ctx.dropped_packets++;
        }
    } else {
        // Unknown destination or broadcast - flood to all ports except rx_port
        if (pkt_num <= 50) {
            printf("  → Flooding (dst_port=%d). Will flood to: ", dst_port_id);
            fflush(stdout);
        }
        
        int flood_count = 0;
        int num_flood_ports = 0;
        
        for (int i = 0; i < ctx.num_ports; i++) {
            if (ctx.ports[i].configured && ctx.ports[i].port_id != rx_port_id) {
                num_flood_ports++;
                if (pkt_num <= 50) {
                    printf("port_id=%u ", ctx.ports[i].port_id);
                    fflush(stdout);
                }
            }
        }
        
        if (pkt_num <= 50) {
            printf("(total: %d ports)\n", num_flood_ports);
            fflush(stdout);
        }
        
        if (num_flood_ports == 0) {
            rte_pktmbuf_free(m);
            ctx.dropped_packets++;
            return;
        }
        
        for (int i = 0; i < ctx.num_ports; i++) {
            if (ctx.ports[i].configured && ctx.ports[i].port_id != rx_port_id) {
                struct rte_mbuf *m_to_send;
                
                if (flood_count == num_flood_ports - 1) {
                    m_to_send = m;
                } else {
                    m_to_send = rte_pktmbuf_clone(m, ctx.mbuf_pool);
                    if (!m_to_send) {
                        continue;
                    }
                }
                
                uint16_t sent = rte_eth_tx_burst(ctx.ports[i].port_id, 0, &m_to_send, 1);
                if (sent > 0) {
                    ctx.tx_packets[ctx.ports[i].port_id]++;
                    if (pkt_num <= 50) {
                        printf("    → Sent to port_id %u (%s)\n", 
                               ctx.ports[i].port_id, ctx.ports[i].veth_name);
                        fflush(stdout);
                    }
                } else {
                    rte_pktmbuf_free(m_to_send);
                    ctx.dropped_packets++;
                }
                flood_count++;
            }
        }
    }
}

static int main_loop(void)
{
    struct rte_mbuf *bufs[MAX_PKT_BURST];
    uint16_t nb_rx;
    static uint64_t loop_count = 0;
    static time_t last_debug = 0;
    
    printf("\nSwitch %d forwarding packets on core %u. [Ctrl+C to quit]\n",
           ctx.switch_id, rte_lcore_id());
    fflush(stdout);
    
    while (!force_quit) {
        loop_count++;
        
        time_t now = time(NULL);
        if (now != last_debug && now % 10 == 0) {
            printf("DEBUG: Loop count: %lu, checking %d ports\n", loop_count, ctx.num_ports);
            fflush(stdout);
            last_debug = now;
        }
        
        for (int i = 0; i < ctx.num_ports; i++) {
            if (!ctx.ports[i].configured) {
                continue;
            }
            
            nb_rx = rte_eth_rx_burst(ctx.ports[i].port_id, 0, bufs, MAX_PKT_BURST);
            
            if (nb_rx > 0) {
                printf("DEBUG: Port %d (%s) received %u packets!\n", 
                       i, ctx.ports[i].veth_name, nb_rx);
                fflush(stdout);
            }
            
            ctx.rx_packets[ctx.ports[i].port_id] += nb_rx;
            
            for (uint16_t j = 0; j < nb_rx; j++) {
                forward_packet(bufs[j], ctx.ports[i].port_id);
            }
        }
    }
    
    return 0;
}

// Fix the configure_ports function for LINE topology

static int configure_ports(void)
{
    ctx.num_ports = 0;
    
    printf("Configuring ports for Switch %d (%s topology, %d switches total)\n",
           ctx.switch_id, ctx.topology, ctx.num_switches);
    fflush(stdout);
    
    // Port 0: Always the local host connection
    snprintf(ctx.ports[0].veth_name, sizeof(ctx.ports[0].veth_name),
             "veth_s%d_h%d", ctx.switch_id, ctx.switch_id);
    ctx.ports[0].type = PORT_TYPE_HOST;
    ctx.ports[0].configured = false;
    ctx.num_ports++;
    printf("  Port 0: %s (local host ns%d)\n", ctx.ports[0].veth_name, ctx.switch_id);
    fflush(stdout);
    
    if (strcmp(ctx.topology, "line") == 0) {
        // LINE TOPOLOGY
        if (ctx.switch_id == 1) {
            // First switch: only has link to switch 2
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s1_s2");
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].peer_switch, sizeof(ctx.ports[1].peer_switch),
                     "s2");
            ctx.ports[1].configured = false;
            ctx.num_ports++;
            printf("  Port 1: %s (to switch 2)\n", ctx.ports[1].veth_name);
            fflush(stdout);
            
        } else if (ctx.switch_id == ctx.num_switches) {
            // Last switch: only has link to previous switch
            int prev = ctx.switch_id - 1;
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, prev);  // FIXED: current_previous
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].peer_switch, sizeof(ctx.ports[1].peer_switch),
                     "s%d", prev);
            ctx.ports[1].configured = false;
            ctx.num_ports++;
            printf("  Port 1: %s (to switch %d)\n", ctx.ports[1].veth_name, prev);
            fflush(stdout);
            
        } else {
            // Middle switch: has links to both neighbors
            int prev = ctx.switch_id - 1;
            int next = ctx.switch_id + 1;
            
            // Link to LEFT (previous) switch
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, prev);  // FIXED: current_previous
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].peer_switch, sizeof(ctx.ports[1].peer_switch),
                     "s%d", prev);
            ctx.ports[1].configured = false;
            ctx.num_ports++;
            printf("  Port 1: %s (to switch %d - left)\n", ctx.ports[1].veth_name, prev);
            fflush(stdout);
            
            // Link to RIGHT (next) switch
            snprintf(ctx.ports[2].veth_name, sizeof(ctx.ports[2].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, next);
            ctx.ports[2].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[2].peer_switch, sizeof(ctx.ports[2].peer_switch),
                     "s%d", next);
            ctx.ports[2].configured = false;
            ctx.num_ports++;
            printf("  Port 2: %s (to switch %d - right)\n", ctx.ports[2].veth_name, next);
            fflush(stdout);
        }
        
// Simpler fix - just use the same pattern as line topology

    } else if (strcmp(ctx.topology, "ring") == 0) {
        // RING TOPOLOGY - use same naming pattern as line topology
        int next = (ctx.switch_id % ctx.num_switches) + 1;
        int prev = (ctx.switch_id == 1) ? ctx.num_switches : ctx.switch_id - 1;
        
        // Clockwise link: to next switch
        snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                 "veth_s%d_s%d", ctx.switch_id, next);
        ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
        snprintf(ctx.ports[1].peer_switch, sizeof(ctx.ports[1].peer_switch),
                 "s%d", next);
        ctx.ports[1].configured = false;
        ctx.num_ports++;
        printf("  Port 1: %s (to switch %d - clockwise)\n", ctx.ports[1].veth_name, next);
        fflush(stdout);
        
        // Counter-clockwise link: from previous switch
        snprintf(ctx.ports[2].veth_name, sizeof(ctx.ports[2].veth_name),
                 "veth_s%d_s%d", ctx.switch_id, prev);
        ctx.ports[2].type = PORT_TYPE_SWITCH_LINK;
        snprintf(ctx.ports[2].peer_switch, sizeof(ctx.ports[2].peer_switch),
                 "s%d", prev);
        ctx.ports[2].configured = false;
        ctx.num_ports++;
        printf("  Port 2: %s (to switch %d - counter-clockwise)\n", ctx.ports[2].veth_name, prev);
        fflush(stdout);
    } 
    
    printf("Total ports configured: %d\n", ctx.num_ports);
    fflush(stdout);

    return 0;
}

static int init_eal_with_veth(int argc, char **argv)
{
    char *eal_argv[64];
    int eal_argc = 0;
    
    eal_argv[eal_argc++] = argv[0];
    eal_argv[eal_argc++] = "-l";
    
    static char lcore_mask[32];
    snprintf(lcore_mask, sizeof(lcore_mask), "%d", ctx.switch_id - 1);
    eal_argv[eal_argc++] = lcore_mask;
    
    eal_argv[eal_argc++] = "--proc-type";
    eal_argv[eal_argc++] = "primary";
    eal_argv[eal_argc++] = "--file-prefix";
    static char file_prefix[32];
    snprintf(file_prefix, sizeof(file_prefix), "switch%d", ctx.switch_id);
    eal_argv[eal_argc++] = file_prefix;
    eal_argv[eal_argc++] = "--no-huge";
    
    static char vdev_args[MAX_PORTS][128];
    for (int i = 0; i < ctx.num_ports; i++) {
        snprintf(vdev_args[i], sizeof(vdev_args[i]),
                 "--vdev=net_af_packet%d,iface=%s",
                 i, ctx.ports[i].veth_name);
        eal_argv[eal_argc++] = vdev_args[i];
        
        printf("Adding vdev: %s\n", vdev_args[i]);
        fflush(stdout);
    }
    
    printf("Switch %d will run on CPU core %d\n", ctx.switch_id, ctx.switch_id - 1);
    printf("Initializing EAL with %d arguments...\n", eal_argc);
    fflush(stdout);
    
    int ret = rte_eal_init(eal_argc, eal_argv);
    if (ret < 0) {
        fprintf(stderr, "ERROR: rte_eal_init() failed with return value %d\n", ret);
        fprintf(stderr, "EAL error: %s\n", rte_strerror(rte_errno));
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }
    
    printf("EAL initialized successfully (returned %d)\n", ret);
    fflush(stdout);
    
    // Immediately check how many ports DPDK found
    uint16_t found_ports = rte_eth_dev_count_avail();
    printf("DPDK found %u ethdev ports immediately after EAL init\n", found_ports);
    fflush(stdout);
    
    if (found_ports == 0) {
        fprintf(stderr, "ERROR: DPDK found 0 ports after initialization!\n");
        fprintf(stderr, "This usually means the vdev arguments were not processed correctly.\n");
        fprintf(stderr, "Checking if veth interfaces exist...\n");
        for (int i = 0; i < ctx.num_ports; i++) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "ip link show %s 2>&1", ctx.ports[i].veth_name);
            fprintf(stderr, "  %s: ", ctx.ports[i].veth_name);
            int result __attribute__((unused)) = system(cmd);
        }
        rte_exit(EXIT_FAILURE, "Cannot continue without ports\n");
    }
    
    return ret;
}

static int parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--switch-id") == 0 && i + 1 < argc) {
            ctx.switch_id = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--topology") == 0 && i + 1 < argc) {
            strncpy(ctx.topology, argv[i + 1], sizeof(ctx.topology) - 1);
            i++;
        } else if (strcmp(argv[i], "--num-switches") == 0 && i + 1 < argc) {
            ctx.num_switches = atoi(argv[i + 1]);
            i++;
        }
    }
    
    if (ctx.switch_id == 0) ctx.switch_id = 1;
    if (ctx.topology[0] == '\0') strcpy(ctx.topology, "ring");
    if (ctx.num_switches == 0) ctx.num_switches = 3;
    
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    
    printf("DOCA Three Port Switch with veth Integration v2\n");
    
    parse_args(argc, argv);
    configure_ports();
    
    printf("Switch %d configuration:\n", ctx.switch_id);
    printf("  Topology: %s\n", ctx.topology);
    printf("  Total switches: %d\n", ctx.num_switches);
    for (int i = 0; i < ctx.num_ports; i++) {
        printf("  Port %d: %s (%s)\n", i, ctx.ports[i].veth_name,
               ctx.ports[i].type == PORT_TYPE_HOST ? "host" : "switch-link");
    }
    
    ret = init_eal_with_veth(argc, argv);
    
    char pool_name[32];
    snprintf(pool_name, sizeof(pool_name), "MBUF_POOL_%d", ctx.switch_id);
    printf("DEBUG: Creating mbuf pool...\n"); 
    fflush(stdout);
    ctx.mbuf_pool = rte_pktmbuf_pool_create(pool_name, NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    if (ctx.mbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    uint16_t nb_ports = rte_eth_dev_count_avail();
    printf("DEBUG: Port count check\n"); 
    fflush(stdout);
    printf("Found %u DPDK ports (expected %d)\n", nb_ports, ctx.num_ports);
    fflush(stdout);
    
    if (nb_ports != ctx.num_ports) {
        printf("ERROR: Port count mismatch! Expected %d but found %u\n",
               ctx.num_ports, nb_ports);
        
        if (nb_ports == 0) {
            printf("FATAL: No DPDK ports found!\n");
            rte_exit(EXIT_FAILURE, "Port initialization failed\n");
        }
        
        if (nb_ports < ctx.num_ports) {
            printf("WARNING: Not all ports available (need %d, got %u)\n",
                   ctx.num_ports, nb_ports);
            printf("Checking veth interfaces:\n");
            for (int i = 0; i < ctx.num_ports; i++) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "ip link show %s 2>&1 | head -1", ctx.ports[i].veth_name);
                printf("  %s: ", ctx.ports[i].veth_name);
                fflush(stdout);
                int result __attribute__((unused)) = system(cmd);
            }
            // DON'T reduce ctx.num_ports here - we'll fail later when trying to configure missing ports
        }
    }

    // IMPORTANT: Check DPDK port enumeration order and match to veth interfaces
    uint16_t port_id;
    
    printf("\n=== DPDK Port Enumeration ===\n");
    RTE_ETH_FOREACH_DEV(port_id) {
        struct rte_ether_addr mac_addr;
        rte_eth_macaddr_get(port_id, &mac_addr);
        
        printf("DPDK port_id=%u has MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
               port_id,
               mac_addr.addr_bytes[0], mac_addr.addr_bytes[1],
               mac_addr.addr_bytes[2], mac_addr.addr_bytes[3],
               mac_addr.addr_bytes[4], mac_addr.addr_bytes[5]);
        fflush(stdout);
    }
    
    printf("\n=== Expected veth Interfaces ===\n");
    for (int i = 0; i < ctx.num_ports; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "cat /sys/class/net/%s/address", ctx.ports[i].veth_name);
        printf("port_idx=%d: %s MAC=", i, ctx.ports[i].veth_name);
        fflush(stdout);
        int result __attribute__((unused)) = system(cmd);
    }
    
    printf("\n=== Assigning Ports (in DPDK enumeration order) ===\n");
    int port_idx = 0;
    RTE_ETH_FOREACH_DEV(port_id) {
        if (port_idx < ctx.num_ports) {
            ctx.ports[port_idx].port_id = port_id;
            printf("Assigned port_idx=%d (%s) -> DPDK port_id=%u\n", 
                   port_idx, ctx.ports[port_idx].veth_name, port_id);
            fflush(stdout);
            port_idx++;
        }
    }
    
    // NOW load the MAC table (after port_ids are assigned)
    char mac_table_file[256];
    snprintf(mac_table_file, sizeof(mac_table_file), 
             "mac_tables/switch_%d_%s.txt", ctx.switch_id, ctx.topology);
    
    int loaded = load_mac_table_from_file(mac_table_file);
    if (loaded < 0) {
        printf("Warning: No static MAC table loaded, will use MAC learning\n");
    } else {
        printf("Static MAC forwarding enabled - no broadcasting needed!\n");
    }
    fflush(stdout);
    
    struct rte_eth_conf port_conf = {0};
    for (int i = 0; i < ctx.num_ports; i++) {
        uint16_t pid = ctx.ports[i].port_id;
        
        printf("Configuring port %u (veth: %s)...\n", pid, ctx.ports[i].veth_name);
        fflush(stdout);
        
        ret = rte_eth_dev_configure(pid, 1, 1, &port_conf);
        if (ret < 0) {
            printf("ERROR: Cannot configure device: err=%d, port=%u\n", ret, pid);
            continue;
        }
        
        ret = rte_eth_rx_queue_setup(pid, 0, RX_RING_SIZE,
                rte_eth_dev_socket_id(pid), NULL, ctx.mbuf_pool);
        if (ret < 0) {
            printf("ERROR: rte_eth_rx_queue_setup: err=%d, port=%u\n", ret, pid);
            continue;
        }
        
        ret = rte_eth_tx_queue_setup(pid, 0, TX_RING_SIZE,
                rte_eth_dev_socket_id(pid), NULL);
        if (ret < 0) {
            printf("ERROR: rte_eth_tx_queue_setup: err=%d, port=%u\n", ret, pid);
            continue;
        }
        
        ret = rte_eth_dev_start(pid);
        if (ret < 0) {
            printf("ERROR: rte_eth_dev_start: err=%d, port=%u\n", ret, pid);
            continue;
        }
        
        ret = rte_eth_promiscuous_enable(pid);
        if (ret != 0) {
            printf("Warning: Cannot enable promiscuous mode: err=%d, port=%u\n", ret, pid);
        }
        
        ctx.ports[i].configured = true;
        printf("Port %u started successfully\n", pid);
        fflush(stdout);
    }
    
    printf("DEBUG: Setting up signal handlers...\n"); 
    fflush(stdout);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("\nSwitch %d ready!\n", ctx.switch_id);
    fflush(stdout);
    
    printf("DEBUG: Entering main loop...\n"); 
    fflush(stdout);
    main_loop();
    
    printf("\nStopping switch %d...\n", ctx.switch_id);
    for (int i = 0; i < ctx.num_ports; i++) {
        if (!ctx.ports[i].configured) continue;
        
        printf("Stopping port %u...\n", ctx.ports[i].port_id);
        ret = rte_eth_dev_stop(ctx.ports[i].port_id);
        if (ret != 0) {
            printf("Warning: rte_eth_dev_stop: err=%d, port=%u\n",
                   ret, ctx.ports[i].port_id);
        }
        rte_eth_dev_close(ctx.ports[i].port_id);
    }
    
    printf("\nSwitch %d statistics:\n", ctx.switch_id);
    for (int i = 0; i < ctx.num_ports; i++) {
        if (!ctx.ports[i].configured) continue;
        printf("  Port %u (%s):\n", ctx.ports[i].port_id, ctx.ports[i].veth_name);
        printf("    RX: %lu packets\n", ctx.rx_packets[ctx.ports[i].port_id]);
        printf("    TX: %lu packets\n", ctx.tx_packets[ctx.ports[i].port_id]);
    }
    printf("  Dropped: %lu packets\n", ctx.dropped_packets);
    printf("  MAC entries learned: %lu\n", ctx.mac_learned);
    
    rte_eal_cleanup();
    
    return 0;
}