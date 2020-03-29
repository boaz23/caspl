#include <stdio.h>
#include <string.h>
#include <errno.h>

void close_file(FILE *file) {
    if (file != NULL) {
        fflush(file);
        if (file != stdin && file != stdout && file != stderr) {
            fclose(file);
        }
    }
}

int main(int argc, char **argv) {
    int exit_code = 0;

    char c, c_old;

    FILE *fout = stdout;
    FILE *fin = stdin;
    FILE *ferr = stderr;
    FILE *fdbg_out = stderr;

    int i;
    char *arg;
    char *file_name;

    int isDebugMode = 0;

    int encription_mode = 0;
    char *enc_start, *enc_current;

    for (i = 1; i < argc; ++i) {
        arg = argv[i];
        if (strcmp(arg, "-D") == 0) {
            isDebugMode = 1;
        }
        else if (strncmp(arg, "+e", 2) == 0 || strncmp(arg, "-e", 2) == 0) {
            if (arg[0] == '+') {
                encription_mode = 1;
            }
            else if (arg[0] == '-') {
                encription_mode = -1;
            }
            else {
                exit_code = -100;
                goto exit;
            }
            
            enc_start = arg + 2;

            if (*enc_start == 0) {
                fprintf(ferr, "invalid encription argument: '%s'\n", arg);
                exit_code = 3;
                goto exit;
            }
            for (enc_current = enc_start; *enc_current != 0; ++enc_current) {
                if (!('0' <= *enc_current && *enc_current <= '9')) {
                    fprintf(ferr, "invalid encription argument: '%s'\n", arg);
                    exit_code = 3;
                    goto exit;
                }
            }
            
            enc_current = enc_start;
        }
        else if (strncmp(arg, "-o", 2) == 0) {
            file_name = arg + 2;
            fout = fopen(file_name, "w");
            if (fout == NULL) {
                exit_code = 1;
                fprintf(ferr, "couldn't open the file '%s' for writing. error code: %u\n", file_name, errno);
                goto exit;
            }
        }
        else if (strncmp(arg, "-i", 2) == 0) {
            file_name = arg + 2;
            fin = fopen(file_name, "r");
            if (fin == NULL) {
                exit_code = 2;
                fprintf(ferr, "couldn't open the file '%s' for reading. error code: %u\n", file_name, errno);
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
        c = c_old = fgetc(fin);
        if (c == EOF) {
            goto input_end;
        }
        else if (c == '\n') {
            enc_current = enc_start;
        }
        else {
            if (encription_mode) {
                int enc_val = *enc_current - 48;
                c += enc_val * encription_mode;

                ++enc_current;
                if (*enc_current == 0) {
                    enc_current = enc_start;
                }
            }
            else if ('a' <= c && c <= 'z') {
                c -= 32;
            }

            if (isDebugMode) {
                fprintf(fdbg_out, "%u\t%u\n", c_old, c);
            }
        }

        fputc(c, fout);
    } while (1);
input_end:

exit:
    close_file(fout);
    close_file(fin);
    close_file(ferr);
    close_file(fdbg_out);
    return exit_code;
}