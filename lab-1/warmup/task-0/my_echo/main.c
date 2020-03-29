#include <stdio.h>

int main(int argc, char **argv) {
	if (argc > 1) {
		int i = 1;
		fputs(argv[i], stdout);
		++i;
		for (; i < argc; ++i) {
			fprintf(stdout, " %s", argv[i]);
		}
		fputc('\n', stdout);
	}
	return 0;
}
