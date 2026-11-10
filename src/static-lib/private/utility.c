#include "utility.h"

#include <stdint.h>
#include <string.h>

static char* temp_buffer[2048] = {0};

void*
memrev(
    void* const src,
    size_t n,
    size_t size
)
{
    uintptr_t first = (uintptr_t) src;
    uintptr_t last  = ((uintptr_t) src + ((n - 1) * size));

    if (last - first < size)
        return src;

    while (last > first)
    {
        memcpy(temp_buffer, (void*) first, size);
        memcpy((void*) first, (void*) last, size);
        memcpy((void*) last, temp_buffer, size);

        first += size;
        last -= size;
    }

    return src;
}

const char*
strnstr(
    const char* const haystack,
    const size_t haystack_size,
    const char* const needle,
    const size_t needle_size
)
{
    if (needle_size > haystack_size)
        return NULL;

    for (size_t i = 0; i < haystack_size - needle_size; i++)
    {
        int comparison = strncmp(haystack + i, needle, needle_size);
        if (comparison == 0)
            return haystack + i;
    }

    return NULL;
}
