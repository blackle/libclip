#pragma once
#include <stdint.h>
#include <stddef.h>

// blackle's janky in-memory trie. returns value from key, or -1 if not present
int find(const uint8_t* trie, const uint8_t* charmap, const uint8_t* key, size_t key_len);