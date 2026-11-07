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
