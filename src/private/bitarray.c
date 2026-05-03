#include "bitarray.h"

#include <stdio.h>
#include <assert.h>

// --- LOCAL DEFINITIONS --- //

#define BITS_IN_BYTE_ (8)
#define BIT_MOST_SIGNIFICANT_ (3)
#define BIT_COUNT_(T) ( sizeof(T) * BITS_IN_BYTE_ )

#define BYTE_FILLING_COUNT_(BIT_AMOUNT) ( (BIT_AMOUNT) >> BIT_MOST_SIGNIFICANT_ )
#define BIT_REST_COUNT_(BIT_AMOUNT) ( (BIT_AMOUNT) & (BITS_IN_BYTE_ - 1) )
#define REQ_BYTE_COUNT_(BIT_AMOUNT) ( BYTE_FILLING_COUNT_(BIT_AMOUNT) + (BIT_REST_COUNT_(BIT_AMOUNT) > 0) )

#define FAIL_ASSERT(MSG) assert(0 && MSG)
#define SWITCH_EXHAUST_DEFAULT FAIL_ASSERT("This case should not be reached: the bitarray is of unkown kind")
#define TODO(MSG) FAIL_ASSERT(MSG)

static void bitarray_clean_impl__mem_(bitarray* bitarray);
static void bitarray_clean_impl__io_(bitarray* bitarray);

static bool bitarray_append_bit_impl__mem_(bitarray* bitarray, bool value);
static bool bitarray_append_bit_impl__io_(bitarray* bitarray, bool value);

static int bitarray_value_at_impl__mem_(const bitarray* bitarray, bitarray_length_t index);

static bitarray_length_t bitarray_length_impl__mem_(const bitarray* bitarray);

static void bitarray_flush_impl__io_(bitarray* bitarray);
static int64_t bitarray_read_file_impl__mem_(bitarray* bitarray, FILE* fp, int64_t bit_count);
static int64_t bitarray_write_file_impl__mem_(bitarray* bitarray, FILE* fp, int64_t bit_count);

static bitarray_bitcontainer_t empty_container_ = 0;

static bitarray_length_t bitarray_write_sizeheader__mem_(const bitarray* bitarray, FILE* fp, int64_t max_bit_count);
static bitarray_length_t bitarray_read_sizeheader__mem_(FILE* fp);

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

bool
bitarray_init_in_mem(
    bitarray* bitarray
)
{
    if (bitarray == NULL)
        return false;

    bitarray->kind = BITARRAY_IN_MEM;
    bitarray->in_mem.content = vector_create(sizeof(bitarray_bitcontainer_t));

    return bitarray->in_mem.content != NULL;
}

bool
bitarray_init_io_buffered(
    bitarray* bitarray,
    FILE* fp
)
{
    if (bitarray == NULL || fp == NULL)
        return false;

    bitarray->kind = BITARRAY_IO_BUFFERED;
    bitarray->buffered.file = fp;

    return true;
}

void
bitarray_clean(
    bitarray* bitarray
)
{
    if (bitarray == NULL)
        return;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            bitarray_clean_impl__mem_(bitarray);
        break;
        case BITARRAY_IO_BUFFERED:
            bitarray_clean_impl__io_(bitarray);
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }
}

