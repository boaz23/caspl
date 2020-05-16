#include "util.h"

typedef unsigned long SYS_CALL_ARG;
typedef int (*SYSCALL)(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
int system_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
SYSCALL SysCall = system_call;

typedef unsigned long FILE;

void *NULL = 0;

#define stdin  0
#define stdout 1
#define stderr 2

typedef int RW;
RW const RW_READ  = 0;
RW const RW_WRITE = 1;

int const SYS_EXIT    = 1;
int const SYS_READ    = 3;
int const SYS_WRITE   = 4;
int const SYS_OPEN    = 5;
int const SYS_CLOSE   = 6;
int const SYS_LSEEK   = 19;

int const SEEK_SET = 0;
int const SEEK_CUR = 1;
int const SEEK_END = 2;

int const O_RDONLY = 0;
int const O_WRONLY = 1;
int const O_CREAT  = 64;
int const O_TRUNC  = 512;

int DebugMode = 0;
FILE const DebugOut = stderr;


int dbg_write(void *buffer, int len) {
    return system_call(SYS_WRITE, DebugOut, (SYS_CALL_ARG)buffer, (SYS_CALL_ARG)len);
}

int dbg_write_c(char c) {
    return dbg_write(&c, 1);
}

int dbg_write_line() {
    return dbg_write_c('\n');
}

int dbg_write_s(char *s) {
    return dbg_write(s, strlen(s));
}

int dbg_write_num(int n) {
    return dbg_write_s(itoa(n));
}

void dbg_print_sys_call_info(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2, int e) {
    dbg_write_s("DEBUG SYS_CALL: ");

    dbg_write_s("ID=");
    dbg_write_num(cid);
    dbg_write_s(", arg0=");
    dbg_write_num(arg0);
    dbg_write_s(", arg1=");
    dbg_write_num(arg1);
    dbg_write_s(", arg2=");
    dbg_write_num(arg2);
    dbg_write_s(", ret=");
    dbg_write_num(e);
    dbg_write_line();
}

int dbg_system_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2) {
    int e = system_call(cid, arg0, arg1, arg2);
    dbg_print_sys_call_info(cid, arg0, arg1, arg2, e);
    return e;
}

void prg_exit(int status) {
    SysCall(SYS_EXIT, status, 0, 0);
}

int write(FILE fd, void *buffer, int len) {
    return SysCall(SYS_WRITE, fd, (SYS_CALL_ARG)buffer, (SYS_CALL_ARG)len);
}

int read(FILE fd, void *buffer, int len) {
    return SysCall(SYS_READ, fd, (SYS_CALL_ARG)buffer, (SYS_CALL_ARG)len);
}

FILE open(char *file_name, int flags, int mode) {
    return SysCall(SYS_OPEN, (SYS_CALL_ARG)file_name, (SYS_CALL_ARG)flags, (SYS_CALL_ARG)mode);
}

int close(FILE fd) {
    if (fd >= 0 && fd != stdin && fd != stdout && fd != stderr) {
        return SysCall(SYS_CLOSE, fd, 0, 0);
    }

    return -1;
}

int lseek(FILE fd, int offset, int whence) {
    return SysCall(SYS_LSEEK, fd, (SYS_CALL_ARG)offset, (SYS_CALL_ARG)whence);
}

int read_c(FILE fd, char *c) {
    return read(fd, c, 1); 
}

int write_c(FILE fd, char c) {
    return write(fd, &c, 1);
}

int write_line(FILE fd) {
    return write_c(fd, '\n');
}

int write_s(FILE fd, char *s) {
    return write(fd, s, strlen(s));
}

int write_num(FILE fd, int n) {
    return write_s(fd, itoa(n));
}

void dbg_print_c_transform(char c, char c_old) {
    dbg_write_num(c_old);
    dbg_write_c('\t');
    dbg_write_num(c);
    dbg_write_line();
}

int read_input(FILE f_in, FILE f_out) {
    int err;

    while (1) {
        char c, c_old;
        
        err = read_c(f_in, &c);
        if (err == 0) {
            /* EOF */
            break;
        }
        else if (err < 0) {
            /* error */
            return 1;
        }

        c_old = c;
        if (c == '\n') {
            write_c(f_out, c);
            continue;
        }
        else if ('a' <= c && c <= 'z') {
            c -= 'a' - 'A';
        }

        if (DebugMode) {
            /*dbg_print_c_transform(c, c_old);*/
        }
        write_c(f_out, c);
    }

    return 0;
}

FILE open_f(char *file_name, RW rw) {
    int f, flags, mode;
    char *rws = NULL;
    
    if (rw == RW_WRITE) {
        flags = O_WRONLY | O_TRUNC | O_CREAT;
        mode = 0777;
        rws = "writing";
    }
    else {
        flags = O_RDONLY;
        mode = 0;
        rws = "reading";
    }

    f = open(file_name, flags, mode);
    if (f < 0) {
        write_s(stdout, "Failed to open the file '");
        write_s(stdout, file_name);
        write_s(stdout, "' for ");
        write_s(stdout, rws);
        write_s(stdout, "\n");
    }

    return f;
}

FILE open_f_arg(char **argv, int i, RW rw) {
    return open_f(argv[i] + 2, rw);
}

int main(int argc, char **argv) {
    int i = 0, err = 0;
    int exit_code = 0;
    FILE f_in = stdin;
    FILE f_out = stdout;

    for (i = 1; i < argc; ++i) {
        if (strncmp("-D", argv[i], 2) == 0) {
            DebugMode = 1;
            SysCall = dbg_system_call;
            break;
        }
    }
    for (i = 1; i < argc; ++i) {
        if (strncmp("-i", argv[i], 2) == 0) {
            f_in = open_f_arg(argv, i, RW_READ);
            if (f_in < 0) {
                exit_code = 2;
                err = f_in;
            }
        }
        else if (strncmp("-o", argv[i], 2) == 0) {
            f_out = open_f_arg(argv, i, RW_WRITE);
            if (f_out < 0) {
                exit_code = 3;
                err = f_out;
            }
        }
    }
    
    if (!err) {
        exit_code = read_input(f_in, f_out);
    }
    close(f_in);
    close(f_out);
    close(DebugOut);
    return exit_code;
}