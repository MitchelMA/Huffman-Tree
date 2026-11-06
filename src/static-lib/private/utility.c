#include "utility.h"

#include <stdint.h>
#include <alloca.h>
#include <string.h>

void*
memrev(
    void* const src,
    size_t n,
    size_t size
)
{
    uintptr_t first = (uintptr_t) src;
    uintptr_t last  = ((uintptr_t) src + ((n - 1) * size));
    void* const temp_buffer = alloca(size);

    if (last - first < size)
        return src;

    while (last > first)
    {
        memcpy(temp_buffer, (void*) first, 1 * size);
        memcpy((void*) first, (void*) last, 1 * size);
        memcpy((void*) last, temp_buffer, 1 * size);

        first += size;
        last -= size;
    }

    return src;
}
