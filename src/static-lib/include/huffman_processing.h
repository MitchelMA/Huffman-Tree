#ifndef HUFFMAN_HUFFMAN_PROCESSING_H
#define HUFFMAN_HUFFMAN_PROCESSING_H

#include <stdint.h>
#include <stdio.h>

#include "huffman_node.h"
#include "bitarray.h"

typedef struct dlinked_list_node_ {
    struct dlinked_list_node_* prev;
    struct dlinked_list_node_* next;

    huffman_node node;
} dlinked_list_node;

/**
 * Counts the input text, and creates
 * a doubly-linked-list containing unique huffman_node leafs
 * with a value and count for each found individual
 * symbol.
 */
dlinked_list_node* huffman_count_text(const uint8_t* text, int64_t length);

dlinked_list_node* huffman_count_stream(FILE* stream);

/**
 * Couts the symbols from the stream, and creates
 * a doubly-linked-list containing unique huffman_node leafs
 * with a value and count for each found individual symbol.
 */
dlinked_list_node* huffman_count_stream_onepass(FILE* stream);

dlinked_list_node* huffman_count_stream_table(FILE* stream);

/**
 * Constructs the tree
 * from the sorted doubly-linked-list
 * built by `huffman_count_text()`.
 * Note that the doubly-linked-list is exhaustively consumed and cleaned;
 * may not be used after calling this function.
 */
huffman_node huffman_construct_tree(dlinked_list_node* root);

bitarray_length_t huffman_compress(const huffman_node* root, size_t buffer_size, const uint8_t* buffer, bitarray* out_bitarray);
bitarray_length_t huffman_compress_stream(const huffman_node* root, FILE* in_stream, bitarray* out_bitarray);

size_t huffman_decompress(const huffman_node* root, const bitarray* bit_array, size_t buffer_size, uint8_t* buffer);
size_t huffman_decompress_stream(const huffman_node* root, const bitarray* bit_array, FILE* out_stream);


void doublyll_clean(dlinked_list_node* root);


#endif // !HUFFMAN_HUFFMAN_PROCESSING_H

