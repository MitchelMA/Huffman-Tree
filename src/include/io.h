#ifndef HUFFMAN_IO_H
#define HUFFMAN_IO_H

#include <stdio.h>

FILE* open_file_write(const char* filename);
FILE* open_file_read(const char* filename);

bool close_file(FILE* fp);

#endif // !HUFFMAN_IO_H

