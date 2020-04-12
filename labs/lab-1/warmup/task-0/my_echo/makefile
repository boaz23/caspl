all: my_echo

my_echo: main.o
	gcc -g -m32 -Wall main.o -o my_echo
	rm *.o

main.o: main.c
	gcc -g -m32 -Wall -c main.c -o main.o

.PHONY: clean
clean:
	rm -f *.o my_echo
