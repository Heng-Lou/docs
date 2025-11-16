/*
 * Three-Port Switch Simulation Test
 * 
 * This is a host-side simulation for testing and debugging
 * the three-port switch logic without hardware.
 * 
 * Compile: gcc -g -O0 -o switch_sim three_port_switch_sim.c
 * Debug:   gdb ./switch_sim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/* Simulate packet structure */
typedef struct {
    uint8_t data[1500];
    uint16_t size;
    uint8_t port_id;
    uint64_t timestamp;
    uint8_t ttl;           /* IPv4 TTL or IPv6 hop limit */
    uint8_t is_ipv4;       /* 1 if IPv4, 0 if IPv6 or non-IP */
} packet_t;

/* Port statistics */
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t ttl_expired;  /* TTL/hop limit expired packets */
} port_stats_t;

/* Port configuration */
typedef struct {
    uint8_t port_id;
    bool enabled;
    char name[32];
    uint32_t mtu;
} port_config_t;

/* Global state */
port_stats_t port_stats[3] = {0};
port_config_t port_config[3] = {
    {0, true, "pci_port", 1500},
    {1, true, "eth_port_1", 1500},
    {2, true, "eth_port_2", 1500}
};

/*
 * Three-Port Switch Forwarding Logic
 * 
 * Port 0 (PCI) <-> Port 1 (Ethernet)
 * Port 1 (Ethernet) <-> Port 2 (Ethernet)  
 * Port 2 (Ethernet) <-> Port 0 (PCI)
 */
uint8_t get_forward_port(uint8_t input_port)
{
    switch (input_port) {
        case 0:
            return 1;  // PCI -> Eth1
        case 1:
            return 2;  // Eth1 -> Eth2
        case 2:
            return 0;  // Eth2 -> PCI
        default:
            return 0;  // Invalid, drop
    }
}

/*
 * Process a packet through the switch
 */
void process_packet(packet_t *pkt)
{
    printf("Processing packet: size=%d, port=%d, TTL=%d, is_ipv4=%d\n", 
           pkt->size, pkt->port_id, pkt->ttl, pkt->is_ipv4);
    
    // Validate input port
    if (pkt->port_id >= 3) {
        printf("  ERROR: Invalid port ID %d\n", pkt->port_id);
        port_stats[0].rx_errors++;
        return;
    }
    
    // Check if port is enabled
    if (!port_config[pkt->port_id].enabled) {
        printf("  ERROR: Port %d is disabled\n", pkt->port_id);
        port_stats[pkt->port_id].rx_errors++;
        return;
    }
    
    // Check MTU
    if (pkt->size > port_config[pkt->port_id].mtu) {
        printf("  ERROR: Packet size %d exceeds MTU %d\n", 
               pkt->size, port_config[pkt->port_id].mtu);
        port_stats[pkt->port_id].rx_errors++;
        return;
    }
    
    // Update RX stats
    port_stats[pkt->port_id].rx_packets++;
    port_stats[pkt->port_id].rx_bytes += pkt->size;
    
    // Check and decrement TTL/hop limit for IP packets (loop prevention)
    if (pkt->is_ipv4 || pkt->ttl > 0) {  /* IP packet (IPv4 or IPv6) */
        if (pkt->ttl <= 1) {
            printf("  DROPPED: TTL/hop limit expired (TTL=%d)\n", pkt->ttl);
            port_stats[pkt->port_id].ttl_expired++;
            return;
        }
        pkt->ttl--;  /* Decrement TTL/hop limit */
        printf("  TTL decremented to %d\n", pkt->ttl);
    }
    
    // Determine output port
    uint8_t out_port = get_forward_port(pkt->port_id);
    
    printf("  Forwarding: port %d -> port %d\n", pkt->port_id, out_port);
    
    // Check output port
    if (!port_config[out_port].enabled) {
        printf("  ERROR: Output port %d is disabled\n", out_port);
        port_stats[out_port].tx_errors++;
        return;
    }
    
    // Update TX stats
    port_stats[out_port].tx_packets++;
    port_stats[out_port].tx_bytes += pkt->size;
    
    printf("  SUCCESS: Packet forwarded\n");
}

