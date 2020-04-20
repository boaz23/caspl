#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARR_LEN(a) ((sizeof((a))) / sizeof(*(a)))

void PrintHex(FILE *file, unsigned char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        fprintf(file, "%02hhX ", buffer[i]);
    }
}

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

void virus_destruct(virus *v) {
    if (v != NULL) {
        free(v->sig);
        free(v);
    }
}

virus* readVirus(FILE* file) {
    virus *v = (virus*)malloc(sizeof(virus));
    unsigned char *sig = NULL;
    int len = 0;

    if (v == NULL) {
        return NULL;
    }

    len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (fread(v, sizeof(char), len, file) < len*sizeof(char)) {
        free(v);
        return NULL;
    }

    sig = (unsigned char*)malloc(v->SigSize);
    if (sig == NULL) {
        free(v);
        return NULL;
    }

    v->sig = sig;
    if (fread(sig, sizeof(unsigned char), v->SigSize, file) < v->SigSize*sizeof(unsigned char)) {
        virus_destruct(v);
        return NULL;
    }

    return v;
}

void print_virus_sig(virus* virus, FILE* output, int max_sig_bytes_amount_in_row) {
    int i = 0;
    int loop_limit = virus->SigSize - max_sig_bytes_amount_in_row;
    for (i = 0; i < loop_limit; i += max_sig_bytes_amount_in_row) {
        PrintHex(output, &virus->sig[i], max_sig_bytes_amount_in_row);
        fprintf(output, "\n");
    }
    PrintHex(output, &virus->sig[i], virus->SigSize - i);
    fprintf(output, "\n");
}

int const VIRUS_SIG_BYTES_IN_ROW = 20;

void printVirus(virus* virus, FILE* output) {
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);
    fprintf(output, "signature:\n");
    print_virus_sig(virus, output, VIRUS_SIG_BYTES_IN_ROW);
}

typedef struct link link;
 
struct link {
    link *nextVirus;
    virus *vir;
};

link* link_create(virus* data) {
    link *new_link = (link*)malloc(sizeof(link));
    new_link->vir = data;
    new_link->nextVirus = NULL;
    return new_link;
}

link** link_address_of_end(link **head) {
    link **current = head;
    while (*current != NULL) {
        current = &((*current)->nextVirus);
    }
    return current;
}

/* Print the data of every link in list to the given stream. Each item followed by a newline character. */
void list_print(link *virus_list, FILE* file) {
    link *current = virus_list;
    while (current != NULL) {
        printVirus(current->vir, file);
        fprintf(file, "\n");
        current = current->nextVirus;
    }
}
 
/* Add a new link with the given data to the list 
  (either at the end or the beginning, depending on what your TA tells you),
  and return a pointer to the list (i.e., the first link in the list).
  If the list is null - create a new entry and return a pointer to the entry. */
link* list_append(link* virus_list, virus* data) {
    link **head = &virus_list;
    link **prev = link_address_of_end(head);
    *prev = link_create(data);
    return *head;
}

/* Free the memory allocated by the list. */
void list_free(link *virus_list) {
    link *current = virus_list;
    while (current != NULL) {
        link *next = current->nextVirus;
        virus_destruct(current->vir);
        free(current);
        current = next;
    }
}

int input_int(int *n) {
    char buffer[256];
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }

    int scan_result = sscanf(buffer, "%d", n);
    if (scan_result == 0 || scan_result == EOF) {
        printf("invalid input\n");
        return 0;
    }

    return 1;
}

typedef link* (*menu_func)(link*);

typedef struct menu_item {
    char *name;
    menu_func fun;
} menu_item;

