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
    char *expand_name;
    char *display_name;
    char *value;
    bool shouldFree;
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

void free_var_link(var_link *var) {
    if (var && var->shouldFree) {
        free(var->expand_name);
        free(var->value);
        free(var);
    }
}

void free_var_list(var_link *list) {
    var_link *current = list;
    while (current) {
        var_link *next = current->next;
        free_var_link(current);
        current = next;
    }
}

var_link* make_var_link(char *expand_name, char *name, char *value, bool shouldFree, var_link *next) {
    var_link *var = (var_link *)malloc(sizeof(var_link));
    var->expand_name = expand_name;
    var->display_name = name;
    var->value = value;
    var->shouldFree = shouldFree;
    var->next = next;
    return var;
}

var_link** var_list_address_of_var(var_link **list, char *expand_name) {
    var_link **p_current = list;
    var_link *current;
    while ((current = *p_current)) {
        if (strcmp(expand_name, current->expand_name) == 0) {
            break;
        }

        p_current = &(current->next);
    }

    return p_current;
}

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

char* cmd_get_path(cmdLine *pCmdLine) {
    return pCmdLine->arguments[0];
}

void print_cmd(FILE *file, cmdLine *pCmdLine) {
    for (int i = 0; i < pCmdLine->argCount; ++i) {
        fprintf(file, " %s", pCmdLine->arguments[i]);
    }
}

cmdLine* cmd_line_list_last(cmdLine *pCmdLine) {
    if (!pCmdLine) {
        return NULL;
    }
    
    cmdLine *current = pCmdLine;
    while (current->next) {
        current = current->next;
    }
    return current;
}

char* get_expand_var_name(char *name) {
    char *clone = NULL;
    if (name) {
        clone = (char *)malloc(strlen(name) + 2);
        clone[0] = '$';
        strcpy(clone + 1, name);
    }

    return clone;
}

int calc_str_arr_join_len(int count, char* const s_arr[], char const *separator) {
    int len = 1;
    len += strlen(separator) * (count - 1);
    for (int i = 0; i < count; ++i) {
        len += strlen(s_arr[i]);
    }
    return len;
}

char* str_cpy_adv(char *dest, char const *src) {
    strcpy(dest, src);
    dest += strlen(src);
    return dest;
}

void str_arr_join_cpy(char *dest, int count, char* const s_arr[], char const *separator) {
    dest = str_cpy_adv(dest, s_arr[0]);
    for (int i = 1; i < count; ++i) {
        dest = str_cpy_adv(dest, separator);
        dest = str_cpy_adv(dest, s_arr[i]);
    }
}

char* str_arr_join(int count, char* const s_arr[], char const *separator) {
    char *s = NULL;
    if (count > 0) {
        int len = calc_str_arr_join_len(count, s_arr, separator);
        s = (char *)malloc(len * sizeof(char));
        str_arr_join_cpy(s, count, s_arr, separator);
        return s;
    }
    else if (count == 0) {
        s = (char *)malloc(sizeof(char));
        s[0] = '\0';
    }

    return s;
}

bool set_var(var_link **var_list, char *expand_name, char *value) {
    var_link **p_var = var_list_address_of_var(var_list, expand_name);
    var_link *var = *p_var;
    if (var) {
        free(var->value);
        var->value = value;
        return FALSE;
    }
    else {
        *p_var = make_var_link(expand_name, expand_name + 1, value, TRUE, NULL);
        return TRUE;
    }
}

void set_var_wrapper(char *name, int value_count, char *values[], var_link **var_list) {
    char *expand_name = get_expand_var_name(name);
    char *value = str_arr_join(value_count, values, " ");
    if (!set_var(var_list, expand_name, value)) {
        free(expand_name);
    }
}

void print_vars(var_link *var_list) {
    var_link *current = var_list;
    while (current) {
        printf("%s: %s\n", current->display_name, current->value);
        current = current->next;
    }
}

INP_LOOP set_var_cmd(cmdLine *pCmdLine, var_link **var_list) {
    if (pCmdLine->argCount < 3) {
        printf("set: expected at least 2 arguments\n");
        return;
    }

    set_var_wrapper(
        pCmdLine->arguments[1],
        pCmdLine->argCount - 2,
        pCmdLine->arguments + 2,
        var_list
    );
    return INP_LOOP_CONTINUE;
}

