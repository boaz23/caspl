#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>

typedef __sighandler_t sighandler;
void signal_printer_stp(int sig);

void looper_print(char const *s, ...) {
	va_list args;
	va_start(args, s);
	printf("LOOPER %d: ", getpid());
	vprintf(s, args);
	printf("\n");
	va_end(args);
}

void signal_printer(int sig, int sig_renew, sighandler handler) {
	looper_print("%s received", strsignal(sig));
	signal(sig, SIG_DFL);
	if (handler) {
		signal(sig_renew, handler);
	}
	raise(sig);
}

void signal_printer_int(int sig) {
	signal_printer(sig, 0, NULL);
}

void signal_printer_cont(int sig) {
	signal_printer(sig, SIGTSTP, signal_printer_stp);
}

void signal_printer_stp(int sig) {
	signal_printer(sig, SIGCONT, signal_printer_cont);
}

int main(int argc, char **argv) {
	looper_print("Starting the program");

	signal(SIGINT, signal_printer_int);
	signal(SIGTSTP, signal_printer_stp);
	signal(SIGCONT, signal_printer_cont);
	while(1) {
		// printf("Waiting for interruption...\n");
		sleep(2);
	}

	return 0;
}