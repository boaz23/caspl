hexaPrint: hexaPrint.o
	gcc -g -m32 -Wall hexaPrint.o -o hexaPrint

hexaPrint.o: hexaPrint.c
	gcc -g -m32 -Wall -c hexaPrint.c -o hexaPrint.o

.PHONY: clean
clean:
	rm -f *.o hexaPrint