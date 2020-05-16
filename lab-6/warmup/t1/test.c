#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char buf[20];
    int len;
    fgets(buf, 20, stdin);
    len = strlen(buf);
    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    printf("%s\n%s\n", buf, buf);
    return 0;
}