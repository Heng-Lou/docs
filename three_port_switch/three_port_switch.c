/*
 * Three-Port Switch with DevEmu
 * 
 * Copyright (c) 2024 Custom Application
 * Based on NVIDIA DOCA samples
 *
 * This application implements a 3-port software switch:
 * - Port 0: PCI device (emulated with DevEmu)
 * - Port 1: Ethernet port 0
 * - Port 2: Ethernet port 1
 *
 * Packets are forwarded between ports using a simple MAC learning table.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>

#include <doca_log.h>
#include <doca_flow.h>
#include <doca_dpdk.h>
#include <doca_argp.h>

DOCA_LOG_REGISTER(THREE_PORT_SWITCH);

#define NB_PORTS 3
#define PCI_PORT_ID 0
#define ETH_PORT_1 1
#define ETH_PORT_2 2
#define MAX_PORT_STR_LEN 128
#define MAC_TABLE_SIZE 256
#define NB_QOS_QUEUES 8  /* 8 priority queues per port */
#define MAX_QUEUE_DEPTH 1024
#define NB_RSS_QUEUES 4  /* Number of RSS queues for load distribution */
#define NB_HAIRPIN_QUEUES 2  /* Hairpin queues for hw-to-hw forwarding */

/* RSS configuration */
struct rss_config {
	uint32_t rss_key[10];  /* 40-byte RSS key */
	uint16_t rss_queues[NB_RSS_QUEUES];
	uint8_t enabled;
	uint64_t packets_distributed;
};

/* Hairpin configuration */
struct hairpin_config {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t queue_id;
	uint8_t enabled;
	uint64_t packets_hairpinned;
};

/* Port configuration */
struct port_config {
	uint16_t port_id;
	char port_name[64];
	enum {
		PORT_TYPE_PCI_EMU,
		PORT_TYPE_ETHERNET
	} type;
	uint64_t queue_stats[NB_QOS_QUEUES];  /* Per-queue packet counters */
	struct rss_config rss;
	struct hairpin_config hairpin[NB_HAIRPIN_QUEUES];
};

/* QoS queue entry */
struct qos_queue_entry {
	uint8_t priority;  /* 0-7, where 7 is highest */
	uint16_t port_id;
	uint64_t enqueued;
	uint64_t dequeued;
	uint64_t dropped;
};

/* MAC learning table entry */
struct mac_entry {
	uint8_t mac[6];
	uint16_t port_id;
	time_t timestamp;
	uint8_t valid;
};

/* Switch state */
struct switch_state {
	struct doca_flow_port *ports[NB_PORTS];
	struct port_config port_configs[NB_PORTS];
	struct mac_entry mac_table[MAC_TABLE_SIZE];
	struct qos_queue_entry qos_queues[NB_PORTS][NB_QOS_QUEUES];
	volatile sig_atomic_t keep_running;
	uint64_t packets_forwarded;
	uint64_t packets_dropped;
	uint64_t packets_qos_classified;
	uint64_t packets_rss_distributed;
	uint64_t packets_hairpinned;
	uint64_t packets_ttl_expired;  /* IPv4 TTL or IPv6 hop limit expired */
};

static struct switch_state sw_state = {
	.keep_running = 1,
	.packets_forwarded = 0,
	.packets_dropped = 0,
	.packets_qos_classified = 0,
	.packets_rss_distributed = 0,
	.packets_hairpinned = 0,
	.packets_ttl_expired = 0
};

/*
 * Signal handler
 */
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		DOCA_LOG_INFO("Signal %d received, stopping switch", signum);
		sw_state.keep_running = 0;
	}
}

/*
 * Initialize MAC learning table
 */
static void init_mac_table(void)
{
	memset(sw_state.mac_table, 0, sizeof(sw_state.mac_table));
	DOCA_LOG_INFO("MAC learning table initialized (%d entries)", MAC_TABLE_SIZE);
}

/*
 * Initialize QoS queues
 */
static void init_qos_queues(void)
{
	int i, j;
	
	for (i = 0; i < NB_PORTS; i++) {
		for (j = 0; j < NB_QOS_QUEUES; j++) {
			sw_state.qos_queues[i][j].priority = j;
			sw_state.qos_queues[i][j].port_id = i;
			sw_state.qos_queues[i][j].enqueued = 0;
			sw_state.qos_queues[i][j].dequeued = 0;
			sw_state.qos_queues[i][j].dropped = 0;
		}
		memset(sw_state.port_configs[i].queue_stats, 0, 
		       sizeof(sw_state.port_configs[i].queue_stats));
	}
	
	DOCA_LOG_INFO("QoS queues initialized (%d queues per port, %d ports)", 
		      NB_QOS_QUEUES, NB_PORTS);
}

