#include <stdio.h>

#define NUMBER_LEN 128

extern void assFunc(int x, int y);

void readNumber(int *number){
    char input[NUMBER_LEN];

    fgets(input, NUMBER_LEN, stdin);
    sscanf(input, "%d", number);
}

char c_checkValidity(int x, int y){
    if(y <= x){
        return 1;
    }
    return 0;
}

int main(int argc, char **argv){
    int x, y;

    readNumber(&x);
    readNumber(&y);

    assFunc(x, y);
    return 0;
}