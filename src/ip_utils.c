#include "./ip_utils.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>


bool ip_str_to_uint32(const char *ip, uint32_t *out_ip) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) != 1) return false;
    *out_ip = ntohl(addr.s_addr);
    return true;
}

bool ip_uint32_to_str(uint32_t ip, char *out_ip) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    return inet_ntop(AF_INET, &addr, out_ip, INET_ADDRSTRLEN) != NULL;
}