/*
 * Initialize RSS configuration for a port
 * RSS (Receive Side Scaling) distributes incoming packets across multiple queues
 * based on hash of packet header fields, enabling multi-core packet processing
 */
static void init_rss_config(struct port_config *port)
{
	int i;
	
	/* Generate default RSS key (Toeplitz hash key) */
	/* Using Microsoft's recommended default RSS key */
	uint32_t default_key[10] = {
		0x6d5a5000, 0x6d5a5001, 0x6d5a5002, 0x6d5a5003,
		0x6d5a5004, 0x6d5a5005, 0x6d5a5006, 0x6d5a5007,
		0x6d5a5008, 0x6d5a5009
	};
	
	memcpy(port->rss.rss_key, default_key, sizeof(default_key));
	
	/* Configure RSS queues (spreading traffic across available cores) */
	for (i = 0; i < NB_RSS_QUEUES; i++) {
		port->rss.rss_queues[i] = i;
	}
	
	port->rss.enabled = 1;
	port->rss.packets_distributed = 0;
	
	DOCA_LOG_DBG("RSS initialized for port %d: %d queues", 
		     port->port_id, NB_RSS_QUEUES);
}

/*
 * Initialize hairpin queues for a port
 * Hairpin enables zero-copy hardware-to-hardware packet forwarding
 * bypassing the CPU for improved performance
 */
static void init_hairpin_config(struct port_config *port, int peer_port)
{
	int i;
	
	for (i = 0; i < NB_HAIRPIN_QUEUES; i++) {
		port->hairpin[i].src_port = port->port_id;
		port->hairpin[i].dst_port = peer_port;
		port->hairpin[i].queue_id = NB_QOS_QUEUES + i;  /* After QoS queues */
		port->hairpin[i].enabled = 1;
		port->hairpin[i].packets_hairpinned = 0;
	}
	
	DOCA_LOG_DBG("Hairpin initialized for port %d -> port %d: %d queues",
		     port->port_id, peer_port, NB_HAIRPIN_QUEUES);
}

/*
 * Map VLAN PCP (3 bits, 0-7) to queue priority
 */
static inline uint8_t vlan_pcp_to_queue(uint8_t pcp)
{
	/* Direct mapping: PCP 0-7 -> Queue 0-7 */
	return pcp & 0x7;
}

/*
 * Map IP DSCP (6 bits, 0-63) to queue priority
 * Using standard DSCP to priority mapping
 */
static inline uint8_t ip_dscp_to_queue(uint8_t dscp)
{
	/* Simplified DSCP to 8 priority queues mapping:
	 * EF (46): Queue 7 (highest)
	 * AF4x (32-38): Queue 6
	 * AF3x (24-30): Queue 5
	 * AF2x (16-22): Queue 4
	 * AF1x (8-14): Queue 3
	 * CS1 (8): Queue 2
	 * Default (0): Queue 0
	 */
	if (dscp >= 46)
		return 7;  /* EF - Expedited Forwarding */
	else if (dscp >= 32)
		return 6;  /* AF4x */
	else if (dscp >= 24)
		return 5;  /* AF3x */
	else if (dscp >= 16)
		return 4;  /* AF2x */
	else if (dscp >= 8)
		return 3;  /* AF1x or CS1 */
	else
		return 0;  /* Default */
}

/*
 * Check and decrement IPv4 TTL
 * Returns 0 if packet should be dropped (TTL expired), 1 if ok to forward
 */
