#include <stdio.h>
#include "virtual_link.h"

int main() {
    printf("Size of vlink_manager_t: %lu bytes\n", sizeof(vlink_manager_t));
    printf("Size of vlink_endpoint_t: %lu bytes\n", sizeof(vlink_endpoint_t));
    printf("Size of vlink_queue_t: %lu bytes\n", sizeof(vlink_queue_t));
    printf("Size of vlink_packet_t: %lu bytes\n", sizeof(vlink_packet_t));
    printf("MAX_VLINKS: %d\n", MAX_VLINKS);
    printf("VLINK_QUEUE_SIZE: %d\n", VLINK_QUEUE_SIZE);
    return 0;
}
