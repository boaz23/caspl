CC		:=	gcc -m32
CC_FLAGS	:=	-Wall -g
ASM		:=	nasm
ASM_FLAGS	:=	-f elf -g
LINK		:=	ld

SRC_DIR		:=	src
OBJ_DIR		:=	obj
LIST_DIR	:=	list
BIN_DIR		:=	bin

all: task1 task2

task1:	$(OBJ_DIR)/main_task1Assignment1.o $(OBJ_DIR)/asm_task1Assignment1.o
	$(CC) -o $(BIN_DIR)/asm_task1Assignment1.bin $(OBJ_DIR)/main_task1Assignment1.o $(OBJ_DIR)/asm_task1Assignment1.o

task2: $(OBJ_DIR)/main_task2Assignment1.o $(OBJ_DIR)/asm_task2Assignment1.o
	$(CC) -o $(BIN_DIR)/asm_task2Assignment1.bin $(OBJ_DIR)/main_task2Assignment1.o $(OBJ_DIR)/asm_task2Assignment1.o

# .c/.s compile rules
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CC_FLAGS) $< -o $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.s
	$(ASM) $(ASM_FLAGS) $< -o $@ -l $(subst .o,.lst,$(subst $(OBJ_DIR),$(LIST_DIR),$@))

clean:
	rm $(BIN_DIR)/*.bin $(OBJ_DIR)/*.o $(LIST_DIR)/*.lst
