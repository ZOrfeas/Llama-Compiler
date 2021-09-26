#include <stdbool.h>
#include <stdio.h>
/**
 * This function prints a boolean to the standard output.
 * One of the strings 'true' and 'false' is printed.
 */
void writeBoolean(bool b);

void writeBoolean(bool b) {
    puts(b ? "true" : "false");
}