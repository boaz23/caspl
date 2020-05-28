#include <stdlib.h>
#include <stdio.h>

#define dbg_out stderr

#define ARR_LEN(a) ((sizeof((a))) / (sizeof(*(a))))

typedef struct {
  char debug_mode;
  char file_name[128];
  int unit_size;
  unsigned char mem_buf[10000];
  size_t mem_count;
  /*
   .
   .
   Any additional fields you deem necessary
  */
} state;

void toggle_debug_mode_act(state *s) {
    s->debug_mode = 1 - s->debug_mode;
    if (s->debug_mode) {
        fprintf(dbg_out, "Debug flag now on\n");
    }
    else {
        fprintf(dbg_out, "Debug flag now off\n");
    }
}
void set_file_name_act(state *s) {

}
void set_unit_size_act(state *s) {
    
}
void quit_act(state *s) {
    exit(0);
}

typedef void (*menu_func)(state *s);
typedef struct menu_item {
    char *name;
    menu_func func;
} menu_item;

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

void print_menu(menu_item const menu[]) {
    menu_item const *current_menu_item = menu;
    int i = 0;
    while (current_menu_item->name != NULL) {
        printf("%d)-%s\n", i, current_menu_item->name);
        ++current_menu_item;
        ++i;
    }
}

menu_func get_menu_func(menu_item const menu[], int option, int len) {
    if (1 < option && option < len) {
        return menu[option].func;
    }

    return NULL;
}

void invoke_menu_action(menu_func func, state *s) {
    func(s);
}

int main(int argc, char *argv[]) {
    menu_item menu[] = {
        { "Toggle Debug Mode", toggle_debug_mode_act },
        { "Set File Name", set_file_name_act },
        { "Set Unit Size", set_unit_size_act },
        { "Quit", quit_act },
        { NULL, NULL },
    };
    state s = {
        0, "", 1, "", 0
    };

    while (1) {
        int option = -1;
        printf("Choose action:");
        print_menu(menu);
        if (!input_int(&option)) {
            continue;
        }

        menu_func func = get_menu_func(menu, option, ARR_LEN(menu));
        if (func == NULL) {
            break;
        }

        invoke_menu_action(func, &s);
        printf("DONE.\n\n");
    }

    return 0;
}