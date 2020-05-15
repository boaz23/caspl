#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

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

pid_t fork_wrapper() {
    pid_t pid = 0;
    dbg_print("(parent_process>forking...)\n");
    pid = fork();
    if (pid > 0) {
        dbg_print("(parent_process>created process with id: )\n", pid);
    }
    return pid;
}

int main(int argc, char *argv[]) {
    int pipefd[2], pipe_fd_write, pipe_fd_read;
    pid_t pid = 0;

    set_dbg_mode_from_args(argc, argv);
    pipe(pipefd);
    pipe_fd_read = pipefd[0];
    pipe_fd_write = pipefd[1];

    pid = fork_wrapper();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // child
        char *args[] = {
            "ls", "-l", NULL
        };
        
        close(pipe_fd_read);

        dbg_print("(child1>redirecting stdout to the write end of the pipe...)\n");
        close(STDOUT_FILENO);
        dup(pipe_fd_write);
        close(pipe_fd_write);

        dbg_print("(child1>going to execute cmd: %s)\n", "ls -l");
        execvp("ls", args);

        perror("child 1 exec");
        _exit(EXIT_FAILURE);
    }
    else {
        // parent
        pid_t pid2 = 0;

        dbg_print("(parent_process>closing the write end of the pipe...)\n");
        close(pipe_fd_write);
        
        pid2 = fork_wrapper();
        if (pid2 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid2 == 0) {
            // child
            char *args[] = {
                "tail", "-n", "2", NULL
            };
   
            dbg_print("(child2>redirecting stdin to the read end of the pipe...)\n");
            close(STDIN_FILENO);
            dup(pipe_fd_read);
            close(pipe_fd_read);

            dbg_print("(child2>going to execute cmd: %s)\n", "tail -n 2");
            execvp("tail", args);
            
            perror("child 2 exec");
            _exit(EXIT_FAILURE);
        }
        else {
            // parent
            int status;
            dbg_print("(parent_process>closing the read end of the pipe...)\n");
            close(pipe_fd_read);

            dbg_print("(parent_process>waiting for child processes to terminate...)\n");
            waitpid(pid, &status, 0);
            waitpid(pid2, &status, 0);
        }
    }

    dbg_print("(parent_process>exiting...)\n");
    return 0;
}