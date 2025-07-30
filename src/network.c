#include "./network.h"
#include "./ip_utils.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>

Network create_network(uint32_t ip, uint32_t mask) {
    Network network;

    network.networkIP = ip & mask;
    network.broadcastIP = ip | ~mask;
    
    network.device_count = network.broadcastIP - network.networkIP - 1;
    if (network.device_count == 0) {
        network.devices = NULL;
        return network;
    }

    network.devices = calloc(network.device_count, sizeof(Device));
    if (!network.devices) {
        perror("calloc");
        network.device_count = 0;
        return network;
    }


    for (size_t i = 0; i < network.device_count; i++) {
        network.devices[i].ip = network.networkIP + i + 1;
        network.devices[i].alive = false;
    }

    return network;
}

void free_network(Network network) {
    if (network.devices) {
        free(network.devices);
    }
}

static void print_mac(const uint8_t *mac) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void print_network(Network network) {
    char ip_str[INET_ADDRSTRLEN];

    if (ip_uint32_to_str(network.networkIP, ip_str))
        printf("Network IP: %s\n", ip_str);
    else
        printf("Network IP: <invalid>\n");

    if (ip_uint32_to_str(network.broadcastIP, ip_str))
        printf("Broadcast IP: %s\n", ip_str);
    else
        printf("Broadcast IP: <invalid>\n");

    printf("Devices (%zu usable hosts):\n", network.device_count);
    for (size_t i = 0; i < network.device_count; i++) {
        if (ip_uint32_to_str(network.devices[i].ip, ip_str)) {
            printf("  Device %zu: IP = %s, Alive = %s",
                i + 1, ip_str, network.devices[i].alive ? "Yes" : "No");

            if (network.devices[i].mac) {
                printf(", MAC = ");
                print_mac(network.devices[i].mac);
            }
            printf("\n");
        } else {
            printf("  Device %zu: IP = <invalid>, Alive = %s\n",
                i + 1, network.devices[i].alive ? "Yes" : "No");
        }
    }
}

size_t network_find_by_ip(Network network, uint32_t ip) {
    if (!network.devices || network.device_count == 0) {
        return -1;
    }

    for (size_t i = 0; i < network.device_count; i++) {
        if (network.devices[i].ip == ip) {
            return (int)i;
        }
    }

    return -1;
}

void network_set_dead(Network network) {
    for (size_t i = 0;i<network.device_count;i++) {
        network.devices[i].alive = false;
    }
}

size_t network_count_alive(Network network) {
    size_t counter = 0;
    for (size_t i = 0;i<network.device_count;i++) {
        if (network.devices[i].alive) {
            counter++;
        }
    }
    return counter;
}