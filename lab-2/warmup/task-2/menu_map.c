#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IS_IN_SPECIAL_CHAR_RANGE(c) (0x20 <= (c) && (c) <= 0x7E)
int const ENC_VAL = 3;

char encdec(char c, int modifier, int val) {
    return IS_IN_SPECIAL_CHAR_RANGE(c) ? (c + modifier * val) : c;
}
 
char censor(char c) {
    if(c == '!')
        return '.';
    else
        return c;
}

/* Gets a char c and returns its encrypted form by adding 3 to its value.
   If c is not between 0x20 and 0x7E it is returned unchanged */
char encrypt(char c) {
    return encdec(c, 1, ENC_VAL);
}

/* Gets a char c and returns its decrypted form by reducing 3 to its value.
   If c is not between 0x20 and 0x7E it is returned unchanged */
char decrypt(char c) {
    return encdec(c, -1, ENC_VAL);
}

/* dprt prints the value of c in a decimal representation followed by a
   new line, and returns c unchanged. */
char dprt(char c) {
    printf("%u\n", c);
    return c;
}

/* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c followed
   by a new line. Otherwise, cprt prints the dot ('.') character. After printing, cprt returns
   the value of c unchanged. */
char cprt(char c) {
    if (IS_IN_SPECIAL_CHAR_RANGE(c)) {
        printf("%c\n", c);
    }
    else {
        printf(".");
    }

    return c;
}

/* Ignores c, reads and returns a character from stdin using fgetc. */
char my_get(char c) {
    return fgetc(stdin);
}

/* Gets a char c,  and if the char is 'q' , ends the program with exit code 0. Otherwise returns c. */
char quit(char c) {
    if (c == 'q') {
        exit(0);
    }

    return c;
}
 
char* map(char *array, int array_length, char (*f) (char)){
    char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
    for (int i = 0; i < array_length; ++i) {
        mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}
 
int main(int argc, char **argv) {
}
