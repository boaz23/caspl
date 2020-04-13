#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
#define ARR_LEN(a) (sizeof((a))/sizeof(*(a)))

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
    if (v == NULL) {
        return NULL;
    }
    
    int len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (fread(v, sizeof(byte), len, file) < sizeof(byte)*len) {
        free(v);
        return NULL;
    }

    v->sig = (unsigned char*)malloc(v->SigSize);
    if (v->sig == NULL) {
        free(v);
        return NULL;
    }

    if (fread(v->sig, sizeof(unsigned char), v->SigSize, file) < sizeof(unsigned char)*v->SigSize) {
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

link* load_virus_list(link* virus_list, FILE *file) {
    link* new_virus_list = read_virus_list(file);
    if (new_virus_list != NULL) {
        list_free(virus_list);
        virus_list = new_virus_list;
    }
    return virus_list;
}

link* load_signatures_action(link* virus_list) {
    char file_name[256];
    printf("Enter file name: ");
    if (fgets(file_name, ARR_LEN(file_name), stdin) == NULL) {
        printf("bad input\n\n");
        return virus_list;
    }

    int last_char_index = strlen(file_name) - 1;
    if (file_name[last_char_index] == '\n') {
        file_name[last_char_index] = '\0';
    }
    
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("could not open '%s' for reading", file_name);
        return virus_list;
    }

    virus_list = load_virus_list(virus_list, file);
    fclose(file);
    return virus_list;
}

link* print_signatures_action(link* virus_list) {
    if (virus_list != NULL) {
        list_print(virus_list, stdout);
    }
    return virus_list;
}

link* quit_action(link *virus_list) {
    exit(0);
    return virus_list;
}

void print_menu(const menu_action_desc *menu) {
    const menu_action_desc *menu_item = menu;
    int i = 1;
    while (menu_item->name != NULL) {
        printf("%d) %s\n", i, menu_item->name);
        ++menu_item;
        ++i;
    }
}

int read_option(int *option) {
    char option_buffer[256];
    if (fgets(option_buffer, ARR_LEN(option_buffer), stdin) == NULL) {
        return 0;
    }

    int parseResult = sscanf(option_buffer, "%d", option);
    if (parseResult == EOF || parseResult == 0) {
        return 0;
    }

    return 1;
}

int main(int argc, char **argv) {
    link* virus_list = NULL;
    const menu_action_desc menu[] = {
        { "Load signatures", load_signatures_action },
        { "Print signatures", print_signatures_action },
        { "Quit", quit_action },
        { NULL, NULL }
    };

    while (1) {
        int option = 0;

        printf("Please choose an action:\n");
        print_menu(menu);

        printf("Option: ");
        if (!read_option(&option)) {
            printf("bad input\n\n");
        }

        if (!(1 <= option && option <= ARR_LEN(menu))) {
            printf("invalid action chosen\n\n");
            break;
        }

        virus_list = menu[option - 1].f(virus_list);
        printf("DONE.\n\n");
    }
    return 0;
}