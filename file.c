// file.c - file operations

#include "file.h"
#include <stdio.h>

int FileExists(const char *filename)
{
    FILE *f;
    f = fopen(filename, "rb");   // "rb" - binary mode, important for .3DF
    if (f == NULL)
        return 0;                      // file does not exist or no permissions

    fclose(f);
    return 1;                          // file exists
}
