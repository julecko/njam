#include "./network.h"
#include "./ip_utils.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#include <netinet/in.h>

#define COLOR_BLUE  "\033[33m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED   "\033[31m"
#define COLOR_RESET "\033[0m"

Network create_network(uint32_t ip, uint32_t mask) {
    Network network;

    network.networkIP = ip & mask;
    network.broadcastIP = ip | ~mask;

    pthread_mutex_init(&network.lock, NULL);
    
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
        network.devices[i].ip = network.networkIP + i;
        network.devices[i].alive = false;
    }

    return network;
}

void free_network(Network network) {
    if (network.devices) {
        free(network.devices);
    }
    pthread_mutex_destroy(&network.lock);
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

size_t network_count_inactive(const Network network) {
    if (!network.devices) {
        return -1;
    }

    size_t counter = 0;
    for (size_t i = 0; i < network.device_count; i++) {
        if (network.devices[i].status == INACTIVE) {
            counter++;
        }
    }
    return counter;
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

static void clear_lines(int n) {
    for (int i = 0; i < n; i++) {
        printf("\033[1A");
        printf("\033[2K");
    }
}

DeviceGroup print_network_nice(Network network, size_t offset, size_t max_visible) {
    size_t alive_count = network_count_alive(network);
    DeviceGroup group = {0};

    if (alive_count == 0) {
        printf("No alive devices found.\n");
        return group;
    }

    if (offset > alive_count) offset = alive_count;
    if (offset + max_visible > alive_count)
        max_visible = alive_count - offset;

    group.devices = calloc(alive_count, sizeof(Device*));
    if (!group.devices) {
        perror("calloc");
        return group;
    }

    char ip_str[INET_ADDRSTRLEN];
    size_t idx = 0;

    printf(" %3s  %-15s  %-17s  %-6s  %-7s\n", "ID", "IP", "MAC", "Alive", "Jamming");
    printf("───────────────────────────────────────────────────────────────\n");

    for (size_t i = 0; i < network.device_count; i++) {
        if (!network.devices[i].alive && network.devices[i].status == INACTIVE) continue;
        group.devices[idx++] = &network.devices[i];
    }
    group.device_count = idx;

    for (size_t i = offset; i < offset + max_visible && i < group.device_count; i++) {
        Device *dev = group.devices[i];

        if (!ip_uint32_to_str(dev->ip, ip_str)) {
            strcpy(ip_str, "<invalid>");
        }

        printf(" %s%3zu%s  ", COLOR_GREEN, i + 1, COLOR_RESET);

        if (dev->type == ROUTER) {
            printf("%s%-15s%s  ", COLOR_BLUE, ip_str, COLOR_RESET);
        } else if (dev->status == JAMMING) {
            printf("%s%-15s%s  ", COLOR_RED, ip_str, COLOR_RESET);
        } else if (dev->status == DISCONNECTING) {
            printf("%s%-15s%s  ", COLOR_GREEN, ip_str, COLOR_RESET);
        } else {
            printf("%-15s  ", ip_str);
        }

        if (dev->mac) {
            print_mac(dev->mac);
        } else {
            printf("N/A");
        }

        printf("  %-6s", dev->alive ? "Yes" : "No");
        printf("  %-7s\n", dev->status == JAMMING ? "Yes" : "No");
    }

    return group;
}
