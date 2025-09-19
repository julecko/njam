#ifndef PROCESS_JAMMER_H
#define PROCESS_JAMMER_H

#include "./network.h"

#include <stdbool.h>

typedef struct JammerArgs{
    Network *network;
    const char *interface;
    volatile bool *stop_flag;
} JammerArgs;


void *jam_jammed_devices(void *arg);

#endif