#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

typedef enum bool {
    FALSE   = 0,
    TRUE    = 1,
} bool;

FILE *dbg_out	= NULL;
bool DebugMode	= FALSE;

bool dbg_print(char const *s, ...) {
    if (DebugMode) {
        va_list args;
        va_start(args, s);
        vfprintf(dbg_out, s, args);
        va_end(args);
        return TRUE;
    }
    
    return FALSE;
}
bool dbg_print_error(char const *err) {
    if (DebugMode) {
        fprintf(dbg_out, "%s: %s\n", err, strerror(errno));
        return TRUE;
    }

    return FALSE;
}

void set_dbg_mode_from_args(int argc, char *argv[]) {
    dbg_out = stderr;
    for (int i = 0; i < argc; ++i) {
        if (strncmp("-d", argv[i], 2) == 0) {
            DebugMode = TRUE;
            break;
        }
    }
}

pid_t parent_fork() {
    pid_t pid = 0;
    dbg_print("(parent_process>forking...)\n");
    pid = fork();
    if (pid > 0) {
        dbg_print("(parent_process>created process with id: %d)\n", pid);
    }
    return pid;
}

void child_close_file(int fd) {
    if (close(fd) < 0) {
        dbg_print_error("close");
        _exit(EXIT_FAILURE);
    }
}

void child_dup(int fd) {
    if (dup(fd) < 0) {
        dbg_print_error("dup");
        _exit(EXIT_FAILURE);
    }
}

void child_pipe_write(int pipe_fd_read, int pipe_fd_write) {
    child_close_file(pipe_fd_read);

    dbg_print("(child1>redirecting stdout to the write end of the pipe...)\n");
    child_close_file(STDOUT_FILENO);
    child_dup(pipe_fd_write);
    child_close_file(pipe_fd_write);
}

void child_pipe_read(int pipe_fd_read) {
    child_close_file(STDIN_FILENO);
    child_dup(pipe_fd_read);
    child_close_file(pipe_fd_read);
}

#define parent_close(fd) \
    if (close((fd)) < 0) {\
        dbg_print_error("close");\
        return;\
    }
#define parent_pipe(pipe_fd) \
    if (pipe((pipe_fd)) < 0) {\
        dbg_print_error("pipe");\
        return;\
    }\
    pipe_fd_read = pipe_fd[0];\
    pipe_fd_write = pipe_fd[1];

void execute_pipeline() {
    int pipe_fd[2], pipe_fd_read, pipe_fd_write;
    pid_t pid, pid2;
    char *args1[] = { "ls", "-l", NULL };
    char *args2[] = { "tail", "-n", "2", NULL };

    parent_pipe(pipe_fd);
    pid = parent_fork();
    if (pid < 0) {
        dbg_print_error("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        child_pipe_write(pipe_fd_read, pipe_fd_write);

        dbg_print("(child1>going to execute cmd: ls -l)\n");
        execvp(args1[0], args1);
        dbg_print_error("execvp");
        _exit(EXIT_FAILURE);
    }
    else {
        dbg_print("(parent_process>closing the write end of the pipe...)\n");
        parent_close(pipe_fd_write);

        pid2 = parent_fork();
        if (pid2 < 0) {
            dbg_print_error("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid2 == 0) {
            dbg_print("(child2>redirecting stdin to the read end of the pipe...)\n");
            child_pipe_read(pipe_fd_read);

            dbg_print("(child2>going to execute cmd: tail -n 2)\n");
            execvp(args2[0], args2);
            dbg_print_error("execvp");
            _exit(EXIT_FAILURE);
        }
        else {
            dbg_print("(parent_process>closing the read end of the pipe...)\n");
            parent_close(pipe_fd_read);

            dbg_print("(parent_process>waiting for child processes to terminate...)\n");
            waitpid(pid, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }

    dbg_print("(parent_process>exiting...)\n");
}

int main(int argc, char *argv[]) {
    set_dbg_mode_from_args(argc, argv);
    execute_pipeline();
    return 0;
}
