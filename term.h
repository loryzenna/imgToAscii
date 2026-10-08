#pragma once

// Columns and rows of the terminal on stdout; returns 0 if the output is not a terminal
int term_size(int *cols, int *rows);

// Prepares the terminal: on Windows it enables ANSI sequences when needed (animation or color).
// animate: full-screen animation; clears the screen, hides the cursor and shows it again on Ctrl+C
void term_init(int animate, int color);
