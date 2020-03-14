CC = gcc
CFLAGS = -g -Wall -m32 -c

ASM = nasm
ASMFLAGS = -g -w+all -f -elf32

LINKFLAGS = -g -Wall -m32

PRGMNAME = encoder
OBJECTS = main.o

compile: encoder

encoder: $(OBJECTS)
	$(CC) $(LINKFLAGS) -o $(PRGMNAME) $(OBJECTS)

main.o: main.c
	$(CC) $(CFLAGS) -o main.o main.c

.PHONY: clean
clean:
	rm -f *.o $(PRGMNAME)