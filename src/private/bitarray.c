#include "bitarray.h"

#include <stdio.h>

// --- LOCAL DEFINITIONS --- //

#define BITS_IN_BYTE_ (8)
#define BIT_COUNT_(TYPE) ( sizeof(TYPE) * BITS_IN_BYTE_ )

static bitarray_bitcontainer_t empty_container = 0;

// --- END LOCAL DEFINITIONS --- //

bool
bitarray_init(
    bitarray* bitarray
)
{
    if (bitarray == NULL)
        return false;

    bitarray->length = 0;
    bitarray->content = vector_create(sizeof(bitarray_bitcontainer_t));

    return bitarray->content != NULL;
}

void bitarray_clean(bitarray* bitarray)
{
    if (bitarray == NULL)
        return;

    bitarray->length = 0;
    vector_clean(bitarray->content);
    bitarray->content = NULL;
}

bool
bitarray_append_bit(
    bitarray* bitarray,
    bool value
)
{
    const bitarray_length_t written = bitarray->length;
    const bitarray_length_t incremental = written + 1;
    const bitarray_length_t containers_required =
        (incremental) / BIT_COUNT_(bitarray_bitcontainer_t) + 
        (((incremental) % BIT_COUNT_(bitarray_bitcontainer_t)) > 0);
    const bitarray_length_t containers_current = vector_get_elem_count(bitarray->content);

    if (containers_required > containers_current)
        vector_append(bitarray->content, &empty_container);

    bitarray_bitcontainer_t to_be_written = value;

    const bitarray_length_t container_idx =
        written / BIT_COUNT_(bitarray_bitcontainer_t);

    const bitarray_bitcontainer_t container_writing_offset =
        (BIT_COUNT_(bitarray_bitcontainer_t) - (written % BIT_COUNT_(bitarray_bitcontainer_t))) - 1;

    to_be_written <<= container_writing_offset;

    bitarray_bitcontainer_t* indexed_container = NULL;
    if (!vector_elem_at(bitarray->content, container_idx, (void**)&indexed_container))
        return false;

    *indexed_container |= to_be_written;
    bitarray->length = incremental;
    return true;
}

int
bitarray_value_at(
    const bitarray* bitarray,
    bitarray_length_t index
)
{
    if (index >= bitarray->length)
        return -1;

    // Determine the container index and
    // the bit index
    const bitarray_length_t container_index = index / BIT_COUNT_(bitarray_bitcontainer_t);
    const bitarray_length_t bit_index =
        BIT_COUNT_(bitarray_bitcontainer_t) -
        (index % BIT_COUNT_(bitarray_bitcontainer_t)) - 1;
    
    // Then read from the vector to retreive the container
    bitarray_bitcontainer_t* container = NULL;
    if (!vector_elem_at(bitarray->content, container_index, (void**)&container))
        return -1;

    // Shift the container and read the value at the right-most bit
    return ((*container) >> bit_index) & 1;
}
