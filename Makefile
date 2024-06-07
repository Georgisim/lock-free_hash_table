CC=gcc
CFLAGS=-I. -g -O0
DEPS = freelist.h
OBJ = freelist.o hash_table.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

freelist_test: freelist.o freelist_test.o 
	$(CC) -o $@ $^ $(CFLAGS) -latomic


hash_table_test:  freelist.o hash_table.o hash_table_test.o
	$(CC) -o $@ $^ $(CFLAGS) -latomic
	
clean:
	rm *.o