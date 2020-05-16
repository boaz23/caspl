CC = gcc
CFLAGS = -g -m32 -Wall -c
LINK_FLAGS = -g -m32 -Wall

OBJECTS = encoder.o
PRGM_NAME = encoder

compile: encoder

encoder: $(OBJECTS)
	$(CC) $(LINK_FLAGS) -o $(PRGM_NAME) $(OBJECTS)

encoder.o: encoder.c
	$(CC) $(CFLAGS) -o encoder.o encoder.c

.PHONY: clean
clean:
	rm *.o $(PRGM_NAME)