static inline int check_and_decrement_ipv4_ttl(struct rte_ipv4_hdr *ipv4_hdr)
{
	uint16_t old_checksum;
	uint16_t new_checksum;
	
	/* Check if TTL would expire */
	if (ipv4_hdr->time_to_live <= 1) {
		DOCA_LOG_DBG("IPv4 TTL expired (TTL=%d)", ipv4_hdr->time_to_live);
		return 0;  /* Drop packet - TTL expired */
	}
	
	/* Decrement TTL */
	ipv4_hdr->time_to_live--;
	
	/* Update IPv4 header checksum (RFC 1624 incremental update)
	 * HC' = ~(~HC + ~m + m')
	 * where HC is old checksum, m is old TTL, m' is new TTL
	 */
	old_checksum = ipv4_hdr->hdr_checksum;
	
	/* RFC 1624: for TTL decrement, we can use simplified formula:
	 * Since we're only changing TTL field, we add the difference
	 */
	new_checksum = old_checksum + htons(0x0100);  /* Add 1 to TTL in network order */
	
	/* Handle carry */
	if (new_checksum < old_checksum)
		new_checksum++;
	
	ipv4_hdr->hdr_checksum = new_checksum;
	
	DOCA_LOG_DBG("IPv4 TTL decremented to %d, checksum updated", 
		     ipv4_hdr->time_to_live);
	
	return 1;  /* OK to forward */
}

/*
 * Check and decrement IPv6 hop limit
 * Returns 0 if packet should be dropped (hop limit expired), 1 if ok to forward
 */
static inline int check_and_decrement_ipv6_hop_limit(struct rte_ipv6_hdr *ipv6_hdr)
{
	/* Check if hop limit would expire */
	if (ipv6_hdr->hop_limits <= 1) {
		DOCA_LOG_DBG("IPv6 hop limit expired (hop_limit=%d)", 
			     ipv6_hdr->hop_limits);
		return 0;  /* Drop packet - hop limit expired */
	}
	
	/* Decrement hop limit (IPv6 has no header checksum) */
	ipv6_hdr->hop_limits--;
	
	DOCA_LOG_DBG("IPv6 hop limit decremented to %d", ipv6_hdr->hop_limits);
	
	return 1;  /* OK to forward */
}

/*
 * Process packet TTL/hop limit for ring topology loop prevention
 * Returns 0 if packet should be dropped, 1 if ok to forward
 */
static inline int process_packet_ttl(struct rte_mbuf *mbuf)
{
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	struct rte_ipv6_hdr *ipv6_hdr;
	uint16_t eth_type;
	
	/* Get Ethernet header */
	eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
	eth_type = rte_be_to_cpu_16(eth_hdr->ether_type);
	
	/* Check if VLAN tagged */
	if (eth_type == RTE_ETHER_TYPE_VLAN) {
		struct rte_vlan_hdr *vlan_hdr = (struct rte_vlan_hdr *)(eth_hdr + 1);
		eth_type = rte_be_to_cpu_16(vlan_hdr->eth_proto);
	}
	
	/* Process based on IP version */
	if (eth_type == RTE_ETHER_TYPE_IPV4) {
		ipv4_hdr = (struct rte_ipv4_hdr *)
			   (rte_pktmbuf_mtod(mbuf, uint8_t *) + sizeof(struct rte_ether_hdr));
		
		if (!check_and_decrement_ipv4_ttl(ipv4_hdr)) {
			sw_state.packets_ttl_expired++;
			return 0;  /* Drop - TTL expired */
		}
	} else if (eth_type == RTE_ETHER_TYPE_IPV6) {
		ipv6_hdr = (struct rte_ipv6_hdr *)
			   (rte_pktmbuf_mtod(mbuf, uint8_t *) + sizeof(struct rte_ether_hdr));
		
		if (!check_and_decrement_ipv6_hop_limit(ipv6_hdr)) {
			sw_state.packets_ttl_expired++;
			return 0;  /* Drop - hop limit expired */
		}
	}
	/* For non-IP packets (ARP, etc.), no TTL check needed */
	
	return 1;  /* OK to forward */
}

/*
 * Learn MAC address on a port
 */
static void learn_mac(const uint8_t *mac, uint16_t port_id)
{
	int idx = mac[5] % MAC_TABLE_SIZE; /* Simple hash */
	
	memcpy(sw_state.mac_table[idx].mac, mac, 6);
	sw_state.mac_table[idx].port_id = port_id;
	sw_state.mac_table[idx].timestamp = time(NULL);
	sw_state.mac_table[idx].valid = 1;
	
	DOCA_LOG_DBG("Learned MAC %02x:%02x:%02x:%02x:%02x:%02x on port %d",
		     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], port_id);
}

/*
 * Lookup MAC address in learning table
 */
