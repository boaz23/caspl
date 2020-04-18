#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
#define ARR_LEN(a) (sizeof((a))/sizeof(*(a)))

byte const NOP_OPCODE = 0x90;

int read_int(int *p_num);

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

typedef struct link link;
 
struct link {
    link *nextVirus;
    virus *vir;
};

typedef struct menu_action_desc {
   char *name;
   link* (*f)(link*); 
} menu_action_desc;

void PrintHex(FILE *file, char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        fprintf(file, "%02hhX ", (unsigned char)buffer[i]);
    }
}

void virus_destruct(virus *v) {
    free(v->sig);
    free(v);
}

virus* readVirus(FILE *file) {
    virus *v = (virus*)malloc(sizeof(virus));
    int len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (v == NULL) {
        return NULL;
    }
    
    len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (fread(v, sizeof(byte), len, file) < len) {
        free(v);
        return NULL;
    }

    v->sig = (unsigned char*)malloc(v->SigSize);
    if (v->sig == NULL) {
        free(v);
        return NULL;
    }

    if (fread(v->sig, sizeof(unsigned char), v->SigSize, file) < v->SigSize) {
        free(v->sig);
        free(v);
        return NULL;
    }
    
    return v;
}

void printVirus(virus *virus, FILE *output) {
    int i = 0;
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);

    fprintf(output, "signature:\n");
    for (i = 0; i + 20 < virus->SigSize; i += 20) {
        PrintHex(output, (char*)(virus->sig + i), 20);
        fprintf(output, "\n");
    }
    PrintHex(output, (char*)(virus->sig + i), virus->SigSize - i);
    fprintf(output, "\n");
}

/* Print the data of every link in list to the given stream. Each item followed by a newline character. */
void list_print(link *virus_list, FILE* file) {
    while (virus_list != NULL) {
        printVirus(virus_list->vir, file);
        fprintf(file, "\n");
        virus_list = virus_list->nextVirus;
    }
}
 
/* Add a new link with the given data to the list 
   (either at the end or the beginning, depending on what your TA tells you),
   and return a pointer to the list (i.e., the first link in the list).
   If the list is null - create a new entry and return a pointer to the entry. */
link* list_append(link* virus_list, virus* data) {
    link **head = &virus_list;
    link **prev = &virus_list;
    link *new = NULL;
    while (*prev != NULL) {
        prev = &((*prev)->nextVirus);
    }
    new = (link*)malloc(sizeof(link));
    new->vir = data;
    new->nextVirus = NULL;
    *prev = new;
    return *head;
}
 
/* Free the memory allocated by the list. */
void list_free(link *virus_list) {
    while (virus_list != NULL) {
        link *next = virus_list->nextVirus;
        virus_destruct(virus_list->vir);
        free(virus_list);
        virus_list = next;
    }
}

link* read_virus_list(FILE *file) {
    link* virus_list = NULL;
    while (!feof(file)) {
        virus *v = readVirus(file);
        if (v == NULL) {
            break;
        }

        virus_list = list_append(virus_list, v);
    }

    return virus_list;
}

link* load_signatures_action(link* virus_list) {
    char file_name[256];
    int last_char_index = -1;
    FILE *file = NULL;

    printf("Enter file name: ");
    if (fgets(file_name, ARR_LEN(file_name), stdin) == NULL) {
        printf("bad input\n\n");
        return virus_list;
    }

    last_char_index = strlen(file_name) - 1;
    if (file_name[last_char_index] == '\n') {
        file_name[last_char_index] = '\0';
    }
    
    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("could not open the file '%s' for reading\n", file_name);
        return virus_list;
    }

    virus_list = read_virus_list(file);
    fclose(file);
    return virus_list;
}

link* print_signatures_action(link* virus_list) {
    if (virus_list != NULL) {
        printf("\n");
        list_print(virus_list, stdout);
    }
    return virus_list;
}

link* quit_action(link *virus_list) {
    exit(0);
    return virus_list;
}

void print_virus_detected(unsigned int offset, virus *v) {
    printf("\n");
    printf("virus found\n");
    printf("offset: 0x%X (%d)\n", offset, offset);
    printf("virus name: %s\n", v->virusName);
    printf("signature size: %d\n", v->SigSize);
}

