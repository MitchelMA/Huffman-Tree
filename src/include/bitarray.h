#ifndef HUFFMAN_HUFFMAN_BITARRAY_H
#define HUFFMAN_HUFFMAN_BITARRAY_H

#include <vector.h>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef size_t  bitarray_length_t;
typedef uint8_t bitarray_bitcontainer_t;

typedef enum bitarray_kinds {
    BITARRAY_IN_MEM,
    BITARRAY_IO_BUFFERED,
} bitarray_kinds;

typedef struct bitarray_ {
    /* The amount of bits written */
    [[deprecated]]
    bitarray_length_t length;
    /* A vector of type vector(uint8_t) */
    [[deprecated]]
    vector_t* content;

    union {
        struct {
            uint8_t bit_offset : 3;
            vector_t*  content;
        } in_mem;
        struct {
            uint8_t bit_offset : 3;
            uint8_t  io_buffer;
            FILE*         file;
        } buffered;
    };

    bitarray_kinds kind;
} bitarray;

[[deprecated]]
bool bitarray_init(bitarray* bitarray);
bool bitarray_init_in_mem(bitarray* bitarray);
bool bitarray_init_io_buffered(bitarray* bitarray, FILE* fp);

void bitarray_clean(bitarray* bitarray);

bool bitarray_append_bit(bitarray* bitarray, bool value);

int bitarray_value_at(const bitarray* bitarray, bitarray_length_t index);

int64_t bitarray_read_file(bitarray* bitarray, FILE* fp, int64_t bit_count);
int64_t bitarray_write_file(bitarray* bitarray, FILE* fp, int64_t bit_count);


#endif // !HUFFMAN_HUFFMAN_BITARRAY_H

