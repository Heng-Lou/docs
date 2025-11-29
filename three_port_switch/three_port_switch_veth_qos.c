/*
 * DPDK Three-Port Switch with QoS Support
 * Fixed port matching by creation order
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_cycles.h>

#define MAX_PORTS 3
#define MAC_TABLE_SIZE 1024
#define MAX_PKT_BURST 32
#define MEMPOOL_CACHE_SIZE 256
#define NUM_MBUFS 8191
#define NB_QOS_QUEUES 8
#define QOS_QUEUE_SIZE 512

struct qos_queue {
    struct rte_mbuf *packets[QOS_QUEUE_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    uint8_t priority;
    uint64_t enqueued;
    uint64_t dequeued;
    uint64_t dropped;
};

struct port_config {
    uint16_t port_id;
    char veth_name[64];
    bool configured;
    enum { PORT_TYPE_HOST, PORT_TYPE_SWITCH_LINK } type;
    struct qos_queue qos_queues[NB_QOS_QUEUES];
    uint64_t qos_classified;
};

struct mac_entry {
    struct rte_ether_addr mac;
    uint16_t port_id;
    bool valid;
};

struct switch_context {
    uint8_t switch_id;
    char topology[32];
    int num_switches;
    int num_ports;
    struct port_config ports[MAX_PORTS];
    struct rte_mempool *mbuf_pool;
    struct mac_entry mac_table[MAC_TABLE_SIZE];
    uint64_t rx_packets[MAX_PORTS];
    uint64_t tx_packets[MAX_PORTS];
    uint64_t dropped_packets;
    uint64_t qos_total_classified;
    uint64_t ttl_expired;
    volatile bool force_quit;
} ctx = {0};

static const uint8_t queue_weights[NB_QOS_QUEUES] = {
    1, 2, 4, 8, 16, 32, 64, 128
};

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n[Switch %d] Signal %d received, stopping...\n", 
               ctx.switch_id, signum);
        ctx.force_quit = 1;
    }
}

static int parse_args(int argc, char **argv)
{
    int opt;
    
    while ((opt = getopt(argc, argv, "s:t:n:")) != -1) {
        switch (opt) {
        case 's':
            ctx.switch_id = atoi(optarg);
            break;
        case 't':
            strncpy(ctx.topology, optarg, sizeof(ctx.topology) - 1);
            break;
        case 'n':
            ctx.num_switches = atoi(optarg);
            break;
        default:
            return -1;
        }
    }
    
    return (ctx.switch_id > 0 && ctx.num_switches > 0 && strlen(ctx.topology) > 0) ? 0 : -1;
}

static void configure_ports(void)
{
    /* Port 0: Always host port */
    ctx.ports[0].type = PORT_TYPE_HOST;
    snprintf(ctx.ports[0].veth_name, sizeof(ctx.ports[0].veth_name),
             "veth_s%d_h%d", ctx.switch_id, ctx.switch_id);
    
    if (strcmp(ctx.topology, "line") == 0) {
        if (ctx.switch_id == 1) {
            /* Switch 1: host + right */
            ctx.num_ports = 2;
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s1_s2");
        } else if (ctx.switch_id == ctx.num_switches) {
            /* Last switch: host + left */
            ctx.num_ports = 2;
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, ctx.switch_id - 1);
        } else {
            /* Middle: host + left + right */
            ctx.num_ports = 3;
            ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, ctx.switch_id - 1);
            ctx.ports[2].type = PORT_TYPE_SWITCH_LINK;
            snprintf(ctx.ports[2].veth_name, sizeof(ctx.ports[2].veth_name),
                     "veth_s%d_s%d", ctx.switch_id, ctx.switch_id + 1);
        }
    } else if (strcmp(ctx.topology, "ring") == 0) {
        ctx.num_ports = 3;
        int next = (ctx.switch_id % ctx.num_switches) + 1;
        int prev = (ctx.switch_id == 1) ? ctx.num_switches : (ctx.switch_id - 1);
        
        ctx.ports[1].type = PORT_TYPE_SWITCH_LINK;
        snprintf(ctx.ports[1].veth_name, sizeof(ctx.ports[1].veth_name),
                 "veth_s%d_s%d", ctx.switch_id, next);
        ctx.ports[2].type = PORT_TYPE_SWITCH_LINK;
        snprintf(ctx.ports[2].veth_name, sizeof(ctx.ports[2].veth_name),
                 "veth_s%d_s%d", ctx.switch_id, prev);
    }
    
    printf("[Switch %d] Port configuration:\n", ctx.switch_id);
    for (int i = 0; i < ctx.num_ports; i++) {
        printf("  Port %d: %s (%s)\n", i, ctx.ports[i].veth_name,
               ctx.ports[i].type == PORT_TYPE_HOST ? "host" : "link");
    }
}

