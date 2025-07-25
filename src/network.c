#include "./network.h"
#include <stdint.h>

Network create_network(uint32_t ip, uint32_t mask) {
    Network network;

    network.networkIP = ip & mask;
    network.broadcastIP = ip | ~mask;

    return network;
}