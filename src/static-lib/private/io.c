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

long
stream_towards(
    FILE* fp,
    const long offset,
    const int whence
)
{
    const long current_pos = ftell(fp);

    if (fseek(fp, offset, whence))
        return -1;

    return current_pos;
}

long
stream_begin(
    FILE* fp
)
{
    return stream_towards(fp, 0, SEEK_SET);
}

long
stream_end(
    FILE* fp
)
{
    return stream_towards(fp, 0, SEEK_END);
}

long
stream_tell_size(
    FILE* fp
)
{
    const long initial = stream_end(fp);

    // Failure in moving to end of stream
    if (initial < 0)
        return initial;

    // Restore initial position and return the position at the end (length)
    return stream_towards(fp, initial, SEEK_SET);
}

long
stream_tell_remaining_size(
    FILE* fp
)
{
    const long start = stream_end(fp);

    if (start < 0)
        return start;

    const long end = stream_towards(fp, start, SEEK_SET);

    if (end < 0)
        return end;

    return end - start;
}

