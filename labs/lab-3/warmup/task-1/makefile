all: task1c.o
	gcc -g -m32 -Wall task1c.o -o task1c

task1c.o: task1c.c
	gcc -g -m32 -Wall -c task1c.c -o task1c.o

.PHONY: clean
clean:
	rm -f *.o task1c