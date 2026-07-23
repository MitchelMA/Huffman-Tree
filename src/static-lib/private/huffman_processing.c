#include "huffman_processing.h"

#include "utility.h"
#include "io.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>


// --- LOCAL DEFINITIONS --- //

static int compare_symbol_(const void* a, const void* b);

static dlinked_list_node* doublyll_create_(void);

static dlinked_list_node* doublyll_get_head_(dlinked_list_node* node);
static const dlinked_list_node* doublyll_get_head_const_(const dlinked_list_node* node);
static dlinked_list_node* doublyll_get_tail_(dlinked_list_node* node);

static dlinked_list_node* doublyll_insert_before_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_insert_after_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_remove_(dlinked_list_node* removed);

static dlinked_list_node* doublyll_reinsert_before_(dlinked_list_node* target, dlinked_list_node* inserted);
static dlinked_list_node* doublyll_reinsert_after_(dlinked_list_node* target, dlinked_list_node* inserted);

static size_t doublyll_get_length_(const dlinked_list_node* node);

static dlinked_list_node* doublyll_insert_leaf_oncount_(dlinked_list_node* root, const huffman_node* entry);
static dlinked_list_node* doublyll_insert_or_update_(dlinked_list_node* root, uint8_t symbol);

static const uint8_t* calculate_consecutive_(const uint8_t* start, const uint8_t* last, huffman_node* output);

static bool compress_symbol_(const huffman_node* root, uint8_t symbol, bool* out_buffer, size_t buffer_size, int64_t* out_processed);
static int64_t compress_symbols_buffer_(const huffman_node* root, const uint8_t* buffer, size_t buffer_size, bitarray* out_bitarray, bitarray_length_t* out_processed);
static int64_t compress_symbols_io_(const huffman_node* root, FILE* in_stream, bitarray* out_bitarray, int64_t* out_processed);

// --- END LOCAL DEFINITIONS --- //