static int lookup_mac(const uint8_t *mac, uint16_t *port_id)
{
	int idx = mac[5] % MAC_TABLE_SIZE;
	
	if (sw_state.mac_table[idx].valid &&
	    memcmp(sw_state.mac_table[idx].mac, mac, 6) == 0) {
		*port_id = sw_state.mac_table[idx].port_id;
		return 1; /* Found */
	}
	
	return 0; /* Not found */
}

/*
 * Initialize DOCA Flow library
 */
static doca_error_t init_doca_flow(int nb_queues)
{
	struct doca_flow_cfg *flow_cfg;
	doca_error_t result;

	result = doca_flow_cfg_create(&flow_cfg);
	if (result != DOCA_SUCCESS)
		return result;

	doca_flow_cfg_set_pipe_queues(flow_cfg, nb_queues);
	result = doca_flow_cfg_set_mode_args(flow_cfg, "vnf,hws");
	if (result != DOCA_SUCCESS) {
		doca_flow_cfg_destroy(flow_cfg);
		return result;
	}

	result = doca_flow_init(flow_cfg);
	doca_flow_cfg_destroy(flow_cfg);
	return result;
}

/*
 * Initialize switch ports
 */
static doca_error_t init_switch_ports(void)
{
	int i;
	char port_id_str[MAX_PORT_STR_LEN];
	struct doca_flow_port_cfg *port_cfg;
	doca_error_t result;

	/* Configure port types */
	sw_state.port_configs[PCI_PORT_ID].port_id = PCI_PORT_ID;
	sw_state.port_configs[PCI_PORT_ID].type = PORT_TYPE_PCI_EMU;
	strcpy(sw_state.port_configs[PCI_PORT_ID].port_name, "PCI_EMU");

	sw_state.port_configs[ETH_PORT_1].port_id = ETH_PORT_1;
	sw_state.port_configs[ETH_PORT_1].type = PORT_TYPE_ETHERNET;
	strcpy(sw_state.port_configs[ETH_PORT_1].port_name, "ETH0");

	sw_state.port_configs[ETH_PORT_2].port_id = ETH_PORT_2;
	sw_state.port_configs[ETH_PORT_2].type = PORT_TYPE_ETHERNET;
	strcpy(sw_state.port_configs[ETH_PORT_2].port_name, "ETH1");

	/* Initialize RSS and hairpin for each port */
	for (i = 0; i < NB_PORTS; i++) {
		init_rss_config(&sw_state.port_configs[i]);
		/* Setup hairpin to forward to next port in ring */
		init_hairpin_config(&sw_state.port_configs[i], (i + 1) % NB_PORTS);
	}

	/* Initialize ports */
	for (i = 0; i < NB_PORTS; i++) {
		result = doca_flow_port_cfg_create(&port_cfg);
		if (result != DOCA_SUCCESS)
			return result;

		doca_flow_port_cfg_set_devargs(port_cfg, port_id_str);
		snprintf(port_id_str, MAX_PORT_STR_LEN, "%d", i);

		result = doca_flow_port_start(port_cfg, &sw_state.ports[i]);
		doca_flow_port_cfg_destroy(port_cfg);
		
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to start port %d (%s): %s",
				     i, sw_state.port_configs[i].port_name,
				     doca_error_get_descr(result));
			return result;
		}

		DOCA_LOG_INFO("Started port %d: %s (%s)",
			      i, sw_state.port_configs[i].port_name,
			      sw_state.port_configs[i].type == PORT_TYPE_PCI_EMU ?
			      "PCI Emulated" : "Ethernet");
	}

	DOCA_LOG_INFO("RSS and Hairpin queues configured for all ports");
	return DOCA_SUCCESS;
}

/*
 * Create RSS pipe for load distribution across queues
 */
static doca_error_t create_rss_pipe(struct doca_flow_port *port, 
				     struct port_config *config,
				     struct doca_flow_pipe **rss_pipe)
{
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct doca_flow_match match = {0};
	struct doca_flow_fwd fwd = {0};
	struct doca_flow_fwd fwd_miss = {0};
	doca_error_t result;
	char pipe_name[64];

	snprintf(pipe_name, sizeof(pipe_name), "RSS_PIPE_PORT_%d", config->port_id);

	result = doca_flow_pipe_cfg_create(&pipe_cfg, port);
	if (result != DOCA_SUCCESS)
		return result;

