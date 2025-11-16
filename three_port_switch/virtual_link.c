/*
 * Virtual Link Infrastructure Implementation
 */

#include "virtual_link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

/* Helper: Get current time in microseconds */
static uint64_t get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/* Helper: Random number 0.0 - 1.0 */
static float rand_float(void)
{
    return (float)rand() / (float)RAND_MAX;
}

/* Initialize queue */
static int queue_init(vlink_queue_t *queue)
{
    memset(queue, 0, sizeof(*queue));
    
    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        pthread_cond_destroy(&queue->not_empty);
        return -1;
    }
    
    return 0;
}

/* Cleanup queue */
static void queue_cleanup(vlink_queue_t *queue)
{
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->lock);
}

/* Enqueue packet (non-blocking) */
static int queue_enqueue(vlink_queue_t *queue, const uint8_t *data, uint16_t size)
{
    pthread_mutex_lock(&queue->lock);
    
    uint32_t next_head = (queue->head + 1) % VLINK_QUEUE_SIZE;
    if (next_head == queue->tail) {
        /* Queue full */
        pthread_mutex_unlock(&queue->lock);
        return -ENOSPC;
    }
    
    vlink_packet_t *pkt = &queue->packets[queue->head];
    memcpy(pkt->data, data, size);
    pkt->size = size;
    pkt->timestamp = get_time_us();
    pkt->seq_num = queue->head;
    
    queue->head = next_head;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
    
    return 0;
}

/* Dequeue packet (blocking with timeout) */
static int queue_dequeue(vlink_queue_t *queue, uint8_t *data, uint16_t *size, 
                         uint16_t max_size, uint32_t timeout_us)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (timeout_us % 1000000) * 1000;
    ts.tv_sec += timeout_us / 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    pthread_mutex_lock(&queue->lock);
    
    while (queue->head == queue->tail) {
        int ret = pthread_cond_timedwait(&queue->not_empty, &queue->lock, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->lock);
            return -ETIMEDOUT;
        }
    }
    
    vlink_packet_t *pkt = &queue->packets[queue->tail];
    
    if (pkt->size > max_size) {
        pthread_mutex_unlock(&queue->lock);
        return -EMSGSIZE;
    }
    
    memcpy(data, pkt->data, pkt->size);
    *size = pkt->size;
    
    queue->tail = (queue->tail + 1) % VLINK_QUEUE_SIZE;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
    
    return 0;
}

/* RX thread for callback mode */
static void *rx_thread_func(void *arg)
{
    vlink_endpoint_t *link = (vlink_endpoint_t *)arg;
    uint8_t buffer[MAX_PACKET_SIZE];
    uint16_t size;
    
    while (link->running) {
        int ret = queue_dequeue(&link->rx_queue, buffer, &size, MAX_PACKET_SIZE, 100000);
        if (ret == 0 && link->rx_callback) {
            link->rx_callback(link->rx_callback_ctx, buffer, size);
        }
    }
    
    return NULL;
}

/*
 * Public API Implementation
 */

int vlink_manager_init(vlink_manager_t *mgr)
{
    memset(mgr, 0, sizeof(*mgr));
    
    if (pthread_mutex_init(&mgr->mgr_lock, NULL) != 0) {
        return -1;
    }
    
    /* Seed random number generator */
    srand(time(NULL));
    
    return 0;
}

void vlink_manager_cleanup(vlink_manager_t *mgr)
{
    /* Stop all links */
    for (uint32_t i = 0; i < mgr->num_links; i++) {
        vlink_stop(mgr, i);
        queue_cleanup(&mgr->links[i].tx_queue);
        queue_cleanup(&mgr->links[i].rx_queue);
    }
    
    pthread_mutex_destroy(&mgr->mgr_lock);
}

int vlink_create(vlink_manager_t *mgr, const char *name, 
                 uint32_t bandwidth_mbps, uint32_t latency_us,
                 float loss_rate, uint32_t *link_id)
{
    return vlink_create_ex(mgr, name, bandwidth_mbps, latency_us, 0, 0, loss_rate, link_id);
}

