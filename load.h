#pragma once

// Reads an image, animated GIF or video: frame_count RGBA frames of width*height back to back, transparent = background.
// frame_delays_ms: how long each frame lasts (NULL for a still image). Free both with free().
// NULL if the file can't be opened or decoded (the reason has already been printed to stderr).
unsigned char *load_frames(const char *path, int *width, int *height, int *frame_count, int **frame_delays_ms);
