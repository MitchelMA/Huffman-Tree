#include "huffman_processing.h"

#include "utility.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>


// --- LOCAL DEFINITIONS --- //

static int compare_symbol_(const void* a, const void* b);

static dlinked_list_node* doublyll_create_(void);

static dlinked_list_node* doublyll_get_head_(dlinked_list_node* node);
static dlinked_list_node* doublyll_get_tail_(dlinked_list_node* node);

static dlinked_list_node* doublyll_insert_before_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_insert_after_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_remove_(dlinked_list_node* removed);

static dlinked_list_node* doublyll_reinsert_before_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_reinsert_after_(dlinked_list_node* target, dlinked_list_node* inserted);

static size_t doublyll_get_length_(dlinked_list_node* node);

static dlinked_list_node* doublyll_insert_leaf_oncount_(dlinked_list_node* root, const huffman_node* entry);
static dlinked_list_node* doublyll_insert_or_update_(dlinked_list_node* root, uint8_t symbol);

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

dlinked_list_node*
huffman_count_stream_onepass(
    FILE* stream
)
{
    int symbol = fgetc(stream);

    if (symbol == EOF)
        return NULL;

    dlinked_list_node* list_root = doublyll_create_();
    huffman_node_init_leaf(&list_root->node, (uint8_t)symbol, 1);

    while ((symbol = fgetc(stream)) != EOF)
    {
        if (doublyll_insert_or_update_(list_root, (uint8_t)symbol) == NULL)
            break;

        list_root = doublyll_get_head_(list_root);
    }

    doublyll_get_head_(list_root);
    return list_root;
}

huffman_node
huffman_construct_tree(
    dlinked_list_node* root
)
{
    while (doublyll_get_length_(root) > 1)
    {
        dlinked_list_node*    ultimate_node = doublyll_get_tail_(root);
        dlinked_list_node* penultimate_node = ultimate_node->prev;

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
        dlinked_list_node* target = penultimate_node->prev;

        while (target != NULL && target->prev != NULL &&
               huffman_node_compare(&penultimate_node->node, &target->node) > 0)
            target = target->prev;

        // Insert *before* the target if the 'to be inserted'-node is
        // larger
        if (huffman_node_compare(&penultimate_node->node, &target->node) > 0)
            doublyll_reinsert_before_(target, penultimate_node);

        // Otherwise it comes *after*
        else
            doublyll_reinsert_after_(target, penultimate_node);

        root = doublyll_get_head_(root);
    }

    huffman_node huffman_root = root->node;
    doublyll_clean(root);

    return huffman_root;
}

bitarray_length_t
huffman_compress(
    const huffman_node* root,
    size_t buffer_size,
    const uint8_t* buffer,
    bitarray* out_bitarray
)
{
    if (root == NULL || buffer == NULL || buffer_size == 0 || out_bitarray == NULL)
        return 0;

    bitarray_length_t processed = 0;
    for (size_t idx = 0; idx < buffer_size; idx++)
    {
        int64_t path_len = 0;
        bool* path = huffman_node_path(root, buffer[idx], &path_len);
        ARRAY_REVERSE(bool, path, path_len);
        int64_t appended = bitarray_append_bits(out_bitarray, path_len, path);
        processed += appended;
        if (appended < path_len)
            return processed;
    }

    return processed;
}

bitarray_length_t
huffman_compress_stream(
    const huffman_node* root,
    FILE* in_stream,
    bitarray* out_bitarray
)
{
    if (root == NULL || in_stream == NULL || out_bitarray == NULL)
        return 0;

    bitarray_length_t processed = 0;
    int symbol = EOF;

    while ((symbol = fgetc(in_stream)) != EOF)
    {
        int64_t path_len = 0;
        bool* path = huffman_node_path(root, (uint8_t)symbol, &path_len);
        ARRAY_REVERSE(bool, path, path_len);
        int64_t appended = bitarray_append_bits(out_bitarray, path_len, path);
        processed += appended;
        if (appended < path_len)
            return processed;
    }

    return processed;
}

