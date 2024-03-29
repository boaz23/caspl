#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "linux/limits.h"
#include "LineParser.h"

#define ARR_LEN(array) (sizeof((array)) / sizeof(*(array)))

typedef enum bool {
    FALSE   = 0,
    TRUE    = 1,
} bool;

FILE *dbg_out	= NULL;
bool DebugMode	= FALSE;

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

bool dbg_print_error(char *err) {
    if (DebugMode) {
        perror(err);
        return TRUE;
    }

    return FALSE;
}

char* cmd_get_path(cmdLine *pCmdLine) {
    return pCmdLine->arguments[0];
}

void print_cmd(FILE *file, cmdLine *pCmdLine) {
    for (int i = 0; i < pCmdLine->argCount; ++i) {
        fprintf(file, " %s", pCmdLine->arguments[i]);
    }
}

INP_LOOP inp_loop_break(cmdLine *pCmdLine) {
    return INP_LOOP_BREAK;
}

INP_LOOP change_working_directory(cmdLine *pCmdLine) {
    int err = 0;
    if (pCmdLine->argCount == 1) {
        // do nothing
    }
    else if (pCmdLine->argCount == 2) {
        err = chdir(pCmdLine->arguments[1]);
        if (err < 0) {
            perror("cd");
        }
    }
    else {
        printf("cd: too many arguments\n");
    }

    return INP_LOOP_CONTINUE;
}

predefined_cmd predefined_commands[] = {
    { "quit",		inp_loop_break, },
    { "cd",			change_working_directory, },
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

void parent_failed_fork(pid_t pid, cmdLine *pCmdLine) {
    if (!dbg_print_error("fork")) {
        printf("fork error, exiting...\n");
    }
    // FREE process list here
    exit(EXIT_FAILURE);
}

void child_do_exec(cmdLine *pCmdLine) {
    execvp(cmd_get_path(pCmdLine), pCmdLine->arguments);
    if (!dbg_print_error("execv")) {
        printf("execv error, exiting...\n");
    }
    _exit(EXIT_FAILURE);
}

void wait_for_child(pid_t pid) {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err < 0) {
        perror("waitpid");
    }
    else {
        // child terminated
    }
}

void parent_post_fork(pid_t pid, cmdLine *pCmdLine) {
    // ADD process here
    if (pCmdLine->blocking) {
        wait_for_child(pid);
    }
    else {
        if (!DebugMode) {
            printf("pid: %d\n", pid);
        }
    }
}

void dbg_print_exec_info(pid_t pid, cmdLine *pCmdLine) {
    fprintf(dbg_out, "pid: %d, exec cmd:", pid);
    print_cmd(dbg_out, pCmdLine);
    fprintf(dbg_out, "\n");
}

void execute(cmdLine *pCmdLine) {
    pid_t pid = fork();
    if (pid != 0 && DebugMode) {
        dbg_print_exec_info(pid, pCmdLine);
    }
    
    if (pid < 0) {
        // fork failed
        parent_failed_fork(pid, pCmdLine);
    }
    else if (pid == 0) {
        // child, executes the command
        child_do_exec(pCmdLine);
    }
    else {
        // parent, actual terminal
        parent_post_fork(pid, pCmdLine);
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
        // REMOVE descruction of the cmdLine object
        freeCmdLines(pCmdLine);
    }

    return inp_loop;
}

void do_input_loop() {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
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

int main(int argc, char *argv[]) {
    set_dbg_mode_from_args(argc, argv);
    do_input_loop();
    // FREE process list
    return EXIT_SUCCESS;
}