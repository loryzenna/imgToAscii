// img-to-ascii: turns an image, a GIF or a video into ASCII art.
//
// The path every image takes:
//   load.c     reads the file and decodes it into RGBA frames
//   subject.c  finds the subject and makes the background transparent
//   image.c    crops to the subject and computes brightness
//   render.c   picks a character for every block of pixels and prints it
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>                                 // isatty, usleep (MinGW too)

#include "load.h"
#include "subject.h"
#include "image.h"
#include "render.h"
#include "term.h"

#define RESIZE_POLL_MS 100                          // with -f on a still image: how often to check the window size

// Everything that can be chosen from the command line
struct options {
    const char *input_path;
    struct style style;
    int columns;                                    // drawing width in characters
    int fit_screen;                                 // -f: fill the terminal instead of using columns
    int remove_background;
    float subject_threshold;
};

// Where to draw inside the terminal with -f
struct layout {
    int cols, rows;                                 // drawing size in characters
    int pad_left, pad_top;                          // margins that center it
};

static void print_usage(const char *program) {
    fprintf(stderr,
        "usage: %s [-c] [-f] img [style] [columns] [edge threshold / shape contrast] [subject 1/0] [subject threshold 0..1]\n"
        "-c: color each character with the average color of the image under it (truecolor ANSI)\n"
        "-f: fill the terminal and follow window resizes until Ctrl+C, still images too (ignores columns)\n"
        "styles: symbols (default), skull, shapes, edges\n"
        "subject threshold: default 0.5, lower = looser crop (keeps dark clothes)\n"
        "animated GIFs and videos: loop until Ctrl+C (when writing to a file, each frame once)\n"
        "mp4, webm, webp and other formats: need ffmpeg on the PATH\n", program);
}

static void fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

// Reads a numeric argument; if it isn't a number, exits saying which argument is wrong
static double parse_number(const char *text, const char *arg_name) {
    char *end;
    double value = strtod(text, &end);
    if (end == text || *end) {
        fprintf(stderr, "%s: \"%s\" is not a number\n", arg_name, text);
        exit(1);
    }
    return value;
}

// Removes the flags (-c, -f) from argv, wherever they are: what's left are the positional arguments in order
static void take_flags(int *argc, char **argv, int *color, int *fit_screen) {
    for (int i = 1; i < *argc; i++) {
        int *flag = NULL;
        if (!strcmp(argv[i], "-c")) flag = color;
        if (!strcmp(argv[i], "-f")) flag = fit_screen;
        if (!flag) continue;

        *flag = 1;
        memmove(argv + i, argv + i + 1, (*argc - i) * sizeof *argv);  // also moves the trailing NULL back
        (*argc)--;
        i--;
    }
}

// The checks are written as !(value within range), so they reject nan too
static struct options parse_options(int argc, char **argv) {
    struct options opt = {0};
    int color = 0;
    take_flags(&argc, argv, &color, &opt.fit_screen);
    if (argc < 2 || argc > 7) {
        print_usage(argv[0]);
        exit(1);
    }
    opt.input_path = argv[1];

    const char *style_name = argc > 2 ? argv[2] : "symbols";
    if (!style_init(&opt.style, style_name)) {
        fprintf(stderr, "unknown style: %s (symbols, skull, shapes, edges)\n", style_name);
        exit(1);
    }
    opt.style.color = color;
    int is_edge_style = !opt.style.charset;

    double columns = argc > 3 ? parse_number(argv[3], "columns") : 100;
    if (!(columns >= 1 && columns <= 100000)) fail("columns: must be between 1 and 100000");
    opt.columns = (int)columns;

    // The same argument is the threshold for the edge style and the contrast for the others
    float tuning = argc > 4 ? parse_number(argv[4], "threshold/contrast") : is_edge_style ? 100 : 2;
    if (!(tuning > 0)) fail("threshold/contrast: must be greater than 0");
    opt.style.edge_threshold = tuning;
    opt.style.contrast = tuning;

    double subject = argc > 5 ? parse_number(argv[5], "subject") : 1;
    if (subject != 0 && subject != 1) fail("subject: must be 1 (remove the background) or 0 (whole image)");
    opt.remove_background = subject == 1;

    opt.subject_threshold = argc > 6 ? parse_number(argv[6], "subject threshold") : .5f;
    if (!(opt.subject_threshold > 0 && opt.subject_threshold < 1)) fail("subject threshold: must be between 0 and 1, exclusive");
    return opt;
}

// The largest drawing that fits in the terminal without distorting the image, centered.
// A character is about twice as tall as it is wide, hence the 2.
static struct layout fit_in_terminal(int term_cols, int term_rows, int width, int height) {
    struct layout fit;
    int usable_rows = term_rows > 1 ? term_rows - 1 : 1;  // the last row stays free for the cursor

