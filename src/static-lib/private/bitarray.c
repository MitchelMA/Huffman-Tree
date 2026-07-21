#include "bitarray.h"

#include "utility.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// --- LOCAL DEFINITIONS --- //

#define BITS_IN_BYTE_ (8)
#define BIT_MOST_SIGNIFICANT_ (3)
#define BIT_COUNT_(T) ( sizeof(T) * BITS_IN_BYTE_ )

#define BYTE_FILLING_COUNT_(BIT_AMOUNT) ( (BIT_AMOUNT) >> BIT_MOST_SIGNIFICANT_ )
#define BIT_REST_COUNT_(BIT_AMOUNT) ( (BIT_AMOUNT) & (BITS_IN_BYTE_ - 1) )
#define REQ_BYTE_COUNT_(BIT_AMOUNT) ( BYTE_FILLING_COUNT_(BIT_AMOUNT) + (BIT_REST_COUNT_(BIT_AMOUNT) > 0) )

#define MASK_FROM_RIGHT_(TYPE, BITS_ON) ((((TYPE)1) << (BITS_ON)) - 1)
#define MASK_FROM_LEFT_(TYPE, BITS_ON) (~MASK_FROM_RIGHT_(TYPE, BIT_COUNT_(TYPE) - (BITS_ON)))

#define FAIL_ASSERT(MSG) assert(0 && MSG)
#define SWITCH_EXHAUST_DEFAULT FAIL_ASSERT("This case should not be reached: the bitarray is of unkown kind")
#define TODO(MSG) FAIL_ASSERT(MSG)

static void bitarray_clean_impl__mem_(bitarray* bitarray);
static void bitarray_clean_impl__io_(bitarray* bitarray);

static bool bitarray_append_bit_impl__mem_(bitarray* bitarray, bool value);
static bool bitarray_append_bit_impl__io_(bitarray* bitarray, bool value);

static int64_t bitarray_append_bits_impl__mem_(bitarray* bitarray, bitarray_length_t count, const bool* buffer);
static int64_t bitarray_append_bits_impl__io_(bitarray* bitarray, bitarray_length_t count, const bool* buffer);

static int bitarray_value_at_impl__mem_(const bitarray* bitarray, bitarray_length_t index);
static int bitarray_value_at_impl__io_(const bitarray* bitarray, bitarray_length_t index);

static int64_t bitarray_read_bits_impl__mem_(const bitarray* bitarray, bitarray_length_t length, bitarray_length_t index, bitarray_length_t count, bool* out);
static int64_t bitarray_read_bits_impl__io_(const bitarray* bitarray, bitarray_length_t length, bitarray_length_t index, bitarray_length_t count, bool* out);

static bitarray_length_t bitarray_length_impl__mem_(const bitarray* bitarray);
static bitarray_length_t bitarray_length_impl__io_(const bitarray* bitarray);

static void bitarray_flush_impl__io_(bitarray* bitarray);
static int64_t bitarray_read_file_impl__mem_(bitarray* bitarray, FILE* fp, int64_t bit_count);
static int64_t bitarray_write_file_impl__mem_(bitarray* bitarray, FILE* fp, int64_t bit_count);
static int64_t bitarray_read_file_impl__io_(bitarray* bitarray, FILE* fp, int64_t bit_count);
static int64_t bitarray_write_file_impl__io_(bitarray* bitarray, FILE* fp, int64_t bit_count);
static uint8_t bitarray_restore_bit_offset_(FILE* fp, uint8_t preferred, uint8_t abide);

static uint8_t bitarray_write_sizeheader_(const bitarray* bitarray, FILE* fp, int64_t max_bit_count);
static uint8_t bitarray_read_sizeheader_(FILE* fp);

static bitarray_bitcontainer_t empty_container_ = 0;

// --- END LOCAL DEFINITIONS --- //

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

int64_t
bitarray_append_bits(
    bitarray* bitarray,
    const bitarray_length_t count,
    const bool* buffer
)
{
    int64_t ret = -1;

    if (bitarray == NULL || buffer == NULL)
        return ret;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_append_bits_impl__mem_(bitarray, count, buffer);
        break;
        case BITARRAY_IO_BUFFERED:
            ret = bitarray_append_bits_impl__io_(bitarray, count, buffer);
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
            ret = bitarray_value_at_impl__io_(bitarray, index);
        break;
        default:
            SWITCH_EXHAUST_DEFAULT;
        break;
    }

    return ret;
}

