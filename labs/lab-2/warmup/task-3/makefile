CC = gcc
CFLAGS = -g -m32 -Wall -c -pedantic
LINK_FLAGS = -g -m32 -Wall

PRGM_NAME =  menu_map
OBJECTS = menu_map.o

compile: menu_map

menu_map: $(OBJECTS)
	$(CC) $(LINK_FLAGS) -o $(PRGM_NAME) $(OBJECTS)

menu_map.o: menu_map.c
	$(CC) $(CFLAGS) -o menu_map.o menu_map.c

.PHONY: clean
clean:
	rm *.o $(PRGM_NAME)