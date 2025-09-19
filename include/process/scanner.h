#ifndef PROCESS_SCANNER_H
#define PROCESS_SCANNER_H

#include "./network.h"

#define ALTERNATE_SCREEN_CODE "\033[?1049h"
#define EXIT_ALTERNATE_SCREEN_CODE "\033[?1049l"
#define TOP_LEFT_CODE "\033[H"
#define CLEAR_SCREEN_CODE_FULL "\033[2J"
#define CLEAR_SCREEN_FROM_CURSOR "\033[J"

void disable_raw_mode();
void scanner_process(Network network, int sockfd, uint32_t ip, const char *interface);

#endif