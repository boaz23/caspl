#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <errno.h>

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))
#define is_str_empty(s) ((s)[0] == '\0')

typedef unsigned char byte;
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

// -------------------------------------
// ---------- Task code start ----------
// -------------------------------------
#define INVALID_FILE -1
bool mapped = FALSE;
int Currentfd = INVALID_FILE;
int map_length = INVALID_FILE;
void *map_start = NULL;

void reset_map_values() {
    map_start = NULL;
    map_length = INVALID_FILE;
    Currentfd = INVALID_FILE;
    mapped = FALSE;
}
void free_map_resources() {
    if (map_start != NULL && map_start != MAP_FAILED) {
        munmap(map_start, map_length);
    }
    if (Currentfd > INVALID_FILE) {
        close(Currentfd);
    }
}
void cleanup() {
    free_map_resources();
    reset_map_values();
}

bool map_file_to_memory_core(char *file_name, int open_flags) {
    struct stat fd_stat; /* this is needed to  the size of the file */
    int fd = open(file_name, open_flags);
    int err;

    if (fd < 0) {
        perror("open");
        return FALSE;
    }

    Currentfd = fd;
    err = fstat(fd, &fd_stat);
    if (err < 0) {
        perror("fstat");
        return FALSE;
    }

    map_length = fd_stat.st_size;
    map_start = mmap(NULL, map_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap");
        return FALSE;
    }
    
    return TRUE;
}

bool map_file_to_memory(char *file_name, int open_flags) {
    cleanup();
    mapped = map_file_to_memory_core(file_name, open_flags);
    if (!mapped) {
        cleanup();
    }
    return mapped;
}

char* e_ident_magic_bytes(byte *ident) {
    return (char*)ident + 1;
}
void e_ident_cpy_magic_bytes(byte *ident, char *magic) {
    memcpy(magic, e_ident_magic_bytes(ident), 3);
}

char* e_ident_data(byte *ident) {
    switch (ident[EI_DATA]) {
        case ELFDATANONE:
            return NULL;
        case ELFDATA2LSB:
            return "2's complement, little endian";
        case ELFDATA2MSB:
            return "2's complement, big endian";
        default:
            return NULL;
    }
}

bool is_valid_elf32_file(Elf32_Ehdr *header) {
    byte* ident = header->e_ident;
    if (map_length < 4 || map_length < sizeof(Elf32_Ehdr)) {
        return FALSE;
    }
    
    if (ident[EI_MAG0] != ELFMAG0 || strncmp("ELF", e_ident_magic_bytes(ident), 3) != 0) {
        return FALSE;
    }

    if (ident[EI_CLASS] != ELFCLASS32 && ident[EI_CLASS] != ELFCLASS64) {
        return FALSE;
    }
    if (ident[EI_DATA] != ELFDATA2LSB && ident[EI_DATA] != ELFDATA2MSB) {
        return FALSE;
    }

    return TRUE;
}

void print_elf_header_info(char *info_name, char *format, ...) {
    va_list args;
    int info_label_len = strlen(info_name);
    printf("%s:%*s", info_name, 35 - info_label_len, "");

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

void print_elf_file_info() {
    Elf32_Ehdr *header = (Elf32_Ehdr*)map_start;
    byte* ident = header->e_ident;
    char magic[3];
    if (!is_valid_elf32_file(header)) {
        printf("File is not a valid elf file.\n");
        return;
    }
    if (ident[EI_CLASS] != ELFCLASS32) {
        printf("Unsupported ELF file\n");
        return;
    }

    e_ident_cpy_magic_bytes(ident, magic);
    printf("\n");
    print_elf_header_info("Magic bytes", "%s", magic);
    print_elf_header_info("Data", "%s", e_ident_data(ident));
    print_elf_header_info("Entry point", "0x%X", header->e_entry);
    print_elf_header_info("Section headers table offset", "0x%X", header->e_shoff);
    print_elf_header_info("Number of section header entries", "%d", header->e_shnum);
    print_elf_header_info("Size of section header entry", "%d (bytes)", header->e_shentsize);
    print_elf_header_info("Program headers table offset", "0x%X", header->e_phoff);
    print_elf_header_info("Number of program header entries", "%d", header->e_phnum);
    print_elf_header_info("Size of program header entry", "%d (bytes)", header->e_phentsize);
}

void examine_elf_file() {
    char filename_buffer[LINE_MAX];
    printf("Enter file name: ");
    if (!input_filename(filename_buffer, ARR_LEN(filename_buffer))) {
        return;
    }
    
    if (!map_file_to_memory(filename_buffer, O_RDWR)) {
        return;
    }

    print_elf_file_info();
}

typedef enum INP_LOOP {
    INP_LOOP_CONTINUE    = 0,
    INP_LOOP_BREAK       = 1,
} INP_LOOP;

INP_LOOP toggle_debug_mode_action() {
    DebugMode = 1 - DebugMode;
    return INP_LOOP_CONTINUE;
}
INP_LOOP examine_elf_file_action() {
    examine_elf_file();
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