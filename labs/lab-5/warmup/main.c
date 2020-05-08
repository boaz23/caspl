#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "linux/limits.h"
#include "LineParser.h"

#define ARR_LEN(array) (sizeof((array)) / sizeof(*(array)))

typedef enum bool {
    FALSE   = 0,
    TRUE    = 1,
} bool;

#define dbg_out stderr
bool DebugMode = FALSE;

#define LINE_MAX 2048

typedef enum INP_LOOP {
    INP_LOOP_CONTINUE    = 0,
    INP_LOOP_BREAK       = 1,
} INP_LOOP;

typedef INP_LOOP (*predefined_cmd_handler)(cmdLine *pCmdLine);
typedef struct predefined_cmd {
    char *cmd_name;
    predefined_cmd_handler handler;
} predefined_cmd;

INP_LOOP inp_loop_break(cmdLine *pCmdLine) {
    return INP_LOOP_BREAK;
}

INP_LOOP change_working_directory(cmdLine *pCmdLine) {
    int err = 0;
    if (pCmdLine->argCount != 2) {
        printf("too many arguments for 'cd' command\n");
    }
    else {
        err = chdir(pCmdLine->arguments[1]);
        if (err < 0) {
            fprintf(stderr, "error %d occurred when tried changing working directory\n", err);
        }
    }

    return INP_LOOP_CONTINUE;
}

predefined_cmd predefined_commands[] = {
    { "quit",   inp_loop_break },
    { "cd",     change_working_directory },
};

predefined_cmd* find_predefined_cmd(char *cmd_name) {
    predefined_cmd *cmd = predefined_commands;
    for (int i = 0; i < ARR_LEN(predefined_commands); ++i, ++cmd) {
        if (strcmp(cmd_name, cmd->cmd_name) == 0) {
            return cmd;
        }
    }

    return NULL;
}

void dbg_print_exec_info(pid_t pid, cmdLine *pCmdLine) {
    fprintf(dbg_out, "pid: %d, exec cmd:", pid);
    for (int i = 0; i < pCmdLine->argCount; ++i) {
        fprintf(dbg_out, " %s", pCmdLine->arguments[i]);
    }
    fprintf(dbg_out, "\n");
}

char* cmd_get_path(cmdLine *pCmdLine) {
    return pCmdLine->arguments[0];
}

void execute(cmdLine *pCmdLine) {
    pid_t pid = fork();
    if (pid != 0 && DebugMode) {
        dbg_print_exec_info(pid, pCmdLine);
    }
    
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // child, executes the command
        execvp(cmd_get_path(pCmdLine), pCmdLine->arguments);
        perror("execv");
        _exit(EXIT_FAILURE);
    }
    else {
        // parent, actual terminal
        if (pCmdLine->blocking) {
            // wait for the process to finish
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

INP_LOOP do_cmd(cmdLine *pCmdLine) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
    if (predef_cmd) {
        inp_loop = predef_cmd->handler(pCmdLine);
    }
    else {
        execute(pCmdLine);
    }

    return inp_loop;
}

INP_LOOP do_cmd_from_input(char *buf) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    cmdLine *pCmdLine = NULL;

    pCmdLine = parseCmdLines(buf);
    if (pCmdLine == NULL) {
        printf("error parsing the command\n");
    }
    else {
        inp_loop = do_cmd(pCmdLine);
        freeCmdLines(pCmdLine);
    }

    return inp_loop;
}

void set_dbg_mode_from_args(int argc, char *argv[]) {
    for (int i = 0; i < argc; ++i) {
        if (strncmp("-d", argv[i], 2) == 0) {
            DebugMode = TRUE;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    
    set_dbg_mode_from_args(argc, argv);

    while (!inp_loop) {
        char buf[LINE_MAX];
        char *res_input = fgets(buf, ARR_LEN(buf), stdin);
        if (feof(stdin)) {
            break;
        }
        if (res_input == NULL) {
            printf("error inputing line");
            continue;
        }

        inp_loop = do_cmd_from_input(buf);
    }
    
    return EXIT_SUCCESS;
}