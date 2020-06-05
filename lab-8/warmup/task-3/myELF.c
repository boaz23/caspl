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
bool mapped;
int Currentfd;
int map_length;
void *map_start;

Elf32_Ehdr *elf_header;
Elf32_Shdr *section_headers_table;
char* section_names_string_table;
int section_names_string_table_size;

void reset_map_values() {
    mapped = FALSE;
    map_start = NULL;
    map_length = INVALID_LEN;
    Currentfd = INVALID_FILE;

    elf_header = NULL;
    section_headers_table = NULL;
    section_names_string_table = NULL;
    section_names_string_table_size = INVALID_LEN;
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

Elf32_Shdr* get_section_header(int index) {
    return &section_headers_table[index];
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

    printf("\n");
}

void print_elf_file_info() {
    byte* ident = elf_header->e_ident;
    printf("\n");
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
    if (!elf_check_header(elf_header)) {
        return;
    }
    section_headers_table = (Elf32_Shdr*)(map_start + elf_header->e_shoff);

    print_elf_file_info(elf_header);
}

bool elf_section_names_string_table() {
    Elf32_Shdr *section_names_string_table_header;
    if (elf_header->e_shstrndx == SHN_UNDEF) {
        printf("The ELF file does not have a section names string table");
        return FALSE;
    }

    section_names_string_table_header = get_section_header(elf_header->e_shstrndx);
    section_names_string_table = (char*)(map_start + section_names_string_table_header->sh_offset);
    section_names_string_table_size = section_names_string_table_header->sh_size;
    return TRUE;
}

void fill_next_str_in_string_table(char *p_str_table, char **p_buf, int max_read_amount, int *len) {
    int read_counter;
    char *buf, *p_current_char;

    *p_buf = NULL;
    read_counter = 1;
    p_current_char = p_str_table;
    while (*p_current_char != '\0' && read_counter < max_read_amount) {
        ++p_current_char;
        ++read_counter;
    }

    *len = read_counter;
    if (read_counter == max_read_amount && *p_current_char != '\0') {
        *p_buf = NULL;
    }
    else {
        p_current_char = p_str_table;
        buf = malloc(read_counter);
        for (int i = 0; i < read_counter; ++i) {
            buf[i] = p_current_char[i];
        }
        
        *p_buf = buf;
    }
}

void elf_name_of_section(Elf32_Shdr *section_header, char **name) {
    char* p_sh_str = section_names_string_table + section_header->sh_name;
    int max_read_amount = section_names_string_table_size - (p_sh_str - section_names_string_table);
    int len;
    fill_next_str_in_string_table(p_sh_str, name, max_read_amount, &len);
}

char* elf_section_type_string(Elf32_Word section_type) {
    char *type_strings[] = {
        "NULL",
        "PROGBITS",
        "SYMTAB",
        "STRTAB",
        "RELA",
        "HASH",
        "DYNAMIC",
        "NOTE",
        "NOBITS",
        "REL",
        "SHLIB",
        "DYNSYM",
    };
    int index = section_type;
    if (SHT_NULL <= index && index <= SHT_DYNSYM) {
        return type_strings[index];
    }
    else {
        return "";
    }
}

void print_section_names() {
    Elf32_Shdr *section_header;
    int sections_count;
    char *section_name;
    if (!elf_section_names_string_table(elf_header)) {
        return;
    }

    printf("\n");
    dbg_printf("Section names string table header index: %d\n", elf_header->e_shstrndx);
    if (!DebugMode) {
        printf("[Nr] Name%*s Address%*s Offset%*s Size%*s Type%*s\n", 16, "", 3, "", 4, "", 5, "", 4, "");
    }
    else {
        printf("[Nr] Name%*s Address%*s Offset%*s Size%*s Type%*s Name-Index\n", 16, "", 3, "", 4, "", 5, "", 4, "");
    }

    section_header = (Elf32_Shdr*)(map_start + elf_header->e_shoff);
    sections_count = elf_header->e_shnum;
    for (int i = 0; i < sections_count; ++i, ++section_header) {
        elf_name_of_section(section_header, &section_name);

        if (!DebugMode) {
            printf(
                "[%-2d] %-20s 0x%08X 0x%08X %-9d %-8s\n",
                i,
                section_name,
                section_header->sh_addr,
                section_header->sh_offset,
                section_header->sh_size,
                elf_section_type_string(section_header->sh_type)
            );
        }
        else {
            printf(
                "[%-2d] %-20s 0x%08X 0x%08X %-9d %-8s %-10d\n",
                i,
                section_name,
                section_header->sh_addr,
                section_header->sh_offset,
                section_header->sh_size,
                elf_section_type_string(section_header->sh_type),
                section_header->sh_name
            );
        }
        free(section_name);
    }
}

void print_symbol_table(Elf32_Shdr *symbol_section_header) {
    Elf32_Sym *symbol_entry;
    Elf32_Shdr *symbol_table_string_table_section_header;
    char *p_symbol_table_string_table;
    int symbol_table_string_table_size;
    int symbols_count;

    symbols_count = symbol_section_header->sh_size / symbol_section_header->sh_entsize;
    symbol_table_string_table_section_header = get_section_header(symbol_section_header->sh_link);
    p_symbol_table_string_table = (char*)(map_start + symbol_table_string_table_section_header->sh_offset);
    symbol_table_string_table_size = symbol_table_string_table_section_header->sh_size;

    printf("\n");
    dbg_printf("Symbol table size: %d (bytes)\n", symbol_section_header->sh_size);
    dbg_printf("Symbol table symbols count: %d\n", symbols_count);

    printf("Num Value%*s Sh-Ndx Sh-Name%*s Size%*s Name%*s\n", 5, "", 13, "", 5, "", 26, "");

    symbol_entry = (Elf32_Sym*)(map_start + symbol_section_header->sh_offset);
    for (int i = 0; i < symbols_count; ++i, ++symbol_entry) {
        Elf32_Shdr *section_header_of_symbol;
        char *section_name;
        char *symbol_name;

        int len;
        char* p_sym_str = p_symbol_table_string_table + symbol_entry->st_name;
        int max_read_amount = symbol_table_string_table_size - (p_sym_str - p_symbol_table_string_table);
        fill_next_str_in_string_table(p_sym_str, &symbol_name, max_read_amount, &len);
        
        printf("%-3d 0x%08X ", i, symbol_entry->st_value);
        switch (symbol_entry->st_shndx) {
            case SHN_ABS:
                printf("%-6s %-20s", "ABS", "");
                break;
            case SHN_UNDEF:
                printf("%-6s %-20s", "UNDEF", "");
                break;
            default:
                section_header_of_symbol = get_section_header(symbol_entry->st_shndx);
                elf_name_of_section(section_header_of_symbol, &section_name);
                printf("%-6d %-20s", symbol_entry->st_shndx, section_name);
                break;
        }
        printf(" %-9d %-30s", symbol_entry->st_size, symbol_name);

        printf("\n");
        free(symbol_name);
    }
}

void print_symbols() {
    Elf32_Shdr *section_header;
    int sections_count;
    if (!elf_section_names_string_table(elf_header)) {
        return;
    }

    section_header = (Elf32_Shdr*)(map_start + elf_header->e_shoff);
    sections_count = elf_header->e_shnum;
    for (int i = 0; i < sections_count; ++i, ++section_header) {
        if (section_header->sh_type != SHT_SYMTAB) {
            continue;
        }
        if (section_header->sh_link == SHN_UNDEF) {
            dbg_printf("WARN: symbol table without string table (%d section header index)\n", i);
            continue;
        }

        print_symbol_table(section_header);
    }
}

void print_relocation_table(Elf32_Shdr *relocation_section_header) {
    Elf32_Rel *relocation_entry;
    char *relocation_section_name;
    int relocations_count;
    
    Elf32_Shdr *symbol_section_header;
    Elf32_Shdr *symbol_table_string_table_section_header;
    char *p_symbol_table_string_table;
    int symbol_table_string_table_size;

    relocations_count = relocation_section_header->sh_size / relocation_section_header->sh_entsize;

    symbol_section_header = get_section_header(relocation_section_header->sh_link);
    symbol_table_string_table_section_header = get_section_header(symbol_section_header->sh_link);
    p_symbol_table_string_table = (char*)(map_start + symbol_table_string_table_section_header->sh_offset);
    symbol_table_string_table_size = symbol_table_string_table_section_header->sh_size;

    elf_name_of_section(relocation_section_header, &relocation_section_name);
    printf("\nRelocation section '%s' at offset 0x%X contains 8 entries:\n", relocation_section_name, relocation_section_header->sh_offset);
    printf("Offset%*s Info%*s Type Sym-Value%*s Sym-Name%*s\n", 4, "", 6, "", 1, "", 11, "");

    relocation_entry = (Elf32_Rel*)(map_start + relocation_section_header->sh_offset);
    for (int i = 0; i < relocations_count; ++i, ++relocation_entry) {
        Elf32_Sym *symbol_entry = (Elf32_Sym*)(map_start + symbol_section_header->sh_offset + (ELF32_R_SYM(relocation_entry->r_info) * symbol_section_header->sh_entsize));
        char *symbol_name;
        int len;
        char* p_sym_str = p_symbol_table_string_table + symbol_entry->st_name;
        int max_read_amount = symbol_table_string_table_size - (p_sym_str - p_symbol_table_string_table);
        fill_next_str_in_string_table(p_sym_str, &symbol_name, max_read_amount, &len);

        printf(
            "0x%08X 0x%08X %-4d 0x%08X %-20s",
            relocation_entry->r_offset,
            relocation_entry->r_info,
            ELF32_R_TYPE(relocation_entry->r_info),
            symbol_entry->st_value,
            symbol_name
        );
        printf("\n");
    }
}

void relocation_table() {
    Elf32_Shdr *section_header;
    int sections_count;
    if (!elf_section_names_string_table(elf_header)) {
        return;
    }

    section_header = (Elf32_Shdr*)(map_start + elf_header->e_shoff);
    sections_count = elf_header->e_shnum;
    for (int i = 0; i < sections_count; ++i, ++section_header) {
        if (section_header->sh_type != SHT_REL) {
            continue;
        }
        if (section_header->sh_link == SHN_UNDEF) {
            dbg_printf("WARN: relocation table without symbol table (%d section header index)\n", i);
            continue;
        }

        print_relocation_table(section_header);
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
INP_LOOP print_symbols_action() {
    if (mapped) {
        print_symbols();
    }
    else {
        printf("No ELF file is loaded.\n");
    }
    return INP_LOOP_CONTINUE;
}
INP_LOOP relocation_table_action() {
    if (mapped) {
        relocation_table();
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
    { "Print Symbols", print_symbols_action },
    { "Relocation Tables", relocation_table_action },
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
    reset_map_values();
    do_input_loop(menu);
    cleanup();
    return 0;
}