/*
 * Print port statistics
 */
void print_port_stats(void)
{
    printf("\n========================================\n");
    printf("Port Statistics\n");
    printf("========================================\n");
    
    for (int i = 0; i < 3; i++) {
        printf("\nPort %d (%s):\n", i, port_config[i].name);
        printf("  Enabled: %s\n", port_config[i].enabled ? "Yes" : "No");
        printf("  MTU: %d\n", port_config[i].mtu);
        printf("  RX: %lu packets, %lu bytes, %lu errors\n",
               port_stats[i].rx_packets,
               port_stats[i].rx_bytes,
               port_stats[i].rx_errors);
        printf("  TX: %lu packets, %lu bytes, %lu errors\n",
               port_stats[i].tx_packets,
               port_stats[i].tx_bytes,
               port_stats[i].tx_errors);
        printf("  TTL expired: %lu packets\n",
               port_stats[i].ttl_expired);
    }
    
    printf("\n========================================\n");
}

/*
 * Reset statistics
 */
void reset_stats(void)
{
    memset(port_stats, 0, sizeof(port_stats));
    printf("Statistics reset\n");
}

/*
 * Create test packet
 */
packet_t create_packet(uint8_t port, uint16_t size)
{
    packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    
    pkt.port_id = port;
    pkt.size = size;
    pkt.timestamp = 0; // Would be real timestamp in hardware
    pkt.ttl = 0;       // Non-IP by default
    pkt.is_ipv4 = 0;
    
    // Fill with test data
    for (int i = 0; i < size && i < sizeof(pkt.data); i++) {
        pkt.data[i] = (uint8_t)(i & 0xFF);
    }
    
    return pkt;
}

/*
 * Create test IP packet with TTL
 */
packet_t create_ip_packet(uint8_t port, uint16_t size, uint8_t ttl, bool is_ipv4)
{
    packet_t pkt = create_packet(port, size);
    pkt.ttl = ttl;
    pkt.is_ipv4 = is_ipv4 ? 1 : 0;
    return pkt;
}

/*
 * Run test scenarios
 */
