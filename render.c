#define _USE_MATH_DEFINES                           // M_PI on Windows (MSVC)
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "glyph.h"
#include "image.h"
#include "render.h"

// A character is split into 2 columns x 3 rows of regions (see glyph.h)
enum { REGIONS = 6 };

static const char brightness_ramp[] = " .:-=+*#%@"; // edge style: from darkest to brightest
static const char edge_chars[] = "|/_\\";           // edge style: one character per direction, 0°, 45°, 90°, 135°

int style_init(struct style *style, const char *name) {
    if (!strcmp(name, "symbols"))      style->charset = " .,:;'`\"^-$";
    else if (!strcmp(name, "skull"))   style->charset = " .,:;'`\"^-_|/\\ijdkoLJISP7?4$";
    else if (!strcmp(name, "shapes"))  style->charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    else if (!strcmp(name, "edges"))   style->charset = NULL;
    else return 0;

    // Copy the chosen characters' shapes and scale them: the fullest character in the set reaches 1
    style->charset_len = style->charset ? (int)strlen(style->charset) : 0;
    float fullest = 0;
    for (int c = 0; c < style->charset_len; c++)
        for (int r = 0; r < REGIONS; r++) {
            style->glyph_shape[c][r] = GLYPH_COVERAGE[style->charset[c] - ' '][r];
            if (style->glyph_shape[c][r] > fullest) fullest = style->glyph_shape[c][r];
        }
    for (int c = 0; c < style->charset_len; c++)
        for (int r = 0; r < REGIONS; r++) style->glyph_shape[c][r] /= fullest;
    return 1;
}

// Edge style. Every pixel in the block with a strong enough gradient (Sobel) votes for the edge direction.
// If one direction gets enough votes an edge is drawn, otherwise a character based on brightness.
static char edge_char(const struct style *style, const int *luma, const unsigned char *rgba,
                      int width, int height, int x, int y, int block_w, int block_h) {
    int direction_votes[4] = {0};
    for (int j = y; j < y + block_h; j++) {
        if (j < 1 || j >= height - 1) continue;     // Sobel needs neighbors: skip the image border
        for (int i = x; i < x + block_w; i++) {
            if (i < 1 || i >= width - 1) continue;
            const int *p = luma + j * width + i;
            const int w = width;                    // p[-w] is the pixel above, p[w] the one below
            int grad_x = (p[-w+1] + 2*p[1] + p[w+1]) - (p[-w-1] + 2*p[-1] + p[w-1]);
            int grad_y = (p[w-1] + 2*p[w] + p[w+1]) - (p[-w-1] + 2*p[-w] + p[-w+1]);
            if (grad_x * grad_x + grad_y * grad_y < style->edge_threshold * style->edge_threshold) continue;

            double angle_deg = atan2(grad_y, grad_x) * 180 / M_PI;  // y grows downwards
            if (angle_deg < 0) angle_deg += 180;    // an edge going left to right is the same as one going right to left
            direction_votes[(int)((angle_deg + 22.5) / 45) % 4]++;
        }
    }
    int best_direction = 0;
    for (int d = 1; d < 4; d++)
        if (direction_votes[d] > direction_votes[best_direction]) best_direction = d;

    // ponytail: vote threshold fixed at 1/8 of the block, make it an argument if it needs tuning
    if (direction_votes[best_direction] * 8 >= block_w * block_h) return edge_chars[best_direction];
    if (!is_opaque(rgba + ((long)y * width + x) * 4)) return ' ';  // background

    const int ramp_last = sizeof brightness_ramp - 2;  // -1 for the terminator, -1 for the index
    return brightness_ramp[luma[y * width + x] * ramp_last / 255];  // one pixel per block is enough
}

// The other styles: measure the block's brightness in the same 6 regions as the characters
// and pick the character in the set with the most similar shape.
static char shape_char(const struct style *style, const int *luma, int width, int x, int y, int block_w, int block_h) {
    float brightness[REGIONS] = {0};
    int pixels[REGIONS] = {0};
    for (int j = y; j < y + block_h; j++)
        for (int i = x; i < x + block_w; i++) {
            int row = (j - y) * 3 / block_h, col = (i - x) * 2 / block_w;
            brightness[row * 2 + col] += luma[j * width + i];
            pixels[row * 2 + col]++;
        }

    float brightest = 0;
    for (int r = 0; r < REGIONS; r++) {
        brightness[r] /= pixels[r] * 255.f;         // average, from 0 to 1
        if (brightness[r] > brightest) brightest = brightness[r];
    }
    // Contrast pushes the block's darker regions towards 0, so the character follows the outline better
    if (brightest > 0)
        for (int r = 0; r < REGIONS; r++)
            brightness[r] = powf(brightness[r] / brightest, style->contrast) * brightest;

    int best_char = 0;
    float best_distance = 1e9f;
    for (int c = 0; c < style->charset_len; c++) {
        float distance = 0;
        for (int r = 0; r < REGIONS; r++) {
            float diff = brightness[r] - style->glyph_shape[c][r];
            distance += diff * diff;
        }
        if (distance < best_distance) {
            best_distance = distance;
            best_char = c;
        }
    }
    return style->charset[best_char];
}

// Prints ch in the average color of the block's opaque pixels
static void print_colored(char ch, const unsigned char *rgba, int width, int x, int y, int block_w, int block_h) {
    long red = 0, green = 0, blue = 0, opaque = 0;
    for (int j = y; j < y + block_h; j++)
        for (int i = x; i < x + block_w; i++) {
            const unsigned char *pixel = rgba + ((long)j * width + i) * 4;
            if (!is_opaque(pixel)) continue;
            red += pixel[0];
            green += pixel[1];
            blue += pixel[2];
            opaque++;
        }
    if (opaque == 0) opaque = 1;                    // fully transparent block: black, but this almost never happens
    printf("\033[38;2;%ld;%ld;%ldm%c", red / opaque, green / opaque, blue / opaque, ch);
}

void render_frame(const struct style *style, const int *luma, const unsigned char *rgba,
                  int width, int height, int block_w, int block_h, int pad_left) {
    for (int y = 0; y + block_h <= height; y += block_h) {
        printf("%*s", pad_left, "");
        for (int x = 0; x + block_w <= width; x += block_w) {
            char ch = style->charset ? shape_char(style, luma, width, x, y, block_w, block_h)
                                     : edge_char(style, luma, rgba, width, height, x, y, block_w, block_h);
            if (style->color && ch != ' ') print_colored(ch, rgba, width, x, y, block_w, block_h);
            else putchar(ch);                       // a space needs no color
        }
        if (style->color) printf("\033[0m");       // reset the color, or it bleeds into the next line
        putchar('\n');
    }
}