dlinked_list_node*
huffman_count_text(
    const uint8_t* const text,
    const int64_t length
)
{
    if (text == NULL || length <= 0)
        return NULL;

    uint8_t* const text_copy = malloc(sizeof(uint8_t) * (length + 1));
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

    const uintptr_t end = (uintptr_t)(text_copy + length);
    const uint8_t* after = calculate_consecutive_(text_copy, (uint8_t*)end, &list_root->node);

    // Then exhaustively insert the rest of the found characters as nodes to the linked listed
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
huffman_count_stream(
    FILE* const stream
)
{
    const long stream_length = stream_tell_size(stream);
    uint8_t* const text_copy = malloc(sizeof(uint8_t) * (stream_length + 1));
    if (text_copy == NULL)
        return NULL;

    if (fread(text_copy, sizeof(uint8_t), stream_length, stream) < (size_t) stream_length)
        return NULL;
    text_copy[stream_length] = '\0';

    qsort(text_copy, stream_length, sizeof(uint8_t), compare_symbol_);

    dlinked_list_node* const list_root = doublyll_create_();

    const uintptr_t end = (uintptr_t)(text_copy + stream_length);
    const uint8_t* after = calculate_consecutive_(text_copy, (uint8_t*)end, &list_root->node);

    while (after != NULL)
    {
        huffman_node symbol_node = {0};
        after = calculate_consecutive_(after, (uint8_t*)end, &symbol_node);
        doublyll_insert_leaf_oncount_(list_root, &symbol_node);
    }

    free(text_copy);

    return doublyll_get_head_(list_root);
}

dlinked_list_node*
huffman_count_stream_onepass(
    FILE* const stream
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

    return doublyll_get_head_(list_root);
}

huffman_node
huffman_construct_tree(
    dlinked_list_node* root
)
{
    while (doublyll_get_length_(root) > 1)
    {
        dlinked_list_node* const    ultimate_node = doublyll_get_tail_(root);
        dlinked_list_node* const penultimate_node = ultimate_node->prev;

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

    const huffman_node huffman_root = root->node;
    doublyll_clean(root);

    return huffman_root;
}

bitarray_length_t
huffman_compress(
    const huffman_node* const root,
    const size_t buffer_size,
    const uint8_t* const buffer,
    bitarray* const out_bitarray
)
{
    if (root == NULL || buffer == NULL || buffer_size == 0 || out_bitarray == NULL)
        return 0;

    bitarray_length_t processed = 0;
    size_t buffer_idx = 0;
    while (buffer_idx < buffer_size)
    {
        bitarray_length_t bits_appended = 0;
        buffer_idx += compress_symbols_buffer_(
            root,
            (buffer + buffer_idx),
            (buffer_size - buffer_idx),
            out_bitarray,
            &bits_appended
        );
        processed += bits_appended;
    }

    return processed;
}

bitarray_length_t
huffman_compress_stream(
    const huffman_node* const root,
    FILE* const in_stream,
    bitarray* const out_bitarray
)
{
    if (root == NULL || in_stream == NULL || out_bitarray == NULL)
        return 0;

    bitarray_length_t processed = 0;
    int64_t bits_appended = 0;
    int64_t advanced = 0;

    do
    {
         advanced = compress_symbols_io_(
            root,
            in_stream,
            out_bitarray,
            &bits_appended
        );

        processed += bits_appended;
    } while (advanced > 0);

    return processed;
}

#define decompress_(path, root, bit_retrieval, writer, written) \
do { \
    if (path->is_branching) \
    { \
        const bool bit_ = (bit_retrieval); \
        (path) = huffman_node_traverse((path), bit_); \
    } \
    if ((path) == NULL) \
        return (written); \
    if ((path)->is_branching) \
        break; \
    writer \
    (written)++; \
    (path) = (root); \
} while (0)

size_t
huffman_decompress(
    const huffman_node* const root,
    const bitarray* const bit_array,
    const size_t buffer_size,
    uint8_t* const buffer
)
{
    if (root == NULL || bit_array == NULL || buffer_size == 0 || buffer == NULL)
        return 0;

    const bitarray_length_t bit_arr_len = bitarray_length(bit_array);
    size_t buffer_written = 0;
    const huffman_node* path = root;

    #define process_buffer_len (5 * 1024)
    bool process_buffer[process_buffer_len] = {0};
    int64_t process_buffer_working_len = 0;
    bitarray_length_t process_buffer_index = 0;
    bitarray_length_t bitarray_prolonged_index = 0;

    while (buffer_written < (buffer_size - 1))
    {
        bitarray_prolonged_index += process_buffer_working_len;
        process_buffer_working_len = bitarray_read_bits(bit_array,
            bitarray_prolonged_index,
            MIN(process_buffer_len, bit_arr_len - bitarray_prolonged_index),
            process_buffer);

        if (process_buffer_working_len <= 0)
            break;

        process_buffer_index = 0;

        for (; process_buffer_index < (bitarray_length_t) process_buffer_working_len &&
            buffer_written < (buffer_size - 1);
            process_buffer_index++)
        {
            decompress_(
                path,
                root,
                process_buffer[process_buffer_index],
                buffer[buffer_written] = path->leaf.val;,
                buffer_written);
        }
    }

    #undef process_buffer_len

    buffer[buffer_written] = '\0';

    return buffer_written;
}

size_t
huffman_decompress_stream(
    const huffman_node* const root,
    const bitarray* const bit_array,
    FILE* const out_stream
)
{
    if (root == NULL || bit_array == NULL || out_stream == NULL)
        return 0;

    const bitarray_length_t bit_arr_len = bitarray_length(bit_array);
    size_t stream_written = 0;
    const huffman_node* path = root;

    #define process_buffer_len (5 * 1024)
    bool process_buffer[process_buffer_len] = {0};
    int64_t process_buffer_working_len = 0;
    bitarray_length_t process_buffer_index = 0;
    bitarray_length_t bitarray_prolonged_index = 0;

    while (true)
    {
        bitarray_prolonged_index += process_buffer_working_len;
        process_buffer_working_len = bitarray_read_bits(bit_array,
            bitarray_prolonged_index,
            MIN(process_buffer_len, bit_arr_len - bitarray_prolonged_index),
            process_buffer);

        if (process_buffer_working_len <= 0)
            break;

        process_buffer_index = 0;

        for (; process_buffer_index < (bitarray_length_t) process_buffer_working_len;
            process_buffer_index++)
        {
            decompress_(
                path,
                root,
                process_buffer[process_buffer_index],
                if (fputc(path->leaf.val, out_stream) == EOF)
                    return stream_written;,
                stream_written
            );
        }
    }

    #undef process_buffer_len

    return stream_written;
}

#undef decompress_

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
    const void* const a,
    const void* const b
)
{
    return *(uint8_t*)a - *(uint8_t*)b;
}

dlinked_list_node*
doublyll_create_(
    void
)
{
    dlinked_list_node* const node = malloc(sizeof(dlinked_list_node));

    if (node == NULL)
        return NULL;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

dlinked_list_node*
doublyll_get_head_(
    dlinked_list_node* const node
)
{
    dlinked_list_node* current = node;
    while (current != NULL && current->prev != NULL)
        current = current->prev;

    return current;
}

const dlinked_list_node*
doublyll_get_head_const_(
    const dlinked_list_node* const node
)
{
    const dlinked_list_node* current = node;
    while (current != NULL && current->prev != NULL)
        current = current->prev;

    return current;
}

dlinked_list_node*
doublyll_get_tail_(
    dlinked_list_node* const node
)
{
    dlinked_list_node* current = node;
    while (current != NULL && current->next != NULL)
        current = current->next;

    return current;
}

dlinked_list_node*
doublyll_insert_before_(
    dlinked_list_node* const target,
    dlinked_list_node* const inserted
)
{
    dlinked_list_node* const prev = target->prev;

    if (prev != NULL)
        prev->next = inserted;

    inserted->prev = prev;
    inserted->next = target;
    target->prev = inserted;

    return inserted;
}

dlinked_list_node*
doublyll_insert_after_(
    dlinked_list_node* const target,
    dlinked_list_node* const inserted
)
{
    dlinked_list_node* const next = target->next;

    if (next != NULL)
        next->prev = inserted;

    inserted->next = next;
    inserted->prev = target;
    target->next = inserted;

    return inserted;
}

dlinked_list_node*
doublyll_remove_(
    dlinked_list_node* const removed
)
{
    if (removed == NULL)
        return NULL;

    dlinked_list_node* const old_prev = removed->prev;
    dlinked_list_node* const old_next = removed->next;
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
    dlinked_list_node* const target,
    dlinked_list_node* const inserted
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
    dlinked_list_node* const target,
    dlinked_list_node* const inserted
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
    const dlinked_list_node* const node
)
{
    const dlinked_list_node* root = doublyll_get_head_const_(node);
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
    const huffman_node* const entry
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
    dlinked_list_node* const root,
    const uint8_t symbol
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
    dlinked_list_node* const newly = target;
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

const uint8_t*
calculate_consecutive_(
    const uint8_t* const start,
    const uint8_t* const last,
    huffman_node* output
)
{
    if (start >= last)
        return NULL;

    const uint8_t* current = start;

    if (!huffman_node_init_leaf(output, *current, 0))
        return NULL;

    while (current < last && output->leaf.val == *current)
    {
        output->leaf.count++;
        current++;
    }

    return current;
}


bool
compress_symbol_(
    const huffman_node* const root,
    const uint8_t symbol,
    bool* const out_buffer,
    const size_t buffer_size,
    int64_t* const out_processed
)
{
    int64_t path_len = 0;
    const bool* const path = huffman_node_path(root, symbol, &path_len);
    if ((size_t) path_len > buffer_size)
        return false;

    while (path_len-- > 0)
        out_buffer[(*out_processed)++] = path[path_len];

    return true;
}

int64_t
compress_symbols_buffer_(
    const huffman_node* const root,
    const uint8_t* const buffer,
    const size_t buffer_size,
    bitarray* const out_bitarray,
    bitarray_length_t* const out_processed
)
{
    int64_t symbols_advanced = 0;

    #define process_buffer_size (5 * 1024)
    bool process_buffer[process_buffer_size] = {0};

    // read into the process buffer
    int64_t read_into_process_buffer = 0;
    for (int64_t i = 0; (size_t) i < buffer_size; i++)
    {
        int64_t processed = 0;
        if (!compress_symbol_(root,
            buffer[i],
            (process_buffer + read_into_process_buffer),
            (process_buffer_size - read_into_process_buffer),
            &processed))
            break;

        read_into_process_buffer += processed;
        symbols_advanced++;
    }

    // insert the booleans into the out bitarray
    *out_processed = bitarray_append_bits(out_bitarray, read_into_process_buffer, process_buffer);
    return symbols_advanced;
    #undef process_buffer_size
}

int64_t
compress_symbols_io_(
    const huffman_node* const root,
    FILE* const in_stream,
    bitarray* const out_bitarray,
    int64_t* const out_processed
)
{
    int64_t symbols_advanced = 0;
    #define process_buffer_size (5 * 1024)
    bool process_buffer[process_buffer_size] = {0};

    bitarray_length_t read_into_process_buffer = 0;
    int symbol = EOF;

    while ((symbol = fgetc(in_stream)) != EOF)
    {
        int64_t processed = 0;
        if (!compress_symbol_(root,
            (uint8_t)symbol,
            (process_buffer + read_into_process_buffer),
            (process_buffer_size - read_into_process_buffer),
            &processed))
        {
            ungetc(symbol, in_stream);
            break;
        }

        read_into_process_buffer += processed;
        symbols_advanced++;
    }

    *out_processed = bitarray_append_bits(out_bitarray, read_into_process_buffer, process_buffer);
    return symbols_advanced;
    #undef process_buffer_size
}

// --- END LOCAL DEFINITIONS --- //

