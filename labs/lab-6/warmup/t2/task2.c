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

char* clone_var_name(char *name) {
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

void set_var_wrapper(cmdLine *pCmdLine, var_link **var_list) {
    char *expand_name = NULL;
    char *value = NULL;
    if (pCmdLine->argCount < 3) {
        printf("set: expected at least 2 arguments\n");
        return;
    }

    expand_name = clone_var_name(pCmdLine->arguments[1]);
    value = str_arr_join(pCmdLine->argCount - 2, pCmdLine->arguments + 2, " ");
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
    set_var_wrapper(pCmdLine, var_list);
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

void parent_failed_fork(pid_t pid, cmdLine *pCmdLine) {
    if (!dbg_print_error("fork")) {
        printf("fork error, exiting...\n");
    }
    freeCmdLines(pCmdLine);
    // FREE processes list
    exit(EXIT_FAILURE);
}

bool redirect_io(cmdLine *pCmdLine) {
    if (pCmdLine->inputRedirect) {
        if (close(STDIN_FILENO) < 0 ||
            !fopen(pCmdLine->inputRedirect, "r"))
        {
            printf("input redirection failed.\n");
            return FALSE;
        }
    }
    if (pCmdLine->outputRedirect) {
        if (close(STDOUT_FILENO) < 0 ||
            !fopen(pCmdLine->outputRedirect, "w"))
        {
            printf("input redirection failed.\n");
            return FALSE;
        }
    }

    return TRUE;
}

void child_do_exec(cmdLine *pCmdLine) {
    if (redirect_io(pCmdLine)) {
        execvp(cmd_get_path(pCmdLine), pCmdLine->arguments);
        if (!dbg_print_error("execv")) {
            printf("execv error, exiting...\n");
        }
    }
    _exit(EXIT_FAILURE);
}

void wait_for_child(pid_t pid) {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err < 0) {
        if (errno == ECHILD) {
            // for some reason, child is not waitable
        }
        else {
            dbg_print_error("waitpid");
        }
    }
    else {
        // child terminated, updating it's status to terminated will be checked by another function
    }
}

void parent_post_fork(pid_t pid, cmdLine *pCmdLine) {
    // ADD process to list
    if (pCmdLine->blocking) {
        wait_for_child(pid);
    }
    else {
        non_dbg_print("pid: %d\n", pid);
    }
}

void dbg_print_exec_info(pid_t pid, cmdLine *pCmdLine) {
    dbg_print("pid: %d, exec cmd:", pid);
    print_cmd(dbg_out, pCmdLine);
    dbg_print("\n");
}

void expand_variables(cmdLine *pCmdLine, var_link **var_list) {
    for (int i = 1; i < pCmdLine->argCount; ++i) {
        var_link **p_var = var_list_address_of_var(var_list, pCmdLine->arguments[i]);
        var_link *var = *p_var;
        if (var) {
            replaceCmdArg(pCmdLine, i, var->value);
        }
    }
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

INP_LOOP do_cmd(cmdLine *pCmdLine, var_link **var_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
    expand_variables(pCmdLine, var_list);
    if (predef_cmd) {
        inp_loop = predef_cmd->handler(pCmdLine, var_list);
    }
    else {
        execute(pCmdLine);
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