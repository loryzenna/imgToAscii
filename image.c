#include <stdlib.h>
#include <string.h>
#include "image.h"

void crop_opaque(unsigned char *frames, int *width, int *height, int frame_count) {
    const int w = *width, h = *height;
    const long pixels_per_frame = (long)w * h;

    // Find the subject's bounds across all frames
    int left = w, top = h, right = -1, bottom = -1;
    for (int f = 0; f < frame_count; f++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                if (!is_opaque(frames + (f * pixels_per_frame + (long)y * w + x) * 4)) continue;
                if (x < left) left = x;
                if (x > right) right = x;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
    if (right < 0) return;                          // no opaque pixels

    // Pack the cropped rows at the start of the buffer.
    // Each row lands before (or where) it was, so copying them in order never overwrites anything still to be read.
    int crop_w = right - left + 1, crop_h = bottom - top + 1;
    for (int f = 0; f < frame_count; f++)
        for (int y = top; y <= bottom; y++) {
            unsigned char *dst = frames + ((long)f * crop_w * crop_h + (long)(y - top) * crop_w) * 4;
            unsigned char *src = frames + (f * pixels_per_frame + (long)y * w + left) * 4;
            memmove(dst, src, crop_w * 4);
        }
    *width = crop_w;
    *height = crop_h;
}

int *luminance(const unsigned char *rgba, long pixel_count) {
    int *luma = malloc(sizeof *luma * pixel_count);
    int darkest = 255, brightest = 0;
    for (long k = 0; k < pixel_count; k++) {
        const unsigned char *pixel = rgba + k * 4;
        if (!is_opaque(pixel)) {
            luma[k] = 0;
            continue;
        }
        luma[k] = (299 * pixel[0] + 587 * pixel[1] + 114 * pixel[2]) / 1000;  // standard weights for the human eye
        if (luma[k] < darkest) darkest = luma[k];
        if (luma[k] > brightest) brightest = luma[k];
    }
    if (brightest == darkest) brightest = darkest + 1;  // image is a single flat gray: avoid dividing by 0

    for (long k = 0; k < pixel_count; k++)
        if (is_opaque(rgba + k * 4)) luma[k] = (luma[k] - darkest) * 255 / (brightest - darkest);
    return luma;
}

// ponytail: nearest pixel, switch to area averaging if fine details flicker
void resample(const int *src_luma, const unsigned char *src_rgba, int src_w, int src_h,
              int *dst_luma, unsigned char *dst_rgba, int dst_w, int dst_h) {
    for (int y = 0; y < dst_h; y++)
        for (int x = 0; x < dst_w; x++) {
            long src = (long)y * src_h / dst_h * src_w + (long)x * src_w / dst_w;
            long dst = (long)y * dst_w + x;
            dst_luma[dst] = src_luma[src];
            memcpy(dst_rgba + dst * 4, src_rgba + src * 4, 4);
        }
}
