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
#include <net/if_arp.h>

#define ARP_PACKET_SIZE 42

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

int get_interface_index(int sockfd, const char *iface_name) {
    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl(SIOCGIFINDEX)");
        return -1;
    }
    return ifr.ifr_ifindex;
}

int get_interface_mac(const char *iface_name, uint8_t *mac_out) {
    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)");
        return -1;
    }
    close(sockfd);

    memcpy(mac_out, ifr.ifr_hwaddr.sa_data, 6);
    return 0;
}

// Setup Ethernet header
void set_ethernet_header(struct ether_header *eth,
                         const uint8_t *dest_mac,
                         const uint8_t *src_mac,
                         uint16_t ethertype) {
    memcpy(eth->ether_dhost, dest_mac, 6);
    memcpy(eth->ether_shost, src_mac, 6);
    eth->ether_type = htons(ethertype);
}

// Setup ARP header
void set_arp_header(struct arp_hdr *arp,
                    uint16_t oper,
                    const uint8_t *sender_mac,
                    const char *sender_ip_str,
                    const uint8_t *target_mac,
                    const char *target_ip_str) {
    arp->htype = htons(1);          // Ethernet
    arp->ptype = htons(ETH_P_IP);   // IPv4
    arp->hlen = 6;                  // MAC length
    arp->plen = 4;                  // IPv4 length
    arp->oper = htons(oper);        // Request or Reply

    memcpy(arp->sha, sender_mac, 6);
    inet_pton(AF_INET, sender_ip_str, arp->spa);
    memcpy(arp->tha, target_mac, 6);
    inet_pton(AF_INET, target_ip_str, arp->tpa);
}

// Main function to send ARP packet
int send_arp_packet(const char *iface_name,
                    const uint8_t *dest_mac,
                    const uint8_t *src_mac,
                    uint16_t arp_oper,
                    const char *sender_ip,
                    const uint8_t *target_mac,
                    const char *target_ip) {

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    int ifindex = get_interface_index(sockfd, iface_name);
    if (ifindex < 0) {
        close(sockfd);
        return -1;
    }

    uint8_t buffer[ARP_PACKET_SIZE] = {0};
    struct ether_header *eth = (struct ether_header *)buffer;
    struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct ether_header));

    set_ethernet_header(eth, dest_mac, src_mac, ETH_P_ARP);
    set_arp_header(arp, arp_oper, src_mac, sender_ip, target_mac, target_ip);

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = 6;
    memcpy(addr.sll_addr, dest_mac, 6);

    ssize_t sent = sendto(sockfd, buffer, sizeof(buffer), 0,
                          (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    printf("ARP packet sent on interface %s\n", iface_name);
    close(sockfd);
    return 0;
}

#include <net/if_arp.h>  // for ARPOP_REQUEST and ARPOP_REPLY

// Sends an ARP Request to ask "Who has target_ip? Tell me!"
int arp_send_request(const char *iface_name,
                     const uint8_t *src_mac,
                     const char *src_ip,
                     const char *target_ip) {
    uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t zero_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    return send_arp_packet(
        iface_name,
        broadcast_mac,     // destination MAC (broadcast)
        src_mac,           // source MAC
        ARPOP_REQUEST,     // ARP operation
        src_ip,
        zero_mac,          // target MAC is unknown
        target_ip
    );
}

int arp_send_reply(const char *iface_name,
                   const uint8_t *src_mac,
                   const char *src_ip,
                   const uint8_t *dest_mac,
                   const char *dest_ip) {
    return send_arp_packet(
        iface_name,
        dest_mac,          // destination MAC (target host)
        src_mac,           // source MAC (us)
        ARPOP_REPLY,       // ARP operation
        src_ip,
        dest_mac,          // target MAC (the one we're replying to)
        dest_ip
    );
}
