# libclip

openai's clip model as a shared library

uses https://github.com/Lednik7/CLIP-ONNX compiled through https://github.com/onnx/onnx-mlir

much slower than using the onnx runtime, due to onnx-mlir producing single-threaded code. however, no runtime dependencies are needed except for the math library.

the main contribution of this library is an implementation of the tokenizer in c99. however, it's not fully finished yet (missing a lowercase operation, performance can be improved.)

## usage

### encoding text

```c
float output[CLIP_TEXTUAL_OUTPUT_DIM]; // 512-dimensional vector
const char* error;
int result = clip_encode_text("my image caption", true, output, CLIP_TEXTUAL_OUTPUT_DIM, &error);
if (result != 0) {
	perror(error);
	return -1;
}
```

### encoding images

```c
// 3-channel 224x224 planar rgb, (channels, width, height)
size_t image_size = CLIP_VISUAL_INPUT_DIM;
float image[image_size] = {};
// load image data from elsewhere
float output[CLIP_TEXTUAL_OUTPUT_DIM]; // 512-dimensional vector
const char* error;
int result = clip_encode_image(image, image_size, output, CLIP_VISUAL_OUTPUT_DIM, &error);
if (result != 0) {
	perror(error);
	return -1;
}
```

## todo

[ ] create a better testing application
[ ] create static library version (I think this requires renaming the symbols coming out of onnx-mlir)
[ ] utf8 lowercase in tokenizer
[ ] improve performance of tokenizer
[ ] better error handling in tokenizer
[ ] reduce number of mallocs (there's one in trie.c which might not need to be there)
[ ] experiment with statically linking the onnx runtime, instead of onnx-mlir. should improve performance, maybe without adding any runtime dependencies