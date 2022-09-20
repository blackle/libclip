#include "libclip_textual.h"
#include <stdio.h>

int main() {
	float output[CLIP_TEXTUAL_OUTPUT_DIM];

	const char* error;
	int val = clip_encode_text("a pixelated red number one on a black background", true, output, CLIP_TEXTUAL_OUTPUT_DIM, &error);

	if (val != 0) {
		printf("error: %s\n", error);
		return -1;
	}

	for (int i = 0; i < CLIP_TEXTUAL_OUTPUT_DIM / 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%f\t", output[i*4 + j]);
		}
		printf("\n");
	}

	return 0;
}
