// #include <stdio.h>
// #include <stdbool.h>
// #include <string.h>
// #include <stdlib.h>

#define MAXSTRING 64

void readString(int size, char *s);
void writeString(char *s);

int _atoi(const char *str) {
    int acc;
    for (int i = 0; str[i] != '\0'; i++) {
        acc = acc * 10 + str[i] - '0';
    }
    return acc;
}

void _reverse(char str[], int length){
    int start = 0;
    int end = length -1;
    int tmp;
    while (start < end)
    {
        // swap(*(str+start), *(str+end));
        tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}
void _itoa(int n, char *buf) {
    int i = 0, rem;
    char isNegative = 0;

    if (n == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return;
    }

    if (n < 0) {
        isNegative = 1;
        n = -n;
    }
    while (n != 0) {
        rem = n % 10;
        buf[i++] = rem + '0';
        n = n / 10;
    }
    if (isNegative)
        buf[i++] = '-';
    buf[i] = '\0';
    _reverse(buf, i);
}

int readInteger() {
    char buffer[MAXSTRING];
    readString(MAXSTRING, buffer);
    return _atoi(buffer);
}
void writeInteger(int n) { 
    char buffer[15];
    _itoa(n, buffer);
    writeString(buffer);
}
char chr(int n) { return (char)n; }
int ord(char c) { return (int)c; }

// int abs(int) is supported from stdlib.h
// float atanf(float) is supported from math.h
// float cosf(float) is supported from math.h
// float expf(float) is supported from math.h
// float fabsf(float) is supported from math.h
// float logf(float) is supported from math.h (this is ln)
// float sinf(float) is supported from math.h
// float sqrtf(float) is supported from math.h
// float tanf(float) is supported from math.h

// char readChar() {
//     int ch;
//     while ((ch=getchar()) != EOF)
//         if (ch != '\n') break;
//     return (char)ch;
// }
// void readString(int size, char *s) {
//     fgets(s, size, stdin);
//     int i = strlen(s) - 1;
//     if (s[i] != '\n') while(getchar()!='\n');
//     if (s[i] == '\n') s[i] = '\0';
// }
// float readReal() {
//     char buffer[MAXSTRING];
//     readString(MAXSTRING, buffer);
//     return strtod(buffer, NULL);
// }
// bool readBoolean() {
//     char buffer[MAXSTRING];
//     readString(MAXSTRING, buffer);
//     if (buffer[0] == 't' || buffer[0] == 'T')
//         return true;
//     else 
//         return false;
// }

// void writeChar(char c) { printf("%c", c); }

// void writeReal(float f) { printf("%.5f", f); }
// void writeString(char *s) { printf("%s", s); }
// void writeBoolean(bool b) { 
//     if (b) puts("true");
//     else   puts("false");
// }

// void exit(int n) {exit}
// float roundf(float) supported from math.h
// float truncf(float) supported from math.h

// without these it fails to link :)
// float pi() { return 0.0f; }
// float ln(float f) { return 0.0f; }
// float exp(float f) { return 0.0f; }



// int main () {
//     char buffer[MAXSTRING];
//     readString(MAXSTRING, buffer);
//     printf("%s\n", buffer);
//     return 0;
// }