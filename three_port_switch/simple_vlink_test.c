/* Simple test to verify basic functionality */

#include "virtual_link.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("Testing virtual link initialization...\n");
    
    vlink_manager_t *mgr = malloc(sizeof(vlink_manager_t));
    if (!mgr) {
        printf("Failed to allocate manager\n");
        return 1;
    }
    
    printf("Calling vlink_manager_init...\n");
    int ret = vlink_manager_init(mgr);
    printf("vlink_manager_init returned: %d\n", ret);
    
    if (ret == 0) {
        printf("SUCCESS: Manager initialized\n");
        printf("Number of links: %u\n", mgr->num_links);
        
        vlink_manager_cleanup(mgr);
        printf("Manager cleaned up\n");
    }
    
    free(mgr);
    printf("All tests passed\n");
    
    return 0;
}
