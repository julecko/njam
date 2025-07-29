#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Device {
    uint32_t ip;
    uint8_t mac[6];
    bool alive;
} Device;

typedef struct Network {
    uint32_t networkIP;
    uint32_t broadcastIP;
    size_t device_count;
    Device* devices;
} Network;

Network create_network(uint32_t ip, uint32_t mask);
void free_network(Network network);
void print_network(Network network);

#endif