#pragma once

// An RGBA pixel counts as subject if it is at least half opaque
static inline int is_opaque(const unsigned char *pixel) {
    return pixel[3] >= 128;
}

// Crops every frame to the smallest rectangle that contains the subject.
// The rectangle is the same for all frames, so an animation doesn't jump around.
// Works in place and updates width and height; if everything is transparent it changes nothing.
void crop_opaque(unsigned char *frames, int *width, int *height, int frame_count);

// Brightness 0..255 of every RGBA pixel (all frames together); transparent pixels are 0.
// Contrast is stretched to the full range, measured only on the subject
// and across all frames together, so an animation doesn't flicker.
int *luminance(const unsigned char *rgba, long pixel_count);

// Resamples a frame (brightness and colors) from src_w*src_h to dst_w*dst_h, taking the nearest pixel
void resample(const int *src_luma, const unsigned char *src_rgba, int src_w, int src_h,
              int *dst_luma, unsigned char *dst_rgba, int dst_w, int dst_h);
