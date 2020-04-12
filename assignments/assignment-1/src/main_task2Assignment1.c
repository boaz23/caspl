#include <stdio.h>
#include <string.h>

#define	MAX_LEN 34			/* maximal input string size */
					/* enough to get 32-bit string + '\n' + null terminator */
extern int convertor(char* buf);

int main(int argc, char** argv)
{
    do {
        // int n = 0;
        // int pow = 1;
        // int j = 0;
        // char hexbuf[9];
        // char temp;

        char buf[MAX_LEN ];
        fgets(buf, MAX_LEN, stdin); /* get user input string */ 
        if (buf[0] == 'q')
        {
            break;
        }
        
        // int len = strlen(buf);
        // printf("%d\n", buf[len - 1]);
        // for (int i = len - 2; i >= 0; --i) {
        //     n += (buf[i] - 48) * pow;
        //     pow *= 10;
        // }

        // printf("%d\n", n);
        // while (n > 0) {
        //     int digit = n % 16;
        //     char cdigit;
        //     if (0 <= digit && digit <= 9) {
        //         cdigit = digit + 48;
        //     }
        //     else {
        //         cdigit = digit + 55;
        //     }
        //     hexbuf[j] = cdigit;
        //     n /= 16;
        //     ++j;
        // }
        // hexbuf[j] = '\0';
        // printf("%s\n", hexbuf);

        // for (int i = 0, k = j - 1; i < k; ++i, --k) {
        //     temp = hexbuf[i];
        //     hexbuf[i] = hexbuf[k];
        //     hexbuf[k] = temp;
        // }
        // printf("%s\n", hexbuf);
        convertor(buf); /* call your assembly function */
    } while (1);
    return 0;
}