void detect_single_virus(char *buffer, unsigned int size, virus *v) {
    char *p_buffer_pos = buffer;
    for (unsigned int i = 0; i < size; ++i, ++p_buffer_pos) {
        if (memcmp(p_buffer_pos, v->sig, v->SigSize) == 0) {
            print_virus_detected(i, v);
        }
    }
}

void detect_virus(char *buffer, unsigned int size, link *virus_list) {
    link *current = virus_list;
    while (current != NULL) {
        detect_single_virus(buffer, size, current->vir);
        current = current->nextVirus;        
    }
}

char *Suspected_File_Name = NULL;

void detect_viruses_in_file(link *virus_list, char *file_name) {
    char file_content[10 * (1 << 10)];
    FILE* file = NULL;
    unsigned int size = 0;
    
    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("cannot open the file '%s' for reading\n", file_name);
        return;
    }

    size = fread(file_content, sizeof(char), ARR_LEN(file_content), file);
    if (ferror(file)) {
        printf("an error has occurred while reading the file '%s'\n", file_name);
    }
    else {
        detect_virus(file_content, size, virus_list);
    }

    fclose(file);
}

link* detect_viruses_action(link *virus_list) {
    if (virus_list != NULL) {
        detect_viruses_in_file(virus_list, Suspected_File_Name);
    }

    return virus_list;
}

void kill_virus(char *fileName, int signitureOffset, int signitureSize) {
    FILE *file = fopen(fileName, "r+");
    if (file == NULL) {
        printf("failed to open the file '%s'\n", fileName);
        return;
    }
    if (fseek(file, signitureOffset, SEEK_SET) != 0) {
        printf("failed to seek to the requested offset\n");
        return;
    }

    for (int i = 0; i < signitureSize; ++i) {
        int w_offset = signitureOffset + i;
        if (fwrite(&NOP_OPCODE, sizeof(byte), 1, file) < 1) {
            printf("failed to write to the file '%s' at offset %d", fileName, w_offset);
            break;
        }
    }

    fclose(file);
}

link* fix_file_action(link *virus_list) {
    int offset, signature_size;
    printf("Enter starting offset: ");
    if (!read_int(&offset)) {
        printf("invalid input\n");
    }

    printf("Enter signature size: ");
    if (!read_int(&signature_size)) {
        printf("invalid input\n");
    }

    kill_virus(Suspected_File_Name, offset, signature_size);
    return virus_list;
}

void print_menu(const menu_action_desc *menu) {
    int i = 1;
    const menu_action_desc *menu_item = menu;
    while (menu_item->name != NULL) {
        printf("%d) %s\n", i, menu_item->name);
        ++menu_item;
        ++i;
    }
}

int read_int(int *p_num) {
    char option_buffer[256];
    int parse_err = -1;
    if (fgets(option_buffer, ARR_LEN(option_buffer), stdin) == NULL) {
        return 0;
    }

    parse_err = sscanf(option_buffer, "%d", p_num);
    if (parse_err == EOF || parse_err == 0) {
        return 0;
    }

    return 1;
}

link* invoke_menu_action(link *virus_list, link* (*f)(link*)) {
    link *temp_list = virus_list;
    virus_list = f(virus_list);
    if (temp_list != NULL && temp_list != virus_list) {
        list_free(temp_list);
    }

    return virus_list;
}

int main(int argc, char **argv) {
    link* virus_list = NULL;
    const menu_action_desc menu[] = {
        { "Load signatures", load_signatures_action },
        { "Print signatures", print_signatures_action },
        { "Detect viruses", detect_viruses_action },
        { "Fix file", fix_file_action },
        { "Quit", quit_action },
        { NULL, NULL }
    };

    Suspected_File_Name = argv[1];
    while (1) {
        int option = 0;

        printf("Please choose an action:\n");
        print_menu(menu);

        printf("Option: ");
        if (!read_int(&option)) {
            printf("bad input\n\n");
        }

        if (!(1 <= option && option <= ARR_LEN(menu))) {
            printf("invalid action chosen\n\n");
            break;
        }

        virus_list = invoke_menu_action(virus_list, menu[option - 1].f);
        printf("DONE.\n\n");
    }
    
    if (virus_list != NULL) {
        list_free(virus_list);
    }
    return 0;
}