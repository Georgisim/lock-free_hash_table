#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

typedef enum {
    E_FOUND,
    E_NOTFOUND,
    E_MEMFULL,
    E_RETRY
} found_t;

bool hashtable_init(size_t table_size);
void hash_table_destroy(void);
extern found_t hashtable_find(const uint8_t *key, uint8_t *data);
extern found_t hashtable_insert(const uint8_t *key, uint8_t *data);
extern bool hashtable_delete(const uint8_t *key);

#endif
