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

    memset(temp_buffer, 0, size);

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

    size_t hay_index = 0;
    do {
        int comparison = strncmp(haystack + hay_index, needle, needle_size);
        if (comparison == 0)
            return haystack + hay_index;
    } while (++hay_index < haystack_size - needle_size);

    return NULL;
}
