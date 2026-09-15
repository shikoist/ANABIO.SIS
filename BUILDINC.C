#include <stdio.h>
#include <stdlib.h>

char* build = "build.txt";

int main(void)
{
    FILE *fp;
    int value = 0;

    /* Trying to open the file for reading */
    fp = fopen(build, "r");
    if (fp != NULL) {
        /* If the file exists - read the number */
        if (fscanf(fp, "%d", &value) != 1) {
            /* If reading failed (empty file or not a number) - start from 0 */
            value = 0;
        }
        fclose(fp);
    } else {
        /* If the file does not exist - start from 0 */
        value = 0;
    }

    /* Increase the value */
    value++;

    /* Open the file for writing (create anew) */
    fp = fopen(build, "w");
    if (fp == NULL) {
        printf("Error: Cannot write to %s\n", build);
        return 1;
    }

    /* Write the new value */
    fprintf(fp, "%d\n", value);
    fclose(fp);

    /* Print the result to the screen */
    printf("Build: %d\n", value);

    return 0;
}
