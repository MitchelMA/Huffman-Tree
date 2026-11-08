#ifndef HUFFMAN_HUFFMAN_HEADER_H
#define HUFFMAN_HUFFMAN_HEADER_H

#include <vector.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct huffman_header_keypair_ {
    size_t key_buffer_size_;   // Buffer size of the key, including '\0'
    size_t value_buffer_size_; // Buffer size of the value, including '\0'

    char* key_string_;
    char* value_string_;
} huffman_header_keypair;

typedef struct huffman_header_ {
    vector_t* header_entries_; // A vector of huffman_header_keypair
} huffman_header;

huffman_header parse_header_stream(FILE* stream);
void header_clean(huffman_header* header);

size_t header_get_value(const huffman_header* header, const char* in_key, size_t out_length, char* out_value);
bool header_contains(const huffman_header* header, const char* in_key);

bool header_replace_key(huffman_header* header, const char* target_key, const char* new_key);
bool header_replace_value(huffman_header* header, const char* target_key, const char* new_value);

bool header_flush_stream(const huffman_header* header, FILE* stream);

#endif // !HUFFMAN_HUFFMAN_HEADER_H

