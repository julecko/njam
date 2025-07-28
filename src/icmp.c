#define _GNU_SOURCE
#include "./ip_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>

#define TIMEOUT 2
#define BUFFER_SIZE 1024

static unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

static void send_ping(int sockfd, uint32_t ip, int ident) {
    struct sockaddr_in addr;
    char sendbuf[64];
    struct icmphdr *icmp = (struct icmphdr *)sendbuf;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);

    memset(sendbuf, 0, sizeof(sendbuf));
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = ident;
    icmp->un.echo.sequence = 1;
    icmp->checksum = checksum(icmp, sizeof(sendbuf));

    sendto(sockfd, sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&addr, sizeof(addr));
}

void icmp_scan_network(uint32_t network_ip, uint32_t broadcast_ip) {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    uint32_t ident = getpid() & 0xFFFF;

    for (uint32_t ip = network_ip + 1; ip < broadcast_ip; ip++) {
        send_ping(sockfd, ip, ident);
        usleep(1000);
    }

    printf("Waiting for replies...\n");

    char recvbuf[BUFFER_SIZE];
    time_t start_time = time(NULL);
    while (time(NULL) - start_time <= TIMEOUT) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        ssize_t n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&addr, &len);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            } else {
                perror("recvfrom");
                break;
            }
        } else if (n == 0) {
            break;
        }

        struct iphdr *ip_hdr = (struct iphdr *)recvbuf;
        struct icmphdr *icmp = (struct icmphdr *)(recvbuf + ip_hdr->ihl * 4);

        if (icmp->type == ICMP_ECHOREPLY && icmp->un.echo.id == ident) {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
            printf("Host active: %s\n", ip_str);
        }
    }

    close(sockfd);
}
