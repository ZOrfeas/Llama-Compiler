#include <stdio.h>
/**
 * This function reads a line from the standard input
 * and stores it into string 's'.  The newline character
 * is not stored.  Up to 'size' characters can be read.
 * This function skips control characters and correctly
 * treats the backspace character.  Finally, a '\0' is
 * always appended.  Assumes that size > 0.
 */
void readString(int size, char *s);

void readString(int size, char *s) {
    fgets(s, size, stdin);
}