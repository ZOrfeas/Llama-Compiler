#include <stdio.h>
/**
 * This function prints a null terminated string to the standard output.
 */
void writeString(char *s);

void writeString(char *s) {
    fputs(s, stdout);
}