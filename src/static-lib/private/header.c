#include "header.h"

#include "utility.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define SLIDING_WINDOW_MAX_SIZE 32
#define HEADER_END_INDICATOR "---"

#define NEWLINE_PREDICATOR "-"
#if defined(WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif 

static char* buffer_shift_append_(size_t buffer_size, char* buffer, char symbol);
static void  keypair_clean_(void* pair);
static huffman_header_keypair* vector_find_keypair_(const vector_t* header_entries, size_t key_length, const char* key);
static huffman_header_keypair* find_keypair_(const huffman_header* header, size_t key_length, const char* key);
static size_t repurpose_buffer_(size_t* out_buffer_size, char** buffer, const char* new_data, size_t new_data_size);
static bool vector_add_keypair_(vector_t* header_entries, size_t key_length, const char* key, size_t value_length, const char* value);

huffman_header
parse_header_stream(
    FILE* stream
)
{
    const size_t newline_pred_size    = strlen(NEWLINE_PREDICATOR);
    const size_t newline_size         = strlen(NEWLINE);
    const size_t sliding_window_size  = strlen(HEADER_END_INDICATOR);
    long header_end_pos               = 0;
    long header_line_count            = 0;
    char sliding_window_buffer[SLIDING_WINDOW_MAX_SIZE] = {0};

    assert(newline_pred_size + newline_size <= sliding_window_size);
    assert(SLIDING_WINDOW_MAX_SIZE > sliding_window_size);

    // Calculate the end of the header
    int sym = EOF;
    while ((sym = fgetc(stream)) != EOF)
    {
        buffer_shift_append_(sliding_window_size, sliding_window_buffer, (char)sym);
        
        if (strnstr(sliding_window_buffer, sliding_window_size,
                    NEWLINE_PREDICATOR NEWLINE, newline_pred_size + newline_size) == sliding_window_buffer)
            header_line_count++;
        
        if (strncmp(HEADER_END_INDICATOR, sliding_window_buffer, sliding_window_size) == 0)
        {
            header_end_pos = (long) (ftell(stream) - sliding_window_size - (newline_size - newline_pred_size));
            rewind(stream);
            break;
        }
    }

    if (header_line_count <= 0)
    {
        // Set the cursor to the line directly after the header-end
        fseek(stream, header_end_pos + 2 * newline_size + sliding_window_size - newline_pred_size, SEEK_SET);
        return header_create_empty();
    }

    // Read the entire header into memory
    char* const raw_text_buffer = (char*) malloc(sizeof(char) * (header_end_pos + 1));
    raw_text_buffer[header_end_pos] = '\0';

    if (fread(raw_text_buffer, sizeof(char), header_end_pos, stream) < (size_t) header_end_pos)
    {
        free(raw_text_buffer);
        return (huffman_header) {0};
    }

    // Set the cursor to the line directly after the header-end
    fseek(stream, header_end_pos + 2 * newline_size + sliding_window_size - newline_pred_size, SEEK_SET);

    // Split the header into seperate lines denoted by local offsets
    const char* raw_text_buffer_pos = raw_text_buffer;
    int* header_line_offsets        = malloc(sizeof(int) * header_line_count);
    header_line_offsets[0]          = 0;

    for (long i = 1; i < header_line_count; i++)
    {
        const char* line_end_addr = strnstr(raw_text_buffer_pos, header_end_pos,
                                            NEWLINE_PREDICATOR NEWLINE, newline_pred_size + newline_size);

        int local_offset       = (int)  ((uintptr_t) line_end_addr - (uintptr_t) raw_text_buffer);
        raw_text_buffer_pos    = (char*) ((uintptr_t) line_end_addr + newline_size + newline_pred_size);
        header_line_offsets[i] = local_offset + newline_size + newline_pred_size;
    }

    // Split the lines into key-value pairs
    vector_t* keyvalue_pairs = vector_create(sizeof(huffman_header_keypair));
    for (long i = 0; i < header_line_count; i++)
    {
        long line_length = 0;
        if (i < header_line_count - 1)
            line_length = (long) (header_line_offsets[i+1] - header_line_offsets[i] - (newline_size + newline_pred_size));
        else
            line_length = (long) (header_end_pos - header_line_offsets[i] - (newline_pred_size + newline_size));

        const char* const local_line_string = raw_text_buffer + header_line_offsets[i];
        void* const split_addr              = memchr(local_line_string, (int)':', line_length);
        const size_t key_length             = (size_t) ((uintptr_t) split_addr - (uintptr_t) local_line_string);
        const char* const value_start       = ((char*) split_addr) + 1;
        const size_t value_length           = line_length - key_length - 1;

        vector_add_keypair_(keyvalue_pairs,
                            key_length,
                            local_line_string,
                            value_length,
                            value_start);
    }

    free(raw_text_buffer);
    free(header_line_offsets);

    return (huffman_header) {
        .header_entries_ = keyvalue_pairs
    };
}

