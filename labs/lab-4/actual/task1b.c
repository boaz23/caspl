#include "util.h"

#define NULL 0

typedef int FILE;

#define stdin	0
#define stdout	1
#define stderr	2

#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100
#define O_TRUNC		00001000
#define O_APPEND	00002000
#define O_DIRECTORY	00200000

#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6

typedef unsigned long SYS_CALL_ARG;
typedef int (*SYS_CALL_FUNC)(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
extern int system_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2);
SYS_CALL_FUNC sys_call = system_call;

int read(FILE fd, void *buf, int len) {
	return sys_call(SYS_READ, fd, (SYS_CALL_ARG)buf, len);
}

int write(FILE fd, void *buf, int len) {
	return sys_call(SYS_WRITE, fd, (SYS_CALL_ARG)buf, len);
}

int open(char *file_name, int flags, int mode) {
	return sys_call(SYS_OPEN, (SYS_CALL_ARG)file_name, flags, mode);
}

int close(FILE fd) {
	int ret = -1;
	if (fd != stdin && fd != stdout && fd != stderr) {
		ret = sys_call(SYS_CLOSE, fd, 0, 0);
	}

	return ret;
}

int read_c(FILE fd, char *pc) {
	return read(fd, pc, 1);
}

int write_c(FILE fd, char c) {
	return write(fd, &c, 1);
}
int write_s(FILE fd, char *s) {
	return write(fd, s, strlen(s));
}
int write_num(FILE fd, int num) {
	char *s = itoa(num);
	return write_s(fd, s);
}
int write_line(FILE fd) {
	return write_c(fd, '\n');
}

#define dbg_out stderr
int DebugMode = 0;

int dbg_print(void *buf, int len) {
	return system_call(SYS_WRITE, dbg_out, (SYS_CALL_ARG)buf, len);
}
int dbg_print_c(char c) {
	return dbg_print(&c, 1);
}
int dbg_print_s(char *s) {
	return dbg_print(s, strlen(s));
}
int dbg_print_num(int num) {
	char *s = itoa(num);
	return dbg_print_s(s);
}
int dbg_print_line() {
	return dbg_print_c('\n');
}

void dbg_print_system_call_info(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2, int ret) {
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
int dbg_system_call(int cid, SYS_CALL_ARG arg0, SYS_CALL_ARG arg1, SYS_CALL_ARG arg2) {
	int ret = system_call(cid, arg0, arg1, arg2);
	dbg_print_system_call_info(cid, arg0, arg1, arg2, ret);
	return ret;
}

#define RW_READ		0
#define RW_WRITE	1

#define F_MODE_ALL 0777

#define EOF 0

int task_open_file(FILE f_out, char *file_name, int rw_mode) {
	FILE fd = -1;
	int flags = 0;
	int mode = 0;
	char *s_rw_mode = NULL;

	if (rw_mode == RW_READ) {
		flags = O_RDONLY;
		mode = 0;
		s_rw_mode = "reading";
	}
	else if (rw_mode == RW_WRITE) {
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		mode = F_MODE_ALL;
		s_rw_mode = "writing";
	}

	fd = open(file_name, flags, mode);
	if (fd < 0) {
		write_s(f_out, "Could not open the file '");
		write_s(f_out, file_name);
		write_s(f_out, "' for ");
		write_s(f_out, s_rw_mode);
		write_line(f_out);
	}

	return fd;
}

void dbg_print_char_transform(char c, char c_old) {
	dbg_print_num(c_old);
	dbg_print_s(",\t");
	dbg_print_num(c);
	dbg_print_line();
}

int read_input(FILE f_in, FILE f_out) {
	int err = 0;
	while (1) {
		char c = '\0', c_old = '\0';
		err = read_c(f_in, &c);
		if (err == EOF) {
			break;
		}
		else if (err < 0) {
			/* ERROR */
			break;
		}

		c_old = c;
		if (c == '\n') {
			write_line(f_out);
			if (DebugMode) {
				dbg_print_line();
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

	return err;
}

void dbg_print_io_name(char *name, char *type) {
	dbg_print_s(type);
	dbg_print_s(": ");
	dbg_print_s(name);
	dbg_print_line();
}
void dbg_print_io_names(char *in_name, char *out_name) {
	dbg_print_io_name(in_name, "input");
	dbg_print_io_name(out_name, "output");
}

int main(int argc , char* argv[], char* envp[]) {
	int i = 0;
	int exit_code = 0;
	FILE f_default_out = stdout;
	FILE f_in = stdin, f_out = f_default_out;
	char *f_in_name = "stdin", *f_out_name = "stdout";

	for (i = 1; i < argc; ++i) {
		if (strncmp("-D", argv[i], 2) == 0) {
			DebugMode = 1;
			sys_call = dbg_system_call;
			break;
		}
	}

	for (i = 1; i < argc; ++i) {
		if (strncmp("-i", argv[i], 2) == 0) {
			f_in_name = argv[i] + 2;
			f_in = task_open_file(f_default_out, f_in_name, RW_READ);
			if (f_in < 0) {
				exit_code = 1;
				break;
			}
		}
		else if (strncmp("-o", argv[i], 2) == 0) {
			f_out_name = argv[i] + 2;
			f_out = task_open_file(f_default_out, f_out_name, RW_WRITE);
			if (f_out < 0) {
				exit_code = 2;
				break;
			}
		}
	}

	if (DebugMode) {
		dbg_print_io_names(f_in_name, f_out_name);
	}
	if (exit_code == 0) {
		exit_code = read_input(f_in, f_out);
	}

	close(f_in);
	close(f_out);
	close(dbg_out);
	return 0;
}
