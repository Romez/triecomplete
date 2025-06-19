#include <stdio.h>
#include <ncurses.h>
#include "trie.h"

#define BUF_SIZE 1024

int main() {
    char buf[BUF_SIZE];
    
    initscr();

    move(0, 0);

    printw("Hello world\n");

    while(true) {
	getnstr(buf, BUF_SIZE - 1);
	
	refresh();
    }
    
    endwin();
    
    return 0;
}
