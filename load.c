#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "load.h"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define PIPE_READ "rb"                              // on Windows the pipe must be opened in binary mode
#else
#define PIPE_READ "r"
#endif

enum { VIDEO_FPS = 15, VIDEO_MAX_WIDTH = 480 };

// The ffmpeg command that writes the file's frames to stdout, one after another, as PPM images.
// The path is quoted for the shell, so spaces, apostrophes and $ in the name do no harm.
static char *ffmpeg_command(const char *path) {
    char *command = malloc(strlen(path) * 4 + 200);  // worst case: every character is a ' that becomes '\''
    char *end = command + sprintf(command, "ffmpeg -v error -i ");
#ifdef _WIN32
    end += sprintf(end, "\"%s\"", path);            // Windows file names can't contain "
#else
    *end++ = '\'';
    for (const char *c = path; *c; c++) {
        if (*c == '\'') end += sprintf(end, "'\\''");
        else *end++ = *c;
    }
    *end++ = '\'';
#endif
    sprintf(end, " -an -vf \"fps=%d,scale='min(%d,iw)':-1\" -f image2pipe -c:v ppm -", VIDEO_FPS, VIDEO_MAX_WIDTH);
    return command;
}

// Expands pixel_count RGB pixels in place into opaque RGBA.
// Goes from the end: each RGBA pixel lands after its RGB source, so nothing still to be read gets overwritten.
static void rgb_to_rgba(unsigned char *pixels, size_t pixel_count) {
    for (size_t k = pixel_count; k-- > 0; ) {
        unsigned char r = pixels[k * 3], g = pixels[k * 3 + 1], b = pixels[k * 3 + 2];
        pixels[k * 4] = r;
        pixels[k * 4 + 1] = g;
        pixels[k * 4 + 2] = b;
        pixels[k * 4 + 3] = 255;
    }
}

// Videos and formats stb doesn't know (mp4, webm, webp, mov, ...), decoded by ffmpeg (must be on the PATH).
// NULL if ffmpeg is missing or doesn't recognize the file.
// ponytail: every frame stays in memory (~1 MB/frame including brightness), stream them if long videos are needed
static unsigned char *ffmpeg_load(const char *path, int *width, int *height, int *frame_count, int **frame_delays_ms) {
    char *command = ffmpeg_command(path);
    FILE *ffmpeg_output = popen(command, PIPE_READ);
    free(command);
    if (!ffmpeg_output) return NULL;

    unsigned char *frames = NULL;
    *frame_count = 0;
    // Each frame is a PPM: the header "P6 width height 255", one whitespace character, then the RGB pixels
    int frame_w, frame_h;
    while (fscanf(ffmpeg_output, "P6 %d %d 255", &frame_w, &frame_h) == 2 && fgetc(ffmpeg_output) != EOF) {
        if (*frame_count == 0) {
            *width = frame_w;
            *height = frame_h;
        } else if (frame_w != *width || frame_h != *height) {
            break;                                  // shouldn't happen: ffmpeg scales them all the same
        }
        size_t pixel_count = (size_t)frame_w * frame_h;
        unsigned char *grown = realloc(frames, pixel_count * 4 * (*frame_count + 1));
        if (!grown) break;
        frames = grown;

        unsigned char *frame = frames + pixel_count * 4 * *frame_count;
        if (fread(frame, 3, pixel_count, ffmpeg_output) != pixel_count) break;
        rgb_to_rgba(frame, pixel_count);
        (*frame_count)++;
    }
    pclose(ffmpeg_output);

    if (*frame_count == 0) {
        free(frames);
        return NULL;
    }
    *frame_delays_ms = malloc(sizeof **frame_delays_ms * *frame_count);
    for (int f = 0; f < *frame_count; f++) (*frame_delays_ms)[f] = 1000 / VIDEO_FPS;
    return frames;
}

// The whole file in memory, because stb reads animated GIF frames only from memory. NULL on failure.
static unsigned char *read_file(const char *path, long *size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror(path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    unsigned char *data = malloc(*size > 0 ? *size : 1);
    if (*size <= 0 || fread(data, 1, *size, file) != (size_t)*size) {
        fprintf(stderr, "%s: read failed\n", path);
        free(data);
        data = NULL;
    }
    fclose(file);
    return data;
}

unsigned char *load_frames(const char *path, int *width, int *height, int *frame_count, int **frame_delays_ms) {
    long file_size;
    unsigned char *file_data = read_file(path, &file_size);
    if (!file_data) return NULL;

    // stb first: animated GIF, then any image (a single frame). If stb doesn't know the format, ffmpeg gets a try.
    int channels_in_file;
    *frame_count = 1;
    *frame_delays_ms = NULL;
    unsigned char *frames = stbi_load_gif_from_memory(file_data, (int)file_size, frame_delays_ms,
                                                      width, height, frame_count, &channels_in_file, 4);
    if (!frames) {
        *frame_count = 1;
        frames = stbi_load_from_memory(file_data, (int)file_size, width, height, &channels_in_file, 4);
    }
    free(file_data);

    if (!frames) frames = ffmpeg_load(path, width, height, frame_count, frame_delays_ms);
    if (!frames) fprintf(stderr, "Error: can't read %s (videos and webp need ffmpeg on the PATH)\n", path);
    return frames;
}
