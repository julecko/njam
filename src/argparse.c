#include "./argparse.h"
#include "./ip_utils.h"
#include "./mask_utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <netinet/in.h>


inline void print_usage() {
    printf("Usage: njam <ip-address>/<subnet> -i <interface>\n");    
}

inline void args_print(Args *args) {
    printf(
        "Args:\n"
        "    IP: %d\n"
        "    Mask: %d\n"
        "    Interface: %s\n",
        args->ip, args->mask, args->interface
    );
}

static inline bool args_check(Args *args) {
    return (args->ip != 0 && args->mask != 0 && args->interface != NULL);
}

static bool get_ip_and_mask(const char *arg, uint32_t *ip, uint32_t *mask) {
    const char *slash = strchr(arg, '/');
    if (!slash) return false;

    size_t ip_len = slash - arg;
    if (ip_len >= INET_ADDRSTRLEN) return false;

    char ip_str[INET_ADDRSTRLEN];
    strncpy(ip_str, arg, ip_len);
    ip_str[ip_len] = '\0';

    const char *mask_str = slash + 1;

    return ip_str_to_uint32(ip_str, ip) && parse_mask_length(mask_str, mask);
}

bool parse_args(Args *args, const int argc, const char *argv[]) {
    if (argc < 4) {
        printf("Not enough arguments\n");
        return false;
    }

    const char *arg;
    for (int i=1;i<argc;i++){
        arg = argv[i];
        if (strcmp(arg, "-i") == 0){
            if (args->interface != NULL) {
                printf("Duplicate use of -i flag detected\n");
                return false;
            }
            arg = argv[++i];
            args->interface = arg;
        } else if (args->ip == 0 && args->mask == 0) {
            if (!get_ip_and_mask(arg, &args->ip, &args->mask)) {
                printf("Wrong IP/Subnet format\n");
                return false;
            }
        } else {
            printf("Unknown argument %s\n", arg);
            return false;
        }

    }


    return args_check(args);
}
