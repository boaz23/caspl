#include "util.h"

#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_LSEEK 19
#define STDOUT 1

#define NULL (void *)0
#define INVALID_FD -1
#define VALID_FD 0

#define SEEK_SET 0x0
#define SEEK_CUR 0x1

#define ERR_EXIT_CODE 0x55
#define FILE_OPEN_FLAGS 0x2
#define FILE_OPEN_MODE 0x777
#define REPLACEMENT_OFFSET 0x291

#define SHIRA_NAME_LENGTH 5
#define POST_NAME_LENGTH 3

int prints(char *s);

int fwrite(int fd, void *buffer, int len) {
    return system_call(SYS_WRITE, fd, buffer, len);
}

int fread(int fd, void *buffer, int len) {
    return system_call(SYS_READ, fd, buffer, len);
}

int fopen(char *file_name, int flags, int mode) {
    return system_call(SYS_OPEN, file_name, flags, mode);
}

int fclose(int fd) {
    if (fd >= VALID_FD) {
        return system_call(SYS_CLOSE, fd);
    }

    return -2;
}

int fseek(int fd, int offset, int whence) {
    return system_call(SYS_LSEEK, fd, offset, whence);
}

int writes(int fd, char *s) {
    return fwrite(fd, s, strlen(s));
}

int prints(char *s) {
    return writes(STDOUT, s);
}

void print_file_msg(char *msg, char *file_name) {
    prints(msg);
    prints(file_name);
    prints("'\n");
}

void print_num(char *msg, int n) {
    prints(msg);
    prints(itoa(n));
    prints("\n");
}

int write_name(int fd, char *name) {
    char post_name[POST_NAME_LENGTH];
    int err = 0;
    int bytes_read = -1;
    int bytes_written = -1;
    int name_len = -1;
    int i = -1;
    int offset;
    char nullterminate = '\0';

    prints("seeking...\n");
    offset = fseek(fd, SHIRA_NAME_LENGTH, SEEK_CUR);
    if (offset <= 0) {
        return -1;
    }
    
    print_num("offset: ", offset);
    prints("reading...\n");
    bytes_read = fread(fd, post_name, POST_NAME_LENGTH);
    print_num("err: ", bytes_read);
    if (bytes_read < POST_NAME_LENGTH) {
        return -1;
    }

    prints("seeking...\n");
    err = fseek(fd, offset-(SHIRA_NAME_LENGTH), SEEK_SET);
    if (err <= 0) {
        return -1;
    }

    prints("writing name...\n");
    bytes_written = writes(fd, name);
    if (bytes_written < strlen(name)) {
        return -1;
    }

    name_len = bytes_written;
    bytes_written += fwrite(fd, post_name, POST_NAME_LENGTH);
    
    i = name_len;
    while (i < SHIRA_NAME_LENGTH) {
        int w = fwrite(fd, &nullterminate, 1);
        if (w < 1) {
            return -1;
        }

        bytes_written += w;

        ++i;
    }

    return bytes_written;
}

int main (int argc, char *argv[], char *envp[]) {
    char *file_name = NULL;
    char *new_name = NULL;
    int exit_code = ERR_EXIT_CODE;
    int err = 0;
    int fd = INVALID_FD;
    int bytes_written = -1;
    if (argc != 3) {
        prints("expected exactly 2 argument\n");
        goto exit;
    }

    file_name = argv[1];
    new_name = argv[2];
    fd = fopen(file_name, FILE_OPEN_FLAGS, FILE_OPEN_MODE);
    if (fd < VALID_FD) {
        print_file_msg("An error occurred when tried opening the file '", file_name);
        goto exit;
    }

    err = fseek(fd, REPLACEMENT_OFFSET, SEEK_SET);
    if (err <= 0) {
        print_file_msg("An error occurred when seeking in file '", file_name);
        goto exit;
    }

    bytes_written = write_name(fd, new_name);
    if (bytes_written <= 0) {
        print_file_msg("An error occurred when replacing the name '", file_name);
        goto exit;
    }

    exit_code = 0;
exit:
    fclose(fd);
    return exit_code;
}