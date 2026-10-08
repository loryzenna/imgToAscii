#include <stdio.h>
#include <signal.h>
#include <unistd.h>                                 // write, _exit (MinGW too)
#ifdef _WIN32
#include <windows.h>                                // console: size and ANSI sequences
#else
#include <sys/ioctl.h>                              // terminal size
#endif
#include "term.h"

int term_size(int *cols, int *rows) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return 0;
    *cols = info.srWindow.Right - info.srWindow.Left + 1;
    *rows = info.srWindow.Bottom - info.srWindow.Top + 1;
#else
    struct winsize size;
    if (ioctl(1, TIOCGWINSZ, &size) || !size.ws_col || !size.ws_row) return 0;
    *cols = size.ws_col;
    *rows = size.ws_row;
#endif
    return 1;
}

// Ctrl+C during the animation: show the cursor again before exiting
static void restore_and_exit(int signal_number) {
    (void)signal_number;
    write(1, "\033[0m\033[?25h\n", 11);               // reset the color too
    _exit(0);
}

void term_init(int animate, int color) {
#ifdef _WIN32
    if (animate || color) {
        DWORD mode;
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (GetConsoleMode(console, &mode)) SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    (void)color;
#endif
    if (animate) {
        signal(SIGINT, restore_and_exit);
        printf("\033[2J\033[?25l");                // clear the screen, hide the cursor
    }
}
