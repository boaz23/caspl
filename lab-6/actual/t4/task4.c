#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
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

typedef struct var_link var_link;
typedef struct var_link {
    char *name;
    char *value;
    var_link *next;
} var_link;

typedef enum INP_LOOP {
    INP_LOOP_CONTINUE    = 0,
    INP_LOOP_BREAK       = 1,
} INP_LOOP;

typedef INP_LOOP (*predefined_cmd_handler)(cmdLine *pCmdLine, var_link **var_list);
typedef struct predefined_cmd {
    char *cmd_name;
    predefined_cmd_handler handler;
} predefined_cmd;

void print_line_separator() {
    printf("\n");
}

bool non_dbg_print(char const *s, ...) {
    if (!DebugMode) {
        va_list args;
        va_start(args, s);
        vfprintf(dbg_out, s, args);
        va_end(args);
        return TRUE;
    }
    
    return FALSE;
}

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

char* str_clone(char *s) {
    char *clone = NULL;
    if (s) {
        clone = strcpy(malloc(strlen(s) + 1), s);
    }

    return clone;
}

char* cmd_get_path(cmdLine *pCmdLine) {
    return pCmdLine->arguments[0];
}

void print_cmd(FILE *file, cmdLine *pCmdLine) {
    for (int i = 0; i < pCmdLine->argCount; ++i) {
        fprintf(file, " %s", pCmdLine->arguments[i]);
    }
}

void var_link_free(var_link *v) {
    if (v) {
        free(v->name);
        free(v->value);
        free(v);
    }
}

void var_list_free(var_link *var_list) {
    var_link *current = var_list;
    while (current) {
        var_link *next = current->next;
        var_link_free(current);
        current = next;
    }
}

var_link* var_link_create(char *name, char *value, var_link *next) {
    var_link *v = malloc(sizeof(var_link));
    if (!v) {
        dbg_print_error("malloc");
        exit(EXIT_FAILURE);
    }

    v->name = name;
    v->value = value;
    v->next = next;
    return v;
}

var_link* var_list_find_var_by_name(var_link *list, char *name) {
    var_link *current = list;
    while (current) {
        if (strcmp(name, current->name) == 0) {
            break;
        }
        current = current->next;
    }
    return current;
}

void set_var(char *name, char *value, var_link **var_list) {
    var_link *var = var_list_find_var_by_name(*var_list, name);
    value = str_clone(value);
    if (var) {
        free(var->value);
        var->value = value;
    }
    else {
        name = str_clone(name);
        *var_list = var_link_create(name, value, *var_list);
    }
}

void print_vars_list(var_link *var_list) {
    var_link *current = var_list;
    while (current) {
        printf("%s: %s\n", current->name, current->value);
        current = current->next;
    }
}

INP_LOOP inp_loop_break(cmdLine *pCmdLine, var_link **var_list) {
    return INP_LOOP_BREAK;
}

INP_LOOP change_working_directory(cmdLine *pCmdLine, var_link **var_list) {
    int err = 0;
    if (pCmdLine->argCount == 1) {
        // do nothing
    }
    else if (pCmdLine->argCount == 2) {
        err = chdir(pCmdLine->arguments[1]);
        if (err < 0) {
            if (!dbg_print_error("cd")) {
                perror("cd");
            }
        }
    }
    else {
        printf("cd: too many arguments\n");
    }

    return INP_LOOP_CONTINUE;
}

void set_var_wrapper(cmdLine *pCmdLine, var_link **var_list) {
    if (pCmdLine->argCount < 3) {
        printf("set: too few arguments\n");
        return;
    }
    if (pCmdLine->argCount > 3) {
        printf("set: value with spaces is unsupported\n");
        return;
    }

    set_var(pCmdLine->arguments[1], pCmdLine->arguments[2], var_list);
}

INP_LOOP set_var_cmd(cmdLine *pCmdLine, var_link **var_list) {
    set_var_wrapper(pCmdLine, var_list);
    return INP_LOOP_CONTINUE;
}

INP_LOOP print_vars_cmd(cmdLine *pCmdLine, var_link **var_list) {
    print_vars_list(*var_list);
    return INP_LOOP_CONTINUE;
}

