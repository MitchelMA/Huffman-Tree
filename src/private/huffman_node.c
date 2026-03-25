#include "huffman_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- LOCAL DEFINITIONS --- //

#define PATH_ARRAY_LENGTH 128

static void huffman_node_print_depth_impl_(const huffman_node* node, int depth);
static bool huffman_node_path_impl_(const huffman_node* root, uint8_t val, bool* path, int64_t* len);
static bool huffman_node_path_inner_(const huffman_node* root, uint8_t val, bool* path, int64_t* len);

// --- END LOCAL DEFINITIONS --- /

bool
huffman_node_init_leaf(
    huffman_node* node,
    uint8_t value,
    uint64_t count
)
{
    node->is_branching = false;
    node->type.leaf.val   = value;
    node->type.leaf.count = count;

    return true;
}

bool
huffman_node_init_branching(
    huffman_node* node
)
{
    node->is_branching = true;
    node->type.branching.right = NULL;
    node->type.branching.left  = NULL;

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
        node->type.leaf.val = 0;
        node->type.leaf.count = 0;
        return;
    }

    huffman_node* right = node->type.branching.right;
    huffman_node* left = node->type.branching.left;

    if (right != NULL)
        huffman_node_clean(right);

    if (left != NULL)
        huffman_node_clean(left);

    if (right != NULL)
        free(right);

    if (left != NULL)
        free(left);

    node->type.branching.right = NULL;
    node->type.branching.left  = NULL;
}

bool
huffman_node_assign_right(
    huffman_node* node,
    const huffman_node* right
)
{
    if (!node->is_branching)
        return false;

    if (node->type.branching.right != NULL)
        return false;

    node->type.branching.right = malloc(sizeof(huffman_node));
    if (node->type.branching.right == NULL)
        return false;

    void* result = memcpy(node->type.branching.right, right, sizeof(huffman_node));

    return result == node->type.branching.right;
}

bool
huffman_node_assign_left(
    huffman_node* node,
    const huffman_node* left
)
{
    if (!node->is_branching)
        return false;

    if (node->type.branching.left != NULL)
        return false;

    node->type.branching.left = malloc(sizeof(huffman_node));
    if (node->type.branching.left == NULL)
        return false;

    void* result = memcpy(node->type.branching.left, left, sizeof(huffman_node));

    return result == node->type.branching.left;
}

uint64_t
huffman_node_count(
    const huffman_node* node
)
{
    if (node == NULL)
        return 0;

    if (!node->is_branching)
        return node->type.leaf.count;

    return huffman_node_count(node->type.branching.left) +
           huffman_node_count(node->type.branching.right);
}

bool*
huffman_node_path(
    const huffman_node* root,
    uint8_t val,
    int64_t* path_length
)
{
    static bool path_array[PATH_ARRAY_LENGTH] = {0};

    *path_length = 0;
    huffman_node_path_impl_(root, val, path_array, path_length);

    return path_array;
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
        printf("{ '%c', %llu }", node->type.leaf.val, node->type.leaf.count);
    }
    else
    {
        printf("{ ");
        huffman_node_print_inline(node->type.branching.left);
        printf(", ");
        huffman_node_print_inline(node->type.branching.right);
        printf(" }");
    }
}

void
huffman_node_print_depth(
    const huffman_node* node
)
{
    huffman_node_print_depth_impl_(node, 0);
}

// --- LOCAL IMPLEMENTATIONS --- //

void
huffman_node_print_depth_impl_(
    const huffman_node* node,
    int depth
)
{
    const char* tabs = "                                   ";
    if (node == NULL)
    {
        printf("%.*s(nil)", depth, tabs);
        return;
    }

    if (!node->is_branching)
    {
        printf("%.*s< '%c', %llu >", depth, tabs, node->type.leaf.val, node->type.leaf.count);
    }
    else
    {
        printf("%.*s{\n", depth, tabs);
        huffman_node_print_depth_impl_(node->type.branching.left, depth + 1);
        printf(",\n");
        huffman_node_print_depth_impl_(node->type.branching.right, depth + 1);
        printf("\n%.*s}", depth, tabs);
    }
}

bool
huffman_node_path_impl_(
    const huffman_node* root,
    uint8_t val,
    bool* path,
    int64_t* len
)
{
    if (root == NULL || !root->is_branching)
        return false;

    if (huffman_node_path_inner_(root->type.branching.left, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_LEFT;
        return true;
    }

    if (huffman_node_path_inner_(root->type.branching.right, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_RIGHT;
        return true;
    }

    return false;
}

bool
huffman_node_path_inner_(
    const huffman_node* root,
    uint8_t val,
    bool* path,
    int64_t* len
)
{
    if (root == NULL)
        return false;

    if (!root->is_branching)
        return root->type.leaf.val == val;

    if (huffman_node_path_inner_(root->type.branching.left, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_LEFT;
        return true;
    }

    if (huffman_node_path_inner_(root->type.branching.right, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_RIGHT;
        return true;
    }

    return false;
}

// --- END LOCAL IMPLEMENTATIONS --- /

