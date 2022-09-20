#include "libclip_textual.h"
#include <stdio.h>

int main() {
	float output[CLIP_TEXTUAL_OUTPUT_DIM];

	const char* error;
	int val = clip_encode_text("☺👖", true, output, CLIP_TEXTUAL_OUTPUT_DIM, &error);

	if (val != 0) {
		printf("error: %s\n", error);
		return -1;
	}

	// for (int i = 0; i < CLIP_TEXTUAL_OUTPUT_DIM; i++) {
	// 	printf("%f\n", output[i]);
	// }

	return 0;
}