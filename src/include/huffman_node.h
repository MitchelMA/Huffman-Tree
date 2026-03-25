#ifndef HUFFMAN_HUFFMAN_NODE_H
#define HUFFMAN_HUFFMAN_NODE_H

/*
For reference as to what a "huffman tree" or "huffman encoding" is:
 - https://en.wikipedia.org/wiki/Huffman_coding
*/

#include <stdint.h>
#include <stdbool.h>

#define HUFFMAN_BRANCH_RIGHT true
#define HUFFMAN_BRANCH_LEFT false

struct huffman_node_;
struct node_branching_;
struct node_leaf_;

typedef struct node_leaf_ {
    uint8_t val;
    uint64_t count;
} node_leaf;

typedef struct node_branching_ {
    struct huffman_node_* right;
    struct huffman_node_* left;
} node_branching;

typedef struct huffman_node_ {
    union {
        struct node_branching_ branching;
        struct node_leaf_           leaf;
    } type_;
    bool is_branching;
} huffman_node;

bool huffman_node_init_leaf     (huffman_node* node, uint8_t value, uint64_t count);
bool huffman_node_init_branching(huffman_node* node);
void huffman_node_clean(huffman_node* node);

bool huffman_node_assign_right(huffman_node* node, const huffman_node* right);
bool huffman_node_assign_left (huffman_node* node, const huffman_node* left);

uint64_t huffman_node_count(const huffman_node* node);

/**
 * Calculates the path of a given value in the tree from the persepctive of the node:
 * i.e. in reverse.  
 * This has to do with the fact that searching for a value is done depth first
 * with preference to the left.
 */
bool* huffman_node_path(const huffman_node* root, uint8_t val, int64_t* path_length);

void huffman_node_print_inline(const huffman_node* node);
void huffman_node_print_depth (const huffman_node* node);


#endif // !HUFFMAN_HUFFMAN_NODE_H

