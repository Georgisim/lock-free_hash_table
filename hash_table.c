#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freelist.h"

#include "hash_table.h"

#define MARKED_PTR(p) ((node_t *)((uintptr_t)(p) | 1))
#define UNMARKED_PTR(p) ((uintptr_t)(p) & ~1)
#define IS_MARKED_PTR(p) ((uintptr_t)(p) & 1)

typedef struct mtag_ptr_s mtag_ptr_t;
typedef struct node_s node_t;

struct mtag_ptr_s {
    node_t *ptr;
    uint64_t tag;
};

typedef struct node_s {
    _Atomic(mtag_ptr_t) next;

    int key;
} node_t;


typedef struct {
    _Atomic(mtag_ptr_t) *head;
    size_t hash_table_size;
} hash_table_t;

static hash_table_t g_hash_table;

// Initialize the hash table
int hashtable_init(size_t hash_table_size)
{
    freelist_init(sizeof(node_t), hash_table_size * 2);

    g_hash_table.head = calloc(sizeof(mtag_ptr_t), hash_table_size);
    if (g_hash_table.head == NULL) {
        return -1; // Memory allocation failed
    }

    for (size_t i = 0; i < hash_table_size; i++) {
        mtag_ptr_t init_head = { .ptr = 0, .tag = i };
        atomic_store(&g_hash_table.head[i], init_head);
    }

    g_hash_table.hash_table_size = hash_table_size;
}


// A simple hash function
size_t hash(int key)
{
    return key % g_hash_table.hash_table_size;
}

bool find(int key, mtag_ptr_t *head, mtag_ptr_t **prev,
        mtag_ptr_t *pmark_cur_ptag, mtag_ptr_t *cmark_next_ctag)
{
    int ckey;

try_again:
    *prev = head;

    // *pmark_cur_ptag = *prev; // D1:
    pmark_cur_ptag->ptr = (*prev)->ptr;
    pmark_cur_ptag->tag = (*prev)->tag;

    while (UNMARKED_PTR(pmark_cur_ptag->ptr)) {
        cmark_next_ctag->ptr = pmark_cur_ptag->ptr->next.ptr;
        cmark_next_ctag->tag = pmark_cur_ptag->ptr->next.tag;

        ckey = pmark_cur_ptag->ptr->key;

        if((*prev)->tag != pmark_cur_ptag->tag || // D5:
                (*prev)->ptr != UNMARKED_PTR(pmark_cur_ptag->ptr) ||
                IS_MARKED_PTR((*prev)->ptr)) {
            goto try_again;
        }

        if(!IS_MARKED_PTR(cmark_next_ctag->ptr)) { // D5:
            if(ckey >= key) { // D6:
                return ckey == key;
            }

            (*prev)->ptr = pmark_cur_ptag->ptr->next.ptr; // D7:
            (*prev)->tag = pmark_cur_ptag->ptr->next.tag;
        } else {
            // D3:

            mtag_ptr_t expected_cur = { .ptr = UNMARKED_PTR(pmark_cur_ptag->ptr), .tag = pmark_cur_ptag->tag};
            mtag_ptr_t new_next = { .ptr = UNMARKED_PTR(cmark_next_ctag->ptr), .tag = pmark_cur_ptag->tag + 1};

            if (atomic_compare_exchange_weak(*prev, &expected_cur, new_next)) {
                freelist_free(pmark_cur_ptag->ptr);
                cmark_next_ctag->tag = pmark_cur_ptag->tag;

                pmark_cur_ptag->ptr = cmark_next_ctag->ptr;
                pmark_cur_ptag->tag = cmark_next_ctag->ptr;

                return true;
            } else {
                goto try_again;
            }
        }

        pmark_cur_ptag->ptr = cmark_next_ctag->ptr;
        pmark_cur_ptag->tag = cmark_next_ctag->ptr;
    }

    return false; // Key not found D2:
}

// Find a key
bool hashtable_find(int key)
{
    _Atomic(mtag_ptr_t) *head;
    mtag_ptr_t pmark_cur_ptag, cmark_next_ctag;
    mtag_ptr_t *prev;

    size_t index = hash(key);

    head = &g_hash_table.head[index];

    return find(key, head, &prev, &pmark_cur_ptag, &cmark_next_ctag);

}

bool hashtable_insert(int key)
{
    node_t *node;
    mtag_ptr_t pmark_cur_ptag, cmark_next_ctag;
    mtag_ptr_t *prev;

    _Atomic(mtag_ptr_t) *head;

    size_t index = hash(key);

    head = &g_hash_table.head[index];

    node = freelist_allocate();
    node->key = key;

    if(find(key, head, &prev, &pmark_cur_ptag, &cmark_next_ctag)) { // A1:
        return false;
    }

    node->next.ptr = UNMARKED_PTR(pmark_cur_ptag.ptr); // A2:
    node->next.tag = 0;

    prev->ptr = node;
    prev->tag++;

    // A3:
    mtag_ptr_t expected_cur = { .ptr = UNMARKED_PTR(pmark_cur_ptag.ptr), .tag = pmark_cur_ptag.tag};
    mtag_ptr_t new_next = { .ptr = UNMARKED_PTR(node), .tag = pmark_cur_ptag.tag + 1};

    if (atomic_compare_exchange_weak(prev, &expected_cur, new_next)) {
        return true;
    }

    return false;
}

bool hashtable_delete(mtag_ptr_t *head, int key)
{

}

void hash_table_destroy(void)
{
    free(g_hash_table.head);
}




