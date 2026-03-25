#include "huffman_processing.h"

#include <string.h>
#include <stdlib.h>

// --- LOCAL DEFINITIONS --- //

static int compare_symbol_(const void* a, const void* b);

static dlinked_list_node* doublyll_create_();
static dlinked_list_node* doublyll_get_head_(dlinked_list_node* node);
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
doublyll_insert_leaf_oncount_(
    dlinked_list_node* root,
    const huffman_node* entry
)
{
    const uint64_t entry_count = huffman_node_count(entry);
    const uint8_t    entry_val = entry->type_.leaf.val;
    if (entry_count == 0)
        return NULL;

    root = doublyll_get_head_(root);
    dlinked_list_node* current = root;

    // Search for the first entry in the list
    // that is equal or tinier in count

    while (current != NULL && current->next != NULL &&
           huffman_node_count(&current->node) > entry_count)
        current = current->next;

    // When we stumbled upon an entry that is the same in count,
    // we search alpabetically for the first entry that should alpabetically
    // come after the current value
    if (huffman_node_count(&current->node) == entry_count)
    {
        while (current != NULL && current->next != NULL &&
               huffman_node_count(&current->node) == entry_count &&
               current->node.type_.leaf.val < entry_val)

            current = current->next;
    }

    // Create a new dll-node to contain the huffman-node
    dlinked_list_node* new_entry_node = doublyll_create_();
    if (new_entry_node == NULL)
        return NULL;

    memcpy(&new_entry_node->node, entry, sizeof(huffman_node));

    // Insert the entry through a newly created node into the doubly linked list

    // When the found count is smaller than the entry,
    // we insert *before* the found one
    if (huffman_node_count(&current->node) < entry_count)
    {
        dlinked_list_node* prev = current->prev;
        
        if (prev != NULL)
            prev->next = new_entry_node;

        new_entry_node->prev = prev;
        new_entry_node->next = current;
        current->prev = new_entry_node;
    }

    // Otherwise we insert *after*
    else
    {
        dlinked_list_node* next = current->next;

        if (next != NULL)
            next->prev = new_entry_node;

        new_entry_node->prev = current;
        new_entry_node->next = next;
        current->next = new_entry_node;
    }

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

    while (current < last && output->type_.leaf.val == *current)
    {
        output->type_.leaf.count++;
        current++;
    }

    return current;
}

// --- END LOCAL DEFINITIONS --- //
