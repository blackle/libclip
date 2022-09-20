#include "trie.h"
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
	int out = 0;
	//read until we see 11
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

int trie_find(const uint8_t* trie, const uint8_t* charmap, const uint8_t* key, size_t key_len) {
	BitReader b = {.offset = trie, .bit = 0};
	int head = read_header(&b);
	bool hasprefix = (head == 0) || (head == 2);
	bool hasvalue = (head == 0) || (head == 1);
	if (hasprefix) {
		//todo: can we do this without a malloc?
		size_t prefix_len = 0;
		uint8_t* prefix = read_pstring(&b, charmap, &prefix_len);

		if (key_len < prefix_len) {
			free(prefix);
			return -1;
		}
		if (memcmp(key, prefix, prefix_len) != 0) {
			free(prefix);
			return -1;
		}
		key += prefix_len;
		key_len -= prefix_len;

		free(prefix);
	}
	if (hasvalue) {
		int value = read_fibonacci(&b);
		if (key_len == 0) {
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

		if (key[0] == ch) {
			myoffset = offset;
		}
	}
	if (myoffset == -1) {
		return -1;
	}
	key += 1;
	key_len -= 1;

	const uint8_t* end = end_of_bits(&b);
	return trie_find(end + myoffset, charmap, key, key_len);
}