huffman_header
header_create_empty()
{
    return (huffman_header) {
        .header_entries_ = vector_create(sizeof(huffman_header_keypair))
    };
}

void
header_clean(
    huffman_header* const header
)
{
    if (header == NULL || header->header_entries_ == NULL)
        return;

    vector_clean_deep(header->header_entries_, keypair_clean_);
    header->header_entries_ = NULL;
}


size_t
header_get_value(
    const huffman_header* const header,
    const size_t in_key_length,
    const char* const in_key,
    const size_t out_length,
    char* const out_value
)
{
    if (header == NULL || in_key == NULL || out_value == NULL ||
        header->header_entries_ == NULL)
        return 0;

    huffman_header_keypair* entry = find_keypair_(header, in_key_length, in_key);

    if (entry == NULL)
        return 0;

    const size_t shortest_value_length = MIN(entry->value_string_length_, out_length - 1);

    memcpy(out_value, entry->value_string_, shortest_value_length);
    out_value[shortest_value_length] = '\0';
    return shortest_value_length;
}

bool
header_contains(
    const huffman_header* const header,
    const size_t in_key_length,
    const char* const in_key
)
{
    return find_keypair_(header, in_key_length, in_key) != NULL;
}

bool
header_replace_key(
    huffman_header* const header,
    const size_t target_key_length,
    const char* const target_key,
    const size_t new_key_length,
    const char* const new_key
)
{
    if (header == NULL || header->header_entries_ == NULL ||
        target_key == NULL || new_key == NULL)
        return false;

    huffman_header_keypair* entry = find_keypair_(header, target_key_length, target_key);

    if (entry == NULL)
        return false;

    const size_t repurpose_size = repurpose_buffer_(&entry->key_buffer_size_,
                                                    &entry->key_string_,
                                                    new_key,
                                                    new_key_length);

    if (repurpose_size != 0)
        entry->key_string_length_ = new_key_length;

    return repurpose_size != 0;
}

bool
header_replace_value(
    huffman_header* header,
    const size_t target_key_length,
    const char* const target_key,
    const size_t new_value_length,
    const char* const new_value
)
{
    if (header == NULL || header->header_entries_ == NULL ||
        target_key == NULL || new_value == NULL)
        return false;

    huffman_header_keypair* entry = find_keypair_(header, target_key_length, target_key);

    if (entry == NULL)
        return false;

    const size_t repurpose_size = repurpose_buffer_(&entry->value_buffer_size_,
                                                    &entry->value_string_,
                                                    new_value,
                                                    new_value_length);

    if (repurpose_size != 0)
        entry->value_string_length_ = new_value_length;

    return repurpose_size != 0;
}

bool
header_remove_entry(
    huffman_header* const header,
    const size_t target_key_length,
    const char* const target_key
)
{
    if (header == NULL || header->header_entries_ == NULL)
        return false;

    const uintptr_t entry_array_start = (uintptr_t) vector_get_start_addr_(header->header_entries_);

    if (entry_array_start == 0)
        return false;

    const uintptr_t entry_addr = (uintptr_t) find_keypair_(header, target_key_length, target_key);

    if (entry_addr == 0)
        return false;

    const intptr_t local_offset = entry_addr - entry_array_start;
    const size_t   index        = local_offset / sizeof(huffman_header_keypair);

    huffman_header_keypair cleanup_entry = {0};
    bool success = vector_remove_at(header->header_entries_, index, (void*) &cleanup_entry);

    if (!success)
        return false;

    keypair_clean_((void*) &cleanup_entry);
    return true;
}

bool
header_add_entry(
    huffman_header* const header,
    const size_t key_length,
    const char* const key,
    const size_t value_length,
    const char* const value
)
{
    if (header == NULL)
        return false;

    return vector_add_keypair_(header->header_entries_, key_length, key, value_length, value);
}

