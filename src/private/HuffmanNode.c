#include "HuffmanNode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- LOCAL DEFINITIONS --- //

static void huffman_node_print_depth_rec_(const huffman_node* node, int depth);

// --- END LOCAL DEFINITIONS --- /

bool
huffman_node_init_leaf(
    huffman_node* node,
    uint8_t value,
    uint64_t count
)
{
    node->is_branching = false;
    node->type_.leaf.val   = value;
    node->type_.leaf.count = count;

    return true;
}

bool
huffman_node_init_branching(
    huffman_node* node
)
{
    node->is_branching = true;
    node->type_.branching.right = NULL;
    node->type_.branching.left  = NULL;

    return true;
}

void
huffman_node_clean(
    huffman_node* node
)
{
    if (node == NULL)
        return;

    if (!node->is_branching)
    {
        node->type_.leaf.val = 0;
        node->type_.leaf.count = 0;
        return;
    }

    huffman_node* right = node->type_.branching.right;
    huffman_node* left = node->type_.branching.left;

    if (right != NULL)
        huffman_node_clean(right);

    if (left != NULL)
        huffman_node_clean(left);

    if (right != NULL)
        free(right);

    if (left != NULL)
        free(left);

    node->type_.branching.right = NULL;
    node->type_.branching.left  = NULL;
}

bool
huffman_node_assign_right(
    huffman_node* node,
    const huffman_node* right
)
{
    if (!node->is_branching)
        return false;

    if (node->type_.branching.right != NULL)
        return false;

    node->type_.branching.right = malloc(sizeof(huffman_node));
    if (node->type_.branching.right == NULL)
        return false;

    void* result = memcpy(node->type_.branching.right, right, sizeof(huffman_node));

    return result == node->type_.branching.right;
}

bool
huffman_node_assign_left(
    huffman_node* node,
    const huffman_node* left
)
{
    if (!node->is_branching)
        return false;

    if (node->type_.branching.left != NULL)
        return false;

    node->type_.branching.left = malloc(sizeof(huffman_node));
    if (node->type_.branching.left == NULL)
        return false;

    void* result = memcpy(node->type_.branching.left, left, sizeof(huffman_node));

    return result == node->type_.branching.left;
}

uint64_t
huffman_node_count(
    const huffman_node* node
)
{
    if (node == NULL)
        return 0;

    if (!node->is_branching)
        return node->type_.leaf.count;

    return huffman_node_count(node->type_.branching.left) +
           huffman_node_count(node->type_.branching.right);
}

void
huffman_node_print_inline(
    const huffman_node* node
)
{
    if (node == NULL)
    {
        printf("(nil)");
        return;
    }

    if (!node->is_branching)
    {
        printf("{ %c, %llu }", node->type_.leaf.val, node->type_.leaf.count);
    }
    else
    {
        printf("{ ");
        huffman_node_print_inline(node->type_.branching.left);
        printf(", ");
        huffman_node_print_inline(node->type_.branching.right);
        printf(" }");
    }
}

void
huffman_node_print_depth(
    const huffman_node* node
)
{
    huffman_node_print_depth_rec_(node, 0);
}

// --- LOCAL IMPLEMENTATIONS --- //

void
huffman_node_print_depth_rec_(
    const huffman_node* node,
    int depth
)
{
    const char* tabs = "                  ";
    if (node == NULL)
    {
        printf("%.*s(nil)", depth, tabs);
        return;
    }

    if (!node->is_branching)
    {
        printf("%.*s< %c, %llu >", depth, tabs, node->type_.leaf.val, node->type_.leaf.count);
    }
    else
    {
        printf("%.*s{\n", depth, tabs);
        huffman_node_print_depth_rec_(node->type_.branching.left, depth + 1);
        printf(",\n");
        huffman_node_print_depth_rec_(node->type_.branching.right, depth + 1);
        printf("\n%.*s}", depth, tabs);
    }
}

// --- END LOCAL IMPLEMENTATIONS --- /

