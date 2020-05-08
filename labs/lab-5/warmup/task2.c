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

FILE *dbg_out	= NULL;
bool DebugMode	= FALSE;

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

typedef struct process {
	cmdLine* cmd;                         /* the parsed command line*/
	pid_t pid; 		                  /* the process id that is running the command*/
	int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
	struct process *next;	                  /* next process in chain */
} process;

char* process_get_status_name(process *p) {
	return  (p->status == TERMINATED)	? "Terminated"	:
			(p->status == RUNNING)		? "Running"		:
			(p->status == SUSPENDED)	? "Suspended"	:
			"???";
}

void freeProcess(process *p) {
	freeCmdLines(p->cmd);
	free(p);
}

process* process_create(cmdLine* cmd, pid_t pid) {
    process *new_node = (process*)malloc(sizeof(process));
	new_node->cmd = cmd;
	new_node->pid = pid;
	new_node->status = RUNNING;
	new_node->next = NULL;
    return new_node;
}

process** process_list_address_of_end(process **head) {
    process **current = head;
    while (*current != NULL) {
        current = &((*current)->next);
    }
    return current;
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process **head = process_list;
    process **end = process_list_address_of_end(head);
    *end = process_create(cmd, pid);
}

void freeProcessList(process *process_list) {
	process *current = process_list;
	while (current) {
		process *next = current->next;
		freeProcess(current);
		current = next;
	}
}

void updateProcessStatus(process* process_list, int pid, int status) {
	process *current = process_list;
	while (current) {
		if (current->pid == pid) {
			current->status = status;
			break;
		}

		current = current->next;
	}
}

void update_process(process *p) {
	int status;
	int res = waitpid(p->pid, &status, WNOHANG | WUNTRACED | WEXITED | WCONTINUED);
	if (res < 0) {
		if (DebugMode) {
			perror("waitpid");
		}
	}
	else if (res > 0) {
		if (WIFEXITED(status)) {
			p->status = TERMINATED;
		} else if (WIFSIGNALED(status)) {
			p->status = TERMINATED;
		} else if (WIFSTOPPED(status)) {
			p->status = SUSPENDED;
		} else if (WIFCONTINUED(status)) {
			p->status = RUNNING;
		}
	}
}

void updateProcessList(process **process_list) {
	process *current = *process_list;
	while (current) {
		update_process(current);
		current = current->next;
	}
}

void print_process(process *p) {
	printf("%-12d %-12s", p->pid, process_get_status_name(p));
	print_cmd(stdout, p->cmd);
	printf("\n");
}

void print_process_list_core(process* process_list) {
	process *current = process_list;
	while (current) {
		print_process(current);
		current = current->next;
	}
}

void remove_terminated_processes(process *process_list) {
	process **current = &process_list;
	while (*current) {
		process *next = *current;
		if (next->status == TERMINATED) {
			*current = next->next;
			freeProcess(next);
		}
		current = &((*current)->next);
	}
}

void printProcessList(process** process_list) {
	process *head = *process_list;
	updateProcessList(process_list);
	print_process_list_core(head);
	remove_terminated_processes(head);
}

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
			perror("cd");
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

bool send_signal_to_process(cmdLine *pCmdLine, process **process_list, int sig, int status) {
	int err = 0;
	pid_t pid = 0;
	if (pCmdLine->argCount != 2) {
		printf("%s: requires exactly 1 argument", cmd_get_path(pCmdLine));
		return FALSE;
	}
	if (sscanf(pCmdLine->arguments[1], "%d", &pid) != 1) {
		printf("%s: invalid pid", cmd_get_path(pCmdLine));
		return FALSE;
	}
	
	err = kill(pid, sig);
	if (err < 0) {
		perror("kill");
		return FALSE;
	}

	updateProcessStatus(*process_list, pid, status);
	return TRUE;
}

INP_LOOP suspend_cmd(cmdLine *pCmdLine, process **process_list) {
	send_signal_to_process(pCmdLine, process_list, SIGTSTP, SUSPENDED);
	return INP_LOOP_CONTINUE;
}

INP_LOOP kill_cmd(cmdLine *pCmdLine, process **process_list) {
	send_signal_to_process(pCmdLine, process_list, SIGINT, TERMINATED);
	return INP_LOOP_CONTINUE;
}

INP_LOOP wake_cmd(cmdLine *pCmdLine, process **process_list) {
	send_signal_to_process(pCmdLine, process_list, SIGCONT, RUNNING);
	return INP_LOOP_CONTINUE;
}

predefined_cmd predefined_commands[] = {
	{ "quit",		inp_loop_break },
	{ "cd",			change_working_directory },
	{ "procs",		print_process_list_cmd }
	{ "suspend",	suspend_cmd },
	{ "kill",		kill_cmd },
	{ "wake",		wake_cmd }
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
	print_cmd(dbg_out, pCmdLine);
	fprintf(dbg_out, "\n");
}

void parent_post_exec(pid_t pid, cmdLine *pCmdLine, process **process_list) {
	addProcess(process_list, pCmdLine, pid);
	if (pCmdLine->blocking) {
		// wait for the process to finish
		int status;
		int res = waitpid(pid, &status, 0);
		if (res < 0) {
			perror("waitpid");
		}
	}
	else {
		printf("pid: %d\n", pid);
	}
}

void execute(cmdLine *pCmdLine, process **process_list) {
	pid_t pid = fork();
	if (pid != 0 && DebugMode) {
		dbg_print_exec_info(pid, pCmdLine);
	}
	
	if (pid < 0) {
		perror("fork");
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
		parent_post_exec(pid, pCmdLine, process_list);
	}
}

INP_LOOP do_cmd(cmdLine *pCmdLine, process **process_list) {
	INP_LOOP inp_loop = INP_LOOP_CONTINUE;
	predefined_cmd *predef_cmd = find_predefined_cmd(cmd_get_path(pCmdLine));
	if (predef_cmd) {
		inp_loop = predef_cmd->handler(pCmdLine, process_list);
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
	}
	else {
		inp_loop = do_cmd(pCmdLine, process_list);
		// REMOVED descruction of the cmdLine object
	}

	return inp_loop;
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
	INP_LOOP inp_loop = INP_LOOP_CONTINUE;
	process **process_list = NULL;
	
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

		inp_loop = do_cmd_from_input(buf, process_list);
	}
	
	freeProcessList(process_list);
	return EXIT_SUCCESS;
}