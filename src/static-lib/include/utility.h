#ifndef HUFFMAN_HUFFMAN_UTILITY_H
#define HUFFMAN_HUFFMAN_UTILITY_H

#define MAX(A_, B_) ((A_) > (B_) ? (A_) : (B_))
#define MIN(A_, B_) ((A_) > (B_) ? (B_) : (A_))

#define UNUSED(A) ((void) (A))

#define SWAP_INNER_(TEMP_, A_, B_) \
do { \
    (TEMP_) = (A_); \
    (A_) = (B_); \
    (B_) = (TEMP_); \
} while(0)

#define SWAP(TYPE_, A_, B_) \
do { \
    (TYPE_) temp_ = {}; \
    SWAP_INNER_(temp_, A_, B_); \
} while(0)

#define ARRAY_REVERSE(TYPE_, ARR_, LEN_) \
do { \
    TYPE_ temp_ = {}; \
    for (long long i_ = 0; i_ < LEN_/2; i_++) \
        SWAP_INNER_(temp_, ARR_[i_], ARR_[LEN_ - 1 - i_]); \
} while (0)

#endif // !HUFFMAN_HUFFMAN_UTILITY_H