void print_menu(menu_item const menu[]) {
    int i = 1;
    menu_item const *current_menu_item = menu;
    while (current_menu_item->name != NULL) {
        printf("%d) %s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

menu_func get_fun(menu_item const *menu, int len, int option) {
    if (1 <= option && option <= len) {
        return menu[option - 1].fun;
    }
    else {
        return NULL;
    }
}

link* invoke_menu_function(link *virus_list, menu_func func) {
    link *temp = virus_list;
    virus_list = func(virus_list);
    if (temp != NULL && temp != virus_list) {
        list_free(temp);
    }
    return virus_list;
}

link* read_viruses_from_file(FILE *file) {
    link *virus_list = NULL;
    while (!feof(file)) {
        virus *v = readVirus(file);
        if (v == NULL) {
            if (!feof(file)) {
                list_free(virus_list);
                virus_list = NULL;
            }
            break;
        }

        virus_list = list_append(virus_list, v);
    }

    return virus_list;
}

int input_file_name(char *file_name_buffer, int buffer_len) {
    if (fgets(file_name_buffer, buffer_len, stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }

    int input_last_index = strlen(file_name_buffer) - 1;
    if (file_name_buffer[input_last_index] == '\n') {
        file_name_buffer[input_last_index] = '\0';
    }

    return 1;
}

link* load_signatures_action(link *virus_list) {
    char file_name[256];

    printf("Enter file name: ");
    if (!input_file_name(file_name, ARR_LEN(file_name))) {
        printf("invalid input\n");
        return virus_list;
    }

    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("could not open the file '%s' for reading\n", file_name);
        return virus_list;
    }

    virus_list = read_viruses_from_file(file);
    if (virus_list == NULL) {
        printf("an error occurred while reading viruses from the file '%s'\n", file_name);
    }
    fclose(file);
    return virus_list;
}

void print_signatures(link *virus_list) {
    if (virus_list != NULL) {
        printf("\n");
        list_print(virus_list, stdout);
    }
}

link* print_signatures_action(link *virus_list) {
    print_signatures(virus_list);
    return virus_list;
}

void detect_virus_single(char *buffer, unsigned int size, virus *v) {
    unsigned char *cmp_start = (unsigned char*)buffer;
    for (unsigned int offset = 0; offset < size; ++offset, ++cmp_start) {
        if (memcmp(cmp_start, v->sig, v->SigSize) == 0) {
            printf("\n");
            printf("Starting offset: %X (%d)\n", offset, offset);
            printf("Virus name: %s\n", v->virusName);
            printf("Signature size: %d\n", v->SigSize);
        }
    }
}

void detect_virus(char *buffer, unsigned int size, link *virus_list) {
    link *current = virus_list;
    while (current != NULL) {
        detect_virus_single(buffer, size, current->vir);
        current = current->nextVirus;
    }
}

void detect_viruses_in_file(char *file_name, link *virus_list) {
    char buffer[10 * 1000];
    unsigned int size = 0;
    FILE *file = NULL;

    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("could not open the file '%s' for reading\n", file_name);
        return;
    }
    
    size = fread(buffer, sizeof(unsigned char), ARR_LEN(buffer), file);
    detect_virus(buffer, size, virus_list);
    fclose(file);
}

char *SuspectedFileName = NULL;

link* detect_viruses_action(link *virus_list) {
    if (SuspectedFileName == NULL) {
        printf("No suspected file was given as a command line argument\n");
    }
    else {
        detect_viruses_in_file(SuspectedFileName, virus_list);
    }

    return virus_list;
}

void fill_file(FILE *file, int amount, unsigned char b) {
    for (int i = 0; i < amount; ++i) {
        if (fwrite(&b, sizeof(unsigned char), 1, file) == 0) {
            printf("an error occurred while writing to the file");
            break;
        }
    }
}

unsigned char NOP = 0x90;

void kill_virus(char *fileName, int signitureOffset, int signitureSize) {
    FILE *file = fopen(fileName, "r+");
    if (file == NULL) {
        printf("could not open the file '%s'\n", fileName);
        return;
    }

    if (fseek(file, signitureOffset, SEEK_SET) != 0) {
        printf("an error occurred while seeking in file '%s'\n", fileName);
        return;
    }

    fill_file(file, signitureSize, NOP);
    fclose(file);
}

void kill_virus_input(char *file_name) {
    int start_offset;
    int size;
    printf("Enter start offset: ");
    if (!input_int(&start_offset)) {
        return;
    }

    printf("Enter size: ");
    if (!input_int(&size)) {
        return;
    }

    kill_virus(file_name, start_offset, size);
}

link* kill_virus_action(link *virus_list) {
    if (SuspectedFileName == NULL) {
        printf("No suspected file was given as a command line argument\n");
    }
    else {
        kill_virus_input(SuspectedFileName);
    }

    return virus_list;
}

link* quit_action(link *virus_list) {
    list_free(virus_list);
    exit(0);
    return virus_list;
}

int main(int argc, char **argv) {
    menu_item const menu[] = {
        { "Load signatures", load_signatures_action },
        { "Print signatures", print_signatures_action },
        { "Detect viruses", detect_viruses_action },
        { "Fix file", kill_virus_action },
        { "Quit", quit_action },
        { NULL, NULL }
    };

    if (argc > 1) {
        SuspectedFileName = argv[1];
    }

    link* virus_list = NULL;
    while (1) {
        int option;
        print_menu(menu);
        printf("Option: ");
        if (!input_int(&option)) {
            printf("\n");
            continue;
        }

        menu_func func = get_fun(menu, ARR_LEN(menu), option);
        if (func == NULL) {
            break;
        }

        virus_list = invoke_menu_function(virus_list, func);
        printf("DONE.\n\n");
    }
    list_free(virus_list);

    return 0;
}