int vlink_create_ex(vlink_manager_t *mgr, const char *name, 
                    uint32_t bandwidth_mbps, uint32_t latency_us,
                    uint32_t jitter_us, uint32_t delay_us,
                    float loss_rate, uint32_t *link_id)
{
    pthread_mutex_lock(&mgr->mgr_lock);
    
    if (mgr->num_links >= MAX_VLINKS) {
        pthread_mutex_unlock(&mgr->mgr_lock);
        return -ENOSPC;
    }
    
    uint32_t id = mgr->num_links++;
    vlink_endpoint_t *link = &mgr->links[id];
    
    memset(link, 0, sizeof(*link));
    link->link_id = id;
    
    /* Configure */
    strncpy(link->config.name, name, sizeof(link->config.name) - 1);
    link->config.bandwidth_mbps = bandwidth_mbps;
    link->config.latency_us = latency_us;
    link->config.jitter_us = jitter_us;
    link->config.delay_us = delay_us;
    link->config.loss_rate = loss_rate;
    link->config.enabled = true;
    
    /* Initialize queues */
    if (queue_init(&link->tx_queue) != 0) {
        mgr->num_links--;
        pthread_mutex_unlock(&mgr->mgr_lock);
        return -1;
    }
    
    if (queue_init(&link->rx_queue) != 0) {
        queue_cleanup(&link->tx_queue);
        mgr->num_links--;
        pthread_mutex_unlock(&mgr->mgr_lock);
        return -1;
    }
    
    *link_id = id;
    
    pthread_mutex_unlock(&mgr->mgr_lock);
    
    printf("Created virtual link %d: %s (BW: %u Mbps, Latency: %u us, Jitter: %u us, Delay: %u us, Loss: %.2f%%)\n",
           id, name, bandwidth_mbps, latency_us, jitter_us, delay_us, loss_rate * 100);
    
    return 0;
}

int vlink_connect(vlink_manager_t *mgr, uint32_t link_id1, uint32_t link_id2)
{
    if (link_id1 >= mgr->num_links || link_id2 >= mgr->num_links) {
        return -EINVAL;
    }
    
    /* In this implementation, we simply forward packets between
     * link1's TX queue and link2's RX queue, and vice versa */
    
    printf("Connected virtual links: %s <-> %s\n",
           mgr->links[link_id1].config.name,
           mgr->links[link_id2].config.name);
    
    return 0;
}

int vlink_set_rx_callback(vlink_manager_t *mgr, uint32_t link_id,
                          void (*callback)(void *ctx, const uint8_t *data, uint16_t size),
                          void *ctx)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    vlink_endpoint_t *link = &mgr->links[link_id];
    link->rx_callback = callback;
    link->rx_callback_ctx = ctx;
    
    return 0;
}

int vlink_send(vlink_manager_t *mgr, uint32_t link_id, 
               const uint8_t *data, uint16_t size)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    vlink_endpoint_t *link = &mgr->links[link_id];
    
    if (!link->config.enabled) {
        link->stats.drops++;
        return -ENETDOWN;
    }
    
    /* Simulate packet loss */
    if (link->config.loss_rate > 0 && rand_float() < link->config.loss_rate) {
        link->stats.drops++;
        return 0;  /* Packet dropped */
    }
    
    /* Calculate total delay with jitter */
    uint32_t total_delay = link->config.latency_us + link->config.delay_us;
    
    /* Add jitter (random variation +/- jitter_us) */
    if (link->config.jitter_us > 0) {
        /* Random jitter between -jitter_us and +jitter_us */
        int32_t jitter = (int32_t)(rand_float() * 2.0f * link->config.jitter_us) - link->config.jitter_us;
        if (jitter > 0) {
            total_delay += jitter;
        } else if ((uint32_t)(-jitter) < total_delay) {
            total_delay -= (-jitter);
        }
    }
    
    /* Simulate latency, jitter, and delay */
    if (total_delay > 0) {
        usleep(total_delay);
    }
    
    /* Enqueue to TX queue */
    int ret = queue_enqueue(&link->tx_queue, data, size);
    if (ret != 0) {
        link->stats.drops++;
        return ret;
    }
    
    link->stats.tx_packets++;
    link->stats.tx_bytes += size;
    
    /* Also put in peer's RX queue if connected */
    /* For simplicity, we'll assume the peer link is link_id ^ 1 for paired links */
    uint32_t peer_id = link_id ^ 1;
    if (peer_id < mgr->num_links) {
        vlink_endpoint_t *peer = &mgr->links[peer_id];
        queue_enqueue(&peer->rx_queue, data, size);
    }
    
    return 0;
}

