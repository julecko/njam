#ifndef PROCESS_SCANNER_H
#define PROCESS_SCANNER_H

#include "./network.h"

void scanner_process(Network network, int sockfd, uint32_t ip);

#endif