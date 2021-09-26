#include <stdlib.h>
/**
 * This function reads a real number from the standard input
 * and returns it.  A whole line (of up to MAXSTRING characters)
 * is actually read by a call to readString.  Leading spaces are
 * ommited.  If the line does not contain a valid number, 0.0 is
 * returned (same behaviour as 'atof' in C).
 */
double readReal();

//dependencies
void readString(int size, char *s);

double readReal() {
#define STRSIZE 256
    char buff[STRSIZE];
    readString(STRSIZE, buff);
    return strtod(buff, NULL);
}