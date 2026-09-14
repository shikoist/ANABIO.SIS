// file.c - операции с файлами

#include "file.h"
#include <stdio.h>

int FileExists(const char *filename)
{
    FILE *f;
    f = fopen(filename, "rb");   // "rb" — бинарный режим, важно для .3DF
    if (f == NULL)
        return 0;                      // файл не существует или нет прав

    fclose(f);
    return 1;                          // файл существует
}
