#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

typedef enum bool {
    FALSE = 0,
    TRUE = 1
} bool;

bool read_struct(FILE *f, void *p_struct, int size_struct) {
    if (fread(p_struct, sizeof(char), size_struct, f) < size_struct) {
        perror("fread");
        return FALSE;
    }

    return TRUE;
}

bool file_seek(FILE *f, int offset) {
    if (fseek(f, offset, SEEK_SET) < 0) {
        perror("fseek");
        return FALSE;
    }

    return TRUE;
}

bool fill_next_str_in_string_table(FILE *f, char **p_buf, int max_read_amount, int *read_amount) {
    int read;
    int read_counter = 0;
    char c;
    char *buf;
    *p_buf = NULL;
    int current_file_offset = ftell(f);
    if (current_file_offset < 0) {
        return FALSE;
    }
    while (read_counter < max_read_amount) {
        read = fread(&c, sizeof(char), 1, f);
        if (read < 0) {
            perror("fread");
            return FALSE;
        }
        else if (read == 0) {
            return FALSE;
        }
        else {
            ++read_counter;
            if (c == '\0') {
                break;
            }
        }
    }
    if (!file_seek(f, current_file_offset)) {
        return FALSE;
    }

    buf = malloc(read_counter);
    for (int i = 0; i < read_counter; ++i) {
        read = fread(&c, sizeof(char), 1, f);
        if (read < 0) {
            perror("fread");
            free(buf);
            return FALSE;
        }
        else if (read == 0) {
            free(buf);
            return FALSE;
        }
        else {
            buf[i] = c;
        }
    }
    
    *p_buf = buf;
    *read_amount = read_counter;
    return TRUE;
}

bool find_string_in_string_table(FILE *f, char *s, int table_file_offset, int section_size, int *index) {
    char *buf = NULL;
    int left_to_read_in_section = section_size;
    int current_index = 0;
    int len;
    if (!file_seek(f, table_file_offset)) {
        return FALSE;
    }
    
    while (1) {
        if (!fill_next_str_in_string_table(f, &buf, left_to_read_in_section, &len)) {
            return FALSE;
        }
        if (buf == NULL) {
            *index = -1;
            break;
        }
        
        if (strcmp(s, buf) == 0) {
            *index = current_index;
            break;
        }

        free(buf);
        left_to_read_in_section -= len;
        current_index += len;
    }

    free(buf);
    return TRUE;
}

bool find_string_in_section(FILE *f, int section_header_offset, char *s, int *index) {
    Elf32_Shdr section_header_entry;
    if (!file_seek(f, section_header_offset)) {
        return FALSE;
    }
    if (!read_struct(f, &section_header_entry, sizeof(Elf32_Shdr))) {
        return FALSE;
    }

    return find_string_in_string_table(f, s, section_header_entry.sh_offset, section_header_entry.sh_size, index);
}

bool find_section_header(FILE *f, int name_index, int section_headers_count, Elf32_Shdr **section_header_entry) {
    for (int i = 0; i < section_headers_count; ++i) {
        if (!read_struct(f, *section_header_entry, sizeof(Elf32_Shdr))) {
            return FALSE;
        }

        if ((*section_header_entry)->sh_name == (Elf32_Word)name_index) {
            return TRUE;
        }
    }
    
    *section_header_entry = NULL;
    return TRUE;
}

int offset_of_section_header(Elf32_Ehdr *elf_header, int section_header_index) {
    return elf_header->e_shoff + (section_header_index * elf_header->e_shentsize);
}

void print_main_info_of_file(FILE *f) {
    Elf32_Ehdr elf_header;
    Elf32_Shdr sym_table_section_header, *p_sym_table_section_header = &sym_table_section_header;
    int sym_table_string_table_index;
    int main_sym_name_index;
    
    if (!read_struct(f, &elf_header, sizeof(elf_header))) {
        return;
    }
    if (strncmp("ELF", (char*)(&elf_header.e_ident) + 1, 3) != 0) {
        printf("bad elf header\n");
        return;
    }
     
    if (!find_string_in_section(
        f,
        offset_of_section_header(&elf_header, elf_header.e_shstrndx),
        ".symtab",
        &sym_table_string_table_index)) {
        return;
    }
    if (sym_table_string_table_index < 0) {
        printf("sym table not found\n");
        return;
    }

    if (!file_seek(f, elf_header.e_shoff)) {
        return;
    }
    if (!find_section_header(f, sym_table_string_table_index, elf_header.e_shnum, &p_sym_table_section_header)) {
        return;
    }
    if (p_sym_table_section_header == NULL) {
        printf("sym table section header entry not found\n");
        return;
    }
     
    if (!find_string_in_section(
        f,
        offset_of_section_header(&elf_header, sym_table_section_header.sh_link),
        "main",
        &main_sym_name_index)) {
        return;
    }
    if (main_sym_name_index < 0) {
        printf("sym table not found\n");
        return;
    }
}

void print_main_info(char *file_name) {
    FILE *f = NULL;
    f = fopen(file_name, "r");
    if (!f) {
        return;
    }

    print_main_info_of_file(f);
    fclose(f);
}

int main(int argc, char *argv[]) {
    print_main_info("abc");
    return 0;
}