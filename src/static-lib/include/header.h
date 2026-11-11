#ifndef HUFFMAN_HUFFMAN_HEADER_H
#define HUFFMAN_HUFFMAN_HEADER_H

#include <vector.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct huffman_header_keypair_ {
    size_t key_buffer_size_;   // Buffer size of the key, including '\0'
    size_t value_buffer_size_; // Buffer size of the value, including '\0'

    size_t key_string_length_;   // String length of the key, since the key-string may have '\0' within it's text
    size_t value_string_length_; // String length of the value, since the value-string may have '\0' within it's text

    char* key_string_;
    char* value_string_;
} huffman_header_keypair;

typedef struct huffman_header_ {
    vector_t* header_entries_; // A vector of huffman_header_keypair
} huffman_header;

huffman_header parse_header_stream(FILE* stream);
huffman_header header_create_empty();
void header_clean(huffman_header* header);

size_t header_get_value(const huffman_header* header, size_t in_key_length, const char* in_key, size_t out_length, char* out_value);
#define header_get_value_s(header_, key_, out_length_, out_value_) header_get_value(header_, strlen(key_), key_, out_length_, out_value_)
bool header_contains(const huffman_header* header, size_t in_key_length, const char* in_key);
#define header_contains_s(header_, key_) header_contains(header_, strlen(key_), key_)

bool header_replace_key(huffman_header* header, size_t target_key_length, const char* target_key, size_t new_key_length, const char* new_key);
#define header_replace_key_s(header_, target_key_, new_key_) header_replace_key(header_, strlen(target_key_), target_key_, strlen(new_key_), new_key_)
bool header_replace_value(huffman_header* header, size_t target_key_length, const char* target_key, size_t new_value_length, const char* new_value);
#define header_replace_value_s(header_, target_key_, new_value_) header_replace_value(header_, strlen(target_key_), target_key_, strlen(new_value_), new_value_)

bool header_remove_entry(huffman_header* header, size_t target_key_length, const char* target_key);
#define header_remove_entry_s(header_, key_) header_remove_entry(header_, strlen(key_), key_)
bool header_add_entry(huffman_header* header, size_t key_length, const char* key, size_t value_length, const char* value);
#define header_add_entry_s(header_, key_, value_) header_add_entry(header_, strlen(key_), key_, strlen(value_), value_)

bool header_flush_stream(const huffman_header* header, FILE* stream);

#endif // !HUFFMAN_HUFFMAN_HEADER_H

