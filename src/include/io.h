#ifndef HUFFMAN_IO_H
#define HUFFMAN_IO_H

#include <stdio.h>

FILE* open_file_write(const char* filename);
FILE* open_file_append(const char* filename);
FILE* open_file_read(const char* filename);

FILE* reopen_file_write(FILE* fp);
FILE* reopen_file_append(FILE* fp);
FILE* reopen_file_read(FILE* fp);

bool close_file(FILE* fp);

#endif // !HUFFMAN_IO_H

