#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Args {
    uint32_t ip;
    uint32_t mask;
    const char *interface;
} Args;

void args_print(Args *args);
void print_usage();
bool parse_args(Args *args, const int argc, const char *argv[]);

#endif