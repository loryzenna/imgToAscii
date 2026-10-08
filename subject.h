#pragma once

// Subject mask from U²-Netp (u2netp.onnx in the current folder): width*height*frame_count bytes, 1 = subject.
// NULL on failure. frames: RGBA frames back to back (animated GIF).
// threshold: cutoff between the prediction's min (0) and max (1); lower = keeps more pixels as subject
unsigned char *subject_mask(const unsigned char *frames, int width, int height, int frame_count, float threshold);
