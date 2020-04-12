#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct fun_desc {
    char *name;
    char (*fun)(char);
} fun_desc;

#define ARR_LEN(arr) (sizeof((arr))/sizeof(*(arr)))
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
        printf(".\n");
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
    char* mapped_array = (char*)(malloc((array_length) * sizeof(char)));
    for (int i = 0; i < array_length; ++i) {
        mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}
 
int main(int argc, char **argv) {
    int const carr_len = 5;
    // QUESTION: can we assume the input for my_get will always be of length 4?
    char *carray = calloc(carr_len, sizeof(char));
    char *prev_carray = NULL;

    const fun_desc menu_items[] = {
        { "Censor", censor },
        { "Encrypt", encrypt },
        { "Decrypt", decrypt },
        { "Print dec",  dprt },
        { "Print string", cprt },
        { "Get string", my_get },
        { "Quit", quit },
        { NULL, NULL },
    };

    carray[0] = '\0';

    do {
        const fun_desc *menu_item = menu_items;
        int i = 0;

        char option_buffer[256];
        int option;

        printf("Please choose a function:\n");
        while (menu_item->name != NULL) {
            printf("%d) %s\n", i, menu_item->name);
            ++menu_item;
            ++i;
        }

        printf("Option: ");

        // TODO: check if some form flushing or special handling is needed
        if (fgets(option_buffer, ARR_LEN(option_buffer), stdin) == NULL) {
            printf("badly formatted input entered\n\n");
            continue;
        }

        // TODO: investigate this line
        // nullify the '\n'
        //option_buffer[1] = '\0';
        if (sscanf(option_buffer, "%d", &option) == EOF) {
            printf("badly formatted input entered\n\n");
            continue;
        }

        // TODO: how are we on the one hand suppost not to explicitly keep the length
        // but on the other hand compute the 'bounds' only once before looping???
        if (0 <= option && option < ARR_LEN(menu_items)) {
            printf("Within bounds\n");
        }
        else {
            printf("Not within bounds\n");
            break;
        }

        prev_carray = carray;
        carray = map(carray, carr_len, menu_items[option].fun);

        // TODO: investigate this line
        // if we got input from the user, we need to nullify the '\n'
        // carray[carr_len - 1] = '\0';
        free(prev_carray);

        printf("DONE.\n\n");
    } while (1);

    free(carray);
}
