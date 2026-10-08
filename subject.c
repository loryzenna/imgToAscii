#include <stdio.h>
#include <stdlib.h>
#include <onnxruntime_c_api.h>
#include "subject.h"

// The network wants a square MODEL_SIZE x MODEL_SIZE image, one color channel after another
enum { MODEL_SIZE = 320, PLANE = MODEL_SIZE * MODEL_SIZE };

// Builds the network input from an RGBA frame: resizes it and normalizes it like the images
// it was trained on (ImageNet).
// ponytail: nearest-pixel resize, switch to bilinear if the subject's edges come out jagged
static void fill_model_input(float *input, const unsigned char *rgba, int width, int height) {
    const float imagenet_mean[3] = {.485f, .456f, .406f};
    const float imagenet_std[3] = {.229f, .224f, .225f};
    float max_value = 1;
    for (int c = 0; c < 3; c++)
        for (int k = 0; k < PLANE; k++) {
            int src_x = k % MODEL_SIZE * width / MODEL_SIZE;
            int src_y = k / MODEL_SIZE * height / MODEL_SIZE;
            input[c * PLANE + k] = rgba[((long)src_y * width + src_x) * 4 + c];
            if (input[c * PLANE + k] > max_value) max_value = input[c * PLANE + k];
        }
    for (int c = 0; c < 3; c++)
        for (int k = 0; k < PLANE; k++)
            input[c * PLANE + k] = (input[c * PLANE + k] / max_value - imagenet_mean[c]) / imagenet_std[c];
}

// Turns the network's prediction (one value per pixel, higher = more subject) into a 0/1 mask
// the size of the frame. The network isn't calibrated, so the cutoff is relative: between the min and the max.
static void mask_from_prediction(unsigned char *mask, const float *prediction, int width, int height, float threshold) {
    float pred_min = prediction[0], pred_max = prediction[0];
    for (int k = 1; k < PLANE; k++) {
        if (prediction[k] < pred_min) pred_min = prediction[k];
        if (prediction[k] > pred_max) pred_max = prediction[k];
    }
    float cutoff = pred_min + (pred_max - pred_min) * threshold;
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            mask[(long)y * width + x] = prediction[y * MODEL_SIZE / height * MODEL_SIZE + x * MODEL_SIZE / width] > cutoff;
}

unsigned char *subject_mask(const unsigned char *frames, int width, int height, int frame_count, float threshold) {
    const OrtApi *ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!ort) {
        fprintf(stderr, "subject: onnxruntime version differs from the header\n");
        return NULL;
    }
    OrtEnv *env = NULL;
    OrtSessionOptions *options = NULL;
    OrtSession *session = NULL;
    OrtMemoryInfo *memory_info = NULL;
    OrtAllocator *allocator = NULL;
    OrtValue *input_tensor = NULL, *output_tensor = NULL;
    OrtStatus *status;
    char *input_name = NULL, *output_name = NULL;
    unsigned char *mask = malloc((size_t)width * height * frame_count);
    float *input = malloc(sizeof *input * 3 * PLANE);
    float *prediction;

    // Any onnxruntime call can fail: if it does, print why, drop the mask and free everything
#define CHECK(call) \
    if ((status = (call))) { \
        fprintf(stderr, "subject: %s\n", ort->GetErrorMessage(status)); \
        ort->ReleaseStatus(status); \
        free(mask); \
        mask = NULL; \
        goto cleanup; \
    }

    CHECK(ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "ascii", &env));
    CHECK(ort->CreateSessionOptions(&options));
    CHECK(ort->CreateSession(env, ORT_TSTR("u2netp.onnx"), options, &session));
    CHECK(ort->GetAllocatorWithDefaultOptions(&allocator));
    CHECK(ort->SessionGetInputName(session, 0, allocator, &input_name));
    CHECK(ort->SessionGetOutputName(session, 0, allocator, &output_name));
    CHECK(ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info));
    // The tensor reads straight from input: for each frame, rewriting input is enough
    const int64_t shape[4] = {1, 3, MODEL_SIZE, MODEL_SIZE};
    CHECK(ort->CreateTensorWithDataAsOrtValue(memory_info, input, sizeof *input * 3 * PLANE, shape, 4,
                                              ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor));

    for (int f = 0; f < frame_count; f++) {
        if (frame_count > 1) fprintf(stderr, "\rsubject: frame %d/%d", f + 1, frame_count);
        fill_model_input(input, frames + (size_t)f * width * height * 4, width, height);
        CHECK(ort->Run(session, NULL, (const char *const *)&input_name, (const OrtValue *const *)&input_tensor, 1,
                       (const char *const *)&output_name, 1, &output_tensor));
        CHECK(ort->GetTensorMutableData(output_tensor, (void **)&prediction));
        mask_from_prediction(mask + (size_t)f * width * height, prediction, width, height, threshold);
        ort->ReleaseValue(output_tensor);
        output_tensor = NULL;
    }
    if (frame_count > 1) fputc('\n', stderr);
#undef CHECK

cleanup:
    if (input_name) allocator->Free(allocator, input_name);
    if (output_name) allocator->Free(allocator, output_name);
    if (output_tensor) ort->ReleaseValue(output_tensor);
    if (input_tensor) ort->ReleaseValue(input_tensor);
    if (memory_info) ort->ReleaseMemoryInfo(memory_info);
    if (session) ort->ReleaseSession(session);
    if (options) ort->ReleaseSessionOptions(options);
    if (env) ort->ReleaseEnv(env);
    free(input);
    return mask;
}
