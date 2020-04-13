#include <stdio.h>
#include <stdlib.h>

typedef unsigned char byte;
#define ARR_LEN(a) (sizeof((a))/sizeof(*(a)))

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

void PrintHex(FILE *file, char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        fprintf(file, "%02hhX ", (unsigned char)buffer[i]);
    }
}

virus* readVirus(FILE *file) {
    virus *v = (virus*)malloc(sizeof(virus));
    
    int len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (fread(v, sizeof(byte), len, file) < sizeof(byte)*len) {
        free(v);
        return NULL;
    }

    v->sig = (unsigned char*)malloc(v->SigSize);
    if (v->sig == NULL) {
        return NULL;
    }

    if (fread(v->sig, sizeof(unsigned char), v->SigSize, file) < sizeof(unsigned char)*v->SigSize) {
        free(v->sig);
        free(v);
        return NULL;
    }
    
    return v;
}

void printVirus(virus *virus, FILE *output) {
    int i = 0;
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);
    fprintf(output, "signature:\n");
    
    for (i = 0; i + 20 < virus->SigSize; i += 20) {
        PrintHex(output, (char*)(virus->sig + i), 20);
        fprintf(output, "\n");
    }
    PrintHex(output, (char*)(virus->sig + i), virus->SigSize - i);
    fprintf(output, "\n");
}

int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "r");
    FILE *fout = stdout;
    virus *v = NULL;
    while (!feof(f)) {
        v = readVirus(f);
        if (v == NULL) {
            // error occurred
            // perror("an error has occurred while reading the file\n");
            break;
        }
        printVirus(v, fout);
        free(v);
        v = NULL;
        fprintf(fout, "\n");
    }
    if (f != NULL) {
        fclose(f);
        f = NULL;
    }
    return 0;
}