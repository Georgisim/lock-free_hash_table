#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H 1

#include <stdint.h>
#include <stddef.h>

typedef struct tag_ptr_s tag_ptr_t;
typedef struct node_s node_t;

struct tag_ptr_s {
    node_t *ptr;
    uint64_t tag;
};

typedef struct node_s {
    int key;
    int value;
    _Atomic(tag_ptr_t) next;
} node_t;

typedef struct {
    _Atomic(tag_ptr_t) head;
} bucket_t;

typedef struct {
    bucket_t *buckets;
    size_t hash_table_size;
} hash_table_t;

/*
extern int hashtable_init(size_t hash_table_size);
extern int hashtable_insert(int key, int value);
extern int hashtable_find(int key, int* value);
extern int hashtable_remove(int key);
*/
#endif
