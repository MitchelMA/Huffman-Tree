#ifndef HUFFMAN_HUFFMAN_PROCESSING_H
#define HUFFMAN_HUFFMAN_PROCESSING_H

#include <stdint.h>

#include "huffman_node.h"

typedef struct dlinked_list_node_ {
    struct dlinked_list_node_* prev;
    struct dlinked_list_node_* next;

    huffman_node node;
} dlinked_list_node;

/**
 * Counts the input text, and creates
 * a vector containing unique huffman_node leafs 
 * with a value and count for each found individual
 * symbol.
 */
dlinked_list_node* huffman_count_text(const uint8_t* text, int64_t length);

void doublyll_clean(dlinked_list_node* root);


#endif // !HUFFMAN_HUFFMAN_PROCESSING_H

