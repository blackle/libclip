#include "trie.h"
#include "trie_data.h"
#include <stdio.h>

int main() {
	printf("%d\n", trie_find(vocab_data, vocab_chars, "stargazing\x00", 11));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "!", 1));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "#", 1));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "mcdermott\x00", 10));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "jomulis\x00", 8));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "(\xf0\x9f\x93\xb7:\x00", 7));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "generational\x00", 13));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "usaid\x00", 6));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "\xe3\x81\xa8\xe7\xb9\x8b\xe3\x81\x8c\xe3\x82\x8a\xe3\x81\x9f\xe3\x81\x84\x00", 19));
	printf("%d\n", trie_find(vocab_data, vocab_chars, "\xe3\x81\xa8\xe7\xb2\x8b\xe3\x81\x8c\xe3\x82\x8a\xe3\x81\x9f\xe3\x81\x84\x00", 19));

	return 0;
}