INP_LOOP print_vars_cmd(cmdLine *pCmdLine, var_link **var_list) {
    print_vars(*var_list);
    return INP_LOOP_CONTINUE;
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

predefined_cmd predefined_commands[] = {
    { "quit",   inp_loop_break, },
    { "cd",     change_working_directory, },
    { "set",    set_var_cmd, },
    { "vars",   print_vars_cmd }
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

bool parent_close(int fd) {
    if (close(fd) < 0 && !dbg_print_error("close")) {
        printf("an error has occurred\n");
        return FALSE;
    }

    return TRUE;
}

bool parent_pipe(int pipefd[2]) {
    if (pipe(pipefd) < 0 && !dbg_print_error("pipe")) {
        printf("an error has occurred\n");
        return FALSE;
    }

    return TRUE;
}

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
    free_var_list(*var_list);
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
    // ADD process to list
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

pid_t fork_with_print(cmdLine *pCmdLine) {
    pid_t pid = fork();
    if (pid > 0 && !dbg_print_exec_info(pid, pCmdLine) && !pCmdLine->blocking) {
        non_dbg_print("pid: %d\n", pid);
    }
    return pid;
}

void execute_single(cmdLine *pCmdLine, var_link **var_list) {
    pid_t pid = fork_with_print(pCmdLine);
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

int cmd_line_list_length(cmdLine *last) {
    return last->idx + 1;
}

void child_read_handle_pipeline_files(int pipe_fd_read) {
    child_close_file(STDIN_FILENO);
    child_dup(pipe_fd_read);
    child_close_file(pipe_fd_read);
}

void child_write_handle_pipeline_files(int pipe_fd_read, int pipe_fd_write) {
    child_close_file(pipe_fd_read);
    child_close_file(STDOUT_FILENO);
    child_dup(pipe_fd_write);
    child_close_file(pipe_fd_write);
}

void execute_pipeline(cmdLine *pCmdLine, var_link **var_list) {
    int pipefd[2], pipe_fd_write, pipe_fd_read;
    pid_t pid = 0;

    cmdLine *last_cmd = cmd_line_list_last(pCmdLine);
    int cmd_list_length = cmd_line_list_length(last_cmd);

    cmdLine *cmd_next = pCmdLine->next;
    pid_t pid2 = 0;

    if (cmd_list_length > 2) {
        printf("Terminal only supports a single pipe\n");
        return;
    }

    if (!parent_pipe(pipefd)) {
        return;
    }
    pipe_fd_read = pipefd[0];
    pipe_fd_write = pipefd[1];

    pid = fork_with_print(pCmdLine);
    if (pid < 0) {
        // fork failed
        // NOTE: we probably should kill all created child process, but ehh...
        parent_fork_failed(pCmdLine, var_list);
    }
    else if (pid == 0) {
        // child, executes the command
        child_write_handle_pipeline_files(pipe_fd_read, pipe_fd_write);
        child_exec(pCmdLine);
    }
    else {
        if (!parent_close(pipe_fd_write)) {
            return;
        }

        pid2 = fork_with_print(cmd_next);
        if (pid2 < 0) {
            // fork failed
            // NOTE: we probably should kill all created child process, but ehh...
            parent_fork_failed(cmd_next, var_list);
        }
        else if (pid2 == 0) {
            child_read_handle_pipeline_files(pipe_fd_read);
            child_exec(cmd_next);
        }
        else {
            parent_close(pipe_fd_read);
            wait_for_child(pid);
            wait_for_child_if_blocking(pid2, pCmdLine);
        }
    }
}

bool cmd_line_has_pipe(cmdLine *pCmdLine) {
    return !!pCmdLine->next;
}

void execute(cmdLine *pCmdLine, var_link **var_list) {
    if (cmd_line_has_pipe(pCmdLine)) {
        execute_pipeline(pCmdLine, var_list);
    }
    else {
        execute_single(pCmdLine, var_list);
    }
}

bool isUserVariable(char *expand_name) {
    return expand_name[0] == '$';
}

void expand_variables(cmdLine *pCmdLine, var_link **var_list) {
    for (int i = 1; i < pCmdLine->argCount; ++i) {
        var_link **p_var = var_list_address_of_var(var_list, pCmdLine->arguments[i]);
        var_link *var = *p_var;
        if (var) {
            replaceCmdArg(pCmdLine, i, var->value);
        }
        else if (isUserVariable(pCmdLine->arguments[i])) {
            dbg_print("tried expanding undefined variable '%s'\n", pCmdLine->arguments[i] + 1);
        }
    }
}

void cmd_list_expand_variables(cmdLine *pCmdLine, var_link **var_list) {
    cmdLine *current = pCmdLine;
    while (current) {
        expand_variables(pCmdLine, var_list);
        current = current->next;
    }
}

INP_LOOP do_cmd(cmdLine *pCmdLine, var_link **var_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
    cmd_list_expand_variables(pCmdLine, var_list);
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
    var_link predefined_vars[] = {
        { "~", "~", getenv("HOME"), FALSE, NULL }
    };
    var_link *var_list = predefined_vars;
    set_dbg_mode_from_args(argc, argv);
    do_input_loop(&var_list);
    free_var_list(var_list);
    return EXIT_SUCCESS;
}