void run_tests(void)
{
    printf("\n========================================\n");
    printf("Running Three-Port Switch Tests\n");
    printf("========================================\n\n");
    
    /* Test 1: Basic forwarding */
    printf("Test 1: Basic Forwarding\n");
    printf("------------------------\n");
    packet_t pkt1 = create_packet(0, 64);
    packet_t pkt2 = create_packet(1, 128);
    packet_t pkt3 = create_packet(2, 256);
    
    process_packet(&pkt1);
    process_packet(&pkt2);
    process_packet(&pkt3);
    
    assert(port_stats[0].rx_packets == 1);
    assert(port_stats[1].rx_packets == 1);
    assert(port_stats[2].rx_packets == 1);
    assert(port_stats[1].tx_packets == 1);  // Port 0 -> Port 1
    assert(port_stats[2].tx_packets == 1);  // Port 1 -> Port 2
    assert(port_stats[0].tx_packets == 1);  // Port 2 -> Port 0
    printf("✓ Test 1 passed\n\n");
    
    /* Test 2: Invalid port */
    printf("Test 2: Invalid Port\n");
    printf("--------------------\n");
    packet_t pkt_bad = create_packet(5, 64);
    uint64_t errors_before = port_stats[0].rx_errors;
    process_packet(&pkt_bad);
    assert(port_stats[0].rx_errors == errors_before + 1);
    printf("✓ Test 2 passed\n\n");
    
    /* Test 3: MTU check */
    printf("Test 3: MTU Check\n");
    printf("-----------------\n");
    packet_t pkt_big = create_packet(0, 2000);  // Exceeds MTU
    errors_before = port_stats[0].rx_errors;
    process_packet(&pkt_big);
    assert(port_stats[0].rx_errors == errors_before + 1);
    printf("✓ Test 3 passed\n\n");
    
    /* Test 4: Disabled port */
    printf("Test 4: Disabled Port\n");
    printf("---------------------\n");
    port_config[1].enabled = false;
    packet_t pkt_disabled = create_packet(1, 64);
    errors_before = port_stats[1].rx_errors;
    process_packet(&pkt_disabled);
    assert(port_stats[1].rx_errors == errors_before + 1);
    port_config[1].enabled = true;  // Re-enable
    printf("✓ Test 4 passed\n\n");
    
    /* Test 5: Byte counting */
    printf("Test 5: Byte Counting\n");
    printf("---------------------\n");
    reset_stats();
    packet_t pkt_100 = create_packet(0, 100);
    packet_t pkt_200 = create_packet(0, 200);
    process_packet(&pkt_100);
    process_packet(&pkt_200);
    assert(port_stats[0].rx_bytes == 300);
    assert(port_stats[1].tx_bytes == 300);
    printf("✓ Test 5 passed\n\n");
    
    /* Test 6: TTL expiration (IPv4) */
    printf("Test 6: TTL Expiration - IPv4\n");
    printf("------------------------------\n");
    reset_stats();
    packet_t pkt_ttl1 = create_ip_packet(0, 64, 1, true);  // TTL=1, will expire
    packet_t pkt_ttl0 = create_ip_packet(0, 64, 0, true);  // TTL=0, will expire
    uint64_t ttl_exp_before = port_stats[0].ttl_expired;
    process_packet(&pkt_ttl1);  // Should be dropped
    process_packet(&pkt_ttl0);  // Should be dropped
    assert(port_stats[0].ttl_expired == ttl_exp_before + 2);
    assert(port_stats[1].tx_packets == 0);  // No packets forwarded
    printf("✓ Test 6 passed\n\n");
    
    /* Test 7: TTL decrement (IPv4) */
    printf("Test 7: TTL Decrement - IPv4\n");
    printf("----------------------------\n");
    reset_stats();
    packet_t pkt_ttl64 = create_ip_packet(0, 64, 64, true);  // TTL=64
    packet_t pkt_ttl2 = create_ip_packet(1, 64, 2, true);    // TTL=2
    process_packet(&pkt_ttl64);  // Should forward with TTL=63
    process_packet(&pkt_ttl2);   // Should forward with TTL=1
    assert(pkt_ttl64.ttl == 63);
    assert(pkt_ttl2.ttl == 1);
    assert(port_stats[1].tx_packets == 1);  // Port 0 -> Port 1
    assert(port_stats[2].tx_packets == 1);  // Port 1 -> Port 2
    printf("✓ Test 7 passed\n\n");
    
    /* Test 8: Hop limit expiration (IPv6) */
    printf("Test 8: Hop Limit Expiration - IPv6\n");
    printf("------------------------------------\n");
    reset_stats();
    packet_t pkt_hop1 = create_ip_packet(2, 64, 1, false);  // Hop=1, will expire
    ttl_exp_before = port_stats[2].ttl_expired;
    process_packet(&pkt_hop1);  // Should be dropped
    assert(port_stats[2].ttl_expired == ttl_exp_before + 1);
    assert(port_stats[0].tx_packets == 0);  // No packets forwarded
    printf("✓ Test 8 passed\n\n");
    
    /* Test 9: Ring topology simulation (prevent infinite loop) */
    printf("Test 9: Ring Topology - Loop Prevention\n");
    printf("---------------------------------------\n");
    reset_stats();
    // Simulate a packet going around the ring
    packet_t pkt_ring = create_ip_packet(0, 64, 4, true);  // TTL=4
    printf("  Simulating ring: Port 0 -> 1 -> 2 -> 0 (loop)\n");
    
    // First hop: port 0 -> 1 (TTL: 4 -> 3)
    process_packet(&pkt_ring);
    assert(pkt_ring.ttl == 3);
    
    // Second hop: port 1 -> 2 (TTL: 3 -> 2)
    pkt_ring.port_id = 1;
    process_packet(&pkt_ring);
    assert(pkt_ring.ttl == 2);
    
    // Third hop: port 2 -> 0 (TTL: 2 -> 1)
    pkt_ring.port_id = 2;
    process_packet(&pkt_ring);
    assert(pkt_ring.ttl == 1);
    
    // Fourth hop: port 0 -> 1 (TTL: 1, will expire)
    pkt_ring.port_id = 0;
    uint64_t exp_before = port_stats[0].ttl_expired;
    process_packet(&pkt_ring);
    assert(port_stats[0].ttl_expired == exp_before + 1);
    
    printf("  ✓ Packet dropped after 3 hops, preventing infinite loop\n");
    printf("✓ Test 9 passed\n\n");
    
    printf("========================================\n");
    printf("All Tests Passed! ✓\n");
    printf("========================================\n");
}