static int init_eal_with_veth(int argc, char **argv)
{
    char *eal_argv[64];
    int eal_argc = 0;
    static char vdev_args[MAX_PORTS][256];
    static char lcore[32], prefix[64];
    
    eal_argv[eal_argc++] = argv[0];
    eal_argv[eal_argc++] = "-l";
    snprintf(lcore, sizeof(lcore), "%d", ctx.switch_id % 8);
    eal_argv[eal_argc++] = lcore;
    eal_argv[eal_argc++] = "--proc-type";
    eal_argv[eal_argc++] = "auto";
    eal_argv[eal_argc++] = "--file-prefix";
    snprintf(prefix, sizeof(prefix), "sw%d", ctx.switch_id);
    eal_argv[eal_argc++] = prefix;
    eal_argv[eal_argc++] = "--no-huge";
    eal_argv[eal_argc++] = "--no-pci";
    
    /* Create vdevs in the SAME ORDER as port configuration */
    for (int i = 0; i < ctx.num_ports; i++) {
        snprintf(vdev_args[i], sizeof(vdev_args[i]),
                 "--vdev=net_af_packet%d,iface=%s,blocksz=4096,framesz=2048,framecnt=512,qdisc_bypass=0",
                 i, ctx.ports[i].veth_name);
        eal_argv[eal_argc++] = vdev_args[i];
    }
    
    return rte_eal_init(eal_argc, eal_argv);
}

static int init_port(uint16_t port_id)
{
    struct rte_eth_conf port_conf = {0};
    int ret;
    
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0) return ret;
    
    ret = rte_eth_rx_queue_setup(port_id, 0, 256,
                                  rte_eth_dev_socket_id(port_id),
                                  NULL, ctx.mbuf_pool);
    if (ret < 0) return ret;
    
    ret = rte_eth_tx_queue_setup(port_id, 0, 256,
                                  rte_eth_dev_socket_id(port_id),
                                  NULL);
    if (ret < 0) return ret;
    
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) return ret;
    
    rte_eth_promiscuous_enable(port_id);
    return 0;
}

static void init_qos_queues(struct port_config *port)
{
    for (int i = 0; i < NB_QOS_QUEUES; i++) {
        port->qos_queues[i].head = 0;
        port->qos_queues[i].tail = 0;
        port->qos_queues[i].count = 0;
        port->qos_queues[i].priority = i;
        port->qos_queues[i].enqueued = 0;
        port->qos_queues[i].dequeued = 0;
        port->qos_queues[i].dropped = 0;
    }
}

static int load_mac_table_from_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    
    char line[256];
    int count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;
            
        unsigned int mac[6];
        int port_idx;
        
        if (sscanf(line, "%x:%x:%x:%x:%x:%x %d",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5],
                   &port_idx) == 7) {
            
            if (port_idx >= ctx.num_ports) continue;
            
            uint32_t hash = (mac[5] + (mac[4] << 8)) % MAC_TABLE_SIZE;
            
            for (int i = 0; i < 6; i++)
                ctx.mac_table[hash].mac.addr_bytes[i] = (uint8_t)mac[i];
            
            if (ctx.ports[port_idx].configured) {
                ctx.mac_table[hash].port_id = ctx.ports[port_idx].port_id;
                ctx.mac_table[hash].valid = true;
                count++;
            }
        }
    }
    
    fclose(fp);
    printf("[Switch %d] Loaded %d MAC entries\n", ctx.switch_id, count);
    return 0;
}

static uint8_t extract_qos_priority(struct rte_mbuf *m)
{
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
    uint16_t eth_type = rte_be_to_cpu_16(eth->ether_type);
    
    if (eth_type == RTE_ETHER_TYPE_IPV4) {
        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
        uint8_t dscp = (ip->type_of_service >> 2) & 0x3F;
        return (dscp >= 46) ? 7 : (dscp >= 32) ? 6 : (dscp >= 24) ? 5 :
               (dscp >= 16) ? 4 : (dscp >= 8) ? 3 : 0;
    }
    return 0;
}

