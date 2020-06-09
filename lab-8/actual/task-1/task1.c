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

#define print_line() printf("\n");

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
    char buffer[LINE_MAX];
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
#define INVALID_LEN  -1
typedef char* str_tab;

int Currentfd;
int map_length;
void *map_start;

Elf32_Ehdr *elf_header;
Elf32_Shdr *section_headers_table;
int sections_count;
str_tab section_names_string_table;

bool is_mapped() {
    return Currentfd > INVALID_FILE;
}

void reset_map_values() {
    map_start = NULL;
    map_length = INVALID_LEN;
    Currentfd = INVALID_FILE;

    elf_header = NULL;
    section_headers_table = NULL;
    sections_count = INVALID_LEN;
    section_names_string_table = NULL;
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

void* offset_to_pointer(Elf32_Off offset) {
    return map_start + offset;
}

Elf32_Shdr* get_section_header(Elf32_Half index) {
    return &section_headers_table[index];
}

str_tab get_str_tab_from_section_index(Elf32_Half str_tab_section_index) {
    Elf32_Shdr* section_header = get_section_header(str_tab_section_index);
    return (str_tab)offset_to_pointer(section_header->sh_offset);
}

char* str_tab_get_name(str_tab string_table, Elf32_Word index) {
    return &string_table[index];
}

bool map_file_to_memory_core(char *file_name, int open_flags) {
    struct stat fd_stat;
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
    bool mapped;
    cleanup();
    mapped = map_file_to_memory_core(file_name, open_flags);
    if (!mapped) {
        cleanup();
    }
    return mapped;
}

char* e_ident_magic_bytes_string(byte *ident) {
    return (char*)ident + 1;
}

char* e_ident_data_string(byte data) {
    char *data_strings[] = {
        "",
        "2's complement, little endian",
        "2's complement, big endian"
    };
    int index = data;
    if (ELFDATANONE <= index && index <= ELFDATA2MSB) {
        return data_strings[index];
    }
    else {
        return "";
    }
}

bool is_valid_elf32_file() {
    byte* ident = elf_header->e_ident;
    if (map_length < 4 || map_length < sizeof(Elf32_Ehdr)) {
        return FALSE;
    }
    
    if (ident[EI_MAG0] != ELFMAG0 || strncmp("ELF", e_ident_magic_bytes_string(ident), 3) != 0) {
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

bool elf_check_header() {
    byte* ident = elf_header->e_ident;
    if (!is_valid_elf32_file(elf_header)) {
        printf("File is not a valid elf file.\n");
        return FALSE;
    }
    if (ident[EI_CLASS] != ELFCLASS32) {
        printf("Unsupported ELF file\n");
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

    print_line();
}

void print_elf_file_info() {
    byte* ident = elf_header->e_ident;
    print_line();
    print_elf_header_info("Magic bytes", "%.3s", e_ident_magic_bytes_string(ident));
    print_elf_header_info("Data", "%s", e_ident_data_string(ident[EI_DATA]));
    print_elf_header_info("Entry point", "0x%X", elf_header->e_entry);
    print_elf_header_info("Section headers table offset", "0x%X", elf_header->e_shoff);
    print_elf_header_info("Number of section header entries", "%d", elf_header->e_shnum);
    print_elf_header_info("Size of section header entry", "%d (bytes)", elf_header->e_shentsize);
    print_elf_header_info("Program headers table offset", "0x%X", elf_header->e_phoff);
    print_elf_header_info("Number of program header entries", "%d", elf_header->e_phnum);
    print_elf_header_info("Size of program header entry", "%d (bytes)", elf_header->e_phentsize);
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

    elf_header = (Elf32_Ehdr*)map_start;
    if (!elf_check_header()) {
        return;
    }
    section_headers_table = (Elf32_Shdr*)offset_to_pointer(elf_header->e_shoff);
    sections_count = elf_header->e_shnum;

    print_elf_file_info();
}

bool elf_find_section_names_string_table() {
    if (section_names_string_table) {
        return TRUE;
    }

    if (elf_header->e_shstrndx == SHN_UNDEF) {
        printf("The ELF file does not have a section names string table\n");
        return FALSE;
    }

    section_names_string_table = get_str_tab_from_section_index(elf_header->e_shstrndx);
    return TRUE;
}

char* elf_name_of_section(Elf32_Shdr *section_header) {
    return str_tab_get_name(section_names_string_table, section_header->sh_name);
}

void print_section_names() {
    Elf32_Shdr *section_header;
    if (!elf_find_section_names_string_table()) {
        return;
    }

    print_line();
    dbg_printf("Section names string table header index %d\n", elf_header->e_shstrndx);

    printf("[Nr] %-20s %-10s %-10s %-10s %-10s", "Name", "Address", "Offset", "Size", "Type");
    if (DebugMode) {
        printf(" %-10s", "Name-Index");
    }
    print_line();

    section_header = section_headers_table;
    for (int i = 0; i < sections_count; ++i, ++section_header) {
        printf(
            "[%-2d] %-20s 0x%08X 0x%08X %-10d %-10d",
            i,
            elf_name_of_section(section_header),
            section_header->sh_addr,
            section_header->sh_offset,
            section_header->sh_size,
            section_header->sh_type
        );
        if (DebugMode) {
            printf(" %-10d", section_header->sh_name);
        }
        print_line();
    }
}

void toggle_debug_mode_action() {
    DebugMode = 1 - DebugMode;
}
void print_section_names_action() {
    if (is_mapped()) {
        print_section_names();
    }
    else {
        printf("ELF file was not loaded\n");
    }
}
void quit_action() {
    cleanup();
    exit(0);
}

typedef void (*menu_func)();
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

menu_item const menu[] = {
    { "Toggle Debug Mode", toggle_debug_mode_action },
    { "Examine ELF File", examine_elf_file },
    { "Print Section Names", print_section_names_action },
    { "Quit", quit_action },
    { NULL, NULL }
};

void do_input_loop() {
    while (TRUE) {
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

        func();
        printf("DONE.\n\n");
    }
}

int main(int argc, char *argv[]) {
    reset_map_values();
    do_input_loop();
    cleanup();
    return 0;
}
