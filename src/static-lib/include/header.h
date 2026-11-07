#ifndef HUFFMAN_HUFFMAN_HEADER_H
#define HUFFMAN_HUFFMAN_HEADER_H

#include <stddef.h>
#include <stdbool.h>

#define HEADER_MAX_LINE_LENGTH 256

typedef struct huffman_header_keypair_ {
    const size_t key_string_length_;
    const size_t value_string_length_;

    const char* key_string_start_;
    const char* value_string_start_;
} huffman_header_keypair;

typedef struct huffman_header_ {
    const size_t header_raw_text_length_;
    const size_t header_entry_count_;
    const char*  header_raw_text_;
    const huffman_header_keypair* header_entries_;
} huffman_header;

huffman_header parse_header_stream(FILE* stream);

#endif // !HUFFMAN_HUFFMAN_HEADER_H
