#include <iostream>
#include <unistd.h>
#include <termios.h>
#include "colormod.h"
#include <cstdlib>
#include <cctype>
#include <cstdio>

// Reference: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
struct termios orig_termios;

// Disable raw mode
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Enable raw mode
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();
    // User input
    char c;
    // Terminate the program if the user enters 'q'
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {

    }

    disableRawMode();
    return 0;
}