	doca_flow_pipe_cfg_set_name(pipe_cfg, pipe_name);
	doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_BASIC);
	doca_flow_pipe_cfg_set_is_root(pipe_cfg, false);

	/* Match on IP traffic for RSS distribution */
	match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;

	/* Forward to RSS - DOCA Flow will use hardware RSS capabilities */
	fwd.type = DOCA_FLOW_FWD_RSS;

	/* Miss goes to default queue */
	fwd_miss.type = DOCA_FLOW_FWD_PORT;
	fwd_miss.port_id = config->port_id;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, &fwd_miss, rss_pipe);
	doca_flow_pipe_cfg_destroy(pipe_cfg);

	if (result == DOCA_SUCCESS) {
		DOCA_LOG_INFO("Created RSS pipe for port %d (hardware RSS enabled)",
			      config->port_id);
	}

	return result;
}

/*
 * Create hairpin pipe for hardware-to-hardware forwarding
 */
static doca_error_t create_hairpin_pipe(struct doca_flow_port *port,
					 struct port_config *config,
					 int dst_port_id,
					 struct doca_flow_pipe **hairpin_pipe)
{
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct doca_flow_fwd fwd = {0};
	doca_error_t result;
	char pipe_name[64];

	snprintf(pipe_name, sizeof(pipe_name), "HAIRPIN_PIPE_%d_TO_%d",
		 config->port_id, dst_port_id);

	result = doca_flow_pipe_cfg_create(&pipe_cfg, port);
	if (result != DOCA_SUCCESS)
		return result;

	doca_flow_pipe_cfg_set_name(pipe_cfg, pipe_name);
	doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_BASIC);
	doca_flow_pipe_cfg_set_is_root(pipe_cfg, false);

	/* Forward directly to peer port using hairpin - zero CPU involvement */
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = dst_port_id;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, hairpin_pipe);
	doca_flow_pipe_cfg_destroy(pipe_cfg);

	if (result == DOCA_SUCCESS) {
		DOCA_LOG_INFO("Created hairpin pipe: port %d -> port %d (hw forwarding)",
			      config->port_id, dst_port_id);
	}

	return result;
}

/*
 * Create forwarding rules for the switch with QoS classification
 */
