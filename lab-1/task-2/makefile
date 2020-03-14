CC = gcc
CFLAGS = -g -Wall -m32 -c

ASM = nasm
ASMFLAGS = -g -w+all -f -elf32

LINKFLAGS = -g -Wall -m32

PRGMNAME = encoder
OBJECTS = encoder.o

compile: encoder

encoder: $(OBJECTS)
	$(CC) $(LINKFLAGS) -o $(PRGMNAME) $(OBJECTS)

encoder.o: encoder.c
	$(CC) $(CFLAGS) -o encoder.o encoder.c

.PHONY: clean
clean:
	rm -f *.o $(PRGMNAME)