int vlink_recv(vlink_manager_t *mgr, uint32_t link_id,
               uint8_t *data, uint16_t *size, uint16_t max_size)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    vlink_endpoint_t *link = &mgr->links[link_id];
    
    int ret = queue_dequeue(&link->rx_queue, data, size, max_size, 10000);
    if (ret == 0) {
        link->stats.rx_packets++;
        link->stats.rx_bytes += *size;
    }
    
    return ret;
}

int vlink_start(vlink_manager_t *mgr, uint32_t link_id)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    vlink_endpoint_t *link = &mgr->links[link_id];
    
    if (link->running) {
        return 0;  /* Already running */
    }
    
    link->running = true;
    
    /* Start RX thread if callback is set */
    if (link->rx_callback) {
        if (pthread_create(&link->rx_thread, NULL, rx_thread_func, link) != 0) {
            link->running = false;
            return -1;
        }
    }
    
    printf("Started virtual link %d: %s\n", link_id, link->config.name);
    
    return 0;
}

int vlink_stop(vlink_manager_t *mgr, uint32_t link_id)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    vlink_endpoint_t *link = &mgr->links[link_id];
    
    if (!link->running) {
        return 0;  /* Already stopped */
    }
    
    link->running = false;
    
    /* Wait for RX thread */
    if (link->rx_callback) {
        pthread_join(link->rx_thread, NULL);
    }
    
    printf("Stopped virtual link %d: %s\n", link_id, link->config.name);
    
    return 0;
}

int vlink_get_stats(vlink_manager_t *mgr, uint32_t link_id, vlink_stats_t *stats)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    memcpy(stats, &mgr->links[link_id].stats, sizeof(*stats));
    return 0;
}

int vlink_reset_stats(vlink_manager_t *mgr, uint32_t link_id)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    memset(&mgr->links[link_id].stats, 0, sizeof(vlink_stats_t));
    return 0;
}

int vlink_get_config(vlink_manager_t *mgr, uint32_t link_id, vlink_config_t *config)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    memcpy(config, &mgr->links[link_id].config, sizeof(*config));
    return 0;
}

int vlink_set_config(vlink_manager_t *mgr, uint32_t link_id, const vlink_config_t *config)
{
    if (link_id >= mgr->num_links) {
        return -EINVAL;
    }
    
    memcpy(&mgr->links[link_id].config, config, sizeof(*config));
    return 0;
}

void vlink_print_stats(vlink_manager_t *mgr)
{
    printf("\n========================================\n");
    printf("Virtual Link Statistics\n");
    printf("========================================\n");
    
    for (uint32_t i = 0; i < mgr->num_links; i++) {
        vlink_endpoint_t *link = &mgr->links[i];
        printf("\nLink %d: %s\n", i, link->config.name);
        printf("  Status: %s\n", link->config.enabled ? "Enabled" : "Disabled");
        printf("  Config: %u Mbps, %u us latency, %.2f%% loss\n",
               link->config.bandwidth_mbps, link->config.latency_us,
               link->config.loss_rate * 100);
        printf("  TX: %lu packets, %lu bytes\n",
               link->stats.tx_packets, link->stats.tx_bytes);
        printf("  RX: %lu packets, %lu bytes\n",
               link->stats.rx_packets, link->stats.rx_bytes);
        printf("  Drops: %lu, Errors: %lu\n",
               link->stats.drops, link->stats.errors);
    }
    
    printf("\n========================================\n");
}
