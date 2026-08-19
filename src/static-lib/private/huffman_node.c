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
    huffman_node* const node,
    const uint8_t value,
    const uint64_t count
)
{
    node->is_branching = false;
    node->leaf.val   = value;
    node->leaf.count = count;

    return true;
}

bool
huffman_node_init_branching(
    huffman_node* const node
)
{
    node->is_branching = true;
    node->branching.right = NULL;
    node->branching.left  = NULL;

    return true;
}

void
huffman_node_clean(
    huffman_node* const node
)
{
    if (node == NULL)
        return;

    if (!node->is_branching)
    {
        node->leaf.val = 0;
        node->leaf.count = 0;
        return;
    }

    huffman_node* right = node->branching.right;
    huffman_node* left = node->branching.left;

    if (right != NULL)
        huffman_node_clean(right);

    if (left != NULL)
        huffman_node_clean(left);

    if (right != NULL)
        free(right);

    if (left != NULL)
        free(left);

    node->branching.right = NULL;
    node->branching.left  = NULL;
}

bool
huffman_node_assign_right(
    huffman_node* const node,
    const huffman_node* const right
)
{
    if (!node->is_branching)
        return false;

    if (node->branching.right != NULL)
        return false;

    node->branching.right = malloc(sizeof(huffman_node));
    if (node->branching.right == NULL)
        return false;

    const void* result = memcpy(node->branching.right, right, sizeof(huffman_node));

    return result == node->branching.right;
}

bool
huffman_node_assign_left(
    huffman_node* const node,
    const huffman_node* const left
)
{
    if (!node->is_branching)
        return false;

    if (node->branching.left != NULL)
        return false;

    node->branching.left = malloc(sizeof(huffman_node));
    if (node->branching.left == NULL)
        return false;

    const void* result = memcpy(node->branching.left, left, sizeof(huffman_node));

    return result == node->branching.left;
}

uint64_t
huffman_node_count(
    const huffman_node* const node
)
{
    if (node == NULL)
        return 0;

    if (!node->is_branching)
        return node->leaf.count;

    return huffman_node_count(node->branching.left) +
           huffman_node_count(node->branching.right);
}

int
huffman_node_compare(
    const huffman_node* const a,
    const huffman_node* const b
)
{
    // First compare the count of a against b
    const int64_t count_compare = (int64_t) huffman_node_count(a) - (int64_t) huffman_node_count(b);

    // Then when either one is not a leaf, immediately return
    // when the count-compare is 0
    if (count_compare != 0 || (a->is_branching || b->is_branching))
        return (int) count_compare;

    // Otherwise we also test for alpanumerical value
    return b->leaf.val - a->leaf.val;
}

bool*
huffman_node_path(
    const huffman_node* const root,
    const uint8_t val,
    int64_t* const path_length
)
{
    static bool path_array[PATH_ARRAY_LENGTH] = {0};

    *path_length = 0;
    huffman_node_path_impl_(root, val, path_array, path_length);

    return path_array;
}

huffman_node*
huffman_node_traverse(
    const huffman_node* const node,
    const bool branch
)
{
    if (node == NULL || !node->is_branching)
        return NULL;

    if (branch)
        return node->branching.right;

    return node->branching.left;
}

void
huffman_node_print_inline(
    const huffman_node* const node
)
{
    if (node == NULL)
    {
        printf("(nil)");
        return;
    }

    if (!node->is_branching)
    {
        printf("{ '%c', %llu }", node->leaf.val, node->leaf.count);
    }
    else
    {
        printf("{ ");
        huffman_node_print_inline(node->branching.left);
        printf(", ");
        huffman_node_print_inline(node->branching.right);
        printf(" }");
    }
}

void
huffman_node_print_depth(
    const huffman_node* const node
)
{
    huffman_node_print_depth_impl_(node, 0);
}

// --- LOCAL IMPLEMENTATIONS --- //

void
huffman_node_print_depth_impl_(
    const huffman_node* const node,
    const int depth
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
        printf("%.*s< '%c', %llu >", depth, tabs, node->leaf.val, node->leaf.count);
    }
    else
    {
        printf("%.*s{\n", depth, tabs);
        huffman_node_print_depth_impl_(node->branching.left, depth + 1);
        printf(",\n");
        huffman_node_print_depth_impl_(node->branching.right, depth + 1);
        printf("\n%.*s}", depth, tabs);
    }
}

bool
huffman_node_path_impl_(
    const huffman_node* const root,
    const uint8_t val,
    bool* const path,
    int64_t* const len
)
{
    if (root == NULL)
        return false;

    if (!root->is_branching)
    {
        if (huffman_node_path_inner_(root, val, path, len))
        {
            path[(*len)++] = HUFFMAN_BRANCH_RIGHT;
            return true;
        }

        return false;
    }

    if (huffman_node_path_inner_(root->branching.left, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_LEFT;
        return true;
    }

    if (huffman_node_path_inner_(root->branching.right, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_RIGHT;
        return true;
    }

    return false;
}

bool
huffman_node_path_inner_(
    const huffman_node* const root,
    const uint8_t val,
    bool* const path,
    int64_t* const len
)
{
    if (root == NULL)
        return false;

    if (!root->is_branching)
        return root->leaf.val == val;

    if (huffman_node_path_inner_(root->branching.left, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_LEFT;
        return true;
    }

    if (huffman_node_path_inner_(root->branching.right, val, path, len))
    {
        path[(*len)++] = HUFFMAN_BRANCH_RIGHT;
        return true;
    }

    return false;
}

// --- END LOCAL IMPLEMENTATIONS --- /

