#ifndef HUFFMAN_HUFFMAN_BITARRAY_H
#define HUFFMAN_HUFFMAN_BITARRAY_H

#include <vector.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef size_t  bitarray_length_t;
typedef uint8_t bitarray_bitcontainer_t;

typedef struct bitarray_ {
    /* The amount of bits written */
    bitarray_length_t length;
    /* A vector of type vector(uint8_t) */
    vector_t* content;
} bitarray;

bool bitarray_init(bitarray* bitarray);
void bitarray_clean(bitarray* bitarray);

bool bitarray_append_bit(bitarray* bitarray, bool value);

int bitarray_value_at(const bitarray* bitarray, bitarray_length_t index);


#endif // !HUFFMAN_HUFFMAN_BITARRAY_H

