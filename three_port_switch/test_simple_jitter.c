/*
 * Simple test for jitter and delay
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtual_link.h"

int main(void)
{
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    if (!mgr) {
        printf("Failed to allocate manager\n");
        return 1;
    }
    
    printf("Initializing vlink manager...\n");
    
    if (vlink_manager_init(mgr) != 0) {
        printf("Failed to initialize vlink manager\n");
        free(mgr);
        return 1;
    }
    
    printf("Creating link with jitter...\n");
    
    uint32_t link_id;
    int ret = vlink_create_ex(mgr, "test_link", 1000, 100, 50, 0, 0.0, &link_id);
    if (ret != 0) {
        printf("Failed to create link\n");
        vlink_manager_cleanup(mgr);
        free(mgr);
        return 1;
    }
    
    printf("Link created successfully!\n");
    printf("Link ID: %u\n", link_id);
    
    vlink_config_t config;
    vlink_get_config(mgr, link_id, &config);
    printf("Config:\n");
    printf("  Name: %s\n", config.name);
    printf("  Latency: %u us\n", config.latency_us);
    printf("  Jitter: %u us\n", config.jitter_us);
    printf("  Delay: %u us\n", config.delay_us);
    
    vlink_manager_cleanup(mgr);
    free(mgr);
    
    printf("Test completed successfully!\n");
    
    return 0;
}