    fit.cols = term_cols;                           // first try: the full width
    fit.rows = fit.cols * height / (2 * width);
    if (fit.rows > usable_rows) {                   // too tall: height decides
        fit.rows = usable_rows;
        fit.cols = usable_rows * 2 * width / height;
    }
    if (fit.cols < 1) fit.cols = 1;
    if (fit.rows < 1) fit.rows = 1;

    fit.pad_left = (term_cols - fit.cols) / 2;
    fit.pad_top = (usable_rows - fit.rows) / 2;
    return fit;
}

int main(int argc, char **argv) {
    struct options opt = parse_options(argc, argv);

    int width, height, frame_count, *frame_delays_ms;
    unsigned char *frames = load_frames(opt.input_path, &width, &height, &frame_count, &frame_delays_ms);
    if (!frames) return 1;

    // The background becomes transparent: from here on only the subject matters
    if (opt.remove_background) {
        unsigned char *mask = subject_mask(frames, width, height, frame_count, opt.subject_threshold);
        for (long k = 0; mask && k < (long)width * height * frame_count; k++)
            if (!mask[k]) frames[k * 4 + 3] = 0;
        free(mask);
    }

    crop_opaque(frames, &width, &height, frame_count);
    const long pixels_per_frame = (long)width * height;
    int *luma_frames = luminance(frames, pixels_per_frame * frame_count);

    // Each character covers a block of pixels, twice as tall as it is wide.
    // At least 2 pixels wide: a character's shape is compared over 2 columns.
    int block_w = width / opt.columns;
    if (block_w < 2) block_w = 2;
    int block_h = block_w * 2;

    // Animation in the terminal: each frame is drawn over the previous one, until Ctrl+C.
    // With -f a still image stays on screen too, redrawn when the window is resized.
    // To a file (or for a still image without -f) each frame is written only once.
    int term_cols, term_rows;
    int animate = isatty(1) && (frame_count > 1 || (opt.fit_screen && term_size(&term_cols, &term_rows)));
    term_init(animate, opt.style.color);

    int *fit_luma = NULL;                           // with -f: the frame resampled to the terminal size
    unsigned char *fit_rgba = NULL;
    int last_term_cols = 0, last_term_rows = 0;

    for (int f = 0; ; f = (f + 1) % frame_count) {
        const int *luma = luma_frames + f * pixels_per_frame;
        const unsigned char *rgba = frames + f * pixels_per_frame * 4;
        int draw_w = width, draw_h = height;
        int draw_block_w = block_w, draw_block_h = block_h;
        int pad_left = 0, pad_top = 0;

        int fit = opt.fit_screen && term_size(&term_cols, &term_rows);
        if (fit) {
            int resized = term_cols != last_term_cols || term_rows != last_term_rows;
            // A still image only needs drawing again when the window changes size
            if (animate && frame_count == 1 && !resized) {
                usleep(RESIZE_POLL_MS * 1000);
                continue;
            }
            // The window changed (or this is the first frame): clear it, or pieces of the old drawing stay behind
            if (resized) printf("\033[2J");
            last_term_cols = term_cols;
            last_term_rows = term_rows;

            // Resample to fixed 6x12 pixel blocks per character, so the drawing comes out at exactly the wanted size
            struct layout layout = fit_in_terminal(term_cols, term_rows, width, height);
            pad_left = layout.pad_left;
            pad_top = layout.pad_top;
            draw_block_w = 6;
            draw_block_h = 12;
            draw_w = layout.cols * draw_block_w;
            draw_h = layout.rows * draw_block_h;
            fit_luma = realloc(fit_luma, sizeof *fit_luma * draw_w * draw_h);
            fit_rgba = realloc(fit_rgba, (size_t)draw_w * draw_h * 4);
            resample(luma, rgba, width, height, fit_luma, fit_rgba, draw_w, draw_h);
            luma = fit_luma;
            rgba = fit_rgba;
        }

        if (animate || fit) printf("\033[H");      // cursor to the top left
        for (int k = 0; k < pad_top; k++) putchar('\n');
        render_frame(&opt.style, luma, rgba, draw_w, draw_h, draw_block_w, draw_block_h, pad_left);

        if (!animate) {
            if (frame_count > 1) putchar('\n');     // to a file: a blank line between frames
            if (f == frame_count - 1) break;
            continue;
        }
        fflush(stdout);
        if (frame_count == 1) continue;             // still image: the next pass waits for a resize
        // Like browsers do: a delay under 20 ms counts as 100 ms
        int delay_ms = frame_delays_ms[f] < 20 ? 100 : frame_delays_ms[f];
        usleep(delay_ms * 1000);
    }

    free(fit_luma);
    free(fit_rgba);
    free(luma_frames);
    free(frame_delays_ms);
    free(frames);
    return 0;
}
