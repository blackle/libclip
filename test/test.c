#include "libclip_textual.h"
#include "libclip_visual.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// neural network eye exam. pair 9 images with 9 descriptions
#define NUM_PROPS 3
#define NUM_DESCS (NUM_PROPS*NUM_PROPS)

static const char one[7*7] = 
	"......."
	"...#..."
	"..##..."
	"...#..."
	"...#..."
	"..###.."
	".......";

static const char cross[7*7] = 
	"......."
	".#...#."
	"..#.#.."
	"...#..."
	"..#.#.."
	".#...#."
	".......";

static const char arrow[7*7] = 
	"......."
	".####.."
	".##...."
	".#.#..."
	".#..#.."
	".....#."
	".......";

static const char* bitmaps[3] = {one, cross, arrow};

// encode a generated image based on the above bitmaps, and a color
void encode_image(float* output, int bitmap, int color) {
	const char* bmp = bitmaps[bitmap];
	float image[CLIP_VISUAL_INPUT_DIM] = {};
	float* in = image + color * CLIP_VISUAL_INPUT_WIDTH * CLIP_VISUAL_INPUT_HEIGHT;
	for (int x = 0; x < CLIP_VISUAL_INPUT_WIDTH; x++) {
		for (int y = 0; y < CLIP_VISUAL_INPUT_HEIGHT; y++) {
			int bx = x / (CLIP_VISUAL_INPUT_WIDTH / 7);
			int by = y / (CLIP_VISUAL_INPUT_WIDTH / 7);
			if (bmp[x * 7 + y] == '#') {
				in[x * CLIP_VISUAL_INPUT_HEIGHT + y] = 1.0f;
			}
		}
	}
	clip_encode_image(image, CLIP_VISUAL_INPUT_DIM, output, CLIP_VISUAL_OUTPUT_DIM, NULL);
}

// encode text based on a generated description
void encode_text(float* output, int bitmap, int color) {
	char* out = NULL; size_t size;
	FILE* str = open_memstream(&out, &size);
	fprintf(str, "a pixelated ");
	switch (color) {
		case 0: fprintf(str, "red"); break;
		case 1: fprintf(str, "green"); break;
		case 2: fprintf(str, "blue"); break;
		default:fprintf(str, "UNK"); break;
	}
	fprintf(str, " ");
	switch (bitmap) {
		case 0: fprintf(str, "numeric number one"); break;
		case 1: fprintf(str, "cross-shaped letter x"); break;
		case 2: fprintf(str, "arrow pointing to the top left"); break;
		default:fprintf(str, "UNK"); break;
	}
	fprintf(str, " on a black background");
	fclose(str);
	printf("%s\n", out);
	clip_encode_text(out, true, output, CLIP_TEXTUAL_OUTPUT_DIM, NULL);

	free(out);
}

float magnitude(float* a) {
	float mag = 0.0f;
	for (int i = 0; i < CLIP_VISUAL_OUTPUT_DIM; i++) {
		mag += a[i]*a[i];
	}
	return sqrt(mag);
}

float dot_product(float* a, float* b) {
	float dot = 0.0f;
	for (int i = 0; i < CLIP_VISUAL_OUTPUT_DIM; i++) {
		dot += a[i]*b[i];
	}
	return dot;
}

float cosine_similarity(float* a, float* b) {
	return dot_product(a, b)/magnitude(a)/magnitude(b);
}

int main() {
	float* image_embeddings = (float*)malloc(sizeof(float) * CLIP_VISUAL_OUTPUT_DIM * NUM_DESCS);
	memset(image_embeddings, 0, sizeof(float) * CLIP_VISUAL_OUTPUT_DIM * NUM_DESCS);
	float* text_embeddings = (float*)malloc(sizeof(float) * CLIP_TEXTUAL_OUTPUT_DIM * NUM_DESCS);
	memset(text_embeddings, 0, sizeof(float) * CLIP_VISUAL_OUTPUT_DIM * NUM_DESCS);


	for (int i = 0; i < NUM_DESCS; i++) {
		encode_image(image_embeddings + CLIP_VISUAL_OUTPUT_DIM*i, i / NUM_PROPS, i % NUM_PROPS);
	}

	for (int i = 0; i < NUM_DESCS; i++) {
		encode_text(text_embeddings + CLIP_VISUAL_OUTPUT_DIM*i, i / NUM_PROPS, i % NUM_PROPS);
	}

	float confusion[NUM_DESCS][NUM_DESCS];
	float min = 1.;
	float max = 0.;
	for (int i = 0; i < NUM_DESCS; i++) {
		for (int j = 0; j < NUM_DESCS; j++) {
			float* image = image_embeddings + CLIP_VISUAL_OUTPUT_DIM*i;
			float* text = text_embeddings + CLIP_VISUAL_OUTPUT_DIM*j;
			float sim = cosine_similarity(image, text);
			// printf("%.4f\t", sim);
			confusion[i][j] = sim;
			if (sim < min) min = sim;
			if (sim > max) max = sim;
		}
		// printf("\n");
	}

	for (int i = 0; i < NUM_DESCS; i++) {
		for (int j = 0; j < NUM_DESCS; j++) {
			float color = (confusion[i][j] - min) / (max - min);
			unsigned char c = color * 0xFF;
			printf("\x1b[48;2;%d;%d;%dm  ",c,c,c);
		}
		printf("\x1b[0m\n");
	}

	free(image_embeddings);
	free(text_embeddings);

	// float red_one_text[CLIP_TEXTUAL_OUTPUT_DIM];
	// clip_encode_text("a pixelated red arrow pointing up on a black background", true, red_one_text, CLIP_TEXTUAL_OUTPUT_DIM, NULL);

	// float red_five_text[CLIP_TEXTUAL_OUTPUT_DIM];
	// clip_encode_text("a pixelated red arrow pointing left on a black background", true, red_five_text, CLIP_TEXTUAL_OUTPUT_DIM, NULL);

	// float red_nine_text[CLIP_TEXTUAL_OUTPUT_DIM];
	// clip_encode_text("a pixelated red arrow pointing right on a black background", true, red_nine_text, CLIP_TEXTUAL_OUTPUT_DIM, NULL);

	// float red_nine_image[CLIP_TEXTUAL_OUTPUT_DIM];
	// encode_image(red_nine_image, 2, 0);

	// printf("up: %f\n", cosine_similarity(red_one_text, red_nine_image));
	// printf("left: %f\n", cosine_similarity(red_five_text, red_nine_image));
	// printf("right: %f\n", cosine_similarity(red_nine_text, red_nine_image));

	return 0;
}