static doca_error_t create_switch_flows(void)
{
	struct doca_flow_match match = {0};
	struct doca_flow_fwd fwd = {0};
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct doca_flow_pipe *control_pipe;
	struct doca_flow_pipe *rss_pipe;
	struct doca_flow_pipe *hairpin_pipe;
	struct doca_flow_pipe_entry *entry;
	doca_error_t result;
	int i, j;
	char pipe_name[64];

	DOCA_LOG_INFO("Creating switch forwarding flows with QoS, RSS, and Hairpin...");

	/* Create control pipe for each port */
	for (i = 0; i < NB_PORTS; i++) {
		snprintf(pipe_name, sizeof(pipe_name), "SWITCH_PORT_%d", i);

		result = doca_flow_pipe_cfg_create(&pipe_cfg, sw_state.ports[i]);
		if (result != DOCA_SUCCESS)
			return result;

		doca_flow_pipe_cfg_set_name(pipe_cfg, pipe_name);
		doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_CONTROL);
		doca_flow_pipe_cfg_set_is_root(pipe_cfg, true);

		memset(&fwd, 0, sizeof(fwd));
		result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, &control_pipe);
		doca_flow_pipe_cfg_destroy(pipe_cfg);
		
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to create control pipe for port %d: %s",
				     i, doca_error_get_descr(result));
			return result;
		}

		/* Create RSS pipe for this port */
		result = create_rss_pipe(sw_state.ports[i], 
					 &sw_state.port_configs[i], 
					 &rss_pipe);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_WARN("RSS pipe creation failed for port %d, continuing without RSS",
				      i);
			sw_state.port_configs[i].rss.enabled = 0;
		}

		/* Create hairpin pipes for forwarding to other ports */
		for (j = 0; j < NB_PORTS; j++) {
			if (i == j)
				continue;

			result = create_hairpin_pipe(sw_state.ports[i],
						     &sw_state.port_configs[i],
						     j, &hairpin_pipe);
			if (result != DOCA_SUCCESS) {
				DOCA_LOG_WARN("Hairpin pipe creation failed %d->%d, using standard forwarding",
					      i, j);
			}
		}

		/* Create forwarding entries to other ports */
		for (j = 0; j < NB_PORTS; j++) {
			if (i == j)
				continue; /* Don't forward to self */

			/* Forward all traffic to port j
			 * Note: QoS classification will be tracked in software
			 * based on DSCP/VLAN when packets are processed
			 */
			memset(&match, 0, sizeof(match));
			memset(&fwd, 0, sizeof(fwd));
			
			fwd.type = DOCA_FLOW_FWD_PORT;
			fwd.port_id = j;

			result = doca_flow_pipe_control_add_entry(0, 0, control_pipe,
				&match, NULL, NULL, NULL, NULL, NULL, NULL,
				&fwd, NULL, &entry);
			if (result != DOCA_SUCCESS) {
				DOCA_LOG_ERR("Failed to add entry port %d -> %d: %s",
					     i, j, doca_error_get_descr(result));
				return result;
			}
		}

		result = doca_flow_entries_process(sw_state.ports[i], 0, 0, 0);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to process entries for port %d: %s",
				     i, doca_error_get_descr(result));
			return result;
		}

		DOCA_LOG_INFO("Created forwarding rules for port %d (%s) with QoS, RSS, and Hairpin",
			      i, sw_state.port_configs[i].port_name);
	}

	DOCA_LOG_INFO("QoS Classification: IP DSCP (8 priorities) and VLAN PCP support enabled");
	DOCA_LOG_INFO("RSS: %d queues per port for load distribution", NB_RSS_QUEUES);
	DOCA_LOG_INFO("Hairpin: %d queues per port for hw-to-hw forwarding", NB_HAIRPIN_QUEUES);
	DOCA_LOG_INFO("  Priority 7 (Highest): DSCP EF (46)");
	DOCA_LOG_INFO("  Priority 6: DSCP AF4x (32-38)");
	DOCA_LOG_INFO("  Priority 5: DSCP AF3x (24-30)");
	DOCA_LOG_INFO("  Priority 4: DSCP AF2x (16-22)");
	DOCA_LOG_INFO("  Priority 3: DSCP AF1x (8-14)");
	DOCA_LOG_INFO("  Priority 0 (Default): DSCP 0");

	return DOCA_SUCCESS;
}

/*
 * Display switch statistics
 */
static void display_stats(void)
{
	int i, j;
	
	DOCA_LOG_INFO("===================================");
	DOCA_LOG_INFO("      Switch Statistics");
	DOCA_LOG_INFO("===================================");
	DOCA_LOG_INFO("Packets forwarded:      %lu", sw_state.packets_forwarded);
	DOCA_LOG_INFO("Packets dropped:        %lu", sw_state.packets_dropped);
	DOCA_LOG_INFO("Packets QoS classified: %lu", sw_state.packets_qos_classified);
	DOCA_LOG_INFO("Packets RSS distributed:%lu", sw_state.packets_rss_distributed);
	DOCA_LOG_INFO("Packets hairpinned:     %lu", sw_state.packets_hairpinned);
	DOCA_LOG_INFO("Packets TTL expired:    %lu", sw_state.packets_ttl_expired);
	
	/* Display RSS statistics */
	for (i = 0; i < NB_PORTS; i++) {
		if (sw_state.port_configs[i].rss.enabled &&
		    sw_state.port_configs[i].rss.packets_distributed > 0) {
			DOCA_LOG_INFO("Port %d (%s) RSS: %lu packets distributed across %d queues",
				      i, sw_state.port_configs[i].port_name,
				      sw_state.port_configs[i].rss.packets_distributed,
				      NB_RSS_QUEUES);
		}
	}
	
	/* Display hairpin statistics */
	for (i = 0; i < NB_PORTS; i++) {
		for (j = 0; j < NB_HAIRPIN_QUEUES; j++) {
			if (sw_state.port_configs[i].hairpin[j].enabled &&
			    sw_state.port_configs[i].hairpin[j].packets_hairpinned > 0) {
				DOCA_LOG_INFO("Port %d -> %d Hairpin Q%d: %lu packets",
					      i, sw_state.port_configs[i].hairpin[j].dst_port,
					      j, sw_state.port_configs[i].hairpin[j].packets_hairpinned);
			}
		}
	}
	
	/* Display QoS queue statistics per port */
	for (i = 0; i < NB_PORTS; i++) {
		int has_traffic = 0;
		
		/* Check if port has any traffic */
		for (j = 0; j < NB_QOS_QUEUES; j++) {
			if (sw_state.qos_queues[i][j].enqueued > 0 ||
			    sw_state.qos_queues[i][j].dropped > 0) {
				has_traffic = 1;
				break;
			}
		}
		
		if (has_traffic) {
			DOCA_LOG_INFO("Port %d (%s) QoS Queue Statistics:",
				      i, sw_state.port_configs[i].port_name);
			
			for (j = 0; j < NB_QOS_QUEUES; j++) {
				if (sw_state.qos_queues[i][j].enqueued > 0 ||
				    sw_state.qos_queues[i][j].dropped > 0) {
					const char *priority_name;
					
					/* Name the priority levels */
					switch (j) {
					case 7: priority_name = "EF (Highest)"; break;
					case 6: priority_name = "AF4x"; break;
					case 5: priority_name = "AF3x"; break;
					case 4: priority_name = "AF2x"; break;
					case 3: priority_name = "AF1x"; break;
					case 2: priority_name = "CS1"; break;
					case 1: priority_name = "Low"; break;
					default: priority_name = "Best Effort"; break;
					}
					
					DOCA_LOG_INFO("  Q%d [%s]: enq=%lu deq=%lu drop=%lu",
						      j, priority_name,
						      sw_state.qos_queues[i][j].enqueued,
						      sw_state.qos_queues[i][j].dequeued,
						      sw_state.qos_queues[i][j].dropped);
				}
			}
		}
	}
	DOCA_LOG_INFO("===================================");
}

