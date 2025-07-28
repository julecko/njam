#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>

void icmp_scan_network(uint32_t network_ip, uint32_t broadcast_ip);

#endif