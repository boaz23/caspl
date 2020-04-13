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
    virus *v = NULL;
    unsigned short sigSize;
    byte sigSizeBuffer[sizeof(v->SigSize)];
    char nameTemp[ARR_LEN(v->virusName)];
    unsigned char *sig = NULL;
    
    if (fread(sigSizeBuffer, sizeof(byte), ARR_LEN(sigSizeBuffer), file) < ARR_LEN(sigSizeBuffer)) {
        return NULL;
    }
    if (fread(nameTemp, sizeof(char), ARR_LEN(nameTemp), file) < ARR_LEN(nameTemp)) {
        return NULL;
    }

    sigSize = sigSizeBuffer[0] | (sigSizeBuffer[1] << 8);
    if (fseek(file, sigSize, SEEK_CUR) == -1) {
        return NULL;
    }
    if (fseek(file, -((int)sigSize), SEEK_CUR) == -1) {
        return NULL;
    }

    sig = (unsigned char*)malloc(sigSize);
    if (sig == NULL) {
        return NULL;
    }
    
    for (unsigned short i = 0; i < sigSize; ++i) {
        if (fread(&sig[i], sizeof(unsigned char), sizeof(unsigned char), file) < sizeof(unsigned char)*sizeof(unsigned char)) {
            free(sig);
            return NULL;
        }
    }

    v = (virus*)malloc(sizeof(virus));
    if (v == NULL) {
        free(sig);
        return NULL;
    }

    v->SigSize = sigSize;
    for (int i = 0; i < ARR_LEN(v->virusName); ++i) {
        v->virusName[i] = nameTemp[i];
    }
    v->sig = sig;
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
            perror("an error has occurred while reading the file\n");
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