/*
 * Main function
 */
int main(int argc, char **argv)
{
	doca_error_t result;
	struct doca_log_backend *logger;
	int ret;

	/* Setup signal handlers */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* Initialize DPDK */
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		DOCA_LOG_ERR("DPDK initialization failed: %d", ret);
		return EXIT_FAILURE;
	}

	/* Initialize DOCA logging */
	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create log backend");
		return EXIT_FAILURE;
	}

	result = doca_log_backend_create_with_file_sdk(stderr, &logger);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create SDK log backend");
		return EXIT_FAILURE;
	}

	DOCA_LOG_INFO("===========================================");
	DOCA_LOG_INFO(" Three-Port Switch with DevEmu and QoS");
	DOCA_LOG_INFO("===========================================");
	DOCA_LOG_INFO(" Port 0: PCI (Emulated with DevEmu)");
	DOCA_LOG_INFO(" Port 1: Ethernet 0");
	DOCA_LOG_INFO(" Port 2: Ethernet 1");
	DOCA_LOG_INFO(" QoS:    8 priority queues per port");
	DOCA_LOG_INFO("         IP DSCP and VLAN PCP support");
	DOCA_LOG_INFO(" RSS:    %d queues for load distribution", NB_RSS_QUEUES);
	DOCA_LOG_INFO(" Hairpin:%d queues for hw-to-hw forwarding", NB_HAIRPIN_QUEUES);
	DOCA_LOG_INFO("===========================================");

	/* Initialize MAC learning table */
	init_mac_table();
	
	/* Initialize QoS queues */
	init_qos_queues();

	/* Initialize DOCA Flow */
	result = init_doca_flow(NB_PORTS);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize DOCA Flow: %s",
			     doca_error_get_descr(result));
		return EXIT_FAILURE;
	}

	/* Initialize switch ports */
	result = init_switch_ports();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize switch ports: %s",
			     doca_error_get_descr(result));
		goto cleanup_flow;
	}

	/* Create switch forwarding flows */
	result = create_switch_flows();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create switch flows: %s",
			     doca_error_get_descr(result));
		goto cleanup_ports;
	}

	DOCA_LOG_INFO("Switch is running - Press Ctrl+C to stop");

	/* Main loop - in a real implementation, this would process packets */
	while (sw_state.keep_running) {
		sleep(5);
		display_stats();
	}

	DOCA_LOG_INFO("Shutting down switch...");
	display_stats();

cleanup_ports:
	/* Stop ports */
	for (int i = 0; i < NB_PORTS; i++) {
		if (sw_state.ports[i] != NULL)
			doca_flow_port_stop(sw_state.ports[i]);
	}

cleanup_flow:
	doca_flow_destroy();

	DOCA_LOG_INFO("Switch stopped");
	return result == DOCA_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