size_t
huffman_decompress(
    const huffman_node* root,
    const bitarray* bit_array,
    size_t buffer_size,
    uint8_t* buffer
)
{
    if (root == NULL || bit_array == NULL || buffer_size == 0 || buffer == NULL)
        return 0;

    const bitarray_length_t bit_arr_len = bitarray_length(bit_array);
    size_t buffer_written = 0;
    const huffman_node* path = root;

    for (bitarray_length_t bit_idx = 0; bit_idx < bit_arr_len; bit_idx++)
    {
        if (path->is_branching)
        {
            bool bit = (bool)bitarray_value_at(bit_array, bit_idx);
            path = huffman_node_traverse(path, bit);
        }

        if (path == NULL)
            break;

        if (path->is_branching)
            continue;

        if (buffer_written >= buffer_size - 1)
            break;

        buffer[buffer_written++] = path->leaf.val;
        path = root;
    }

    buffer[buffer_written] = '\0';

    return buffer_written;
}

size_t
huffman_decompress_stream(
    const huffman_node* root,
    const bitarray* bit_array,
    FILE* out_stream
)
{
    if (root == NULL || bit_array == NULL || out_stream == NULL)
        return 0;

    const bitarray_length_t bit_arr_len = bitarray_length(bit_array);
    size_t stream_written = 0;
    const huffman_node* path = root;

    for (bitarray_length_t bit_idx = 0; bit_idx < bit_arr_len; bit_idx++)
    {
        if (path->is_branching)
        {
            bool bit = (bool)bitarray_value_at(bit_array, bit_idx);
            path = huffman_node_traverse(path, bit);
        }

        if (path == NULL)
            break;

        if (path->is_branching)
            continue;

        if (fputc(path->leaf.val, out_stream) == EOF)
            break;

        stream_written++;
        path = root;
    }

    return stream_written;
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
doublyll_create_(
    void
)
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

dlinked_list_node*
doublyll_remove_(
    dlinked_list_node* removed
)
{
    if (removed == NULL)
        return NULL;

    dlinked_list_node* old_prev = removed->prev;
    dlinked_list_node* old_next = removed->next;
    removed->prev = NULL;
    removed->next = NULL;

    if (old_prev != NULL)
        old_prev->next = old_next;
    if (old_next != NULL)
        old_next->prev = old_prev;

    return removed;
}

dlinked_list_node*
doublyll_reinsert_before_(
    dlinked_list_node* target,
    dlinked_list_node* inserted
)
{
    if (target == inserted)
        return NULL;

    // First check if target actually exists
    if (target == NULL || inserted == NULL)
        return NULL;

    // Remove the target from the list and reinsert before the target
    return doublyll_insert_before_(target, doublyll_remove_(inserted));
}

dlinked_list_node*
doublyll_reinsert_after_(
    dlinked_list_node* target,
    dlinked_list_node* inserted
)
{
    if (target == inserted)
        return NULL;

    // First check if target actually exists
    if (target == NULL || inserted == NULL)
        return NULL;

    // Remove the target from the list and reinsert after the target
    return doublyll_insert_after_(target, doublyll_remove_(inserted));
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

dlinked_list_node*
doublyll_insert_or_update_(
    dlinked_list_node* root,
    uint8_t symbol
)
{
    dlinked_list_node* target = root;
    dlinked_list_node* previous = target;

    // First search if this symbol already exists in the list
    while (target != NULL)
    {
        if (target->node.leaf.val == symbol)
            break;

        previous = target;
        target = target->next;
    }

    // If not, create a new one
    if (target == NULL)
    {
        target = doublyll_create_();
        huffman_node_init_leaf(&target->node, symbol, 0);

        // Append the target at the end of the linked list
        previous->next = target;
        target->prev = previous;
    }

    target->node.leaf.count++;

    // Find the place where we should *actually* insert the newly created/updated node
    dlinked_list_node* newly = target;
    target = target->prev;

    // When 'newly' was already the root of the linked-list
    if (target == NULL)
        return newly;

    // Find the insert place
    while (target != NULL && target->prev != NULL &&
        huffman_node_compare(&newly->node, &target->node) > 0)
        target = target->prev;

    // Insert before or after the target depending on the comparison
    if (huffman_node_compare(&newly->node, &target->node) > 0)
        doublyll_reinsert_before_(target, newly);
    else
        doublyll_reinsert_after_(target, newly);

    return newly;
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

    while (current < last && output->leaf.val == *current)
    {
        output->leaf.count++;
        current++;
    }

    return current;
}

// --- END LOCAL DEFINITIONS --- //

