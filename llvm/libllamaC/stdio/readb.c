#include <stdbool.h>
/**
 * This function reads a boolean from the standard input.
 * A whole line (of up to MAXSTRING characters) is actually
 * read by a call to readString.  The result is true if the
 * first character of the line is a 't', ignoring case.
 * Leading spaces are also ignored.
 */
bool readBoolean();

//dependencies
void readString(int size, char *s);

bool readBoolean() {
#define STRSIZE 256
    char buff[STRSIZE];
    readString(STRSIZE, buff);
    return buff[0] == 't';
}
