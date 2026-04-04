#include "io.h"

static const char* write_mode  = "w+b";
static const char* append_mode = "a+b";
static const char* read_mode   = "rb";

FILE*
open_file_write(
    const char* filename
)
{
    return fopen(filename, write_mode);
}

FILE*
open_file_append(
    const char* filename
)
{
    return fopen(filename, append_mode);
}

FILE*
open_file_read(
    const char* filename
)
{
    return fopen(filename, read_mode);
}

FILE*
reopen_file_write(
    FILE* fp
)
{
    return freopen(NULL, write_mode, fp);
}

FILE*
reopen_file_append(
    FILE* fp
)
{
    return freopen(NULL, append_mode, fp);
}

FILE*
reopen_file_read(
    FILE* fp
)
{
    return freopen(NULL, read_mode, fp);
}

bool
close_file(
    FILE* fp
)
{
    return fclose(fp) == 0;
}
