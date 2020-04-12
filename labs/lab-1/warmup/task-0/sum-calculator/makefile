all: main

main: main.o numbers.o add.o
	gcc -g -m32 -Wall main.o numbers.o add.o -o main

main.o: main.c
	gcc -g -m32 -Wall -c main.c -o main.o

numbers.o: numbers.c
	gcc -g -m32 -Wall -c numbers.c -o numbers.o

add.o: add.s
	nasm -g -f elf -w+all add.s -o add.o
