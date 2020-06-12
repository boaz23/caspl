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

#define INVALID_FILE -1
#define INVALID_LEN  -1
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

// ------------------
// ----- task 0 -----
// ------------------
typedef char* str_tab;

int Currentfd;
int map_length;
void *map_start;

Elf32_Ehdr *elf_header;

bool is_mapped() {
    return Currentfd > INVALID_FILE;
}

void reset_map_values() {
    map_start = NULL;
    map_length = INVALID_LEN;
    Currentfd = INVALID_FILE;

    elf_header = NULL;
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

int elf_get_section_entry_count(Elf32_Shdr *section_header) {
    return section_header->sh_size / section_header->sh_entsize;
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

void print_program_headers() {
    int program_headers_count = elf_header->e_phnum;
    Elf32_Phdr *program_header = offset_to_pointer(elf_header->e_phoff);

    printf(
        "%-10s %-10s %-10s %-10s %-7s %-7s %-5s %-6s\n",
        "Type",
        "Offset",
        "VirtAddr",
        "PhysAddr",
        "FileSiz",
        "MemSiz",
        "Flg",
        "Align"
    );
    for (int i = 0; i < program_headers_count; ++i, ++program_header) {
        printf(
            "%-10d 0x%08X 0x%08X 0x%08X 0x%05X 0x%05X %-5d 0x%04X\n",
            program_header->p_type,
            program_header->p_offset,
            program_header->p_vaddr,
            program_header->p_paddr,
            program_header->p_filesz,
            program_header->p_memsz,
            program_header->p_flags,
            program_header->p_align
        );
    }
}

void load_file(char *file_name) {    
    if (!map_file_to_memory(file_name, O_RDWR)) {
        return;
    }

    elf_header = (Elf32_Ehdr*)map_start;
    if (!elf_check_header()) {
        return;
    }

    print_program_headers();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage elf-ph <ELF-File>\n");
        return 0;
    }

    reset_map_values();
    load_file(argv[1]);
    cleanup();
    return 0;
}