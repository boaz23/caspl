#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define dbg_out stderr

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))

#define dbg_printf(s, ...) if (s->debug_mode) fprintf(dbg_out, __VA_ARGS__)

int input_int(int *n) {
    char buffer[256];
    int scan_result = 0;
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }
    
    scan_result = sscanf(buffer, "%d", n);
    if (scan_result == EOF || scan_result == 0) {
        printf("invalid input\n");
        return 0;
    }
    
    return 1;
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

typedef struct {
  char debug_mode;
  char file_name[128];
  int unit_size;
  unsigned char mem_buf[10000];
  size_t mem_count;
  /*
   .
   .
   Any additional fields you deem necessary
  */
} state;

void toggle_debug_mode_act(state *s) {
    s->debug_mode = 1 - s->debug_mode;
    if (s->debug_mode) {
        fprintf(dbg_out, "Debug flag now on\n");
    }
    else {
        fprintf(dbg_out, "Debug flag now off\n");
    }
}

void set_file_name_act(state *s) {
    char filename_buffer[256];
    printf("Enter file name: ");
    if (!input_filename(filename_buffer, ARR_LEN(filename_buffer))) {
        return;
    }

    strcpy(s->file_name, filename_buffer);
    dbg_printf(s, "Debug: file name set to '%s'\n", s->file_name);
}

void set_unit_size_act(state *s) {
    int unit_size;
    printf("Enter unit size: ");
    if (!input_int(&unit_size)) {
        return;
    }
    if (unit_size != 1 && unit_size != 2 && unit_size != 4) {
        printf("invalid unit size. value must be 1, 2 or 4\n");
    }

    s->unit_size = unit_size;
    dbg_printf(s, "Debug: set size to %d\n", s->unit_size);
}

void quit_act(state *s) {
    exit(0);
}

typedef void (*menu_func)(state *s);
typedef struct menu_item {
    char *name;
    menu_func func;
} menu_item;

void print_menu(menu_item const menu[]) {
    menu_item const *current_menu_item = menu;
    int i = 0;
    while (current_menu_item->name != NULL) {
        printf("%d)-%s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

void dbg_print_values(state *s) {
    dbg_printf(s, "unit size: %d\n", s->unit_size);
    dbg_printf(s, "file name: %s\n", s->file_name);
    dbg_printf(s, "mem count: %d\n", s->mem_count);
}

menu_func get_menu_func(menu_item const menu[], int option, int len) {
    if (0 <= option && option < len) {
        return menu[option].func;
    }

    return NULL;
}

void invoke_menu_action(menu_func func, state *s) {
    func(s);
}

int main(int argc, char *argv[]) {
    menu_item menu[] = {
        { "Toggle Debug Mode", toggle_debug_mode_act },
        { "Set File Name", set_file_name_act },
        { "Set Unit Size", set_unit_size_act },
        { "Quit", quit_act },
        { NULL, NULL },
    };
    state s = {
        0, "", 1, "", 0
    };
    state *ps = &s;

    while (1) {
        int option = -1;
        dbg_print_values(ps);
        printf("Choose action:\n");
        print_menu(menu);
        if (!input_int(&option)) {
            continue;
        }

        menu_func func = get_menu_func(menu, option, ARR_LEN(menu));
        if (func == NULL) {
            break;
        }

        invoke_menu_action(func, ps);
        printf("DONE.\n\n");
    }

    return 0;
}