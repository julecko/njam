#include "./version.h"
#include "./argparse.h"
#include "./ip_utils.h"
#include "./mask_utils.h"
#include "./network.h"
#include "./icmp.h"
#include "./arp.h"
#include "./process/scanner.h"
#include "./process/jammer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/capability.h>

#include <netinet/in.h>


volatile bool stop_flag = false;
Network *global_network = NULL;
pthread_t jam_thread;
int sockfd_scanner = -1;

void handle_sigint(int signum) {
    (void)signum;
    printf(TOP_LEFT_CODE);
    printf(CLEAR_SCREEN_CODE_FULL);
    printf("Cleaning up, can take up to 10 seconds\n");
    printf("Program is sending correct arp mapings to devices\n");

    if (global_network) {
        pthread_mutex_lock(&global_network->lock);
        for (size_t i=0;i<global_network->device_count;i++) {
            Device *device = &global_network->devices[i];
            if (device->status == JAMMING){
                device->status = DISCONNECTING;
            }
        }
        
        stop_flag = true;
        pthread_mutex_unlock(&global_network->lock);

        pthread_join(jam_thread, NULL);

        close(sockfd_scanner);
        free_network(*global_network);
        disable_raw_mode();
    }
    global_network = NULL;
    exit(0);
}

bool has_cap_net_raw() {
    cap_t caps = cap_get_proc();
    if (!caps) return 0;

    cap_flag_value_t val;
    cap_get_flag(caps, CAP_NET_RAW, CAP_EFFECTIVE, &val);

    cap_free(caps);
    return (val == CAP_SET);
}

bool has_root() {
    if (geteuid() != 0) {
        return false;
    }
    return true;
}

int main(int argc, char *argv[]){
    printf("%i\n", has_cap_net_raw());

    exit(0);
    if (argc == 2) {
        const char *arg = argv[1];
        if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            print_version();
            return EXIT_SUCCESS;
        }
    }
    if (!has_root() && !has_cap_net_raw()) {
        fprintf(stderr, "[!] This program must be run as root (use sudo) or have cap_net_raw+ep capabilities.\n");
    }

    Args args = {0};
    if (!parse_args(&args, argc, (const char **)argv)) {
        args_print(&args);
        print_usage();
        return EXIT_FAILURE;
    }

    sockfd_scanner = arp_create_socket();
    

    signal(SIGINT, handle_sigint);

    Network network = create_network(args.ip, args.mask);
    global_network = &network;

    JammerArgs jammer_args = {
        .network = &network,
        .interface = args.interface,
        .stop_flag = &stop_flag
    };
    pthread_create(&jam_thread, NULL, jam_jammed_devices, &jammer_args);

    if (sockfd_scanner != -1){
        scanner_process(network, sockfd_scanner, args.ip, args.interface);
    }
    handle_sigint(0);
}
