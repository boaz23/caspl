#include <stdio.h>
#include <string.h>
#include <errno.h>

void close_file(FILE *f, FILE *unless) {
	if (f != NULL)
	{
		fflush(f);
		if (f != unless) {
			fclose(f);
		}
	}
}

int main(int argc, char **argv) {
	FILE *fin = stdin;
	FILE *fout = stdout;
	FILE *fdbg_out = stderr;

	int i;
	char *arg;

	char c, c_old;

	int isDebugMode = 0;

	int encription_mode = 0;
	char *pc_enc_key_start, *pc_enc_key;

	char *file_name;

	for (i = 1; i < argc; ++i)
	{
		arg = argv[i];
		if (strcmp(arg, "-D") == 0) {
			isDebugMode = 1;
		}
		else if (strncmp(arg, "+e", 2) == 0)
		{
			encription_mode = 1;
			pc_enc_key_start = arg + 2;
			pc_enc_key = pc_enc_key_start;
		}
		else if (strncmp(arg, "-e", 2) == 0)
		{
			encription_mode = -1;
			pc_enc_key_start = arg + 2;
			pc_enc_key = pc_enc_key_start;
		}
		else if (strncmp(arg, "-o", 2) == 0)
		{
			file_name = arg + 2;
			fout = fopen(file_name, "w");
			if (fout == NULL)
			{
				fprintf(stderr, "cannot open the file '%s' for writing. error code: %d", file_name, errno);
				goto exit;	
			}
		}
		else if (strncmp(arg, "-i", 2) == 0)
		{
			file_name = arg + 2;
			fin = fopen(file_name, "r");
			if (fin == NULL)
			{
				fprintf(stderr, "cannot open the file '%s' for reading. error code: %d", file_name, errno);
				goto exit;
			}
		}
	}
	
	if (isDebugMode) {
		if (argc > 1) {
			i = 1;
			fputs(argv[i], fdbg_out);
			++i;
			for (; i < argc; ++i) {
				fprintf(fdbg_out, " %s", argv[i]);
			}

			fputc('\n', fdbg_out);
		}
	}

	do {
		do
		{
			c = c_old = fgetc(fin);
			if (c == EOF) {
				goto input_end;
			}
			else if (c == '\n')
			{
				break;
			}
			
			
			if (encription_mode) {
				int enc_val = *pc_enc_key - 48;
				c = c + encription_mode * enc_val;
				
				++pc_enc_key;
				if (*pc_enc_key == 0)
				{
					pc_enc_key = pc_enc_key_start;
				}
			}
			else if ('a' <= c && c <= 'z') {
				c -= 32;
			}

			if (isDebugMode)
			{
				fprintf(fdbg_out, "%-8u %-u\n", c_old, c);
			}
			
			fputc(c, fout);
		} while (1);

		fputc('\n', fout);
		pc_enc_key = pc_enc_key_start;
	} while (1);
input_end:

exit:
	close_file(fout, stdout);
	close_file(fin, stdin);
	return 0;
}