#pragma once

// Drawing style. edges (charset NULL): brightness ramp + Sobel edges; the others: match the shape of the charset's characters
struct style {
    const char *charset;                            // candidate characters, NULL = edge style
    int charset_len;
    float glyph_shape[95][6];                       // shape of each charset character, the fullest one scaled to 1
    float edge_threshold;                           // edge style: minimum gradient magnitude to count as an edge
    float contrast;                                 // other styles: higher = sharper outlines
    int color;                                      // color each character with the block's average color
};

// Sets up the style by name (symbols, skull, shapes, edges); returns 0 if the name is unknown.
// edge_threshold, contrast and color must be set afterwards
int style_init(struct style *style, const char *name);

// Prints a frame to stdout: one character per block_w*block_h block, pad_left spaces at the start of each line.
// luma: brightness 0..255, rgba: colors, both width*height
void render_frame(const struct style *style, const int *luma, const unsigned char *rgba,
                  int width, int height, int block_w, int block_h, int pad_left);
