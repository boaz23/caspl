#include <stdlib.h>
#include <stdio.h>

int digit_count(char *s) {
    int counter = 0;
    for (char *ps = s; *ps != '\0'; ++ps) {
        char c = *ps;
        if ('0' <= c && c <= '9') {
            counter++;
        }
    }

    return counter;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: digit_cnt <string>\n");
    }
    else {
        printf("%d\n", digit_count(argv[1]));
    }
    return 0;
}