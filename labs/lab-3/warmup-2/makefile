compile: task2b.o
	gcc -g -m32 -Wall task2b.o -o virusDetector
	
task2b.o: task2b.c
	gcc -g -m32 -Wall -c task2b.c -o task2b.o

.PHONY: clean
clean:
	rm -f *.o virusDetector