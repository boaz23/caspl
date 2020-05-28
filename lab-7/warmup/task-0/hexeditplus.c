#include <stdlib.h>

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

typedef void (*menu_func)(state *s);
typedef struct menu_act {
    char *name;
    menu_func *func;
} menu_act;

void toggle_debug_mode_act(state *s);
void set_file_name_act(state *s);
void set_unit_size_act(state *s);
void quit_act(state *s);

int main(int argc, char *argv[]) {
    menu_act menu[] = {
        { "Toggle Debug Mode", toggle_debug_mode_act },
        { "Set File Name", set_file_name_act },
        { "Set Unit Size", set_unit_size_act },
        { "Quit", quit_act },
        { NULL, NULL },
    };
    return 0;
}