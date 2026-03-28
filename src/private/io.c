#include "io.h"

FILE*
open_file_write(
    const char* filename
)
{
    return fopen(filename, "w+b");
}

FILE*
open_file_read(
    const char* filename
)
{
    return fopen(filename, "rb");
}

bool
close_file(
    FILE* fp
)
{
    return fclose(fp) == 0;
}
