#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//tokenize text and fill output with the token ids.
//return -1 on error
// sets `truncated` if we had to truncate
int tokenize_string(const char* text, int64_t* output, size_t output_size, bool* truncated);