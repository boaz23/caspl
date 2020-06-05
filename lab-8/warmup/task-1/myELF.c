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
#define INVALID_LEN  -1
bool mapped = FALSE;
int Currentfd = INVALID_FILE;
int map_length = INVALID_LEN;
void *map_start = NULL;

Elf32_Ehdr *elf_header = NULL;
char* section_name_string_table = NULL;
int section_name_string_table_size = INVALID_LEN;

void reset_map_values() {
    mapped = FALSE;
    map_start = NULL;
    map_length = INVALID_LEN;
    Currentfd = INVALID_FILE;

    elf_header = NULL;
    section_name_string_table = NULL;
    section_name_string_table_size = INVALID_LEN;
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

char* e_ident_magic_bytes_string(byte *ident) {
    return (char*)ident + 1;
}

char* e_ident_data_string(byte *ident) {
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

bool elf_check_header(Elf32_Ehdr *header) {
    byte* ident = header->e_ident;
    if (!is_valid_elf32_file(header)) {
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

    printf("\n");
}

void print_elf_file_info(Elf32_Ehdr *header) {
    byte* ident = header->e_ident;
    
    printf("\n");
    print_elf_header_info("Magic bytes", "%.3s", e_ident_magic_bytes_string(ident));
    print_elf_header_info("Data", "%s", e_ident_data_string(ident));
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

    elf_header = (Elf32_Ehdr*)map_start;
    if (!elf_check_header(elf_header)) {
        return;
    }
    print_elf_file_info(elf_header);
}

bool elf_section_names_string_table(Elf32_Ehdr *header) {
    Elf32_Shdr *section_names_string_table_header;
    if (header->e_shstrndx == SHN_UNDEF) {
        printf("The ELF file does not have a section names string table");
        return FALSE;
    }

    section_names_string_table_header = (Elf32_Shdr*)(map_start + header->e_shoff + (header->e_shstrndx * header->e_shentsize));
    section_name_string_table = (char*)(map_start + section_names_string_table_header->sh_offset);
    section_name_string_table_size = section_names_string_table_header->sh_size;
    return TRUE;
}

void fill_next_str_in_string_table(char *p_str_table, char **p_buf, int max_read_amount) {
    int read_counter;
    char *buf, *p_current_char;

    *p_buf = NULL;
    read_counter = 1;
    p_current_char = p_str_table;
    while (*p_current_char != '\0' && read_counter < max_read_amount) {
        ++p_current_char;
        ++read_counter;
    }

    p_current_char = p_str_table;
    buf = malloc(read_counter);
    for (int i = 0; i < read_counter; ++i) {
        buf[i] = p_current_char[i];
    }
    
    *p_buf = buf;
}

char* elf_section_type_string(Elf32_Word section_type) {
    switch (section_type) {
        case SHT_NULL:
            return "NULL";
        case SHT_PROGBITS:
            return "PROGBITS";
        case SHT_SYMTAB:
            return "SYMTAB";
        case SHT_STRTAB:
            return "STRTAB";
        case SHT_RELA:
            return "RELA";
        case SHT_HASH:
            return "HASH";
        case SHT_DYNAMIC:
            return "DYNAMIC";
        case SHT_NOTE:
            return "NOTE";
        case SHT_NOBITS:
            return "NOBITS";
        case SHT_REL:
            return "REL";
        case SHT_SHLIB:
            return "SHLIB";
        case SHT_DYNSYM:
            return "DYNSYM";
        default:
            return "";
    }
}

void print_section_names() {
    Elf32_Ehdr *header = elf_header;
    Elf32_Shdr *section_header;
    int sections_count;
    char *section_name;
    if (!elf_section_names_string_table(header)) {
        return;
    }

    printf("\n[Nr] Name%*s Address%*s Offset%*s Size%*s Type%*s\n", 16, "", 3, "", 4, "", 5, "", 4, "");

    section_header = (Elf32_Shdr*)(map_start + header->e_shoff);
    sections_count = header->e_shnum;
    for (int i = 0; i < sections_count; ++i) {
        char* p_sh_str = section_name_string_table + section_header->sh_name;
        int max_read_amount = section_name_string_table_size - (p_sh_str - section_name_string_table);
        fill_next_str_in_string_table(p_sh_str, &section_name, max_read_amount);
        printf(
            "[%-2d] %-20s 0x%08X 0x%08X %-9d %-8s\n",
            i,
            section_name,
            section_header->sh_addr,
            section_header->sh_offset,
            section_header->sh_size,
            elf_section_type_string(section_header->sh_type)
        );
        free(section_name);
        ++section_header;
    }
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
INP_LOOP print_section_names_action() {
    if (mapped) {
        print_section_names();
    }
    else {
        printf("No ELF file is loaded.\n");
    }
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
    { "Print Section Names", print_section_names_action },
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