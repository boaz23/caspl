CC          := gcc
CC_FLAGS    := -g -m32 -Wall
ASM         := nasm
ASM_FLAGS   := -g -f elf
LINK        := ld
LINK_FLAGS  := -g -m elf_i386

SRC_DIR     := .
OBJ_DIR     := .
LIST_DIR    := .
BIN_DIR     := .

PRG_NAME    := myELF
OBJECTS     := $(OBJ_DIR)/task2.o

$(PRG_NAME): $(OBJECTS)
	$(CC) $(CC_FLAGS) -o $(BIN_DIR)/$(PRG_NAME) $(OBJECTS)

# .c/.s compile rulesint
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ASM) $(ASM_FLAGS) $< -o $@ -l $(subst .o,.lst,$(subst $(OBJ_DIR),$(LIST_DIR),$@))

.PHONY: clean
clean:
	rm -f $(PRG_NAME) $(OBJ_DIR)/*.o $(LIST_DIR)/*.lst