predefined_cmd predefined_commands[] = {
    { "quit",   inp_loop_break, },
    { "cd",     change_working_directory, },
    { "set",    set_var_cmd, },
    { "vars",    print_vars_cmd, },
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

void child_dup(int fd) {
    if (dup(fd) < 0) {
        dbg_print_error("dup");
        _exit(EXIT_FAILURE);
    }
}

void child_close_file(int fd) {
    if (close(fd) < 0) {
        dbg_print_error("close");
        _exit(EXIT_FAILURE);
    }
}

void child_pipe_write(int pipe_fd_read, int pipe_fd_write) {
    child_close_file(pipe_fd_read);
    child_close_file(STDOUT_FILENO);
    child_dup(pipe_fd_write);
    child_close_file(pipe_fd_write);
}

void child_pipe_read(int pipe_fd_read) {
    child_close_file(STDIN_FILENO);
    child_dup(pipe_fd_read);
    child_close_file(pipe_fd_read);
}

void child_open_read_file(char const *file_name) {
    if (fopen(file_name, "r") < 0) {
        dbg_print_error("fopen");
        _exit(EXIT_FAILURE);
    }
}

void child_open_write_file(char const *file_name) {
    if (fopen(file_name, "w") < 0) {
        dbg_print_error("fopen");
        _exit(EXIT_FAILURE);
    }
}

void child_redirect_input_from_file(char const *file_name) {
    if (file_name) {
        child_close_file(STDIN_FILENO);
        child_open_read_file(file_name);
    }
}

void child_redirect_output_to_file(char const *file_name) {
    if (file_name) {
        child_close_file(STDOUT_FILENO);
        child_open_write_file(file_name);
    }
}

void child_redirect_io_to_files(cmdLine *pCmdLine) {
    child_redirect_input_from_file(pCmdLine->inputRedirect);
    child_redirect_output_to_file(pCmdLine->outputRedirect);
}

void parent_fork_failed(cmdLine *pCmdLine, var_link **var_list) {
    if (!dbg_print_error("fork")) {
        printf("fork error, exiting...\n");
    }
    freeCmdLines(pCmdLine);
    var_list_free(*var_list);
    exit(EXIT_FAILURE);
}

void child_exec(cmdLine *pCmdLine) {
    execvp(cmd_get_path(pCmdLine), pCmdLine->arguments);
    if (!dbg_print_error("execv")) {
        printf("execv error, exiting...\n");
    }
    _exit(EXIT_FAILURE);
}

bool wait_for_child(pid_t pid) {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err < 0) {
        dbg_print_error("waitpid");
        return FALSE;
    }
    else {
        // child terminated
        return TRUE;
    }
}

bool wait_for_child_if_blocking(pid_t pid, cmdLine *pCmdLine) {
    if (pCmdLine->blocking) {
        return wait_for_child(pid);
    }
    
    return TRUE;
}

bool dbg_print_exec_info(pid_t pid, cmdLine *pCmdLine) {
    if (DebugMode) {
        dbg_print("pid: %d, exec cmd:", pid);
        print_cmd(dbg_out, pCmdLine);
        dbg_print("\n");
        return TRUE;
    }

    return FALSE;
}

pid_t parent_fork(cmdLine *pCmdLine) {
    pid_t pid = fork();
    if (pid > 0 && !dbg_print_exec_info(pid, pCmdLine) && !pCmdLine->blocking) {
        non_dbg_print("pid: %d\n", pid);
    }
    return pid;
}

void execute_single(cmdLine *pCmdLine, var_link **var_list) {
    pid_t pid = parent_fork(pCmdLine);
    if (pid < 0) {
        // fork failed
        parent_fork_failed(pCmdLine, var_list);
    }
    else if (pid == 0) {
        // child, executes the command
        child_redirect_io_to_files(pCmdLine);
        child_exec(pCmdLine);
    }
    else {
        // parent, actual terminal
        wait_for_child_if_blocking(pid, pCmdLine);
    }
}

