#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#define dbg_out stderr

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))
#define is_str_empty(s) ((s)[0] == '\0')

typedef enum bool {
    FALSE = 0,
    TRUE  = 1,
} bool;

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

char* file_open_mode_string(char *mode) {
    if (mode[0] == '\0') {
        return NULL;
    }
    else if (mode[1] == '+') {
        return "reading and writing";
    }
    else if (strcmp("r", mode) == 0) {
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

void get_source_buffers_bounds(state *s, void *addr, int count, char **p_buffer, char **p_end) {
    char *end;
    char *buffer;
    int bytes_count = s->unit_size * count;
    if (addr == NULL) {
        char *buffer_end;
        buffer = (char*)s->mem_buf;
        buffer_end = buffer + s->mem_count;
        end = buffer + bytes_count;
        if (end > buffer_end) {
            end = buffer_end;
        }
    }
    else {
        buffer = (char*)addr;
        end = buffer + bytes_count;
    }

    *p_buffer = buffer;
    *p_end = end;
}

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

bool load_file_bytes_to_memory(state *s, FILE *f, int offset, int length) {
    unsigned char *mem_buf = s->mem_buf;
    int size = s->unit_size;
    int bytes_count;
    if (fseek(f, offset, SEEK_SET) < 0) {
        perror("ERROR - fseek");
        return FALSE;
    }
    bytes_count = fread(mem_buf, size, length, f);
    if (bytes_count < 0) {
        perror("ERROR: fread");
        return FALSE;
    }

    mem_buf[bytes_count] = '\0';
    s->mem_count = bytes_count;
    printf("Loaded %d units into memory\n", length);
    return TRUE;
}

void load_into_memory_act(state *s) {
    FILE *f = NULL;
    int location, length;
    if (is_str_empty(s->file_name)) {
        printf("No file has been specified.\n");
        return;
    }

    printf("Please enter <location> <length>\n");
    if (!input_args(2, "%X %d", &location, &length)) {
        return;
    }

    f = open_file(s->file_name, "r");
    if (!f) {
        return;
    }

    dbg_printf(s, "file name: %s, location: 0x%X, length: %d\n", s->file_name, location, length);
    load_file_bytes_to_memory(s, f, location, length);
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

char* unit_to_format(state *s) {
    switch (s->unit_size) {
        case 1:
            if (s->display_mode) {
                return "%hhX\n";
            }
            else {
                return "%hhd\n";
            }
        case 2:
            if (s->display_mode) {
                return "%hX\n";
            }
            else {
                return "%hd\n";
            }
        case 4:
            if (s->display_mode) {
                return "%X\n";
            }
            else {
                return "%d\n";
            }
        default:
            return NULL;
    }
}

void print_from_buffer(state *s, char *buffer, char *end) {
    char *format = unit_to_format(s);
    while (buffer < end) {
        int var = *((int*)(buffer));
        printf(format, var);
        buffer += s->unit_size;
    }
}

void memory_display(state *s, void *addr, int count) {
    char *buffer;
    char *end;
    get_source_buffers_bounds(s, addr, count, &buffer, &end);
    print_from_buffer(s, buffer, end);
}

void memory_display_act(state *s) {
    int u, addr;

    if (s->display_mode) {
        printf("Hexadecimal\n===========\n");
    }
    else {
        printf("Decimal\n=======\n");
    }

    if (!input_int_dec(&u)) {
        return;
    }
    if (!input_int_hex(&addr)) {
        return;
    }

    memory_display(s, (void*)addr, u);
}

bool validate_enough_bytes_in_memory_buffer(state *s, char *buffer, char *end, int length) {
    int available_bytes_count = end - buffer;
    int requested_bytes_count = length * s->unit_size;
    if (available_bytes_count != requested_bytes_count) {
        printf("requested %d bytes but memory buffer only has %d\n", requested_bytes_count, available_bytes_count);
        return FALSE;
    }

    return TRUE;
}

bool prepare_file_for_save(FILE *f, int target_location) {
    int last_file_offset;
    if (fseek(f, 0, SEEK_END) < 0) {
        perror("ERROR: fseek");
        return FALSE;
    }
    last_file_offset = ftell(f);
    if (last_file_offset < 0) {
        perror("ERROR: ftell");
        return FALSE;
    }
    if (last_file_offset < target_location) {
        printf("The offset is too big for the file\n");
        return FALSE;
    }
    if (fseek(f, target_location, SEEK_SET) < 0) {
        perror("ERROR: fseek");
    }

    return TRUE;
}

void save_to_file_act(state *s) {
    char *buffer;
    char *end;
    int source_address, target_location, length;
    FILE *f = NULL;
    printf("Please enter <source-address> <target-location> <length>\n");
    if (!input_args(3, "%X %X %d", &source_address, &target_location, &length)) {
        return;
    }
    
    get_source_buffers_bounds(s, (void*)source_address, length, &buffer, &end);
    if (source_address == 0 &&
        !validate_enough_bytes_in_memory_buffer(s, buffer, end, length)) {
        return;
    }

    f = open_file(s->file_name, "r+");
    if (!f) {
        return;
    }
    if (prepare_file_for_save(f, target_location)) {
        if (fwrite(buffer, s->unit_size, length, f) < (s->unit_size * length)) {
            if (!dbg_print_error(s, "fwrite")) {
                printf("Error writing to file\n");
            }
        }
    }

    fclose(f);
}

void memory_modify_act(state *s) {
    int location, val;
    printf("Please enter <location> <val>\n");
    if (!input_args(2, "%X %X", &location, &val)) {
        return;
    }

    dbg_printf(s, "location: %X, val: %X\n", location, val);
    if (location + s->unit_size >= ARR_LEN(s->mem_buf)) {
        printf("Address and unit size is out of bounds of mem_buf\n");
        return;
    }

    void *p_mem = (void*)s->mem_buf + location;
    switch (s->unit_size) {
        case 1:
            *((char*)p_mem) = (char)val;
            break;
        case 2:
            *((short*)p_mem) = (short)val;
            break;
        case 4:
            *((int*)p_mem) = (int)val;
            break;
        default:
            dbg_printf(s, "Invalid unit size detected (%d)\n", s->unit_size);
            break;
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
        printf("%d-%s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

void dbg_print_values(state *s) {
    dbg_printf(s, "unit size: %d\n", s->unit_size);
    dbg_printf(s, "file name: %s\n", s->file_name);
    dbg_printf(s, "mem count: %d\n", s->mem_count);
    dbg_printf(s, "display mode: %s\n", s->display_mode ? "hexadecimal" : "decimal");
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
        { "Memory Display", memory_display_act },
        { "Save Into File", save_to_file_act },
        { "Memory Modify", memory_modify_act },
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