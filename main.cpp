#include <iostream>
#include <unistd.h>
#include <termios.h>
#include "colormod.h"
#include <cstdlib>
#include <cctype>
#include <cstdio>

#define CTRL_KEY(k) ((k) & 0x1f)

// Reference: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
struct termios orig_termios;

struct editorConfig {
    struct termios orig_termios;
};
struct editorConfig E;

void die(const char *s) {
    // Clear the screen upon exit
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

// Disable raw mode
void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

// Enable raw mode
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/*** input ***/
void editorProcessKeypress() {
    char c = editorReadKey();
    switch (c) {
        case ('q'):
            // Clear the screen upon exit
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

void editorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);



}



int main() {
    enableRawMode();
    // User input
    while (1) {
        editorRefreshScreen();
        std::cout << Color::Modifier(Color::FG_BLUE) << "Press Q to exit" << Color::Modifier(Color::FG_DEFAULT) << std::endl;
        editorProcessKeypress();
    }

    return 0;
}
