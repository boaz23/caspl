CC     = gcc
CFLAGS = -g -m32 -Wall -ansi -c -nostdlib -fno-stack-protector

compile: start.o util.o main.o
	ld -m elf_i386 start.o util.o main.o -o task0

start.o: start.s
	nasm -f elf32 start.s -o start.o

util.o: util.c util.h
	$(CC) $(CFLAGS) util.c -o util.o

main.o: main.c
	$(CC) $(CFLAGS) main.c -o main.o

.PHONY: clean
clean:
	rm -f *.o task0