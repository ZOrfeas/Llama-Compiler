#include <stdio.h>
/**
 * This function prints a character to the standard output.
 */
void writeChar(char c);

void writeChar(char c) {
    putc(c, stdout);
}