#include "trie_data.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const uint8_t* offset;
	uint8_t bit;
} BitReader;

const uint8_t* end_of_bits(BitReader* b) {
	return b->offset + (b->bit != 0);
}

bool read_bit(BitReader* b) {
	bool out = ((*b->offset) & (1 << (7 - b->bit))) != 0;
	b->bit++;
	if (b->bit > 7) {
		b->bit = 0;
		b->offset++;
	}
	return out;
}

int read_fibonacci(BitReader* b) {
	int f1 = 1;
	int f2 = 2;
	bool bit = false;
	bool last_bit = false;
	//read until we see 11
	int out = 0;
	while ((bit = read_bit(b)) != last_bit || last_bit == 0) {
		if (bit) {
			out += f1;
		}
		f2 += f1;
		f1 = f2 - f1;
		last_bit = bit;
	}
	return out - 1;
}

uint8_t* read_pstring(BitReader* b, const uint8_t* charmap, size_t* length) {
	*length = read_fibonacci(b);
	uint8_t* string = (uint8_t*)malloc(*length);
	for (int i = 0; i < *length; i++) {
		string[i] = charmap[read_fibonacci(b)];
	}
	return string;
}

int read_header(BitReader* b) {
	int head = 0;
	bool b1 = read_bit(b);
	if (b1) {
		head++;
		b1 = read_bit(b);
		if (b1) {
			head++;
			b1 = read_bit(b);
			if (b1) {
				head++;
			}
		}
	}
	return head;
}

int find(const uint8_t* trie, const uint8_t* charmap, const uint8_t* data, size_t data_len) {
	BitReader b = {.offset = trie, .bit = 0};
	int head = read_header(&b);
	bool hasprefix = (head == 0) || (head == 2);
	bool hasvalue = (head == 0) || (head == 1);
	if (hasprefix) {
		size_t prefix_len = 0;
		uint8_t* prefix = read_pstring(&b, charmap, &prefix_len);

		if (data_len < prefix_len) {
			return -1;
		}
		if (memcmp(data, prefix, prefix_len) != 0) {
			return -1;
		}
		data += prefix_len;
		data_len -= prefix_len;

		free(prefix);
	}
	if (hasvalue) {
		int value = read_fibonacci(&b);
		if (data_len == 0) {
			return value;
		}
	}

	int children = read_fibonacci(&b);

	int myoffset = -1;
	int offset = 0;
	int lastch = 0;
	for (int i = 0; i < children; i++) {
		int ch_un = read_fibonacci(&b) + lastch;
		lastch = ch_un;
		uint8_t ch = charmap[ch_un];
		if (i != 0) {
			offset += read_fibonacci(&b);
		}

		if (data[0] == ch) {
			myoffset = offset;
		}
	}
	if (myoffset == -1) {
		return -1;
	}
	data += 1;
	data_len -= 1;

	const uint8_t* end = end_of_bits(&b);
	return find(end + myoffset, charmap, data, data_len);
}

int main() {
	printf("%d\n", find(vocab_data, vocab_chars, "stargazing\x00", 11));
	printf("%d\n", find(vocab_data, vocab_chars, "!", 1));
	printf("%d\n", find(vocab_data, vocab_chars, "#", 1));
	printf("%d\n", find(vocab_data, vocab_chars, "mcdermott\x00", 10));
	printf("%d\n", find(vocab_data, vocab_chars, "jomulis\x00", 8));
	printf("%d\n", find(vocab_data, vocab_chars, "(\xf0\x9f\x93\xb7:\x00", 7));
	printf("%d\n", find(vocab_data, vocab_chars, "generational\x00", 13));
	printf("%d\n", find(vocab_data, vocab_chars, "usaid\x00", 6));
	printf("%d\n", find(vocab_data, vocab_chars, "\xe3\x81\xa8\xe7\xb9\x8b\xe3\x81\x8c\xe3\x82\x8a\xe3\x81\x9f\xe3\x81\x84\x00", 19));
	printf("%d\n", find(vocab_data, vocab_chars, "\xe3\x81\xa8\xe7\xb2\x8b\xe3\x81\x8c\xe3\x82\x8a\xe3\x81\x9f\xe3\x81\x84\x00", 19));

	return 0;
}