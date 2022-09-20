#include "libclip_textual.h"
#include "tokenizer.h"
#include "OnnxMlirRuntime.h"
#include <string.h>

// Declare the inference entry point.
OMTensorList *run_main_graph(OMTensorList *);

#define CLIP_TEXTUAL_INPUT_DIM 77

#define SET_ERROR(error, string) do { if (error != NULL) *error = string; } while(0)

int clip_encode_text(const char* text, bool truncate, float* output, size_t output_size, const char** error) {
  if (output_size != CLIP_TEXTUAL_OUTPUT_DIM) {
    SET_ERROR(error, "output dimensions incorrect");
    return -1;
  }
  if (text == NULL) {
    SET_ERROR(error, "text is null");
    return -1;
  }
  if (output == NULL) {
    SET_ERROR(error, "output is null");
    return -1;
  }

  int64_t rank = 2;
  int64_t shape[] = {1, CLIP_TEXTUAL_INPUT_DIM};

  int64_t context[CLIP_TEXTUAL_INPUT_DIM] = {};
  bool truncated = false;
  int res = tokenize_string(text, context, CLIP_TEXTUAL_INPUT_DIM, &truncated);
  if (res == -1) {
    SET_ERROR(error, "tokenization error");
    return -1;	
  }
  if (truncated && !truncate) {
    SET_ERROR(error, "tokenization too long");
    return -1;	
  }

  OMTensor *tensor = omTensorCreate(context, shape, rank, ONNX_TYPE_INT64);
  OMTensorList *tensorListIn = omTensorListCreate(&tensor, 1);

  OMTensorList *tensorListOut = run_main_graph(tensorListIn);

  OMTensor *y = omTensorListGetOmtByIndex(tensorListOut, 0);
  float *out = (float *)omTensorGetDataPtr(y);

  memcpy(output, out, sizeof(float) * CLIP_TEXTUAL_OUTPUT_DIM);

  omTensorListDestroy(tensorListIn);
  omTensorListDestroy(tensorListOut);

  return 0;
}