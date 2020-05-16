#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

void PrintHex(FILE *file, unsigned char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        fprintf(file, "%02hhX ", buffer[i]);
    }
}

void virus_destruct(virus *v) {
    if (v != NULL) {
        free(v->sig);
        free(v);
    }
}

virus* readVirus(FILE* file) {
    virus *v = malloc(sizeof(virus));
    int len = 0;

    if (v == NULL) {
        return NULL;
    }

    len = sizeof(v->SigSize) + sizeof(v->virusName);
    if (fread(v, sizeof(char), len, file) < len*sizeof(char)) {
        free(v);
        return NULL;
    }

    v->sig = malloc(v->SigSize);
    if (v->sig == NULL) {
        free(v);
        return NULL;
    }

    if (fread(v->sig, sizeof(unsigned char), v->SigSize, file) < v->SigSize*sizeof(unsigned char)) {
        virus_destruct(v);
        return NULL;
    }

    return v;
}

void print_virus_sig(virus *v, FILE *file, int bytes_in_row) {
    int i = 0;
    int limit = v->SigSize - bytes_in_row;
    for (i = 0; i < limit; i += bytes_in_row) {
        PrintHex(file, &v->sig[i], bytes_in_row);
        fprintf(file, "\n");
    }
    PrintHex(file, &v->sig[i], v->SigSize - i);
    fprintf(file, "\n");
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

link* link_create(link *next, virus *v) {
    link *new_link = malloc(sizeof(link));
    new_link->vir = v;
    new_link->nextVirus = next;
    return new_link;
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
    return link_create(virus_list, data);
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

// ----------------------------------------
// --------- BEGIN USER INTERFACE ---------
// ----------------------------------------

char *SuspectedFileName = NULL;
unsigned char NOP = 0x90;

int input_int(int *n) {
    char buffer[256];
    int scan_result = 0;
    if (fgets(buffer, ARR_LEN(buffer), stdin) == NULL) {
        printf("invalid input\n");
        return 0;
    }
    
    scan_result = sscanf(buffer, "%d", n);
    if (scan_result == EOF || scan_result == 0) {
        printf("invalid input\n");
        return 0;
    }
    
    return 1;
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

link* read_viruses_from_file(FILE *file) {
    link *vl = NULL;
    while (!feof(file)) {
        virus *v = readVirus(file);
        if (v == NULL) {
            if (!feof(file)) {
                list_free(vl);
                vl = NULL;
            }
            break;
        }
        vl = list_append(vl, v);
    }

    return vl;
}

void print_virus_detected(unsigned int offset, virus *v) {
    printf("Start offset: %X (%d)\n", offset, offset);
    printf("Virus name: %s\n", v->virusName);
    printf("Signature size: %d\n", v->SigSize);
}

void detect_virus_single(unsigned char *buffer, unsigned int size, virus *v) {
    unsigned int offset = 0;
    unsigned char *p_buffer_offset = buffer;
    while (offset < size) {
        if (memcmp(p_buffer_offset, v->sig, v->SigSize) == 0) {
            printf("\n");
            print_virus_detected(offset, v);
        }

        ++offset;
        ++p_buffer_offset;
    }
}

void detect_virus(char *buffer, unsigned int size, link *virus_list) {
    unsigned char *u_buffer = (unsigned char*)buffer;
    link *current = virus_list;
    while (current != NULL) {
        detect_virus_single(u_buffer, size, current->vir);
        current = current->nextVirus;
    }
}

void detect_viruses_in_file(char *file_name, link *vl) {
    char buffer[10 * 1000];
    FILE *file = NULL;
    int size = -1;

    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("failed to open the file '%s' for reading\n", file_name);
        return;
    }

    size = fread(buffer, sizeof(char), ARR_LEN(buffer), file);
    fclose(file);
    detect_virus(buffer, size, vl);
}

void fill_file(FILE *file, int amount, unsigned char b) {
    for (int i = 0; i < amount; ++i) {
        if (fwrite(&b, sizeof(unsigned char), 1, file) < 1*sizeof(unsigned char)) {
            printf("error writing to the file\n");
            break;
        }
    }
}

void kill_virus_file(FILE *file, int signitureOffset, int signitureSize) {
    if (fseek(file, signitureOffset, SEEK_SET) != 0) {
        printf("failed to seek in the file\n");
        return;
    }

    fill_file(file, signitureSize, NOP);
}

void kill_virus(char *fileName, int signitureOffset, int signitureSize) {
    FILE *file = fopen(fileName, "r+");
    if (file == NULL) {
        printf("failed to open the file '%s'\n", fileName);
        return;
    }

    kill_virus_file(file, signitureOffset, signitureSize);
    fclose(file);
}

void fix_file(char *file_name) {
    int start_offset = -1;
    int size = -1;

    printf("Enter starting offset: ");
    if (!input_int(&start_offset)) {
        return;
    }

    printf("Enter size: ");
    if (!input_int(&size)) {
        return;
    }

    kill_virus(file_name, start_offset, size);
}

link* load_signatures_action(link *vl) {
    char filename_buffer[256];
    FILE *file = NULL;

    printf("Enter file name: ");
    if (!input_filename(filename_buffer, ARR_LEN(filename_buffer))) {
        return vl;
    }

    file = fopen(filename_buffer, "r");
    if (file == NULL) {
        printf("failed to open the file '%s' for reading\n", filename_buffer);
        return vl;
    }

    vl = read_viruses_from_file(file);
    if (vl == NULL) {
        printf("an error occurred while reading viruses from file '%s'\n", filename_buffer);
    }
    fclose(file);
    return vl;
}

link* print_signatures_action(link *vl) {
    if (vl != NULL) {
        printf("\n");
        list_print(vl, stdout);
    }
    return vl;
}

link* detect_viruses_action(link *vl) {
    if (SuspectedFileName == NULL) {
        printf("No suspected file was given as an argument\n");
    }
    else {
        detect_viruses_in_file(SuspectedFileName, vl);
    }

    return vl;
}

link* fix_file_action(link *vl) {
    if (SuspectedFileName == NULL) {
        printf("No suspected file was given as an argument\n");
    }
    else {
        fix_file(SuspectedFileName);
    }

    return vl;
}

link* quit_action(link *vl) {
    list_free(vl);
    exit(0);
    return vl;
}

typedef link* (*menu_func)(link*);

typedef struct menu_item {
    char *name;
    menu_func fun;
} menu_item;

void print_menu(menu_item const menu[]) {
    menu_item const *current_menu_item = menu;
    int i = 1;
    while (current_menu_item->name != NULL) {
        printf("%d) %s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

menu_func get_menu_func(menu_item const menu[], int option, int len) {
    if (1 <= option && option <= len) {
        return menu[option - 1].fun;
    }

    return NULL;
}

link* invoke_menu_action(link *vl, menu_func func) {
    link *temp = vl;
    vl = func(vl);
    if (temp != vl) {
        list_free(temp);
    }
    return vl;
}

int main(int argc, char **argv) {
    menu_item const menu[] = {
        { "Load signatures", load_signatures_action },
        { "Print signatures", print_signatures_action },
        { "Detect viruses", detect_viruses_action },
        { "Fix file", fix_file_action },
        { "Quit", quit_action },
        { NULL, NULL }
    };
    link *vl = NULL;

    if (argc > 1) {
        SuspectedFileName = argv[1];
    }

    while (1) {
        int option = -1;
        print_menu(menu);
        printf("Option: ");
        if (!input_int(&option)) {
            continue;
        }

        menu_func func = get_menu_func(menu, option, ARR_LEN(menu));
        if (func == NULL) {
            break;
        }

        vl = invoke_menu_action(vl, func);
        printf("DONE.\n\n");
    }
    list_free(vl);

    return 0;
}