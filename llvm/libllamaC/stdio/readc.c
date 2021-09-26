/**
 * This function reads a character from the standard input
 * and returns it.
 */
char readChar();

//dependencies
void readString(int size, char *s);

char readChar() {
    char res;
    readString(1, &res);
    return res;
}