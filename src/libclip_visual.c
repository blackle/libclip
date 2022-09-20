#include <string.h>
#include "libclip_visual.h"
#include "OnnxMlirRuntime.h"

// Declare the inference entry point.
OMTensorList *run_main_graph(OMTensorList *);

#define SET_ERROR(error, string) do{ if (error != NULL) *error = string; } while(0)

void normalize_channel(float* image, int channel, float mean, float std) {
  image += CLIP_VISUAL_INPUT_WIDTH * CLIP_VISUAL_INPUT_HEIGHT * channel;
  for (int i = 0; i < CLIP_VISUAL_INPUT_WIDTH * CLIP_VISUAL_INPUT_HEIGHT; i++) {
    image[i] = (image[i] - mean) / std;
  }
}

int clip_encode_image(const float* input, size_t input_size, float* output, size_t output_size, const char** error) {
  if (input_size != CLIP_VISUAL_INPUT_DIM) {
    SET_ERROR(error, "input dimensions incorrect");
    return -1;
  }
  if (output_size != CLIP_VISUAL_OUTPUT_DIM) {
    SET_ERROR(error, "output dimensions incorrect");
    return -1;
  }
  if (input == NULL) {
    SET_ERROR(error, "input is null");
    return -1;
  }
  if (output == NULL) {
    SET_ERROR(error, "output is null");
    return -1;
  }

  float* in = (float*)malloc(sizeof(float) * CLIP_VISUAL_INPUT_DIM);
  memcpy(in, input, sizeof(float) * CLIP_VISUAL_INPUT_DIM);

  normalize_channel(in, 0, 0.48145466, 0.26862954);
  normalize_channel(in, 1, 0.45782750, 0.26130258);
  normalize_channel(in, 2, 0.40821073, 0.27577711);

  int64_t rank = 4;
  int64_t shape[] = {1, CLIP_VISUAL_INPUT_CHANNELS, CLIP_VISUAL_INPUT_WIDTH, CLIP_VISUAL_INPUT_HEIGHT};

  OMTensor *tensor = omTensorCreate(in, shape, rank, ONNX_TYPE_FLOAT);
  OMTensorList *tensorListIn = omTensorListCreate(&tensor, 1);

  OMTensorList *tensorListOut = run_main_graph(tensorListIn);

  OMTensor *y = omTensorListGetOmtByIndex(tensorListOut, 0);
  float *out = (float *)omTensorGetDataPtr(y);

  memcpy(output, out, sizeof(float) * CLIP_VISUAL_OUTPUT_DIM);

  omTensorListDestroy(tensorListIn);
  omTensorListDestroy(tensorListOut);
  free(in);

  return 0;
}
