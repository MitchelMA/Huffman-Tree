#ifndef HUFFMAN_IO_H
#define HUFFMAN_IO_H

#include <stdio.h>
#include <stdbool.h>

FILE* open_file_write(const char* filename);
FILE* open_file_append(const char* filename);
FILE* open_file_read(const char* filename);

FILE* reopen_file_write(FILE* fp);
FILE* reopen_file_append(FILE* fp);
FILE* reopen_file_read(FILE* fp);

bool close_file(FILE* fp);

long stream_towards(FILE* fp, long offset, int whence);
long stream_begin(FILE* fp);
long stream_end(FILE* fp);
long stream_tell_size(FILE* fp);
long stream_tell_remaining_size(FILE* fp);

#endif // !HUFFMAN_IO_H

