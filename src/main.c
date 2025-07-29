#include "./ip_utils.h"
#include "./mask_utils.h"
#include "./network.h"
#include "./icmp.h"
#include "./arp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>


#ifndef VERSION
#define VERSION "unknown"
#endif


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

    uint32_t ip;
    uint32_t mask;

    if (!get_ip_and_mask(argv[1], &ip, &mask)) {
        printf("Usage: njam <IP-Address>/<Subnet>\n");
        return EXIT_FAILURE;
    }

    Network network = create_network(ip, mask);
    send_hardcoded_arp("wlan0");

}