void execute_pipeline(cmdLine *pCmdLine, var_link **var_list) {
    int pipe_fd[2], pipe_fd_read, pipe_fd_write;
    pid_t pid1, pid2;
    cmdLine *cmd1 = pCmdLine, *cmd2 = pCmdLine->next;
    
    parent_pipe(pipe_fd);
    pid1 = parent_fork(cmd1);
    if (pid1 < 0) {
        // fork failed
        parent_fork_failed(pCmdLine, var_list);
    }
    else if (pid1 == 0) {
        // child 1, executes the first command
        child_pipe_write(pipe_fd_read, pipe_fd_write);
        child_exec(cmd1);
    }
    else {
        parent_close(pipe_fd_write);
        pid2 = parent_fork(cmd2);
        if (pid2 < 0) {
            // fork failed
            parent_fork_failed(pCmdLine, var_list);
        }
        else if (pid2 == 0) {
            // child 2, executes the second command
            child_pipe_read(pipe_fd_read);
            child_exec(cmd2);
        }
        else {
            parent_close(pipe_fd_read);
            wait_for_child(pid1);
            wait_for_child_if_blocking(pid2, cmd2);
        }
    }
}

#define should_redirect_output_to_pipe(cmd) (cmd)->next

void execute(cmdLine *pCmdLine, var_link **var_list) {
    if (should_redirect_output_to_pipe(pCmdLine)) {
        if (should_redirect_output_to_pipe(pCmdLine->next)) {
            printf("multiple pipes is not supported\n");
            return;
        }

        execute_pipeline(pCmdLine, var_list);
    }
    else {
        execute_single(pCmdLine, var_list);
    }
}

#define is_user_var(arg) (arg)[0] == '$'

char* expand_var(char *arg, var_link *var_list) {
    char *new_value = NULL;
    char *var_name = arg + 1;
    var_link *var = var_list_find_var_by_name(var_list, var_name);
    if (var) {
        new_value = var->value;
    }
    else {
        dbg_print("usage of undefined variable %s\n", var_name);
    }

    return new_value;
}

char* expand_cmd_arg(int i, char *cmd_arg, var_link *var_list) {
    char *new_value = NULL;
    if (strcmp("~", cmd_arg) == 0) {
        new_value = getenv("HOME");
    }
    else if (is_user_var(cmd_arg)) {
        new_value = expand_var(cmd_arg, var_list);
    }
    return new_value;
}

void expand_cmd_vars(cmdLine *cmd, var_link *var_list) {
    for (int i = 0; i < cmd->argCount; ++i) {
        char *new_value = expand_cmd_arg(i, cmd->arguments[i], var_list);
        if (new_value) {
            replaceCmdArg(cmd, i, new_value);
        }
    }
}

void expand_cmd_line_vars(cmdLine *pCmdLine, var_link *var_list) {
    cmdLine *cmd = pCmdLine;
    while (cmd) {
        expand_cmd_vars(cmd, var_list);
        cmd = cmd->next;
    }
}

INP_LOOP do_cmd(cmdLine *pCmdLine, var_link **var_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
    expand_cmd_line_vars(pCmdLine, *var_list);
    if (predef_cmd) {
        inp_loop = predef_cmd->handler(pCmdLine, var_list);
    }
    else {
        execute(pCmdLine, var_list);
    }

    return inp_loop;
}

INP_LOOP do_cmd_from_input(char *buf, var_link **var_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    cmdLine *pCmdLine = NULL;

    pCmdLine = parseCmdLines(buf);
    if (pCmdLine == NULL) {
        printf("error parsing the command\n");
    }
    else {
        inp_loop = do_cmd(pCmdLine, var_list);
    }
    freeCmdLines(pCmdLine);

    return inp_loop;
}

void do_input_loop(var_link **var_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    while (!inp_loop) {
        char buf[LINE_MAX];
        char *res_input = fgets(buf, ARR_LEN(buf), stdin);
        if (feof(stdin)) {
            break;
        }
        if (res_input == NULL) {
            printf("error inputing line\n");
            continue;
        }

        inp_loop = do_cmd_from_input(buf, var_list);
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
    var_link *var_list = NULL;
    set_dbg_mode_from_args(argc, argv);
    do_input_loop(&var_list);
    var_list_free(var_list);
    return EXIT_SUCCESS;
}