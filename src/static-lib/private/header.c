#include "header.h"

#include <alloca.h>
#include <string.h>
#include <sdtio.h>

#define HEADER_END_INDICATOR "---"
#if defined(WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"

static char* buffer_shift_append_(size_t buffer_size, char* buffer, char symbol);

huffman_header
parse_header_stream(
    FILE* stream
)
{
    const size_t newline_size = sizeof(NEWLINE);
    const size_t sliding_window_size = sizeof(HEADER_END_INDICATOR);
    char* const sliding_window_buffer = alloca(sliding_window_size + 1);
    sliding_window_buffer[sliding_window_size] = '\0';
    long header_end_pos = 0;
    long header_line_count = 0;

    // Calculate the end of the header
    while ((int sym = fgetc(stream)) != EOF)
    {
        buffer_shift_append_(sliding_window_size, sliding_window_buffer, (char)sym);
        
        if (strstr(sliding_window_buffer, "\n") == sliding_window_buffer)
            header_line_count++;
        
        if (strncmp(HEADER_END_INDICATOR, sliding_window_buffer) == 0)
        {
            header_end_pos = ftell(stream) - sliding_window_size - newline_size;
            rewind(stream);
            break;
        }
    }

    char* const raw_text_buffer = malloc(sizeof(char) * header_end_pos + 1);
    raw_text_buffer[header_end_pos] = '\0';

    fread(raw_text_buffer, sizeof(char), header_end_pos, stream);

    // Split the header into seperate lines denoted by local offsets
    const char* raw_text_buffer_pos = raw_text_buffer;
    int* header_line_offsets = malloc(sizeof(int) * header_line_count);
    header_line_offsets[0] = 0;
    for (long i = 1; i < header_line_count; i++)
    {
        void* line_end_addr = memcrh(raw_text_buffer_pos, (int)'\n', HEADER_MAX_LINE_LENGTH);
        int local_offset = (int)  ((uintptr_t) line_end_addr - (uintptr_t) raw_text_buffer);
        raw_text_buffer_pos = line_end_addr + 1;
        header_line_offsets[i] = local_offset + 1;
    }

    // Split the lines into key-value pairs

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

memchr(3)                                                                                     Library Functions Manual                                                                                     memchr(3)

NAME
     memchr, memrchr - scan memory for a character

LIBRARY
     Standard C library (libc, -lc)

SYNOPSIS
     #include <string.h>

     void *memchr(size_t n;
                  const void s[n], int c, size_t n);
     void *memrchr(size_t n;
                  const void s[n], int c, size_t n);

 Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

     memrchr():
         _GNU_SOURCE

DESCRIPTION
     The memchr() function scans the initial n bytes of the memory area pointed to by s for the first instance of c.  Both c and the bytes of the memory area pointed to by s are interpreted as unsigned char.

     The memrchr() function is like the memchr() function, except that it searches backward from the end of the n bytes pointed to by s instead of forward from the beginning.

RETURN VALUE
     The memchr() and memrchr() functions return a pointer to the matching byte or NULL if the character does not occur in the given memory area.

ATTRIBUTES
     For an explanation of the terms used in this section, see attributes(7).
     ┌───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┬───────────────┬─────────┐
     │ Interface                                                                                                                                                                         │ Attribute     │ Value   │
     ├───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────┼─────────┤
     │ memchr(), memrchr()                                                                                                                                                               │ Thread safety │ MT-Safe │
     └───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┴───────────────┴─────────┘

STANDARDS
     memchr()
            C11, POSIX.1-2008.

     memrchr()
            GNU.

HISTORY
     memchr()
            POSIX.1-2001, C89, SVr4, 4.3BSD.

     memrchr()
            glibc 2.2.

SEE ALSO
     bstring(3), ffs(3), memmem(3), strchr(3), strpbrk(3), strrchr(3), strsep(3), strspn(3), strstr(3), wmemchr(3)

Linux man-pages 6.18                                                                                 2026-02-25                                                                                            memchr(3)