bool
header_flush_stream(
    const huffman_header* const header,
    FILE* const stream
)
{
    if (header == NULL || header->header_entries_ == NULL)
        return false;

    size_t vector_count = vector_get_elem_count(header->header_entries_);
    huffman_header_keypair* vector_start = (huffman_header_keypair*) vector_get_start_addr_(header->header_entries_);

    for (size_t i = 0; i < vector_count; i++)
    {
        const char* const entry_key   = vector_start[i].key_string_;
        const char* const entry_value = vector_start[i].value_string_;
        const size_t entry_key_size   = vector_start[i].key_string_length_;
        const size_t entry_value_size = vector_start[i].value_string_length_;

        fwrite(entry_key,          sizeof(char), entry_key_size,             stream);
        fwrite(":",                sizeof(char), 1,                          stream);
        fwrite(entry_value,        sizeof(char), entry_value_size,           stream);
        fwrite(NEWLINE_PREDICATOR, sizeof(char), strlen(NEWLINE_PREDICATOR), stream);
        fwrite(NEWLINE,            sizeof(char), strlen(NEWLINE),            stream);
    }

    fwrite(HEADER_END_INDICATOR, sizeof(char), strlen(HEADER_END_INDICATOR), stream);
    fwrite(NEWLINE,              sizeof(char), strlen(NEWLINE),              stream);
    return true;
}


char*
buffer_shift_append_(
    const size_t buffer_size,
    char* const buffer,
    const char symbol
)
{
    for (size_t i = 0; i < buffer_size - 1; i++)
        buffer[i] = buffer[i + 1];

    buffer[buffer_size - 1] = symbol;
    return buffer;
}

void
keypair_clean_(
    void* const pair
)
{
    huffman_header_keypair* keyvalue = (huffman_header_keypair*) pair;

    if (keyvalue->key_string_ != NULL)
        free(keyvalue->key_string_);

    if (keyvalue->value_string_ != NULL)
        free(keyvalue->value_string_);
}

huffman_header_keypair*
vector_find_keypair_(
    const vector_t* header_entries,
    size_t key_length,
    const char* key
)
{
     if (header_entries == NULL)
        return NULL;

    const size_t entry_count = vector_get_elem_count(header_entries);

    for (size_t i = 0; i < entry_count; i++)
    {
        huffman_header_keypair* entry = NULL;
        if (!vector_elem_at(header_entries, i, (void**)&entry))
            continue;

        if (entry->key_string_length_ != key_length)
            continue;

        if (strncmp(entry->key_string_, key, entry->key_string_length_) != 0)
            continue;

        return entry;
    }

    return NULL;
}

huffman_header_keypair*
find_keypair_(
    const huffman_header* const header,
    const size_t key_length,
    const char* const key
)
{
    if (header == NULL)
        return NULL;

    return vector_find_keypair_(header->header_entries_, key_length, key);
}

size_t
repurpose_buffer_(
    size_t* const buffer_size,
    char** const buffer,
    const char* const new_data,
    const size_t new_data_size
)
{
    if ((new_data_size + 1) > *buffer_size)
    {
        char* new_ptr = realloc(*buffer, new_data_size + 1);
        if (new_ptr == NULL)
            return 0;

        *buffer = new_ptr;
        *buffer_size = new_data_size + 1;
    }

    memcpy(*buffer, new_data, new_data_size);
    (*buffer)[new_data_size] = '\0';
    return *buffer_size;
}

bool
vector_add_keypair_(
    vector_t* const header_entries,
    const size_t key_length,
    const char* const key,
    const size_t value_length,
    const char* const value
)
{
    if (header_entries == NULL || key == NULL || value == NULL)
        return false;

    if (vector_find_keypair_(header_entries, key_length, key) != NULL)
        return false;

    char* const key_buffer = malloc(key_length + 1);
    if (key_buffer == NULL)
        return false;

    char* const value_buffer = malloc(value_length + 1);
    if (value_buffer == NULL)
    {
        free(key_buffer);
        return false;
    }

    key_buffer[key_length]     = '\0';
    value_buffer[value_length] = '\0';

    memcpy(key_buffer,   key,   key_length);
    memcpy(value_buffer, value, value_length);

    const huffman_header_keypair entry = {
        .key_buffer_size_     = key_length + 1,
        .value_buffer_size_   = value_length + 1,
        .key_string_length_   = key_length,
        .value_string_length_ = value_length,
        .key_string_          = key_buffer,
        .value_string_        = value_buffer
    };

    vector_append(header_entries, (void*) &entry);

    return true;
}
