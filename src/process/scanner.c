#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "./process/scanner.h"
#include "./network.h"
#include "./arp.h"

#define MAX_LINE 1024

struct termios orig_termios;
static DeviceGroup group = {0};

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf(EXIT_ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // raw input
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf(ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

void print_table(Network network) {
    static int counter = 0;
    if (group.devices != NULL) {
        free(group.devices);
    }

    printf(TOP_LEFT_CODE);
    printf(CLEAR_SCREEN_CODE_FULL);

    group = print_network_nice(network);

    printf("NJam console > ", counter++);
    fflush(stdout);
}

static void reset_input(char *input, size_t *input_len) {
    *input_len = 0;
    input[0] = '\0';
}

static void print_help() {
    printf("\nCommands:\n");
    printf("  help          Show this help message\n");
    printf("  scan          Trigger network scan\n");
    printf("  <number>      Trigger device jamming by index\n");
    printf("  all           Trigger jamming of all devices alive\n");
    printf("  die           Set all devices to disconnecting\n");
    printf("  armagedon     Trigger jamming everything\n");
    printf("  q             Quit\n\n");
}

static int is_number(const char *str) {
    for (; *str; str++) {
        if (!isdigit((unsigned char)*str)) return 0;
    }
    return 1;
}

void stall_program() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag |= ICANON;
    newt.c_lflag |= ECHO;
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt);

    printf("Press any key to continue...\n");
    fflush(stdout);

    char dummy;
    read(STDIN_FILENO, &dummy, 1);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
}


static void handle_backspace(char *input, size_t *input_len) {
    if (*input_len > 0) {
        (*input_len)--;
        input[*input_len] = '\0';
        printf("\b \b");
        fflush(stdout);
    }
}

static void handle_char(char c, char *input, size_t *input_len) {
    if ((c >= 32 && c <= 126) && *input_len < MAX_LINE - 1) {
        input[(*input_len)++] = c;
        write(STDOUT_FILENO, &c, 1);
    }
}

static int handle_enter(Network network, char *input, size_t *input_len) {
    int return_code = 0;
    input[*input_len] = '\0';

    if (*input_len == 0 || strcmp(input, "help") == 0) {
        print_help();
        stall_program();
    } else if (strcmp(input, "q") == 0 || strcmp(input, "exit") == 0) {
        return 2;
    } else if (strcmp(input, "scan") == 0) {
        printf("\nScaning...\n");
        return_code = 1;
    } else if (strcmp(input, "all") == 0) {
        pthread_mutex_lock(&network.lock);
        for (size_t i=0;i<group.device_count;i++){
            if (group.devices[i]->alive) {
                group.devices[i]->status = JAMMING;
            }
        }
        pthread_mutex_unlock(&network.lock);
    } else if (strcmp(input, "die") == 0) {
        pthread_mutex_lock(&network.lock);
        for (size_t i=1;i<network.device_count;i++){
            if (network.devices[i].status == JAMMING) {
                network.devices[i].status = DISCONNECTING;
            }
        }
        pthread_mutex_unlock(&network.lock);
    } else if (strcmp(input, "armagedon") == 0) {
        pthread_mutex_lock(&network.lock);
        for (size_t i=1;i<network.device_count;i++){
            network.devices[i].status = JAMMING;
        }
        pthread_mutex_unlock(&network.lock);
    }  else if (strncmp(input, "jam ", 4) == 0 && is_number(input + 4)) {
        int idx = atoi(input + 4);
        if (idx < 1 || idx > (int)group.device_count) {
            printf("\nInvalid device index: %d\n", idx);
            stall_program();
        } else {
            pthread_mutex_lock(&network.lock);
            DeviceStatus status = group.devices[idx - 1]->status;
            if (status == INACTIVE || status == DISCONNECTING) {
                group.devices[idx - 1]->status = JAMMING;
            } else if (status == JAMMING) {
                group.devices[idx - 1]->status = DISCONNECTING;
            }
            pthread_mutex_unlock(&network.lock);
        }
    } else if (strncmp(input, "router ", 7) == 0 && is_number(input + 7)) {
        int idx = atoi(input + 7);
        if (idx < 1 || idx > (int)group.device_count) {
            printf("\nInvalid device index: %d\n", idx);
            stall_program();
        } else {
            pthread_mutex_lock(&network.lock);
            DeviceType type = group.devices[idx - 1]->type;
            if (type == CLIENT) {
                group.devices[idx - 1]->type = ROUTER;
            } else {
                group.devices[idx - 1]->type = CLIENT;
            }
            pthread_mutex_unlock(&network.lock);
        }
    }
    else {
        printf("\nUnknown command: %s\n", input);
        print_help();
        stall_program();
    }

    fflush(stdout);
    reset_input(input, input_len);
    return return_code;
}

static void redraw_screen(Network network, const char *input) {
    print_table(network);
    printf("%s", input);
    fflush(stdout);
}

void scanner_process(Network network, int sockfd, uint32_t ip) {
    uint8_t my_mac[6];
    char input[MAX_LINE] = {0};
    size_t input_len = 0;

    get_interface_mac("wlan0", my_mac);
    
    enable_raw_mode();

    int epfd = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = STDIN_FILENO };
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    printf("Scanning...\n");
    arp_scan_range(network, sockfd, "wlan0", my_mac, &ip);

    while (1) {
        struct epoll_event events[1];
        int nfds = epoll_wait(epfd, events, 1, 500);

        if (nfds > 0) {
            char c;
            ssize_t bytes_read = read(STDIN_FILENO, &c, 1);
            if (bytes_read > 0) {
                if (c == '\n') {
                    switch(handle_enter(network, input, &input_len)){
                        case 1:
                            arp_scan_range(network, sockfd, "wlan0", my_mac, &ip);
                            break;
                        case 2:
                            return;
                    }
                    redraw_screen(network, input);
                } else if (c == 127 || c == '\b') {
                    handle_backspace(input, &input_len);
                } else {
                    handle_char(c, input, &input_len);
                }
            }
        } else {
            redraw_screen(network, input);
        }
    }
}
