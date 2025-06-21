#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include "trie.h"

#define BUF_SIZE 1024

void usage(char *program) { printf("Usage: %s ./file.txt\n", program); }

void clear_words_list(int rows) {
    for (size_t i = 0; i < rows - 3; i++) {
        move(3 + i, 0);
        clrtoeol();
    }
}

void print_words_count(size_t count) {
    move(2, 0);
    clrtoeol();
    printw("found: %ld", count);
}

void draw_line(int cols) {
    move(1, 0);
    hline(ACS_HLINE, cols);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    node_t* trie = load_trie(argv[1]);

    char buf[BUF_SIZE] = {0};
    size_t buf_len = 0;

    initscr();
    noecho();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    draw_line(cols);

    words_t* words = NULL;

    while(true) {
        if (buf_len > 0) {
            if (words != NULL) {
                free_words(words);
                words = NULL;
            }

            words = prefix_to_words(trie, buf);

            clear_words_list(rows);

            if (words != NULL) {
                print_words_count(words->len);

                for (size_t i = 0; i < words->len; i++) {
                    move(3 + i, 0);

                    word_t w = words->words[i];
                    printw("%.*s %ld", (int)w.word_len, w.word, w.freq);
                }
            } else {
                print_words_count(0);
            }
        } else {
            if (words != NULL) {
                free_words(words);
                words = NULL;
            }

            print_words_count(0);
            clear_words_list(rows);
        }

        move(0, 0);
        clrtoeol();

        printw(">%s", buf);

        move(0, buf_len + 1);

        int c = getch();

        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
            buf[buf_len++] = c;
        } else if (c == KEY_BACKSPACE || c == 127 || c == 8) {
            if (buf_len > 0) {
                buf_len--;
                buf[buf_len] = '\0';
            }
        }

        refresh();
    }

    endwin();

    return 0;
}
