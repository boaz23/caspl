#include <stdio.h>

int digit_cnt(char *s) {
    int digit_count = 0;
    for (char *ps = s; *ps != '\0'; ++ps) {
        char c = *ps;
        if ('0' <= c && c <= '9') {
            digit_count++;
        }
    }

    return digit_count;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: digit_cnt <string>\n");
    }
    else {
        printf("%d\n", digit_cnt(argv[1]));
    }
    return 0;
}