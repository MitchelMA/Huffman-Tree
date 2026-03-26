#include "huffman_processing.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>


// --- LOCAL DEFINITIONS --- //

static int compare_symbol_(const void* a, const void* b);

static dlinked_list_node* doublyll_create_();
static dlinked_list_node* doublyll_get_head_(dlinked_list_node* node);
static dlinked_list_node* doublyll_get_tail_(dlinked_list_node* node);
static dlinked_list_node* doublyll_insert_before_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_insert_after_(dlinked_list_node* target, dlinked_list_node* inserted);
static size_t doublyll_get_length_(dlinked_list_node* node);
static dlinked_list_node* doublyll_insert_leaf_oncount_(dlinked_list_node* root, const huffman_node* entry);

static uint8_t* calculate_consecutive_(uint8_t* start, uint8_t* last, huffman_node* output);

// --- END LOCAL DEFINITIONS --- //

dlinked_list_node*
huffman_count_text(
    const uint8_t* text,
    int64_t length
)
{
    if (text == NULL || length <= 0)
        return NULL;

    uint8_t* text_copy = malloc(sizeof(uint8_t) * (length + 1));
    if (text_copy == NULL)
        return NULL;

    if (memcpy(text_copy, text, length) != text_copy)
    {
        free(text_copy);
        return NULL;
    }

    qsort(text_copy, length, sizeof(uint8_t), compare_symbol_);

    dlinked_list_node* list_root = doublyll_create_();

    // Manually set the first entry into the linked list
    // to initialize properly

    uintptr_t end = (uintptr_t)(text_copy + length);
    uint8_t* after = calculate_consecutive_(text_copy, (uint8_t*)end, &list_root->node);

    // Then exaustively insert the rest of the found characters as nodes to the linked listed
    while (after != NULL)
    {
        huffman_node symbol_node = {0};
        after = calculate_consecutive_(after, (uint8_t*)end, &symbol_node);
        doublyll_insert_leaf_oncount_(list_root, &symbol_node);
    }

    // Wont need the copy after this point
    free(text_copy);

    list_root = doublyll_get_head_(list_root);

    return list_root;
}

huffman_node
huffman_construct_tree(
    dlinked_list_node* root
)
{
    while (doublyll_get_length_(root) > 1)
    {
        root = doublyll_get_head_(root);

        dlinked_list_node*    ultimate_node = doublyll_get_tail_(root);
        dlinked_list_node* penultimate_node = ultimate_node->prev;
        dlinked_list_node* chain_linker = penultimate_node->prev;

        // Disconnect the last two from the linked-list
        if (chain_linker != NULL)
            chain_linker->next = NULL;

        penultimate_node->prev = NULL;
        penultimate_node->next = NULL;
        ultimate_node->prev = NULL;

        // Create a branching huffman-node that will be the
        // parent of the ultimate and penultimate nodes

        huffman_node parent = {0};
        huffman_node_init_branching(&parent);
        huffman_node_assign_left(&parent, &penultimate_node->node);
        huffman_node_assign_right(&parent, &ultimate_node->node);

        // Clean up the tail of the list, as this won't be reused
        doublyll_clean(ultimate_node);

        // Copy the branching parent over into the penultimate-node
        memcpy(&penultimate_node->node, &parent, sizeof(huffman_node));

        if (penultimate_node == root)
            break;

        // Find the best fitting reinsert target for the penultimate-node.
        dlinked_list_node* target = root;

        while (target != NULL && target->next != NULL &&
               huffman_node_compare(&penultimate_node->node, &target->node) <= 0)
            target = target->next;

        const int comp = huffman_node_compare(&penultimate_node->node, &target->node);

        // Insert *before* the target if the 'to be inserted'-node is
        // larger
        if (comp > 0)
            doublyll_insert_before_(target, penultimate_node);

        // Otherwise it comes *after*
        else
            doublyll_insert_after_(target, penultimate_node);
    }

    huffman_node huffman_root = root->node;
    doublyll_clean(root);

    return huffman_root;
}

void
doublyll_clean(
    dlinked_list_node* root
)
{
    root = doublyll_get_head_(root);

    while (root != NULL)
    {
        dlinked_list_node* next = root->next;
        root->prev = NULL;
        root->next = NULL;

        free(root);

        root = next;
    }
}

// --- LOCAL IMPLEMENTATIONS --- //

int
compare_symbol_(
    const void* a,
    const void* b
)
{
    return *(uint8_t*)a - *(uint8_t*)b;
}


dlinked_list_node*
doublyll_create_()
{
    dlinked_list_node* node = malloc(sizeof(dlinked_list_node));

    if (node == NULL)
        return NULL;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

dlinked_list_node*
doublyll_get_head_(
    dlinked_list_node* node
)
{
    while (node != NULL && node->prev != NULL)
        node = node->prev;

    return node;
}

dlinked_list_node*
doublyll_get_tail_(
    dlinked_list_node* node
)
{
    while (node != NULL && node->next != NULL)
        node = node->next;

    return node;
}

dlinked_list_node*
doublyll_insert_before_(
    dlinked_list_node* target,
    dlinked_list_node* inserted
)
{
    dlinked_list_node* prev = target->prev;

    if (prev != NULL)
        prev->next = inserted;

    inserted->prev = prev;
    inserted->next = target;
    target->prev = inserted;

    return inserted;
}

dlinked_list_node*
doublyll_insert_after_(
    dlinked_list_node* target,
    dlinked_list_node* inserted
)
{
    dlinked_list_node* next = target->next;

    if (next != NULL)
        next->prev = inserted;

    inserted->next = next;
    inserted->prev = target;
    target->next = inserted;

    return inserted;
}

size_t
doublyll_get_length_(
    dlinked_list_node* node
)
{
    dlinked_list_node* root = doublyll_get_head_(node);
    size_t count = 1;

    while (root != NULL && root->next != NULL)
    {
        root = root->next;
        count++;
    }

    return count;
}

dlinked_list_node*
doublyll_insert_leaf_oncount_(
    dlinked_list_node* root,
    const huffman_node* entry
)
{
    const uint64_t entry_count = huffman_node_count(entry);
    if (entry_count == 0)
        return NULL;

    root = doublyll_get_head_(root);
    dlinked_list_node* current = root;

    // Search for the first entry in the list
    // that the new entry is tinier to in comparison.
    while (current != NULL && current->next != NULL &&
           huffman_node_compare(entry, &current->node) < 0)
        current = current->next;

    // Create a new dll-node to contain the huffman-node
    dlinked_list_node* new_entry_node = doublyll_create_();
    if (new_entry_node == NULL)
        return NULL;

    memcpy(&new_entry_node->node, entry, sizeof(huffman_node));

    // Insert the entry through a newly created node into the doubly linked list

    // When the found count is smaller than the entry,
    // we insert *before* the found one
    if (huffman_node_count(&current->node) < entry_count)
        doublyll_insert_before_(current, new_entry_node);

    // Otherwise we insert *after*
    else
        doublyll_insert_after_(current, new_entry_node);

    return new_entry_node;
}

uint8_t*
calculate_consecutive_(
    uint8_t* start,
    uint8_t* last,
    huffman_node* output
)
{
    if (start >= last)
        return NULL;

    uint8_t* current = start;

    if (!huffman_node_init_leaf(output, *current, 0))
        return NULL;

    while (current < last && output->type.leaf.val == *current)
    {
        output->type.leaf.count++;
        current++;
    }

    return current;
}

// --- END LOCAL DEFINITIONS --- //

