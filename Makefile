CC=gcc
CFLAGS=-I. -g -O0 -D_DEBUG 
DEPS = freelist.h hash_table_test.h
OBJ = freelist.o hash_table.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

freelist_test: freelist.o freelist_test.o 
	$(CC) -o $@ $^ $(CFLAGS) -latomic


hash_table_test:  freelist.o hash_table.o hash_table_test.o
	$(CC) -o $@ $^ $(CFLAGS) -latomic
	
clean:
	rm *.o
	
all:
	hash_table_test hash_table_test