bool
bitarray_append_bit(
    bitarray* bitarray,
    bool value
)
{
    if (bitarray == NULL)
        return false;

    bool ret = false;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_append_bit_impl__mem_(bitarray, value);
        break;
        case BITARRAY_IO_BUFFERED:
            ret = bitarray_append_bit_impl__io_(bitarray, value);
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

int
bitarray_value_at(
    const bitarray* bitarray,
    bitarray_length_t index
)
{
    int ret = -1;

    if (bitarray == NULL)
        return ret;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_value_at_impl__mem_(bitarray, index);
        break;
        case BITARRAY_IO_BUFFERED:
            TODO("value_at not implemented for IO_BUFFERED yet");
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

bitarray_length_t
bitarray_length(
    const bitarray* bitarray
)
{
    bitarray_length_t ret = 0;

    if (bitarray == NULL)
        return ret;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_length_impl__mem_(bitarray);
        break;
        case BITARRAY_IO_BUFFERED:
            TODO("length not implemented for IO_BUFFERED yet");
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

void
bitarray_flush(
    bitarray* bitarray
)
{
    if (bitarray == NULL)
        return;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
        break;
        case BITARRAY_IO_BUFFERED:
            bitarray_flush_impl__io_(bitarray);
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }
}

int64_t
bitarray_read_file(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    int64_t ret = -1;

    if (bitarray == NULL || fp == NULL)
        return ret;

    if (bit_count == -1)
        bit_count = INT64_MAX;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_read_file_impl__mem_(bitarray, fp, bit_count);
        break;
        case BITARRAY_IO_BUFFERED:
            TODO("read_file not implemented for IO_BUFFERED yet");
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

int64_t
bitarray_write_file(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    int64_t ret = -1;

    if (bitarray == NULL || fp == NULL)
        return ret;

    if (bit_count == -1)
        bit_count = INT64_MAX;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_write_file_impl__mem_(bitarray, fp, bit_count);
        break;
        case BITARRAY_IO_BUFFERED:
            TODO("write_file not implemented for IO_BUFFERED yet");
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

// --- LOCAL IMPLEMENTATIONS --- //

void
bitarray_clean_impl__mem_(
    bitarray* bitarray
)
{
    bitarray->in_mem.bit_offset = 0;
    vector_clean(bitarray->in_mem.content);
    bitarray->in_mem.content = NULL;
}

void
bitarray_clean_impl__io_(
    bitarray* bitarray
)
{
    bitarray_flush_impl__io_(bitarray);
    bitarray->buffered.io_buffer = 0;
    fclose(bitarray->buffered.file);
    bitarray->buffered.file = NULL;
}

bool
bitarray_append_bit_impl__mem_(
    bitarray* bitarray,
    bool value
)
{
    // Check wether a new container needs to be allocated
    if (bitarray->in_mem.bit_offset == 0)
        vector_append(bitarray->in_mem.content, &empty_container_);

    // Retreive the container where the bit is going to be written into
    bitarray_bitcontainer_t* container = NULL;
    if (!vector_elem_at(
        bitarray->in_mem.content,
        vector_get_elem_count(bitarray->in_mem.content) - 1,
        (void**)&container))
        return false;

    // Write the value offset into the container
    const bitarray_bitcontainer_t writing_offset = (BITS_IN_BYTE_ - 1) - bitarray->in_mem.bit_offset;
    const bitarray_bitcontainer_t to_be_written = value << writing_offset;

    *container |= to_be_written;
    bitarray->in_mem.bit_offset = BIT_REST_COUNT_(bitarray->in_mem.bit_offset + 1);

    return true;
}

bool
bitarray_append_bit_impl__io_(
    bitarray* bitarray,
    const bool value
)
{
    const bitarray_bitcontainer_t writing_offset = (BITS_IN_BYTE_ - 1) - bitarray->buffered.bit_offset;
    const bitarray_bitcontainer_t to_be_written = value << writing_offset;
    const bitarray_bitcontainer_t next_offset_count = bitarray->buffered.bit_offset + 1;

    bitarray->buffered.io_buffer |= to_be_written;

    if (next_offset_count > BIT_REST_COUNT_(next_offset_count))
        bitarray_flush_impl__io_(bitarray);

    bitarray->buffered.bit_offset = BIT_REST_COUNT_(next_offset_count);
    return true;
}

int
bitarray_value_at_impl__mem_(
    const bitarray* bitarray,
    bitarray_length_t index
)
{
    if (index >= bitarray_length_impl__mem_(bitarray))
        return -1;

    // Determine the container index and the bit index
    const bitarray_length_t container_index = BYTE_FILLING_COUNT_(index);
    const bitarray_length_t bit_index = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(index);
    
    // Then read from the vector to retreive the container
    bitarray_bitcontainer_t* container = NULL;
    if (!vector_elem_at(bitarray->in_mem.content, container_index, (void**)&container))
        return -1;

    // Shift the container and read the value at the right-most bit
    return ((*container) >> bit_index) & 1;
}


bitarray_length_t
bitarray_length_impl__mem_(
    const bitarray* bitarray
)
{
    return BIT_COUNT_(bitarray_bitcontainer_t) *
        vector_get_elem_count(bitarray->in_mem.content) -
        (
            (bitarray->in_mem.bit_offset > 0) *
            ((BITS_IN_BYTE_) - bitarray->in_mem.bit_offset)
        );
}

void
bitarray_flush_impl__io_(
    bitarray* bitarray
)
{
    if (bitarray->buffered.bit_offset == 0)
        return;

    fputc(bitarray->buffered.io_buffer, bitarray->buffered.file);
    bitarray->buffered.io_buffer  = 0;
    bitarray->buffered.bit_offset = 0;
}

int64_t
bitarray_read_file_impl__mem_(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    int read_value = EOF;
    bitarray_length_t read_amount = 0;

    bitarray_length_t max_bit_count = bitarray_read_sizeheader__mem_(fp);
    max_bit_count = max_bit_count > (bitarray_length_t)bit_count
        ? bit_count
        : max_bit_count;

    const bitarray_length_t max_byte_count = REQ_BYTE_COUNT_(max_bit_count);

    while (read_amount < max_byte_count && (read_value = getc(fp)) != EOF)
    {
        read_amount++;
        bitarray_bitcontainer_t insert_value = (bitarray_bitcontainer_t)read_value;
        vector_append(bitarray->in_mem.content, (void*)&insert_value);
    }

    bitarray->in_mem.bit_offset = BIT_REST_COUNT_(max_bit_count);

    return read_amount;
}

int64_t
bitarray_write_file_impl__mem_(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    const bitarray_length_t container_bit_count = bitarray_length_impl__mem_(bitarray);

    if (bit_count == -1 || (bitarray_length_t)bit_count > container_bit_count)
        bit_count = (int64_t)container_bit_count;

    const bitarray_length_t byte_count = REQ_BYTE_COUNT_(bit_count);
    const bitarray_bitcontainer_t* buffer = vector_get_start_addr_(bitarray->in_mem.content);

    bitarray_write_sizeheader__mem_(bitarray, fp, bit_count);
    return (int64_t) fwrite(buffer, sizeof(uint8_t), byte_count, fp);
}


bitarray_length_t
bitarray_write_sizeheader__mem_(
    const bitarray* bitarray,
    FILE* fp,
    int64_t max_bit_count
)
{
    const size_t item_write_count = 1;

    if (max_bit_count == -1)
        max_bit_count = INT64_MAX;

    const bitarray_length_t bit_count = bitarray_length_impl__mem_(bitarray) > (bitarray_length_t)max_bit_count ?
        max_bit_count
        : bitarray_length_impl__mem_(bitarray);

    if (fwrite(&bit_count, sizeof(bitarray_length_t), item_write_count, fp) < item_write_count)
        return 0;

    return bit_count;
}

bitarray_length_t
bitarray_read_sizeheader__mem_(
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

