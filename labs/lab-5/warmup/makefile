CC			:= gcc
CC_FLAGS	:= -g -m32 -Wall
ASM			:= nasm
ASM_FLAGS	:= -g -f elf
LINK		:= ld
LINK_FLAGS	:= -g -m elf_i386

SRC_DIR		:= .
OBJ_DIR		:= bin
LIST_DIR	:= bin
BIN_DIR		:= bin

PRG_NAME	:= task1
OBJECTS		:= $(OBJ_DIR)/task1.o $(OBJ_DIR)/LineParser.o

all: task1 looper

task1: $(OBJECTS)
	$(CC) $(CC_FLAGS) -o $(BIN_DIR)/$(PRG_NAME) $(OBJECTS)

looper: $(OBJ_DIR)/looper.o
	$(CC) $(CC_FLAGS) -o $(BIN_DIR)/looper $(OBJ_DIR)/looper.o

# .c/.s compile rulesint
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ASM) $(ASM_FLAGS) $< -o $@ -l $(subst .o,.lst,$(subst $(OBJ_DIR),$(LIST_DIR),$@))

.PHONY: clean
clean:
	rm -f $(PRG_NAME) looper $(OBJ_DIR)/*.o $(LIST_DIR)/*.lst