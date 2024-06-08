#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H 1

#include <stdint.h>
#include <stddef.h>


typedef struct mtag_ptr_s mtag_ptr_t;
typedef struct node_s node_t;

struct mtag_ptr_s {
    node_t *ptr;
    uint64_t tag;
};

#define DATA_SIZE (NODE_SIZE - KEY_SIZE - 16)

#define KEY_SIZE 64
#define NODE_SIZE 128

typedef struct node_s {
    mtag_ptr_t next;

    uint8_t key[KEY_SIZE];
    uint8_t data[DATA_SIZE];
} node_t;

/*
extern int hashtable_init(size_t hash_table_size);
extern int hashtable_insert(int key, int value);
extern int hashtable_find(int key, int* value);
extern int hashtable_remove(int key);
*/
#endif
