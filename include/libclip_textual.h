#pragma once
#include <stddef.h>
#include <stdbool.h>

#define CLIP_TEXTUAL_OUTPUT_DIM 512

// input is a null-terminated utf8 string
// if the tokenized string is too big to fit the 77 token context, truncate is checked.
// if true the input is truncated, otherwise we'll return an error
// output is of the shape (dim (512))
// returns 0 on success, fills error with error string if available
int clip_encode_text(const char* text, bool truncate, float* output, size_t output_size, const char** error);
