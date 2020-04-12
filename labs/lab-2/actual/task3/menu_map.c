#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARR_LEN(a) (sizeof((a))/sizeof(*(a)))
#define IS_CHAR_PRINTABLE(c) (0x20 <= (c) && (c) <= 0x7E)

int const ENC_VAL = 3;
int const ENC_MOD = 1;
int const DEC_MOD = -1;

typedef struct fun_desc {
  char *name;
  char (*fun)(char);
} fun_desc;

char encdec(char c, int modifier, int value) {
	return IS_CHAR_PRINTABLE(c) ? (c + modifier * value) : c;
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
	return encdec(c, ENC_MOD, ENC_VAL);
}

/* Gets a char c and returns its decrypted form by reducing 3 to its value.
   If c is not between 0x20 and 0x7E it is returned unchanged */
char decrypt(char c) {
	return encdec(c, DEC_MOD, ENC_VAL);
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
	if (IS_CHAR_PRINTABLE(c)) {
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
    char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
    for (int i = 0; i < array_length; ++i) {
		mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}

void print_menu(const fun_desc menu[], FILE *file) {
	int i = 0;
	const fun_desc *p_menu_item = menu;
	while (p_menu_item->name != NULL) {
		fprintf(file, "%d) %s\n", i, p_menu_item->name);
		++p_menu_item;
		++i;
	}
}

int get_num(FILE *file, int *num) {
	char buffer[256];
	int result_flag;
	if (fgets(buffer, ARR_LEN(buffer), file) == NULL) {
		return 0;
	}

	result_flag = sscanf(buffer, "%d", num);
	if (result_flag == EOF || result_flag == 0) {
		return 0;
	}

	return 1;
}

int main(int argc, char **argv) {
	int const base_len = 5;
	char *carray;
	char *prev_carray;

	const fun_desc menu[] = {
		{ "Censor", censor },
		{ "Encrypt", encrypt },
		{ "Decrypt", decrypt },
		{ "Print dec", dprt },
		{ "Print string", cprt },
		{ "Get string", my_get },
		{ "Quit", quit },
		{ NULL, NULL }
	};

	carray = malloc(base_len * sizeof(char));
	carray[0] = '\0';

	do {
		int option;

		printf("Please choose a function:\n");
		print_menu(menu, stdout);
		printf("Option: ");
		if (!get_num(stdin, &option)) {
			printf("invalid input\n\n");
			continue;
		}

		if (0 <= option && option <= ARR_LEN(menu) - 2) {
			printf("Within bounds\n");
		}
		else {
			printf("Not within bounds\n");
			break;
		}

		prev_carray = carray;
		carray = map(carray, base_len, menu[option].fun);
		free(prev_carray);

		printf("DONE.\n\n");
	} while (1);
	free(carray);

	return 0;
}