#include "bitarray.h"

#include <stdio.h>

// --- LOCAL DEFINITIONS --- //

#define BITS_IN_BYTE_ (8)
#define BIT_COUNT_(T) ( sizeof(T) * BITS_IN_BYTE_ )
#define REQ_BYTE_COUNT_(B) ( B / BIT_COUNT_(uint8_t) + (( B % BIT_COUNT_(uint8_t)) > 0) )

static bitarray_bitcontainer_t empty_container = 0;

static bitarray_length_t bitarray_write_sizeheader_(const bitarray* bitarray, FILE* fp, int64_t max_bit_count);
static bitarray_length_t bitarray_read_sizeheader_(FILE* fp);

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

void
bitarray_clean(
    bitarray* bitarray
)
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

int64_t
bitarray_read_file(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    if (bitarray == NULL || fp == NULL)
        return -1;

    if (bit_count == -1)
        bit_count = INT64_MAX;

    int read_value = EOF;
    bitarray_length_t read_amount = 0;

    bitarray_length_t max_bit_count = bitarray_read_sizeheader_(fp);
    max_bit_count = max_bit_count > (bitarray_length_t)bit_count
        ? bit_count
        : max_bit_count;

    const bitarray_length_t max_byte_count = REQ_BYTE_COUNT_(max_bit_count);

    while (read_amount < max_byte_count && (read_value = getc(fp)) != EOF)
    {
        read_amount++;
        bitarray_bitcontainer_t insert_value = (bitarray_bitcontainer_t)read_value;
        vector_append(bitarray->content, (void*)&insert_value);
        bitarray->length += BIT_COUNT_(bitarray_bitcontainer_t);
    }

    if (bitarray->length > max_bit_count)
        bitarray->length = max_bit_count;

    return read_amount;
}

int64_t
bitarray_write_file(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    if (bitarray == NULL || fp == NULL)
        return -1;

    if (bit_count == -1)
        bit_count = INT64_MAX;

    const bitarray_length_t container_bit_count = bitarray->length;

    if (bit_count == -1 || (bitarray_length_t)bit_count > container_bit_count)
        bit_count = (int64_t)container_bit_count;

    const bitarray_length_t byte_count = REQ_BYTE_COUNT_(bit_count);
    const bitarray_bitcontainer_t* buffer = vector_get_start_addr_(bitarray->content);

    bitarray_write_sizeheader_(bitarray, fp, bit_count);
    return (int64_t) fwrite(buffer, sizeof(uint8_t), byte_count, fp);
}

// --- LOCAL IMPLEMENTATIONS --- //

bitarray_length_t
bitarray_write_sizeheader_(
    const bitarray* bitarray,
    FILE* fp,
    int64_t max_bit_count
)
{
    const size_t item_write_count = 1;

    if (max_bit_count == -1)
        max_bit_count = INT64_MAX;

    const bitarray_length_t bit_count = bitarray->length > (bitarray_length_t)max_bit_count ?
        max_bit_count
        : bitarray->length;

    if (fwrite(&bit_count, sizeof(bitarray_length_t), item_write_count, fp) < item_write_count)
        return 0;

    return bit_count;
}

bitarray_length_t
bitarray_read_sizeheader_(
    FILE* fp
)
{
    const size_t read_count = 1;

    bitarray_length_t bit_count = 0;
    if (fread(&bit_count, sizeof(bitarray_length_t), read_count, fp) < read_count)
        return 0;

    return bit_count;
}

// --- END LOCAL IMPLEMENTATIONS --- //