int64_t
bitarray_read_bits(
    const bitarray* bitarray,
    const bitarray_length_t index,
    const bitarray_length_t count,
    bool* out
)
{
    int64_t ret = -1;

    if (bitarray == NULL || out == NULL)
        return ret;

    // First test if index + count is within the bounds of the array
    const bitarray_length_t bitarr_length = bitarray_length(bitarray);
    const int64_t           offset_range  = (int64_t) bitarr_length - (int64_t) index;

    if (count > (bitarray_length_t) offset_range)
        return ret;

    switch (bitarray->kind)
    {
        case BITARRAY_IN_MEM:
            ret = bitarray_read_bits_impl__mem_(bitarray, bitarr_length, index, count, out);
        break;
        case BITARRAY_IO_BUFFERED:
            ret = bitarray_read_bits_impl__io_(bitarray, bitarr_length, index, count, out);
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
            ret = bitarray_length_impl__io_(bitarray);
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
            ret = bitarray_read_file_impl__io_(bitarray, fp, bit_count);
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
            ret = bitarray_write_file_impl__io_(bitarray, fp, bit_count);
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
    if (bitarray->in_mem.content == NULL)
        return;

    bitarray->in_mem.bit_offset = 0;
    vector_clean(bitarray->in_mem.content);
    bitarray->in_mem.content = NULL;
}

void
bitarray_clean_impl__io_(
    bitarray* bitarray
)
{
    if (bitarray->buffered.file == NULL)
        return;

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

int64_t
bitarray_append_bits_impl__mem_(
    bitarray* bitarray,
    const bitarray_length_t count,
    const bool* buffer
)
{
    int64_t written = 0;

    while ((bitarray_length_t) written < count)
    {
        // Check wether a new container needs to be allocated
        if (bitarray->in_mem.bit_offset == 0)
            vector_append(bitarray->in_mem.content, &empty_container_);

        // Get the last container
        bitarray_bitcontainer_t* container = NULL;
        if (!vector_elem_at(
            bitarray->in_mem.content,
            vector_get_elem_count(bitarray->in_mem.content) - 1,
            (void**)&container))
            break;

        // Write to the last container with the correct offsets
        while ((bitarray_length_t) written < count)
        {
            const bitarray_bitcontainer_t container_write_offset = (BITS_IN_BYTE_ - 1) - bitarray->in_mem.bit_offset;
            const bitarray_bitcontainer_t to_be_written = buffer[written] << container_write_offset;

            // Write the value into the container and update the bit-offset
            *container |= to_be_written;
            bitarray->in_mem.bit_offset = BIT_REST_COUNT_(bitarray->in_mem.bit_offset + 1);
            ++written;

            // When the bit-offset is 0, break out of the inner loop
            if (bitarray->in_mem.bit_offset == 0)
                break;
        }
    }

    return written;
}

int64_t
bitarray_append_bits_impl__io_(
    bitarray* bitarray,
    const bitarray_length_t count,
    const bool* buffer
)
{
    int64_t written = 0;

    while ((bitarray_length_t) written < count)
    {
        if (bitarray_append_bit_impl__io_(bitarray, buffer[written]))
            written++;
        else
            break;
    }

    return written;
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

int
bitarray_value_at_impl__io_(
    const bitarray* bitarray,
    const bitarray_length_t index
)
{
    // Calculate the stream size
    const long size = bitarray_length_impl__io_(bitarray);

    if (size < 0 || index >= (bitarray_length_t)size)
        return -1;

    const long stream_size = size - bitarray->buffered.bit_offset;
    const long idx_in_buffer = index - stream_size;

    // Read from the buffered (not-yet-flushed) value
    if (idx_in_buffer >= 0)
    {
        const bitarray_length_t bit_index = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(idx_in_buffer);
        return (bitarray->buffered.io_buffer >> bit_index) & 1;
    }

    // Read from the stream
    const bitarray_length_t container_index = BYTE_FILLING_COUNT_(index);
    const bitarray_length_t bit_index = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(index);

    int container = 0;

    // Move to the correct position within the stream
    const long restore = stream_towards(bitarray->buffered.file, container_index, SEEK_SET);

    if (restore < 0)
        return -1;

    container = fgetc(bitarray->buffered.file);

    // Restore the previous position
    if (stream_towards(bitarray->buffered.file, restore, SEEK_SET) < 0)
        return -1;

    if (container == EOF)
        return -1;

    // Extract only the lower 8 bits
    const uint8_t lower_mask = ~0;
    container = container & lower_mask;

    return (container >> bit_index) & 1;
}

int64_t
bitarray_read_bits_impl__mem_(
    const bitarray* bitarray,
    const bitarray_length_t length,
    const bitarray_length_t index,
    const bitarray_length_t count,
    bool* out
)
{
    UNUSED(length);

    // Prepare the proper reading offsets and acquire a pointer to the raw bitarray buffer
    const int64_t  container_index     = BYTE_FILLING_COUNT_(index);
    const uint8_t  starting_bit_offset = BIT_REST_COUNT_(index);
    const uint8_t* mem_buffer          = (uint8_t*) vector_get_start_addr_(bitarray->in_mem.content);

    // Calculate local byte/bit offsets and write to the output buffer
    for (bitarray_length_t i = 0; i < count; ++i)
    {
        const uint8_t local_bit_offset       = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(starting_bit_offset + i);
        const int64_t local_container_offset = container_index + BYTE_FILLING_COUNT_(starting_bit_offset + i);
        const uint8_t container              = mem_buffer[local_container_offset];
        const bool    bit_value              = (container >> local_bit_offset) & 1;

        out[i] = (bool) bit_value;
    }

    return count;
}

int64_t
bitarray_read_bits_impl__io_(
    const bitarray* bitarray,
    const bitarray_length_t length,
    const bitarray_length_t index,
    const bitarray_length_t count,
    bool* out
)
{
    // Prepare the proper reading offsets and acquire a pointer to the raw bitarray buffer
    const bitarray_length_t stream_bit_count = length - bitarray->buffered.bit_offset;
    const int64_t container_index            = BYTE_FILLING_COUNT_(index);
    const uint8_t starting_bit_offset        = BIT_REST_COUNT_(index);

    int64_t already_read = 0;

    // When the index falls in the bits in the stream, read from the stream
    if (index < (bitarray_length_t) stream_bit_count) {
        // Move the cursor in the stream
        const long initial_pos = stream_towards(bitarray->buffered.file, container_index, SEEK_SET);

        const bitarray_length_t stream_remaining_bit_count = stream_bit_count - index;
        const int64_t stream_remaining_byte_count          = REQ_BYTE_COUNT_(stream_remaining_bit_count);
        const int64_t stream_required_read_byte_count      = MIN(REQ_BYTE_COUNT_((int64_t)count), stream_remaining_byte_count);

        // Allocate a temp buffer and transfer bytes from stream to the in-mem buffer
        uint8_t* mem_buff = malloc(sizeof(uint8_t) * stream_required_read_byte_count);
        if (mem_buff == NULL)
            return -1;

        const size_t file_read_count = fread(
            mem_buff,
            sizeof(uint8_t),
            stream_required_read_byte_count,
            bitarray->buffered.file
        );

        if (file_read_count < (size_t) stream_required_read_byte_count)
        {
            free(mem_buff);
            return -1;
        }

        // Restore the stream cursor
        stream_towards(bitarray->buffered.file, initial_pos, SEEK_SET);

        // Write from the in-mem buffer to the output buffer
        for (;
            (bitarray_length_t) already_read < count &&
            BYTE_FILLING_COUNT_(already_read) < (int64_t)file_read_count;
            already_read++)
        {
            const uint8_t local_bit_offset       = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(starting_bit_offset + already_read);
            const int64_t local_container_offset = BYTE_FILLING_COUNT_(already_read);
            const uint8_t container              = mem_buff[local_container_offset];
            const bool    bit_value              = (container >> local_bit_offset) & 1;

            out[already_read] = bit_value;
        }

        // Free the in-mem buffer
        free(mem_buff);
    }

    // When available, read from the bitarray buffer to the output buffer
    for (; (bitarray_length_t) already_read < count; already_read++)
    {
        const uint8_t local_bit_offset = (BITS_IN_BYTE_ - 1) - BIT_REST_COUNT_(starting_bit_offset + already_read);
        const bool    bit_value        = (bitarray->buffered.io_buffer >> local_bit_offset) & 1;

        out[already_read] = bit_value;
    }

    return already_read;
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

bitarray_length_t
bitarray_length_impl__io_(
    const bitarray* bitarray
)
{
    const long stream_size = stream_tell_size(bitarray->buffered.file);

    if (stream_size < 0)
        return 0;

    return ((bitarray_length_t)stream_size) * BITS_IN_BYTE_ + bitarray->buffered.bit_offset;
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
    const uint8_t bit_offset_count = bitarray_read_sizeheader_(fp);
    const bitarray_length_t max_byte_read = REQ_BYTE_COUNT_(bit_count);

    // Read the values into the bit-array
    while (read_amount < max_byte_read && (read_value = getc(fp)) != EOF)
    {
        read_amount++;
        bitarray_bitcontainer_t insert_value = (bitarray_bitcontainer_t)read_value;
        vector_append(bitarray->in_mem.content, (void*)&insert_value);
    }

    // Set the correct bit_offset to keep track of bit-count
    if (read_amount >= max_byte_read)
    {
        const uint8_t preferred_offset = BIT_REST_COUNT_(bit_count);

        // Respect the given bit_offset from the file
        bitarray->in_mem.bit_offset =
            bitarray_restore_bit_offset_(fp, preferred_offset, bit_offset_count);
    }
    else
    {
        bitarray->in_mem.bit_offset = bit_offset_count;
    }

    // Return the amount of bytes read from file-stream
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

    bitarray_write_sizeheader_(bitarray, fp, bit_count);
    return (int64_t) fwrite(buffer, sizeof(uint8_t), byte_count, fp);
}

int64_t
bitarray_read_file_impl__io_(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    int read_value = EOF;
    int64_t read_amount = 0;

    // Calculate the file written bit_offset and the maximum amount of
    // bits that can be read
    const uint8_t bit_offset_count = bitarray_read_sizeheader_(fp);
    const long file_byte_length = stream_tell_remaining_size(fp);
    const int64_t max_bit_reads = ((int64_t)file_byte_length << BIT_MOST_SIGNIFICANT_)
        - (bit_offset_count > 0) * (BITS_IN_BYTE_ - bit_offset_count);

    bit_count = MIN(bit_count, max_bit_reads);
    int64_t remaining_bit_count = bit_count;

    // Read the values into the bit-array
    while (remaining_bit_count >= BITS_IN_BYTE_ && (read_value = getc(fp)) != EOF)
    {
        read_amount++;
        remaining_bit_count -= BITS_IN_BYTE_;
        fputc(read_value, bitarray->buffered.file);
    }

    // Read the remaining bits that total at less than 8 bits into the io_buffer
    if (remaining_bit_count > 0)
    {
        const uint8_t preferred_offset = BIT_REST_COUNT_(remaining_bit_count);

        // Respect the given bit_offset from the file
        bitarray->buffered.bit_offset =
            bitarray_restore_bit_offset_(fp, preferred_offset, bit_offset_count);

        bitarray->buffered.io_buffer = getc(fp);
    }

    return read_amount;
}

int64_t
bitarray_write_file_impl__io_(
    bitarray* bitarray,
    FILE* fp,
    int64_t bit_count
)
{
    // Calculate the proper bit_length and required bytes to write from the bit-array stream
    const bitarray_length_t container_bit_count = bitarray_length_impl__io_(bitarray);
    bit_count = MIN((bitarray_length_t)bit_count, container_bit_count);

    const bitarray_length_t requested_stream_bit_count = bit_count - bitarray->buffered.bit_offset;
    const bitarray_length_t required_byte_count        = REQ_BYTE_COUNT_(MAX(requested_stream_bit_count, 0));

    // Write the header
    bitarray_write_sizeheader_(bitarray, fp, bit_count);

    int read_val = EOF;
    int64_t byte_write_count = 0;

    const long restore = stream_begin(bitarray->buffered.file);

    // Write from the bitarray-stream into the file stream
    while ((bitarray_length_t) byte_write_count < required_byte_count && (read_val = fgetc(bitarray->buffered.file)) != EOF)
    {
        fputc(read_val, fp);
        byte_write_count++;
    }

    // Write the remaining bits that didn't form a full byte
    const uint8_t remaining_bit_count = BIT_REST_COUNT_(bit_count);
    if (remaining_bit_count > 0)
    {
        int to_write = fgetc(bitarray->buffered.file);

        if (to_write == EOF)
            to_write = (int) bitarray->buffered.io_buffer;

        const uint8_t mask = MASK_FROM_LEFT_(uint8_t, remaining_bit_count);
        byte_write_count += fputc(to_write & mask, fp) != EOF;
    }

    stream_towards(bitarray->buffered.file, restore, SEEK_SET);

    return byte_write_count;
}

uint8_t
bitarray_restore_bit_offset_(
    FILE* fp,
    uint8_t preferred,
    uint8_t abide
)
{
    // Test if position is at the end of the file
    const long old_pos = stream_end(fp);
    const bool at_end = old_pos == ftell(fp);
    // Restore the old position
    stream_towards(fp, old_pos, SEEK_SET);

    if (!at_end)
        return preferred;

    // Respect the given bit_offset from the "abide"
    return preferred == 0
        ? abide
        : MIN(preferred, abide);
}

uint8_t
bitarray_write_sizeheader_(
    const bitarray* bitarray,
    FILE* fp,
    int64_t max_bit_count
)
{
    const size_t item_write_count = 1;

    if (max_bit_count == -1)
        max_bit_count = INT64_MAX;

    bitarray_length_t bit_count = bitarray_length(bitarray);
    bit_count = MIN(bit_count, (bitarray_length_t)max_bit_count);

    uint8_t bit_offset_count = BIT_REST_COUNT_(bit_count);

    if (fwrite(&bit_offset_count, sizeof(bit_offset_count), item_write_count, fp) < item_write_count)
        return 0;

    return bit_offset_count;
}

uint8_t
bitarray_read_sizeheader_(
    FILE* fp
)
{
    const size_t read_count = 1;

    uint8_t bit_count = 0;
    if (fread(&bit_count, sizeof(bit_count), read_count, fp) < read_count)
        return 0;

    return bit_count;
}

// --- END LOCAL IMPLEMENTATIONS --- //

