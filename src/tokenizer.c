#include "tokenizer.h"
#include "trie.h"
#include "trie_data.inc"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

pcre2_code *pattern;
__attribute__((constructor))
static void clip_encode_text_tokenizer_initialize() {
	int errornumber;
	PCRE2_SIZE erroroffset;
	pattern = pcre2_compile("'s|'t|'re|'ve|'m|'ll|'d|[\\p{L}]+|[\\p{N}]|[^\\s\\p{L}\\p{N}]+",
		PCRE2_ZERO_TERMINATED, PCRE2_UTF, &errornumber, &erroroffset, NULL);
	if (pattern == NULL) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
		printf("PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
		abort();
	}
}

__attribute__((destructor))
static void clip_encode_text_tokenizer_deinitialize() {
	pcre2_code_free(pattern);
}

typedef struct {
	int64_t* output;
	size_t output_size;
	bool* truncated;
} TokenList;

int token_list_add(TokenList* list, int token) {
	if (list->output_size <= 0) {
		*list->truncated = true;
		return -1;
	} else if (list->output_size == 1) {
		list->output[0] = END_OF_TEXT;
		*list->truncated = token != END_OF_TEXT;
		list->output++;
		list->output_size--;
	} else {
		list->output[0] = token;
		list->output++;
		list->output_size--;
	}
	return 0;
}

// void printfbytes(const uint8_t* word, int word_size) {
// 	for (int i = 0; i < word_size; i++) {
// 		if (word[i] == 0) printf("\\0");
// 		else if (isprint(word[i])) printf("%c", word[i]);
// 		else printf("\\x%02x", word[i]);
// 	}
// }

int find_vocab(const char* word, int word_size) {
	return trie_find(vocab_data, vocab_chars, word, word_size);
}

int find_merge(const char* word1, int word1_size, const char* word2, int word2_size) {
	int scratch_size = word1_size+1+word2_size;
	uint8_t scratch[scratch_size];
	memcpy(scratch, word1, word1_size);
	scratch[word1_size] = '\0';
	memcpy(scratch + word1_size + 1, word2, word2_size);
	int val = trie_find(merges_data, merges_chars, scratch, scratch_size);
	return val;
}

//returns # of tokens written to output
int tokenize_word(const char* word, int word_size, TokenList* list) {
	//the null terminator will serve as </w>
	char word_null[word_size + 1];
	memset(word_null, 0, word_size + 1);
	memcpy(word_null, word, word_size);

	int val = find_vocab(word_null, word_size + 1);
	if (val != -1 && token_list_add(list, val) == -1) {
		return 0;
	}

	//list of pairs an their sizes (skip-based data structure)
	uint8_t skips[word_size];
	memset(skips, 1, word_size);
	skips[word_size - 1] = 2; //pair last character with </w>

	// int priorities[word_size];
	// memset(priorities, 0, word_size * sizeof(int));

	//todo: priority queue this shit up, get rid of the infinite loop
	// at least cache the output of find_merge...
	while (true) {
		int min_priority = -1;
		int argmin_priority = -1;
		for (int i = 0; i + skips[i] < word_size; i += skips[i]) {
			int next = i + skips[i];
			int priority = find_merge(word_null + i, skips[i], word_null + next, skips[next]);

			if (min_priority == -1 || (priority < min_priority && priority != -1)) {
				min_priority = priority;
				argmin_priority = i;
			}
		}
		if (min_priority == -1) {
			break;
		} else {
			int next = argmin_priority + skips[argmin_priority];
			skips[argmin_priority] += skips[next];
		}
	}

	for (int i = 0; i < word_size; i += skips[i]) {
		val = find_vocab(word_null + i, skips[i]);
		if (val != -1 && token_list_add(list, val) == -1) {
			return 0;
		}
	}

	return 0;
}

int tokenize_string(const char* text, int64_t* output, size_t output_size, bool* truncated) {
	TokenList list = {.output = output, .output_size = output_size, .truncated = truncated};
	if (token_list_add(&list, START_OF_TEXT) == -1) {
		return 0;
	}
	//todo: unicode aware lowercase
	size_t text_size = strlen(text);

	pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(pattern, NULL);
	PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

	int offset = 0;
	int res = 0;
	while ((res = pcre2_match(pattern, text, text_size, offset, 0, match_data, NULL)) >= 0) {
		const char* match = text + ovector[0];
		int match_size = ovector[1] - ovector[0];
		if (match_size == 0) break;
		tokenize_word(match, match_size, &list);
		if (*list.truncated == true) {
			break; //early exit on truncate
		}
		offset = ovector[1];
	}
	//todo: return with error if res != PCRE2_ERROR_NOMATCH
	token_list_add(&list, END_OF_TEXT);

	pcre2_match_data_free(match_data);
	return 0;
}