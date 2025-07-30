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
#include <unistd.h>

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

    uint8_t my_mac[6];

    const char* my_ip = "192.168.0.135";
    uint32_t my_ip_num;
    ip_str_to_uint32(my_ip, &my_ip_num);
    const char* target_ip = "192.168.0.153";
    uint32_t target_ip_num;
    ip_str_to_uint32(target_ip, &target_ip_num);

    my_ip_num = htonl(my_ip_num);
    target_ip_num = htonl(target_ip_num);

    get_interface_mac("wlan0", my_mac);

    //uint8_t victim_mac[6] = {0x74, 0xda, 0x88, 0xe1, 0x8b, 0xfc};
    //arp_send_reply("wlan0", my_mac, "192.168.0.153", victim_mac, "192.168.0.1");
    arp_send_request("wlan0", my_mac, &my_ip_num, &target_ip_num);
}
