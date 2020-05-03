PRG_NAME = task1b
MAIN_OBJ = $(PRG_NAME).o
OBJECTS = $(MAIN_OBJ) start.o util.o

compile: $(OBJECTS)
	ld -g -m elf_i386 $(OBJECTS) -o $(PRG_NAME)

start.o:
	nasm -g -f elf32 start.s -o start.o

util.o:
	gcc -g -m32 -Wall -ansi -c -nostdlib -fno-stack-protector util.c -o util.o

$(MAIN_OBJ):
	gcc -g -m32 -Wall -ansi -c -nostdlib -fno-stack-protector $(PRG_NAME).c -o $(MAIN_OBJ)

.PHONY: clean
clean:
	rm -f *.o $(PRG_NAME)