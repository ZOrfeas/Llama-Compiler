/**
 * This function returns the character corresponding to an ASCII code.
 * Only the lower 8 bits of the parameter are considered, thus the
 * parameter should be a number between 0 and 255.
 */
char chr(int n);


char chr(int n) {
    return (char)n;
}