#pragma once
#include <stddef.h>

#define CLIP_VISUAL_INPUT_WIDTH 224
#define CLIP_VISUAL_INPUT_HEIGHT 224
#define CLIP_VISUAL_INPUT_CHANNELS 3
#define CLIP_VISUAL_INPUT_DIM (CLIP_VISUAL_INPUT_CHANNELS * CLIP_VISUAL_INPUT_WIDTH * CLIP_VISUAL_INPUT_HEIGHT)
#define CLIP_VISUAL_OUTPUT_DIM 512

// input is of the shape (channels (3), width (224), height (224))
// NOTE: that is planar RGB, with pixel values in the range [0, 1] (0 is black, 1 is white)
// output is of the shape (dim (512))
// returns 0 on success, fills error with error string if available
int clip_encode_image(const float* input, size_t input_size, float* output, size_t output_size, const char** error);
