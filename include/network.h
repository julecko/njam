#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

typedef struct Network {
    uint32_t networkIP;
    uint32_t broadcastIP;
} Network;

Network create_network(uint32_t ip, uint32_t mask);

#endif