#include <stdlib.h>
/**
 * This function reads an integer from the standard input
 * and returns it.  A whole line (of up to MAXSTRING characters)
 * is actually read by a call to readString.  Leading spaces are
 * ommited.  If the line does not contain a valid number, 0 is
 * returned (same behaviour as 'atoi' in C).
 */
int readInteger();

//dependencies
void readString(int size, char *s);

int readInteger() {
#define STRSIZE 256
    char buff[STRSIZE];
    readString(STRSIZE, buff);
    return atoi(buff);
}