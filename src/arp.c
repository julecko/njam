#define _GNU_SOURCE

#include "arp.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

struct eth_hdr {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t ethertype;
} __attribute__((packed));

struct arp_hdr {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint8_t spa[4];
    uint8_t tha[6];
    uint8_t tpa[4];
} __attribute__((packed));

int send_hardcoded_arp(const char *interface) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(sockfd);
        return -1;
    }
    int ifindex = ifr.ifr_ifindex;

    // Get MAC address
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        close(sockfd);
        return -1;
    }
    uint8_t src_mac[6];
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);

    // Build packet
    uint8_t buffer[42];
    struct eth_hdr *eth = (struct eth_hdr *)buffer;
    struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct eth_hdr));

    // Ethernet header
    memset(eth->dest, 0xFF, 6);              // Broadcast
    memcpy(eth->src, src_mac, 6);            // Our MAC
    eth->ethertype = htons(ETH_P_ARP);

    // ARP header
    arp->htype = htons(1);                   // Ethernet
    arp->ptype = htons(ETH_P_IP);            // IPv4
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(1);                    // ARP Request
    memcpy(arp->sha, src_mac, 6);            // Sender MAC
    inet_pton(AF_INET, "192.168.1.100", arp->spa); // Sender IP (spoofed if needed)
    memset(arp->tha, 0x00, 6);               // Target MAC unknown
    inet_pton(AF_INET, "192.168.1.1", arp->tpa);   // Target IP

    // Socket address
    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = 6;
    memset(addr.sll_addr, 0xFF, 6);          // Broadcast

    // Send
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    printf("ARP packet sent.\n");
    close(sockfd);
    return 0;
}
