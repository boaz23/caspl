#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char *msg = "hello";
    int err = 0;
    int pipefd[2];
    int fd_pipe_write, fd_pipe_read;
    pid_t cpid;
    char c;

    err = pipe(pipefd);
    if (err < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    fd_pipe_write = pipefd[1];
    fd_pipe_read = pipefd[0];
    cpid = fork();
    if (cpid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (cpid == 0) {
        // child
        if (close(fd_pipe_read) < 0) {
            perror("close");
            _exit(EXIT_FAILURE);
        }
        if (write(fd_pipe_write, msg, strlen(msg)) < 0) {
            perror("write");
            _exit(EXIT_FAILURE);
        }
        if (close(fd_pipe_write) < 0) {
            perror("close");
            _exit(EXIT_FAILURE);
        }

        _exit(EXIT_SUCCESS);
    }
    else {
        // parent
        
        if (close(fd_pipe_write) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        while (read(fd_pipe_read, &c, 1) > 0) {
            fwrite(&c, sizeof(char), 1, stdout);
        }
        fwrite("\n", sizeof(char), 1, stdout);

        if (close(fd_pipe_read) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        wait(NULL);
        exit(EXIT_SUCCESS);
    }

    return 0;
}