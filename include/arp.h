#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <net/ethernet.h>

int send_hardcoded_arp(const char *interface);

#endif // ARP_H
