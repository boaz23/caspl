PRG_NAME = flame2
MAIN_NAME = task2b
MAIN_OBJ = $(MAIN_NAME).o
MAIN_C = $(MAIN_NAME).c
OBJECTS = $(MAIN_OBJ) start.o util.o

compile: $(OBJECTS)
	ld -g -m elf_i386 $(OBJECTS) -o $(PRG_NAME)

start.o: start.s
	nasm -g -f elf32 start.s -o start.o

util.o: util.c util.h
	gcc -g -m32 -Wall -ansi -c -nostdlib -fno-stack-protector util.c -o util.o

$(MAIN_OBJ): $(MAIN_C)
	gcc -g -m32 -Wall -ansi -c -nostdlib -fno-stack-protector $(MAIN_C) -o $(MAIN_OBJ)

.PHONY: clean
clean:
	rm -f *.o $(PRG_NAME)