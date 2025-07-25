#ifndef MASK_UTILS_H
#define MASK_UTILS_H

#include <stdint.h>
#include <stdbool.h>

bool parse_mask_length(const char *mask_str, uint32_t *mask);
bool mask_uint32_to_str(uint32_t mask, char *out_mask);
bool mask_uint32_to_str(uint32_t mask, char *out_mask);


#endif