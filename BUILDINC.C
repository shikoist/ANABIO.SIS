#include <stdio.h>
#include <stdlib.h>

char* build = "build.txt";

int main(void)
{
    FILE *fp;
    int value = 0;

    /* Пытаемся открыть файл для чтения */
    fp = fopen(build, "r");
    if (fp != NULL) {
        /* Если файл существует - читаем число */
        if (fscanf(fp, "%d", &value) != 1) {
            /* Если прочитать не удалось (пустой файл или не число) - начинаем с 0 */
            value = 0;
        }
        fclose(fp);
    } else {
        /* Файла нет - начнём с 0 */
        value = 0;
    }

    /* Увеличиваем значение */
    value++;

    /* Открываем файл для записи (создаём заново) */
    fp = fopen(build, "w");
    if (fp == NULL) {
        printf("Error: Cannot write to %s\n", build);
        return 1;
    }

    /* Записываем новое значение */
    fprintf(fp, "%d\n", value);
    fclose(fp);

    /* Выводим результат на экран */
    printf("Build: %d\n", value);

    return 0;
}
