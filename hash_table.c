#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "freelist.h"

#include "hash_table.h"

#define MARKED_PTR(p) ((node_t *)((uintptr_t)(p) | 1))
#define UNMARKED_PTR(p) ((uintptr_t)(p) & ~1)
#define IS_MARKED_PTR(p) ((uintptr_t)(p) & 1)

static hash_table_t g_hash_table;

// A simple hash function
size_t hash(int key)
{
    return key % g_hash_table.hash_table_size;
}

// Initialize the hash table
int hashtable_init(size_t hash_table_size)
{
    freelist_init(sizeof(node_t), hash_table_size * 2);

    g_hash_table.buckets = calloc(sizeof(bucket_t), hash_table_size);
    if (g_hash_table.buckets == NULL) {
        return -1; // Memory allocation failed
    }

    for (size_t i = 0; i < hash_table_size; i++) {
        tag_ptr_t init_head = { .ptr = 0, .tag = 0 };
        atomic_store(&g_hash_table.buckets[i].head, init_head);
    }

    g_hash_table.hash_table_size = hash_table_size;
}


void hashtable_store_keyval(node_t *node, int key, int value)
{
    node->key = key;
    node->value = value;
}

int hashtable_cmp_key(node_t *node, int key)
{
    if(node->key < key) {
        return -1;
    } else if(node->key > key) {
        return 0;
    }
}

// Find a key
int hashtable_find(int key, int* value)
{
    size_t index = hash(key);
    bucket_t *bucket = &g_hash_table.buckets[index];

    tag_ptr_t next, head = atomic_load(&bucket->head);
    node_t* current = (node_t*)UNMARKED_PTR(head.ptr);

    while (current) {
        if (current->key == key) {
            *value = current->value;
            return 1; // Key found
        }

        next = atomic_load(&current->next);
        current = (node_t*)UNMARKED_PTR(head.ptr);
    }

    return 0; // Key not found
}

// Find a key
int hashtable_find1(int key, int* value)
{
    size_t index = hash(key);
    bucket_t *bucket = &g_hash_table.buckets[index];

    uint64_t ctag, ptag;
    int cmark;

    tag_ptr_t next, head = atomic_load(&bucket->head), prev;
    node_t* cur = (node_t*)UNMARKED_PTR(head.ptr);

try_again:
    prev = head;
    // <pmark, cur, ptag> = *prev;
    while (cur) {
        //if(*prev != <0,cur,ptag>)
//            goto try_again;

        if(!MARKED_PTR(cur->next.ptr)) {
            if (cur->key >= key) {
                if (cur->key == key) {
                    *value = cur->value;
                    return 1; // Key found
                } else {
                    return 0;
                }
            }
        } else {
            /*
            if(CAS())
                freelist_free(current);
                */

        }

        next = atomic_load(&cur->next);
        cur = (node_t*)UNMARKED_PTR(next.ptr);

        // ptag = next.tag;

    }

    return 0; // Key not found D2
}

// Insert a key-value pair into the hash table
int hashtable_insert(int key, int value)
{
    node_t *new_node, *current, *prev;
    tag_ptr_t old_head, new_head, expected;

    size_t index = hash(key);
    bucket_t *bucket = &g_hash_table.buckets[index];

    new_node = freelist_allocate();

    hashtable_store_keyval(new_node, key, value);

    while(1) {
        old_head = atomic_load(&bucket->head);
        current = (node_t *)UNMARKED_PTR(old_head.ptr);

        prev = NULL;

        while (current && current->key < key) {
            prev = current;
            tag_ptr_t next = atomic_load(&current->next);
            current = (node_t*)UNMARKED_PTR(next.ptr);
        }

        if (prev) {
            new_node->next = (tag_ptr_t){ .ptr = current, .tag = old_head.tag + 1 };
            expected = atomic_load(&prev->next);

            new_head = (tag_ptr_t){ .ptr = new_node, .tag = expected.tag + 1 };
            if (atomic_compare_exchange_weak(&prev->next, &expected, new_head)) {
                break;
            }
        } else {
            new_node->next = old_head;
            new_head = (tag_ptr_t){ .ptr = new_node, .tag = old_head.tag + 1 };
            if (atomic_compare_exchange_weak(&bucket->head, &old_head, new_head)) {
                break;
            }
        }
    };

    return 0;
}



// Delete a key-value pair
int hashtable_delete(int key)
{
    size_t index = hash(key);
    bucket_t *bucket = &g_hash_table.buckets[index];
    node_t *current, *prev;

    tag_ptr_t old_head, new_head, next, expected;

    do {
        old_head = atomic_load(&bucket->head);
        current = (node_t*)UNMARKED_PTR(old_head.ptr);
        prev = NULL;

        while (current && current->key != key) {
            prev = current;
            next = atomic_load(&current->next);
            current = (node_t*)UNMARKED_PTR(next.ptr);
        }

        if (current == NULL) {
            return 0; // Key not found
        }

        next = atomic_load(&current->next);

        if (prev) {
            expected = atomic_load(&prev->next);
            new_head = (tag_ptr_t) { .ptr = MARKED_PTR(next.ptr), .tag = expected.tag + 1 };
            if (atomic_compare_exchange_weak(&prev->next, &expected, new_head)) {
                break;
            }
        } else {
            new_head = (tag_ptr_t) { .ptr = MARKED_PTR(next.ptr), .tag = old_head.tag + 1 };
            if (atomic_compare_exchange_weak(&bucket->head, &old_head, new_head)) {
                break;
            }
        }
    } while (1);

    return 1; // Key deleted
}

void hash_table_destroy(void)
{
    freelist_destroy();
}