static int qos_enqueue(struct qos_queue *q, struct rte_mbuf *m)
{
    if (q->count >= QOS_QUEUE_SIZE) {
        q->dropped++;
        rte_pktmbuf_free(m);
        return -1;
    }
    q->packets[q->tail] = m;
    q->tail = (q->tail + 1) % QOS_QUEUE_SIZE;
    q->count++;
    q->enqueued++;
    return 0;
}

static struct rte_mbuf *qos_dequeue(struct qos_queue *q)
{
    if (q->count == 0) return NULL;
    struct rte_mbuf *m = q->packets[q->head];
    q->head = (q->head + 1) % QOS_QUEUE_SIZE;
    q->count--;
    q->dequeued++;
    return m;
}

static uint16_t qos_schedule(struct port_config *port, struct rte_mbuf **pkts, uint16_t max_pkts)
{
    uint16_t nb_tx = 0;
    uint8_t credits[NB_QOS_QUEUES];
    memcpy(credits, queue_weights, sizeof(credits));
    
    bool done = false;
    while (!done && nb_tx < max_pkts) {
        done = true;
        for (int i = NB_QOS_QUEUES - 1; i >= 0 && nb_tx < max_pkts; i--) {
            if (credits[i] > 0 && port->qos_queues[i].count > 0) {
                pkts[nb_tx] = qos_dequeue(&port->qos_queues[i]);
                if (pkts[nb_tx]) {
                    nb_tx++;
                    credits[i]--;
                    done = false;
                }
            }
        }
    }
    return nb_tx;
}

static int process_packet_ttl(struct rte_mbuf *m)
{
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
    uint16_t eth_type = rte_be_to_cpu_16(eth->ether_type);
    
    if (eth_type == RTE_ETHER_TYPE_IPV4) {
        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
        if (ip->time_to_live <= 1) {
            ctx.ttl_expired++;
            return 0;
        }
        ip->time_to_live--;
        uint16_t cksum = ip->hdr_checksum + rte_cpu_to_be_16(0x0100);
        if (cksum < ip->hdr_checksum) cksum++;
        ip->hdr_checksum = cksum;
    }
    return 1;
}

static int lookup_mac(struct rte_ether_addr *mac, uint16_t *out_port_id)
{
    uint32_t hash = (mac->addr_bytes[5] + (mac->addr_bytes[4] << 8)) % MAC_TABLE_SIZE;
    
    if (ctx.mac_table[hash].valid &&
        rte_is_same_ether_addr(&ctx.mac_table[hash].mac, mac)) {
        *out_port_id = ctx.mac_table[hash].port_id;
        return 0;
    }
    return -1;
}

static int get_port_idx_by_dpdk_id(uint16_t dpdk_port_id)
{
    for (int i = 0; i < ctx.num_ports; i++) {
        if (ctx.ports[i].configured && ctx.ports[i].port_id == dpdk_port_id)
            return i;
    }
    return -1;
}

static void forward_packets_with_qos(uint16_t port_idx)
{
    struct rte_mbuf *pkts[MAX_PKT_BURST];
    uint16_t nb_rx = rte_eth_rx_burst(ctx.ports[port_idx].port_id, 0, pkts, MAX_PKT_BURST);
    
    if (nb_rx == 0) return;
    
    ctx.rx_packets[port_idx] += nb_rx;
    
    for (uint16_t i = 0; i < nb_rx; i++) {
        struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkts[i], struct rte_ether_hdr *);
        
        if (!process_packet_ttl(pkts[i])) {
            rte_pktmbuf_free(pkts[i]);
            ctx.dropped_packets++;
            continue;
        }
        
        uint8_t qos = extract_qos_priority(pkts[i]);
        ctx.qos_total_classified++;
        
        uint16_t out_port_id;
        if (lookup_mac(&eth->dst_addr, &out_port_id) != 0) {
            rte_pktmbuf_free(pkts[i]);
            ctx.dropped_packets++;
            continue;
        }
        
        int out_idx = get_port_idx_by_dpdk_id(out_port_id);
        if (out_idx < 0 || qos_enqueue(&ctx.ports[out_idx].qos_queues[qos], pkts[i]) < 0) {
            ctx.dropped_packets++;
        }
    }
}

