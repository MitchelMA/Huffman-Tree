#include "header.h"

#include "utility.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HEADER_END_INDICATOR "---"

#if defined(WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif 

static char* buffer_shift_append_(size_t buffer_size, char* buffer, char symbol);
static void  keypair_clean_(void* pair);

huffman_header
parse_header_stream(
    FILE* stream
)
{
    const size_t newline_size         = strlen(NEWLINE);
    const size_t sliding_window_size  = strlen(HEADER_END_INDICATOR);
    char* const sliding_window_buffer = malloc(sliding_window_size + 1);
    long header_end_pos               = 0;
    long header_line_count            = 0;

    memset(sliding_window_buffer, 0, sliding_window_size + 1);

    // Calculate the end of the header
    int sym = EOF;
    while ((sym = fgetc(stream)) != EOF)
    {
        buffer_shift_append_(sliding_window_size, sliding_window_buffer, (char)sym);
        
        if (strstr(sliding_window_buffer, "\n") == sliding_window_buffer)
            header_line_count++;
        
        if (strncmp(HEADER_END_INDICATOR, sliding_window_buffer, sliding_window_size) == 0)
        {
            header_end_pos = (long) (ftell(stream) - sliding_window_size - newline_size);
            rewind(stream);
            break;
        }
    }

    free(sliding_window_buffer);

    // Read the entire header into memory
    char* const raw_text_buffer = (char*) malloc(sizeof(char) * header_end_pos + 1);
    raw_text_buffer[header_end_pos] = '\0';
    
    if (fread(raw_text_buffer, sizeof(char), header_end_pos, stream) < (size_t) header_end_pos)
    {
        free(raw_text_buffer);
        return (huffman_header) {0};
    }

    // Set the cursor to the line directly after the header-end
    fseek(stream, header_end_pos + 2 * newline_size + sliding_window_size, SEEK_SET);

    // Split the header into seperate lines denoted by local offsets
    const char* raw_text_buffer_pos = raw_text_buffer;
    int* header_line_offsets        = malloc(sizeof(int) * header_line_count);
    header_line_offsets[0]          = 0;

    for (long i = 1; i < header_line_count; i++)
    {
        void* line_end_addr    = memchr(raw_text_buffer_pos, (int)'\n', HEADER_MAX_LINE_LENGTH);
        int local_offset       = (int)  ((uintptr_t) line_end_addr - (uintptr_t) raw_text_buffer);
        raw_text_buffer_pos    = (void*) ((uintptr_t) line_end_addr + 1);
        header_line_offsets[i] = local_offset + 1;
    }

    // Split the lines into key-value pairs
    vector_t* keyvalue_pairs = vector_create(sizeof(huffman_header_keypair));
    for (long i = 0; i < header_line_count; i++)
    {
        long line_length = 0;
        if (i < header_line_count - 1)
            line_length = (long) (header_line_offsets[i+1] - header_line_offsets[i] - newline_size);
        else
            line_length = (long) strlen(raw_text_buffer + header_line_offsets[i]);

        const char* const local_line_string = raw_text_buffer + header_line_offsets[i];
        void* split_addr                    = memchr(local_line_string, (int)':', line_length);

        const size_t key_length       = (size_t) ((uintptr_t) split_addr - (uintptr_t) local_line_string);
        const char* const value_start = ((char*) split_addr) + 1;
        const size_t value_length     = line_length - key_length - 1;

        char* key_string   = malloc(key_length + 1);
        char* value_string = malloc(value_length + 1);

        key_string[key_length]     = '\0';
        value_string[value_length] = '\0';

        strncpy(key_string, local_line_string, key_length);
        strncpy(value_string, value_start, value_length);

        const huffman_header_keypair keypair = {
            .key_string_ = key_string,
            .value_string_ = value_string
        };

        vector_append(keyvalue_pairs, (void*) &keypair);
    }

    free(raw_text_buffer);
    free(header_line_offsets);

    return (huffman_header) {
        .header_entries_ = keyvalue_pairs
    };
}

void
header_clean(
    huffman_header* const header
)
{
    if (header == NULL && header->header_entries_ != NULL)
        return;

    vector_clean_deep(header->header_entries_, keypair_clean_);
    header->header_entries_ = NULL;
}


size_t
header_get_value(
    const huffman_header* const header,
    const char* const in_key,
    const size_t out_length,
    char* const out_value
)
{
    if (header == NULL || in_key == NULL || out_value == NULL ||
        header->header_entries_ == NULL)
        return 0;

    const size_t entry_count = vector_get_elem_count(header->header_entries_);

    for (size_t i = 0; i < entry_count; i++)
    {
        huffman_header_keypair* entry = NULL;
        if (!vector_elem_at(header->header_entries_, i, (void**)&entry))
            continue;

        const size_t key_string_length = strlen(entry->key_string_);

        if (key_string_length != strlen(in_key))
            continue;
        
        if (strncmp(entry->key_string_, in_key, key_string_length) != 0)
            continue;

        const size_t shortest_value_length = MIN(strlen(entry->value_string_), out_length - 1);

        strncpy(out_value, entry->value_string_, shortest_value_length);
        out_value[shortest_value_length] = '\0';
        return shortest_value_length;
    }

    return 0;
}

bool
header_contains(
    const huffman_header* const header,
    const char* const in_key
)
{
    if (header == NULL || in_key == NULL || header->header_entries_ == NULL)
        return false;

    const size_t entry_count = vector_get_elem_count(header->header_entries_);

    for (size_t i = 0; i < entry_count; i++)
    {
        huffman_header_keypair* entry = NULL;
        if (!vector_elem_at(header->header_entries_, i, (void**)&entry))
            continue;

        const size_t key_string_length = strlen(entry->key_string_);

        if (key_string_length  != strlen(in_key))
            continue;

        if (strncmp(entry->key_string_, in_key, key_string_length ) != 0)
            continue;

        return true;
    }

    return false;
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
