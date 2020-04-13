#include <stdio.h>
#include <stdlib.h>

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
   void (*f)(); 
} menu_action_desc;

void PrintHex(FILE *file, char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        fprintf(file, "%02hhX ", (unsigned char)buffer[i]);
    }
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
void list_print(link *virus_list, FILE*); 
 
/* Add a new link with the given data to the list 
   (either at the end or the beginning, depending on what your TA tells you),
   and return a pointer to the list (i.e., the first link in the list).
   If the list is null - create a new entry and return a pointer to the entry. */
link* list_append(link* virus_list, virus* data); 
 
/* Free the memory allocated by the list. */
void list_free(link *virus_list);

int main(int argc, char **argv) {
    menu_action_desc menu[] = {
        { "Load signatures", NULL },
        { "Print signatures", NULL },
        { "Quit", NULL },
        { NULL, NULL }
    };
    
    while (1) {
        const menu_action_desc *menu_item = menu;
        int i = 0;
        char option_buffer[256];
        int option = 0;

        printf("Please choose an action:\n");
        while (menu_item->name != NULL) {
            printf("%d) %s\n", i, menu->name);
            ++menu_item;
            ++i;
        }

        printf("Option: ");
        if (fgets(option_buffer, ARR_LEN(option_buffer), stdin) == NULL) {
            printf("bad input\n\n");
            continue;
        }
        int parseResult = sscanf(option_buffer, "%d", &option);
        if (parseResult == EOF || parseResult == 0) {
            printf("bad input\n\n");
            continue;
        }

        --option;
        if (0 <= option && option < ARR_LEN(menu)) {
            menu[option].f();
        }
        else {
            printf("invalid action chosen\n\n");
            break;
        }

        printf("DONE.\n\n");
    }
    return 0;
}