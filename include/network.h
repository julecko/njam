#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>


typedef enum DeviceType {
    CLIENT,
    ROUTER,
} DeviceType;

typedef enum DeviceStatus {
    INACTIVE,
    JAMMING,
    DISCONNECTING,
} DeviceStatus;

typedef struct Device {
    uint32_t ip;
    uint8_t mac[6];
    bool alive;
    DeviceType type;
    DeviceStatus status;
    int disconnecting_counter;
} Device;

typedef struct Network {
    uint32_t networkIP;
    uint32_t broadcastIP;
    size_t device_count;
    Device* devices;

    pthread_mutex_t lock;
} Network;

typedef struct DeviceGroup {
    size_t device_count;
    Device** devices;
} DeviceGroup;

Network create_network(uint32_t ip, uint32_t mask);
void free_network(Network network);
void print_network(Network network);
size_t network_count_inactive(const Network network);
size_t network_find_by_ip(Network network, uint32_t ip);
size_t network_count_alive(Network network);
DeviceGroup print_network_nice(Network network, size_t offset, size_t max_visible);

#endif