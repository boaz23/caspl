#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define dbg_out stderr

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))
#define is_str_empty(s) ((s)[0] == '\0')

typedef enum bool {
    FALSE = 0,
    TRUE  = 1,
} bool;

int input_int(int *n, char *f) {
    char buffer[256];
    int scan_result = 0;
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }

    scan_result = sscanf(buffer, f, n);
    if (scan_result == EOF || scan_result == 0) {
        printf("invalid input\n");
        return 0;
    }
    
    return 1;
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

char* file_open_mode_string(char *mode) {
    if (strcmp("r", mode) == 0) {
        return "reading";
    }
    else if (strcmp("w", mode) == 0) {
        return "writing";
    }
    else {
        return NULL;
    }
}

void print_file_open_err(char *file_name, char *mode) {
    char *mode_s = file_open_mode_string(mode);
    if (mode_s) {
        printf("Unable to open file '%s' for %s\n", file_name, mode_s);
    }
    else {
        printf("Unable to open file '%s'", file_name);
    }
}

FILE* open_file(char *file_name, char *mode) {
    FILE *f = fopen(file_name, mode);
    if (!f) {
        print_file_open_err(file_name, mode);
    }

    return f;
}

typedef struct {
  char debug_mode;
  char file_name[128];
  int unit_size;
  unsigned char mem_buf[10000];
  size_t mem_count;
  bool display_mode;
  /*
   .
   .
   Any additional fields you deem necessary
  */
} state;

bool dbg_printf(state *s, char const *f, ...) {
    if (s->debug_mode) {
        va_list args;
        va_start(args, f);
        vfprintf(dbg_out, f, args);
        va_end(args);
        return TRUE;
    }
    
    return FALSE;
}
bool dbg_print_error(state *s, char const *err) {
    if (s->debug_mode) {
        fprintf(dbg_out, "%s: %s\n", err, strerror(errno));
        return TRUE;
    }

    return FALSE;
}

char* unit_to_format(int unit) {
    switch (unit) {
        case 1:
            return "%#hhx\n";
        case 2:
            return "%#hx\n";
        case 4:
            return "%#hhx\n";
        default:
            return "Unknown unit";
    }
}  

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
    if (!input_int_dec(&unit_size)) {
        return;
    }
    if (unit_size != 1 && unit_size != 2 && unit_size != 4) {
        printf("invalid unit size. value must be 1, 2 or 4\n");
    }

    s->unit_size = unit_size;
    dbg_printf(s, "Debug: set size to %d\n", s->unit_size);
}

bool load_file_bytes_to_memory(FILE *f, int offset, int length, int size, unsigned char *mem_buf) {
    if (fseek(f, offset, SEEK_SET) < 0) {
        perror("ERROR - fseek");
        return FALSE;
    }
    if (fread(mem_buf, size, length, f) < 0) {
        perror("ERROR: fread");
        return FALSE;
    }

    mem_buf[size * length] = '\0';
    printf("Loaded %d units into memory\n", length);
    return TRUE;
}

int input_location_length(int *location, int *length) {
    char buffer[256];
    int scan_result = 0;
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }
    
    scan_result = sscanf(buffer, "%X %d", location, length);
    if (scan_result == EOF || scan_result < 2) {
        printf("invalid input\n");
        return 0;
    }
    
    return 1;
}

void load_into_memory_act(state *s) {
    FILE *f = NULL;
    int location, length;
    if (is_str_empty(s->file_name)) {
        printf("No file has been specified.\n");
        return;
    }

    printf("Please enter <location> <length>\n");
    if (!input_location_length(&location, &length)) {
        return;
    }

    f = open_file(s->file_name, "r");
    if (!f) {
        return;
    }

    dbg_printf(s, "file name: %s, location: 0x%X, length: %d\n", s->file_name, location, length);
    load_file_bytes_to_memory(f, location, length, s->unit_size, s->mem_buf);
    fclose(f);
}

void toggle_display_mode_act(state *s) {
    s->display_mode = 1 - s->display_mode;
    if (s->display_mode) {
        dbg_printf(s, "Display flag now on, hexadecimal representation\n");
    }
    else {
        dbg_printf(s, "Display flag now off, decimal representations\n");
    }
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
        { "Load Into Memory", load_into_memory_act },
        { "Toggle Display Mode", toggle_display_mode_act },
        { "Quit", quit_act },
        { NULL, NULL },
    };
    state s = {
        0, "", 1, "", 0, FALSE
    };
    state *ps = &s;

    while (1) {
        int option = -1;
        dbg_print_values(ps);
        printf("Choose action:\n");
        print_menu(menu);
        if (!input_int_dec(&option)) {
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