static void transmit_from_qos_queues(uint16_t port_idx)
{
    struct rte_mbuf *pkts[MAX_PKT_BURST];
    uint16_t nb = qos_schedule(&ctx.ports[port_idx], pkts, MAX_PKT_BURST);
    
    if (nb == 0) return;
    
    uint16_t nb_tx = rte_eth_tx_burst(ctx.ports[port_idx].port_id, 0, pkts, nb);
    ctx.tx_packets[port_idx] += nb_tx;
    
    for (uint16_t i = nb_tx; i < nb; i++) {
        rte_pktmbuf_free(pkts[i]);
        ctx.dropped_packets++;
    }
}

static void display_stats(void)
{
    static uint64_t last = 0;
    uint64_t now = rte_get_timer_cycles();
    
    if ((now - last) < rte_get_timer_hz() * 10) return;
    last = now;
    
    printf("\n[Switch %d] RX=%lu TX=%lu QoS=%lu Drop=%lu TTL=%lu\n",
           ctx.switch_id,
           ctx.rx_packets[0] + ctx.rx_packets[1] + ctx.rx_packets[2],
           ctx.tx_packets[0] + ctx.tx_packets[1] + ctx.tx_packets[2],
           ctx.qos_total_classified, ctx.dropped_packets, ctx.ttl_expired);
    fflush(stdout);
}

static int lcore_main(__rte_unused void *arg)
{
    printf("[Switch %d] Packet processing started\n", ctx.switch_id);
    
    while (!ctx.force_quit) {
        for (int i = 0; i < ctx.num_ports; i++) {
            if (!ctx.ports[i].configured) continue;
            forward_packets_with_qos(i);
            transmit_from_qos_queues(i);
        }
        display_stats();
    }
    
    printf("[Switch %d] Stopped\n", ctx.switch_id);
    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (parse_args(argc, argv) < 0)
        return EXIT_FAILURE;
    
    configure_ports();
    
    if (init_eal_with_veth(argc, argv) < 0)
        return EXIT_FAILURE;
    
    char pool_name[64];
    snprintf(pool_name, sizeof(pool_name), "MBUF_SW%d", ctx.switch_id);
    
    ctx.mbuf_pool = rte_pktmbuf_pool_create(pool_name,
        NUM_MBUFS, MEMPOOL_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    if (!ctx.mbuf_pool)
        return EXIT_FAILURE;
    
    /* CRITICAL FIX: Map DPDK ports by ORDER, not by name matching!
     * AF_PACKET PMD creates ports as net_af_packet0, net_af_packet1, etc.
     * in the SAME ORDER as vdev arguments, so port index = DPDK port ID
     */
    uint16_t port_id;
    int nb_ports = 0;
    
    RTE_ETH_FOREACH_DEV(port_id) {
        if (port_id >= ctx.num_ports) {
            printf("[Switch %d] Warning: Extra port %u found, ignoring\n",
                   ctx.switch_id, port_id);
            continue;
        }
        
        /* Direct mapping: DPDK port ID = our port index */
        ctx.ports[port_id].port_id = port_id;
        ctx.ports[port_id].configured = true;
        
        if (init_port(port_id) < 0) {
            fprintf(stderr, "[Switch %d] Port %u init failed\n", 
                    ctx.switch_id, port_id);
            return EXIT_FAILURE;
        }
        
        init_qos_queues(&ctx.ports[port_id]);
        
        printf("[Switch %d] ✓ Port %u: %s\n", 
               ctx.switch_id, port_id, ctx.ports[port_id].veth_name);
        
        nb_ports++;
    }
    
    if (nb_ports != ctx.num_ports) {
        fprintf(stderr, "[Switch %d] ERROR: Expected %d ports but found %d\n",
                ctx.switch_id, ctx.num_ports, nb_ports);
        return EXIT_FAILURE;
    }
    
    char mac_file[256];
    snprintf(mac_file, sizeof(mac_file),
             "mac_tables/switch_%d_%s.txt", ctx.switch_id, ctx.topology);
    load_mac_table_from_file(mac_file);
    
    printf("[Switch %d] ✓ Ready: %d ports, %d QoS queues/port\n",
           ctx.switch_id, ctx.num_ports, NB_QOS_QUEUES);
    
    lcore_main(NULL);
    
    rte_eal_cleanup();
    return 0;
}
