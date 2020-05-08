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

PRG_NAME	:= main
OBJECTS		:= $(OBJ_DIR)/main.o $(OBJ_DIR)/LineParser.o

all: main looper

main: $(OBJECTS)
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
	rm -f $(BIN_DIR)/*.bin $(OBJ_DIR)/*.o $(LIST_DIR)/*.lst