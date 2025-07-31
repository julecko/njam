#include "./ip_utils.h"
#include "./mask_utils.h"
#include "./network.h"
#include "./icmp.h"
#include "./arp.h"
#include "./process/scanner.h"
#include "./process/jammer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>


#ifndef VERSION
#define VERSION "unknown"
#endif

volatile bool stop_flag = false;
Network *global_network = NULL;
pthread_t jam_thread;
int sockfd_scanner = -1;

void handle_sigint(int signum) {
    (void)signum;
    printf(TOP_LEFT_CODE);
    printf(CLEAR_SCREEN_CODE);
    printf("Cleaning up, can take up to 10 seconds\n");
    printf("Program is sending correct arp mapings to devices\n");

    if (global_network) {
        pthread_mutex_lock(&global_network->lock);
        for (size_t i=0;i<global_network->device_count;i++) {
            Device *device = &global_network->devices[i];
            if (device->status == JAMMING){
                device->status = DISCONNECTING;
            }
        }
        pthread_mutex_unlock(&global_network->lock);

        stop_flag = true;
        pthread_join(jam_thread, NULL);

        close(sockfd_scanner);
        free_network(*global_network);
        disable_raw_mode();
    }
    global_network = NULL;
    exit(0);
}


bool get_ip_and_mask(const char *arg, uint32_t *ip, uint32_t *mask) {
    const char *slash = strchr(arg, '/');
    if (!slash) return false;

    size_t ip_len = slash - arg;
    if (ip_len >= INET_ADDRSTRLEN) return false;

    char ip_str[INET_ADDRSTRLEN];
    strncpy(ip_str, arg, ip_len);
    ip_str[ip_len] = '\0';

    const char *mask_str = slash + 1;

    return ip_str_to_uint32(ip_str, ip) && parse_mask_length(mask_str, mask);
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Usage: njam <IP-Address>/<Subnet>\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    uint32_t ip;
    uint32_t mask;

    if (!get_ip_and_mask(argv[1], &ip, &mask)) {
        printf("Usage: njam <IP-Address>/<Subnet>\n");
        return EXIT_FAILURE;
    }

    Network network = create_network(ip, mask);
    global_network = &network;

    JammerArgs args = {
        .network = &network,
        .stop_flag = &stop_flag
    };
    pthread_create(&jam_thread, NULL, jam_jammed_devices, &args);

    sockfd_scanner = arp_create_socket();
    if (sockfd_scanner != -1){
        scanner_process(network, sockfd_scanner, ip);
    }
    handle_sigint(0);
}