/*
 * Interactive mode
 */
void interactive_mode(void)
{
    char cmd[256];
    
    printf("\n========================================\n");
    printf("Interactive Three-Port Switch Simulator\n");
    printf("========================================\n\n");
    
    printf("Commands:\n");
    printf("  send <port> <size>         - Send non-IP packet\n");
    printf("  sendip <port> <size> <ttl> - Send IPv4 packet with TTL\n");
    printf("  sendip6 <port> <size> <hop>- Send IPv6 packet with hop limit\n");
    printf("  stats                      - Show statistics\n");
    printf("  reset                      - Reset statistics\n");
    printf("  enable <port>              - Enable port\n");
    printf("  disable <port>             - Disable port\n");
    printf("  quit                       - Exit\n");
    printf("\n");
    
    while (1) {
        printf("> ");
        if (!fgets(cmd, sizeof(cmd), stdin))
            break;
            
        // Parse command
        char action[32];
        int port, size, ttl;
        
        if (sscanf(cmd, "%s", action) == 1) {
            if (strcmp(action, "stats") == 0) {
                print_port_stats();
            }
            else if (strcmp(action, "reset") == 0) {
                reset_stats();
            }
            else if (strcmp(action, "quit") == 0) {
                break;
            }
            else if (sscanf(cmd, "sendip6 %d %d %d", &port, &size, &ttl) == 3) {
                packet_t pkt = create_ip_packet(port, size, ttl, false);
                process_packet(&pkt);
            }
            else if (sscanf(cmd, "sendip %d %d %d", &port, &size, &ttl) == 3) {
                packet_t pkt = create_ip_packet(port, size, ttl, true);
                process_packet(&pkt);
            }
            else if (sscanf(cmd, "send %d %d", &port, &size) == 2) {
                packet_t pkt = create_packet(port, size);
                process_packet(&pkt);
            }
            else if (sscanf(cmd, "enable %d", &port) == 1) {
                if (port >= 0 && port < 3) {
                    port_config[port].enabled = true;
                    printf("Port %d enabled\n", port);
                }
            }
            else if (sscanf(cmd, "disable %d", &port) == 1) {
                if (port >= 0 && port < 3) {
                    port_config[port].enabled = false;
                    printf("Port %d disabled\n", port);
                }
            }
            else {
                printf("Unknown command. Type 'quit' to exit.\n");
            }
        }
    }
}

/*
 * Main entry point
 */
int main(int argc, char *argv[])
{
    printf("========================================\n");
    printf("Three-Port Switch Simulator\n");
    printf("========================================\n");
    
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        // Run automated tests
        run_tests();
        print_port_stats();
    }
    else if (argc > 1 && strcmp(argv[1], "interactive") == 0) {
        // Interactive mode
        interactive_mode();
    }
    else {
        // Default: run tests then interactive
        run_tests();
        print_port_stats();
        interactive_mode();
    }
    
    return 0;
}

/*
 * GDB Debugging Tips:
 * 
 * Compile with debug symbols:
 *   gcc -g -O0 -o switch_sim three_port_switch_sim.c
 * 
 * Start debugging:
 *   gdb ./switch_sim
 * 
 * Useful GDB commands:
 *   (gdb) break process_packet
 *   (gdb) run test
 *   (gdb) print pkt->port_id
 *   (gdb) print port_stats[0]
 *   (gdb) watch port_stats[0].rx_packets
 *   (gdb) step
 *   (gdb) continue
 * 
 * Debug specific test:
 *   (gdb) break run_tests
 *   (gdb) run test
 *   (gdb) step
 * 
 * Examine memory:
 *   (gdb) x/16x pkt->data
 *   (gdb) x/3 port_stats
 */
