#include "header.h"

#include "utility.h"

#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <stdint.h>

#define HEADER_END_INDICATOR "---"

#if defined(WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif 

static char* buffer_shift_append_(size_t buffer_size, char* buffer, char symbol);

huffman_header
parse_header_stream(
    FILE* stream
)
{
    const size_t newline_size         = strlen(NEWLINE);
    const size_t sliding_window_size  = strlen(HEADER_END_INDICATOR);
    char* const sliding_window_buffer = alloca(sliding_window_size + 1);
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
            header_end_pos = ftell(stream) - sliding_window_size - newline_size;
            rewind(stream);
            break;
        }
    }

    char* const raw_text_buffer = (char*) malloc(sizeof(char) * header_end_pos + 1);
    raw_text_buffer[header_end_pos] = '\0';
    
    if (fread(raw_text_buffer, sizeof(char), header_end_pos, stream) < (size_t) header_end_pos)
    {
        free(raw_text_buffer);
        return (huffman_header) {0};
    }

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
    huffman_header_keypair* keyvalue_pairs = malloc(sizeof(huffman_header_keypair) * header_line_count);
    for (long i = 0; i < header_line_count; i++)
    {
        long line_length = 0;
        if (i < header_line_count - 1)
            line_length = header_line_offsets[i+1] - header_line_offsets[i] - newline_size;
        else
            line_length = strlen(raw_text_buffer + header_line_offsets[i]);

        const char* const local_line_string = raw_text_buffer + header_line_offsets[i];
        void* split_addr                    = memchr(local_line_string, (int)':', line_length);

        const size_t key_length       = (size_t) ((uintptr_t) split_addr - (uintptr_t) local_line_string);
        const char* const value_start = ((char*) split_addr) + 1;
        const size_t value_length     = line_length - key_length - 1;

        const huffman_header_keypair keypair = {
            .key_string_length_   = key_length,
            .value_string_length_ = value_length,
            .key_string_start_    = local_line_string,
            .value_string_start_  = value_start
        };

        memcpy(&keyvalue_pairs[i], &keypair, sizeof(huffman_header_keypair));
    }

    free(header_line_offsets);

    return (huffman_header) {
        .header_raw_text_length_ = header_end_pos,
        .header_entry_count_     = header_line_count,
        .header_raw_text_        = raw_text_buffer,
        .header_entries_         = keyvalue_pairs
    };
}

void
header_clean(
    huffman_header* const header
)
{
    if (header == NULL)
        return;

    if (header->header_raw_text_ != NULL)
    {
        free(header->header_raw_text_);
        header->header_raw_text_ = NULL;
    }

    if (header->header_entries_ != NULL)
    {
        free(header->header_entries_);
        header->header_entries_ = NULL;
    }
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

    for (size_t i = 0; i < header->header_entry_count_; i++)
    {
        const huffman_header_keypair* entry = &header->header_entries_[i];
        const size_t shortest_key_length = MIN(entry->key_string_length_, strlen(in_key));
        
        if (strncmp(entry->key_string_start_, in_key, shortest_key_length) != 0)
            continue;

        const size_t shortest_value_length = MIN(entry->value_string_length_, out_length - 1);
        strncpy(out_value, entry->value_string_start_, shortest_value_length);
        out_value[shortest_value_length] = '\0';
        return shortest_value_length;
    }

    return 0;
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
