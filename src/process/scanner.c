#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <time.h>

#include "./network.h"

#define MAX_LINE 1024
#define ALTERNATE_SCREEN_CODE "\033[?1049h"
#define EXIT_ALTERNATE_SCREEN_CODE "\033[?1049l"
#define TOP_LEFT_CODE "\033[H"
#define CLEAR_SCREEN_CODE "\033[2J"

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
    printf(CLEAR_SCREEN_CODE);

    group = print_network_nice(network);

    printf("Press 'q' to quit(%d)...\n", counter++);
    fflush(stdout);
}

static void reset_input(char *input, size_t *input_len) {
    *input_len = 0;
    input[0] = '\0';
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

static void handle_enter(char *input, size_t *input_len) {
    input[*input_len] = '\0';
    if (strcmp(input, "q") == 0) {
        exit(0);
    }
    printf("\nYou typed: %s\n", input);
    fflush(stdout);
    reset_input(input, input_len);
}

static void redraw_screen(Network network, const char *input) {
    print_table(network);
    printf("%s", input);
    fflush(stdout);
}

void scanner_process(Network network) {
    enable_raw_mode();

    int epfd = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = STDIN_FILENO };
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    char input[MAX_LINE] = {0};
    size_t input_len = 0;

    print_table(network);

    while (1) {
        struct epoll_event events[1];
        int nfds = epoll_wait(epfd, events, 1, 2000);

        if (nfds > 0) {
            char c;
            ssize_t bytes_read = read(STDIN_FILENO, &c, 1);
            if (bytes_read > 0) {
                if (c == '\n') handle_enter(input, &input_len);
                else if (c == 127 || c == '\b') handle_backspace(input, &input_len);
                else handle_char(c, input, &input_len);
            }
        } else {
            redraw_screen(network, input);
        }
    }
}
