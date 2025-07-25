#include "./mask_utils.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

bool parse_mask_length(const char *mask_str, uint32_t *mask) {
    char *endptr = NULL;
    long mask_len = strtol(mask_str, &endptr, 10);
    if (*endptr != '\0' || mask_len < 0 || mask_len > 32) return false;
    *mask = mask_len == 0 ? 0 : (0xFFFFFFFF << (32 - mask_len)) & 0xFFFFFFFF;
    return true;
}


bool mask_uint32_to_str(uint32_t mask, char *out_mask) {
    unsigned char bytes[4];
    bytes[0] = (mask >> 24) & 0xFF;
    bytes[1] = (mask >> 16) & 0xFF;
    bytes[2] = (mask >> 8) & 0xFF;
    bytes[3] = mask & 0xFF;

    snprintf(out_mask, INET_ADDRSTRLEN, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
    return true;
}