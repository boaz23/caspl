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
#define dbg_out stdin
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
#define RW_READ  0
#define RW_WRITE 1

void dbg_print_char_transform(char c, char c_old) {
    dbg_print_num(c_old);
    dbg_print_s("\t");
    dbg_print_num(c);
    dbg_print_line();
}

int read_input(FILE f_in, FILE f_out) {
    int ec = 0;
    while (1) {
        char c = '\0', c_old = '\0';
        int r = read_c(f_in, &c);
        if (r == 0) {
            /* EOF */
            break;
        }
        else if (r < 0) {
            /* error */
            ec = 1;
            break;
        }

        c_old = c;
        if (c == '\n') {
            write_c(f_out, c);
            if (DebugMode) {
                dbg_print_c('\n');
            }
        }
        else {
            if ('a' <= c && c <= 'z') {
                c += 'A' - 'a';
            }

            if (DebugMode) {
                dbg_print_char_transform(c, c_old);
            }

            write_c(f_out, c);
        }
    }

    return ec;
}

int task_open_file(FILE f_out, char *file_name, int rw_mode) {
    char *s_open_mode = NULL;
    int flags = 0;
    int mode = 0;
    FILE f = -1;

    if (rw_mode == RW_READ) {
        flags = O_RDONLY;
        mode = 0;
        s_open_mode = "reading";
    }
    else if (rw_mode == RW_WRITE) {
        flags = O_CREAT | O_TRUNC | O_WRONLY;
        mode = F_MODE_ALL;
        s_open_mode = "writing";
    }
    else {
        return -1;
    }

    f = open(file_name, flags, mode);
    if (f < 0) {
        write_s(f_out, "could not open the file '");
        write_s(f_out, file_name);
        write_s(f_out, "' for ");
        write_s(f_out, s_open_mode);
        write_line(f_out);
    }

    return f;
}

void dbg_print_io_names(char *f_name, char *io_name) {
    dbg_print_s(io_name);
    dbg_print_s(" file name: ");
    dbg_print_s(f_name);
    dbg_print_line();
}

int main (int argc , char* argv[], char* envp[]) {
    int exit_code = 0;
    FILE f_default_out = stdout;
    FILE f_in = stdin, f_out = f_default_out;
    char *f_in_name = "stdin", *f_out_name = "stdout";

    int i = 0;
    for (i = 0; i < argc; ++i) {
        if (strncmp("-D", argv[i], 2) == 0) {
            DebugMode = 1;
            sys_call = dbg_sys_call;
            break;
        }
    }

    for (i = 0; i < argc; ++i) {
        if (strncmp("-i", argv[i], 2) == 0) {
            f_in_name = argv[i] + 2;
            f_in = task_open_file(f_default_out, f_in_name, RW_READ);
            if (f_in < 0) {
                exit_code = 2;
                break;
            }
        }
        else if (strncmp("-o", argv[i], 2) == 0) {
            f_out_name = argv[i] + 2;
            f_out = task_open_file(f_default_out, f_out_name, RW_WRITE);
            if (f_out < 0) {
                exit_code = 3;
                break;
            }
        }
    }

    if (DebugMode) {
        dbg_print_io_names(f_in_name, "input");
        dbg_print_io_names(f_out_name, "output");
    }

    if (exit_code == 0) {
        exit_code = read_input(f_in, f_out);
    }

    close(f_in);
    close(f_out);
    close(dbg_out);
    return exit_code;
}
