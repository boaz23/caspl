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

typedef struct process process;

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

typedef INP_LOOP (*predefined_cmd_handler)(cmdLine *pCmdLine, process **process_list);
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

char* cmd_get_path(cmdLine *pCmdLine) {
    return pCmdLine->arguments[0];
}

void print_cmd(FILE *file, cmdLine *pCmdLine) {
    for (int i = 0; i < pCmdLine->argCount; ++i) {
        fprintf(file, " %s", pCmdLine->arguments[i]);
    }
}

#define TERMINATED  -1
#define RUNNING     1
#define SUSPENDED   0

#define UNCHANGED   2
#define NO_CHILD    3
#define STATUS_ERR  4

typedef struct process {
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

char *process_status_string(int status) {
    return  (status == TERMINATED)   ? "Terminated"  :
            (status == RUNNING)      ? "Running"     :
            (status == SUSPENDED)    ? "Suspended"   :
            NULL;
}
char *process_status_string_p(process *p) {
    return process_status_string(p->status);
}

bool send_signal_to_process(pid_t pid, int sig) {
    int err = kill(pid, sig);
    if (err < 0) {
        dbg_print_error("kill");
        return FALSE;
    }

    return TRUE;
}

int convert_wait_status(int status) {
    if (WIFEXITED(status)) {
        return TERMINATED;
    }
    else if (WIFSIGNALED(status)) {
        return TERMINATED;
    }
    else if (WIFSTOPPED(status)) {
        return SUSPENDED;
    }
    else if (WIFCONTINUED(status)) {
        return RUNNING;
    }
    else {
        return UNCHANGED;
    }
}

int check_process_state(pid_t pid) {
    int status;
    int state_change = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (state_change < 0) {
        if (errno == ECHILD) {
            return NO_CHILD;
        }
        dbg_print_error("waitpid");
        return STATUS_ERR;
    }
    else if (state_change == 0) {
        // process state hasn't changed, do nothing
        return UNCHANGED;
    }
    else {
        // process state changed, update it's status accordingly
        return convert_wait_status(status);
    }
}

bool terminate_process_from_pid_and_status(pid_t pid, int status) {
    if (status == TERMINATED) {
        // do nothing
        return TRUE;
    }
    else if (status == RUNNING) {
        return send_signal_to_process(pid, SIGINT);
    }
    else if (status == SUSPENDED) {
        if (send_signal_to_process(pid, SIGINT) &&
            send_signal_to_process(pid, SIGCONT)) {
            // we sent all the relevant signals, no need to do anything further
        }
        else {
            // we failed to terminate it
            return FALSE;
        }
    }

    return FALSE;
}

bool check_process_state_and_update_status(process *p) {
    if (p->status != TERMINATED) {
        int status = check_process_state(p->pid);
        switch (status) {
            case STATUS_ERR:
                break;
            
            case UNCHANGED:
                return TRUE;
            
            case NO_CHILD:
                p->status = TERMINATED;
                return TRUE;

            default:
                p->status = status;
                return TRUE;
        }
    }

    return FALSE;
}

bool terminateProcess(process *p) {
    if (p) {
        if (check_process_state_and_update_status(p)) {
            return terminate_process_from_pid_and_status(p->pid, p->status);
        }
    }
    else {
        dbg_print("DEBUG: tried terminating a NULL process\n");
    }

    return FALSE;
}

void freeProcess(process *p) {
    if (p) {
        terminateProcess(p);
        freeCmdLines(p->cmd);
        free(p);
    }
    else {
        dbg_print("DEBUG: tried freeing a NULL process\n");
    }
}

void freeProcessList(process* process_list) {
    process *next = NULL;
    for (process *current = process_list; current; current = next) {
        next = current->next;
        freeProcess(current);
    }
}

process *process_create(pid_t pid, cmdLine *cmd, int status, process *next) {
    process *p = (process *)malloc(sizeof(process));
    p->pid      = pid;
    p->cmd      = cmd;
    p->status   = status;
    p->next     = next;
    return p;
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    *process_list = process_create(pid, cmd, RUNNING, *process_list);
}

process** addr_of_process_by_pid(process** process_list, int pid) {
    process *current = NULL;
    process **p_current = process_list;
    while ((current = *p_current)) {
        if (current->pid == pid) {
            return p_current;
        }

        p_current = &(current->next);
    }

    return NULL;
}

process* find_process_by_pid(process* process_list, int pid) {
    process **p_p = addr_of_process_by_pid(&process_list, pid);
    if (p_p) {
        return *p_p;
    }

    return NULL;
}

void remove_process(process **p_p) {
    process *p = *p_p;
    *p_p = p->next;
    freeProcess(p);
}

void remove_process_by_pid(process** process_list, pid_t pid) {
    process **p_p = addr_of_process_by_pid(process_list, pid);
    remove_process(p_p);
}

void updateProcessStatus(process* process_list, int pid, int status) {
    process *p = find_process_by_pid(process_list, pid);
    if (p) {
        p->status = status;
    }
}

void remove_terminated_process(process **process_list) {
    process *current = NULL;
    process **p_current = process_list;
    while ((current = *p_current)) {
        if (current->status == TERMINATED) {
            remove_process(p_current);
        }
        else {
            p_current = &(current->next);
        }
    }
}

void updateProcessList(process **process_list) {
    for (process *current = *process_list; current; current = current->next) {
        check_process_state_and_update_status(current);
    }
}

void print_process_list_column_names() {
    printf("%-12s %-12s %-12s\n", "PID", "STATUS", "Command");
}

void print_process_basic_info(process *p, char *s_status) {
    printf("%-12d ", p->pid);
    if (s_status) {
        printf("%-12s", s_status);
    }
    else {
        printf("%-12d", p->status);
    }
}

void print_process_info(process *p) {
    char *s_status = process_status_string_p(p);
    print_process_basic_info(p, s_status);
    print_cmd(stdout, p->cmd);
    print_line_separator();
}

void print_process_list_core(process *process_list) {
    print_process_list_column_names();
    for (process *current = process_list; current; current = current->next) {
        print_process_info(current);
    }
    print_line_separator();
}

void printProcessList(process** process_list) {
    process *head = *process_list;
    updateProcessList(process_list);
    print_process_list_core(head);
    remove_terminated_process(process_list);
}

INP_LOOP inp_loop_break(cmdLine *pCmdLine, process **process_list) {
    return INP_LOOP_BREAK;
}

INP_LOOP change_working_directory(cmdLine *pCmdLine, process **process_list) {
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

INP_LOOP print_process_list_cmd(cmdLine *pCmdLine, process **process_list) {
    printProcessList(process_list);
    return INP_LOOP_CONTINUE;
}

pid_t parse_send_signal_to_process_args(cmdLine *pCmdLine) {
    pid_t pid = 0;
    if (pCmdLine->argCount != 2) {
        printf("%s: expected exactly 1 arguments\n", cmd_get_path(pCmdLine));
    }
    else if (sscanf(pCmdLine->arguments[1], "%d", &pid) != 1 || pid <= 0) {
        printf("%s: invalid pid", cmd_get_path(pCmdLine));
        pid = 0;
    }

    return pid;
}

void send_signal_to_process_cmd(cmdLine *pCmdLine, process **process_list, int sig, int newStatus) {
    pid_t pid = parse_send_signal_to_process_args(pCmdLine);
    if (pid > 0 && send_signal_to_process(pid, sig)) {
        updateProcessStatus(*process_list, pid, newStatus);
    }
}

INP_LOOP suspend_process_cmd(cmdLine *pCmdLine, process **process_list) {
    send_signal_to_process_cmd(pCmdLine, process_list, SIGTSTP, SUSPENDED);
    return INP_LOOP_CONTINUE;
}

INP_LOOP kill_process_cmd(cmdLine *pCmdLine, process **process_list) {
    pid_t pid = parse_send_signal_to_process_args(pCmdLine);
    if (pid > 0) {
        process *p = find_process_by_pid(*process_list, pid);
        if (p) {
            terminateProcess(p);
        }
        else {
            printf("%s: process with id %d was not found\n", cmd_get_path(pCmdLine), pid);
        }
    }
    return INP_LOOP_CONTINUE;
}

INP_LOOP wake_process_cmd(cmdLine *pCmdLine, process **process_list) {
    send_signal_to_process_cmd(pCmdLine, process_list, SIGCONT, RUNNING);
    return INP_LOOP_CONTINUE;
}

predefined_cmd predefined_commands[] = {
    { "quit",		inp_loop_break, },
    { "cd",			change_working_directory, },
    { "procs",		print_process_list_cmd, },
    { "suspend",	suspend_process_cmd, },
    { "kill",	    kill_process_cmd, },
    { "wake",	    wake_process_cmd, },
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

void parent_failed_fork(pid_t pid, cmdLine *pCmdLine, process **process_list) {
    if (!dbg_print_error("fork")) {
        printf("fork error, exiting...\n");
    }
    freeCmdLines(pCmdLine);
    freeProcessList(*process_list);
    exit(EXIT_FAILURE);
}

void child_do_exec(cmdLine *pCmdLine) {
    execvp(cmd_get_path(pCmdLine), pCmdLine->arguments);
    if (!dbg_print_error("execv")) {
        printf("execv error, exiting...\n");
    }
    _exit(EXIT_FAILURE);
}

void wait_for_child(pid_t pid, process **process_list) {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err < 0) {
        if (errno == ECHILD) {
            remove_process_by_pid(process_list, pid);
        }
        else {
            dbg_print_error("waitpid");
        }
    }
    else {
        // child terminated, updating it's status to terminated will be checked by another function
    }
}

void parent_post_fork(pid_t pid, cmdLine *pCmdLine, process **process_list) {
    addProcess(process_list, pCmdLine, pid);
    if (pCmdLine->blocking) {
        wait_for_child(pid, process_list);
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

void execute(cmdLine *pCmdLine, process **process_list) {
    pid_t pid = fork();
    if (pid != 0 && DebugMode) {
        dbg_print_exec_info(pid, pCmdLine);
    }
    
    if (pid < 0) {
        // fork failed
        parent_failed_fork(pid, pCmdLine, process_list);
    }
    else if (pid == 0) {
        // child, executes the command
        child_do_exec(pCmdLine);
    }
    else {
        // parent, actual terminal
        parent_post_fork(pid, pCmdLine, process_list);
    }
}

INP_LOOP do_cmd(cmdLine *pCmdLine, process **process_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
    if (predef_cmd) {
        inp_loop = predef_cmd->handler(pCmdLine, process_list);
        freeCmdLines(pCmdLine);
    }
    else {
        execute(pCmdLine, process_list);
    }

    return inp_loop;
}

INP_LOOP do_cmd_from_input(char *buf, process **process_list) {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    cmdLine *pCmdLine = NULL;

    pCmdLine = parseCmdLines(buf);
    if (pCmdLine == NULL) {
        printf("error parsing the command\n");
        freeCmdLines(pCmdLine);
    }
    else {
        inp_loop = do_cmd(pCmdLine, process_list);
        // REMOVE descruction of the cmdLine object
    }

    return inp_loop;
}

void do_input_loop(process **process_list) {
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

        inp_loop = do_cmd_from_input(buf, process_list);
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
    process *process_list = NULL;
    set_dbg_mode_from_args(argc, argv);
    do_input_loop(&process_list);
    freeProcessList(process_list);
    return EXIT_SUCCESS;
}