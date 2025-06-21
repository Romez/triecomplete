#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "trie.h"

#define READ_BUF_SIZE 1024
#define SEARCH_BUF_SIZE 1024
#define MAX_EVENTS 8

#define BREAK_LINT_ROW 1
#define WORDS_COUNT_ROW 2
#define WORDS_ROW 3

bool running = true;

void handle_exit(int sig) { running = false; }

void usage(char *program) { printf("Usage: %s ./file.txt\n", program); }

void clear_words_list(int rows) {
    for (size_t i = 0; i < rows - 3; i++) {
        move(WORDS_ROW + i, 0);
        clrtoeol();
    }
}

void print_words_count(size_t count) {
    move(WORDS_COUNT_ROW, 0);
    clrtoeol();
    printw("found: %ld", count);
}

void draw_break_line(int cols) {
    move(BREAK_LINT_ROW, 0);
    hline(ACS_HLINE, cols);
}

void *screen_input(void* args) {
    int pipe_fd = *(int*)args;
    char buf[1];

    while(true) {
        int c = getch();

        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
            buf[0] = c;
            ssize_t bytes_written = write(pipe_fd, buf, 1);
            if (bytes_written == -1) {
                perror("write char");
                exit(1);
            }

        } else if (c == KEY_BACKSPACE || c == 127 || c == 8) {
            buf[0] = 127;
            ssize_t bytes_written = write(pipe_fd, buf, 1);
            if (bytes_written == -1) {
                perror("write char");
                exit(1);
            }
        }
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    // signal(SIGINT, handle_exit);

    int ep_fd = epoll_create(1);
    if (ep_fd == -1) {
        perror("epoll create");
        exit(1);
    }

    // Create Pipe

    int pipe_fd[2];
    if (pipe2(pipe_fd, O_NONBLOCK) == -1) {
        perror("pipe");
        exit(1);
    }

    // -----------

    // Register Event

    struct epoll_event ep_event;

    ep_event.events = EPOLLIN;
    ep_event.data.fd = pipe_fd[0]; // The only way to unders the source of event

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, pipe_fd[0], &ep_event) == -1) {
        perror("epoll ctl pipe");
        exit(1);
    }

    // -------------

    // Read Input Buffer

    char buf[READ_BUF_SIZE] = {0};
    size_t buf_len = 0;

    // -------------

    // Search Buffer

    char search_buf[SEARCH_BUF_SIZE] = {0};
    size_t search_buf_len = 0;

    // ------------

    // Init Screen
    initscr();
    noecho();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    draw_break_line(cols);

    move(0, 0);
    printw(">");
    move(0, 1);

    // ------------

    // Create Trie

    node_t* trie = load_trie(argv[1]);

    // TODO: exit if NULL with message

    words_t* words = NULL;

    // -----------

    // Create Thread

    pthread_t screen_thread;

    pthread_create(&screen_thread, NULL, screen_input, &pipe_fd[1]);

    // epoll wait

    struct epoll_event events[MAX_EVENTS];

    while(true) {
        int events_count = epoll_wait(ep_fd, events, MAX_EVENTS, -1);
        if (events_count == -1) {
            perror("epoll wait");
            exit(1);
        }

        for (int i = 0; i < events_count; i++) {
            int ev_fd = events[i].data.fd;

            if (ev_fd == pipe_fd[0]) {
                int read_bytes = read(pipe_fd[0], buf, READ_BUF_SIZE);
                if (read_bytes == -1) {
                    perror("read pipe");
                        exit(1);
                }

                for (int i = 0; i < read_bytes; i++) {
                    int c = buf[i];
                    if (c == 127) {
                        if (search_buf_len > 0) {
                            search_buf_len--;
                            search_buf[search_buf_len] = '\0';
                        }
                    } else {
                        search_buf[search_buf_len++] = c;
                    }
                }

                // Render screen

                // Print Search Line
                move(0, 0);
                clrtoeol();

                printw(">%s", search_buf);

                if (search_buf_len == 0) {
                    print_words_count(0);
                    clear_words_list(rows);
                } else {
                    if (words != NULL) {
                        free_words(words);
                        words = NULL;
                    }

                    words = prefix_to_words(trie, search_buf);

                    clear_words_list(rows);

                    if (words != NULL) {
                        print_words_count(words->len);

                        size_t max_words_to_show = rows - WORDS_ROW < words->len ? rows - WORDS_ROW : words->len;

                        for (size_t i = 0; i < max_words_to_show; i++) {
                            move(WORDS_ROW + i, 0);

                            word_t w = words->words[i];
                            printw("%.*s %ld", (int)w.word_len, w.word, w.freq);
                        }
                    } else {
                        print_words_count(0);
                    }
                }

                refresh();
            } else {
                assert(NULL && "Unreachable");
            }
        }
    }

    pthread_join(screen_thread, NULL);

    endwin();

    printf("Buy\n");

    return 0;
}
