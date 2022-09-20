#include "libclip_textual.h"
#include "libclip_visual.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

// neural network eye exam. pair 9 images with 9 descriptions
#define NUM_PROPS 3
#define NUM_DESCS (NUM_PROPS*NUM_PROPS)

static const char A[7*7] = 
	"......."
	".#####."
	".#####."
	".#####."
	".#####."
	".#####."
	".......";

static const char B[7*7] = 
	"#.....#"
	".#...#."
	"..#.#.."
	"...#..."
	"..#.#.."
	".#...#."
	"#.....#";

static const char C[7*7] = 
	"......."
	"...#..."
	"..##..."
	"...#..."
	"...#..."
	"..###.."
	".......";

static const char* bitmaps[3] = {A, B, C};

typedef struct {
	float* output;
	int bitmap;
	int color;
} ThreadTask;

// encode a generated image based on the above bitmaps, and a color
void* encode_image(void* user_data) {
	ThreadTask* task = (ThreadTask*)user_data;
	const char* bmp = bitmaps[task->bitmap];
	float image[CLIP_VISUAL_INPUT_DIM] = {};
	float* in = image + task->color * CLIP_VISUAL_INPUT_WIDTH * CLIP_VISUAL_INPUT_HEIGHT;
	for (int x = 0; x < CLIP_VISUAL_INPUT_WIDTH; x++) {
		for (int y = 0; y < CLIP_VISUAL_INPUT_HEIGHT; y++) {
			int bx = x / (CLIP_VISUAL_INPUT_WIDTH / 7);
			int by = y / (CLIP_VISUAL_INPUT_HEIGHT / 7);
			if (bmp[by * 7 + bx] == '#') {
				in[x * CLIP_VISUAL_INPUT_HEIGHT + y] = 1.0f;
			} else {
				in[x * CLIP_VISUAL_INPUT_HEIGHT + y] = 0.0f;
			}
		}
	}
	clip_encode_image(image, CLIP_VISUAL_INPUT_DIM, task->output, CLIP_VISUAL_OUTPUT_DIM, NULL);
	return NULL;
}

// encode text based on a generated description
void* encode_text(void* user_data) {
	ThreadTask* task = (ThreadTask*)user_data;
	char* out = NULL; size_t size;
	FILE* str = open_memstream(&out, &size);
	fprintf(str, "a pixelated ");
	switch (task->color) {
		case 0: fprintf(str, "red"); break;
		case 1: fprintf(str, "green"); break;
		case 2: fprintf(str, "blue"); break;
		default:fprintf(str, "UNK"); break;
	}
	fprintf(str, " ");
	switch (task->bitmap) {
		case 0: fprintf(str, "square"); break;
		case 1: fprintf(str, "x"); break;
		case 2: fprintf(str, "number one"); break;
		default:fprintf(str, "UNK"); break;
	}
	fprintf(str, " on a black background");
	fclose(str);
	printf("%s\n", out);
	clip_encode_text(out, true, task->output, CLIP_TEXTUAL_OUTPUT_DIM, NULL);

	free(out);
	return NULL;
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


	pthread_t img_threads[NUM_DESCS];
	ThreadTask img_tasks[NUM_DESCS];
	for (int i = 0; i < NUM_DESCS; i++) {
		img_tasks[i].output = image_embeddings + CLIP_VISUAL_OUTPUT_DIM*i;
		img_tasks[i].bitmap = i / NUM_PROPS;
		img_tasks[i].color = i % NUM_PROPS;

		pthread_create(&img_threads[i], NULL, &encode_image, (void*)&img_tasks[i]);
	}

	for (int i = 0; i < NUM_DESCS; i++) {
		pthread_join(img_threads[i], NULL);
	}

	pthread_t txt_threads[NUM_DESCS];
	ThreadTask txt_tasks[NUM_DESCS];
	for (int i = 0; i < NUM_DESCS; i++) {
		txt_tasks[i].output = text_embeddings + CLIP_VISUAL_OUTPUT_DIM*i;
		txt_tasks[i].bitmap = i / NUM_PROPS;
		txt_tasks[i].color = i % NUM_PROPS;
		pthread_create(&txt_threads[i], NULL, &encode_text, (void*)&txt_tasks[i]);
	}

	for (int i = 0; i < NUM_DESCS; i++) {
		pthread_join(txt_threads[i], NULL);
	}

	float confusion[NUM_DESCS][NUM_DESCS];
	float min = 1.;
	float max = 0.;
	for (int i = 0; i < NUM_DESCS; i++) {
		for (int j = 0; j < NUM_DESCS; j++) {
			float* image = image_embeddings + CLIP_VISUAL_OUTPUT_DIM*i;
			float* text = text_embeddings + CLIP_VISUAL_OUTPUT_DIM*j;
			float sim = cosine_similarity(image, text);
			printf("%.4f\t", sim);
			confusion[i][j] = sim;
			if (sim < min) min = sim;
			if (sim > max) max = sim;
		}
		printf("\n");
	}

	//todo: hungarian algorithm to pair them up
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

	return 0;
}