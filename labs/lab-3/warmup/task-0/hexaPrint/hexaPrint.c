#include <stdlib.h>
#include <stdio.h>

void PrintHex(char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        printf("%02hhX ", (unsigned char)buffer[i]);
    }
}

int main(int argc, char **argv) {
    char c;

    if (argc != 2) {
        printf("expected a filename as an argument");
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("an error has occured when opened the file for reading");
    }

    do {
        fread(&c, sizeof(char), 1, file);
        if (feof(file)) {
            break;
        }

        if (ferror(file)) {
            printf("\nan error has occured while reading the file");
            break;
        }

        PrintHex(&c, 1);
    } while (1);
    printf("\n");
    return 0;
}