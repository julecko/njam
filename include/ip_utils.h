#ifndef IP_UTILS_H
#define IP_UTILS_H

#include <stdint.h>
#include <stdbool.h>


bool ip_uint32_to_str(uint32_t ip, char *out_ip);
bool ip_str_to_uint32(const char *ip, uint32_t *out_ip);

#endif