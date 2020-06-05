#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>
#include <errno.h>

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))
#define is_str_empty(s) ((s)[0] == '\0')

typedef enum bool {
    FALSE = 0,
    TRUE  = 1,
} bool;

#define LINE_MAX 256
#define dbg_out stderr
bool DebugMode = FALSE;

bool dbg_printf(char const *f, ...) {
    if (DebugMode) {
        va_list args;
        va_start(args, f);
        vfprintf(dbg_out, f, args);
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

int input_args(int num_args, char const *f, ...) {
    va_list args;
    char buffer[256];
    int scan_result = 0;
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }
    
    va_start(args, f);
    scan_result = vsscanf(buffer, f, args);
    va_end(args);
    if (scan_result == EOF || scan_result < num_args) {
        printf("invalid input\n");
        return 0;
    }
    
    return 1;
}

int input_int(int *n, char *f) {
    return input_args(1, f, n);
}
int input_int_dec(int *n) {
    return input_int(n, "%d");
}
int input_int_hex(int *n) {
    return input_int(n, "%X");
}

int input_filename(char *buffer, int len) {
    if (fgets(buffer, len, stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }
    
    int input_len = strlen(buffer) - 1;
    if (buffer[input_len] == '\n') {
        buffer[input_len] = '\0';
    }

    return 1;
}

int Currentfd = -1;
int map_length = -1;
void *map_start = NULL;

void cleanup() {
    if (map_start) {
        munmap(map_start, map_length);
    }
    if (Currentfd >= 0) {
        close(Currentfd);
    }
}

typedef enum INP_LOOP {
    INP_LOOP_CONTINUE    = 0,
    INP_LOOP_BREAK       = 1,
} INP_LOOP;

INP_LOOP toggle_debug_mode_action() {
    return INP_LOOP_CONTINUE;
}
INP_LOOP examine_elf_file_action() {
    return INP_LOOP_CONTINUE;
}
INP_LOOP quit_action() {
    return INP_LOOP_BREAK;
}

typedef INP_LOOP (*menu_func)();
typedef struct menu_item {
    char *name;
    menu_func func;
} menu_item;

void print_menu(menu_item const menu[]) {
    menu_item const *current_menu_item = menu;
    int i = 0;
    while (current_menu_item->name != NULL) {
        printf("%d-%s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

menu_func get_menu_func(menu_item const menu[], int option, int len) {
    if (0 <= option && option < len) {
        return menu[option].func;
    }

    return NULL;
}

INP_LOOP invoke_menu_action(menu_func func) {
    return func();
}

menu_item const menu[] = {
    { "Toggle Debug Mode", toggle_debug_mode_action },
    { "Examine ELF File", examine_elf_file_action },
    { "Quit", quit_action },
    { NULL, NULL }
};

void do_input_loop() {
    INP_LOOP inp_loop = INP_LOOP_CONTINUE;
    while (!inp_loop) {
        int option = -1;
        print_menu(menu);
        printf("Option: ");
        if (!input_int_dec(&option)) {
            continue;
        }

        menu_func func = get_menu_func(menu, option, ARR_LEN(menu));
        if (func == NULL) {
            break;
        }

        inp_loop = invoke_menu_action(func);
        printf("DONE.\n\n");
    }
}

int main(int argc, char *argv[]) {
    do_input_loop(menu);
    cleanup();
    return 0;
}