#define _GNU_SOURCE

#include "./arp.h"
#include "./network.h"

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
#define MAC_LEN 6
#define IP_LEN 4

#define SCAN_TIMEOUT 5.0

struct arp_hdr {
    uint16_t htype;         // Hardware type (Ethernet 0x0001)
    uint16_t ptype;         // Protocol type (IPv4 0x0800)
    uint8_t hlen;           // Hardware length (Mac 6)
    uint8_t plen;           // Protocol length (IPv4 4)
    uint16_t oper;          // Operation (1 Request 2 Response)
    uint8_t sha[MAC_LEN];   // Sender hardware address (AA:BB:CC:DD:FF)
    uint8_t spa[IP_LEN];    // Sender protocol address (192.168.0.1)
    uint8_t tha[MAC_LEN];   // Target hardware address (AA:BB:CC:DD:FF)
    uint8_t tpa[IP_LEN];    // Target protocol address (192.168.0.1)
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

void set_ethernet_header(struct ether_header *eth,
                         const uint8_t *dest_mac,
                         const uint8_t *src_mac,
                         uint16_t ethertype) {
    memcpy(eth->ether_dhost, dest_mac, 6);
    memcpy(eth->ether_shost, src_mac, 6);
    eth->ether_type = htons(ethertype);
}

void set_arp_header(struct arp_hdr *arp,
                    uint16_t oper,
                    const uint8_t *sender_mac,
                    const uint32_t *sender_ip,
                    const uint8_t *target_mac,
                    const uint32_t *target_ip) {
    arp->htype = htons(1);          // Ethernet
    arp->ptype = htons(ETH_P_IP);   // IPv4
    arp->hlen = MAC_LEN;            // MAC length
    arp->plen = IP_LEN;             // IPv4 length
    arp->oper = htons(oper);        // Request or Reply

    memcpy(arp->sha, sender_mac, MAC_LEN);
    memcpy(arp->spa, sender_ip, IP_LEN);
    memcpy(arp->tha, target_mac, MAC_LEN);
    memcpy(arp->tpa, target_ip, IP_LEN);
}

int arp_create_socket() {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("socket");
    }

    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return sockfd;
}


int send_arp_packet(int sockfd,
                    const char *iface_name,
                    const uint8_t *dest_mac,
                    uint16_t arp_oper,
                    const uint8_t *sender_mac,
                    const uint32_t *sender_ip,
                    const uint8_t *target_mac,
                    const uint32_t *target_ip) {

    int ifindex = get_interface_index(sockfd, iface_name);
    if (ifindex < 0) {
        return -1;
    }

    uint8_t buffer[ARP_PACKET_SIZE] = {0};
    struct ether_header *eth = (struct ether_header *)buffer;
    struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct ether_header));

    set_ethernet_header(eth, dest_mac, sender_mac, ETH_P_ARP);
    set_arp_header(arp, arp_oper, sender_mac, sender_ip, target_mac, target_ip);

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = 6;
    memcpy(addr.sll_addr, dest_mac, 6);

    ssize_t sent = sendto(sockfd, buffer, sizeof(buffer), 0,
                          (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) {
        perror("sendto");
        return -1;
    }

    return 0;
}

int arp_send_request(int sockfd,
                     const char *iface_name,
                     const uint8_t *src_mac,
                     const uint32_t *src_ip,
                     const uint32_t *target_ip) {
    uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};

    return send_arp_packet(sockfd, iface_name, broadcast_mac, ARPOP_REQUEST,
                           src_mac, src_ip, zero_mac, target_ip);
}

int arp_send_reply(int sockfd,
                   const char *iface_name,
                   const uint8_t *src_mac,
                   const uint32_t *src_ip,
                   const uint8_t *dest_mac,
                   const uint32_t *dest_ip) {
    return send_arp_packet(
        sockfd,
        iface_name,
        dest_mac,          // destination MAC (target host)
        ARPOP_REPLY,       // ARP operation
        src_mac,           // source MAC (us)
        src_ip,            // source IP (spoofed target)
        dest_mac,          // target MAC (the one we're replying to)
        dest_ip            // Target IP (the one we're replying to)
    );
}

int receive_arp_reply(int sockfd, uint32_t *target_ip, uint8_t *resolved_mac) {
    uint8_t buffer[ARP_PACKET_SIZE];
    ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    if (len < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return -2;
        }
        perror("recvfrom");
        return -1;
    }

    if (len < (ssize_t)(sizeof(struct ether_header) + sizeof(struct arp_hdr))) {
        return 1;
    }

    struct ether_header *eth = (struct ether_header *)buffer;
    if (ntohs(eth->ether_type) != ETH_P_ARP) {
        return 1;
    }

    struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct ether_header));
    if (ntohs(arp->oper) == ARPOP_REPLY) {
        memcpy(target_ip, arp->spa, IP_LEN);
        memcpy(resolved_mac, arp->sha, MAC_LEN);
        return 0;
    }

    return 1;
}

void arp_scan_range(Network network,
                    int sockfd,
                    const char *iface_name,
                    const uint8_t *src_mac,
                    const uint32_t *src_ip) {

    for (uint32_t ip = network.networkIP; ip < network.broadcastIP; ip++) {
        uint32_t target_ip = htonl(ip);
        arp_send_request(sockfd, iface_name, src_mac, src_ip, &target_ip);
        usleep(5000);
    }

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) +
                         (now.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed >= SCAN_TIMEOUT) {
            break;
        }

        uint32_t reply_ip = 0;
        uint8_t reply_mac[MAC_LEN] = {0};

        int res = receive_arp_reply(sockfd, &reply_ip, reply_mac);
        if (res == 0) {
            pthread_mutex_lock(&network.lock);
            size_t index = network_find_by_ip(network, ntohl(reply_ip));
            if (index != -1) {
                network.devices[index].alive = true;
                memcpy(network.devices[index].mac, reply_mac, MAC_LEN); 
            } else {
                fprintf(stderr, "Device not found\n");
            }
            pthread_mutex_unlock(&network.lock);
        } else if (res == -2 || res == -1) {
            break;
        }
    }
    printf("Scanning finished\n");
}
