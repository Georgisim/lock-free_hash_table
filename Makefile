CC=gcc

DEPS = freelist.h hash_table_test.h
OBJ = freelist.o hash_table.o 

BUILD := debug

cflags.common := 
cflags.debug := -g -O0 -Wall -D_DEBUG
cflags.release := -O2 -Wall
CFLAGS := ${cflags.${BUILD}} ${cflags.common}

.PHONY : all

all: hash_table_test  hash_table_perftest freelist_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

freelist_test: freelist.o freelist_test.o 
	$(CC) -o $@ $^ $(CFLAGS) -latomic

hash_table_test: freelist.o hash_table.o hash_table_test.o
	$(CC) -o $@ $^ $(CFLAGS) -latomic

hash_table_perftest: freelist.o hash_table.o hash_table_perftest.o
	$(CC) -o $@ $^ $(CFLAGS) -latomic

clean:
	rm *.o
	

