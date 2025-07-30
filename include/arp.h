#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <net/ethernet.h>

int get_interface_mac(const char *iface_name, uint8_t *mac_out);
int arp_send_reply(const char *iface_name,
                   const uint8_t *src_mac,
                   const uint32_t *src_ip,
                   const uint8_t *dest_mac,
                   const uint32_t *dest_ip);
int arp_send_request(const char *iface_name,
                     const uint8_t *src_mac,
                     const uint32_t *src_ip,
                     const uint32_t *target_ip);

#endif
