#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "freelist.h"
#include "hash_table.h"

#include <pthread.h>

#define SET_MARK(p, b) ((node_t *)((uintptr_t)(p) | (b)))
#define GET_PTR(p) ((node_t *)((uintptr_t)(p) & ~1))
#define IS_MARKED_PTR(p) ((uintptr_t)(p) & 1)

#define DEBUG(tag1, tag2) do { \
    pthread_t         self; \
    self = pthread_self(); \
    printf("tag:<%lu, %lu> %lu, %s:%d\n", tag1, tag2, self, __FUNCTION__, __LINE__); \
} while(0)

typedef struct {
    mtag_ptr_t*head;
    size_t table_size;
    size_t size_ocuppied;
} hash_table_t;

hash_table_t g_hash_table;

uint64_t hash_function(const uint8_t *key, size_t len) 
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++) {
        hash ^= key[i];
        hash *= 0x100000001b3;
    }

    return hash;
}

bool hashtable_init(size_t table_size)
{
    g_hash_table.head = aligned_alloc(64, table_size * sizeof(mtag_ptr_t));
    if(g_hash_table.head == NULL) {
        return false;
    }

    for (size_t i = 0; i < table_size; i++) {
        mtag_ptr_t init_head = { .ptr = NULL, .tag = 0 };
        atomic_store(&g_hash_table.head[i], init_head);
    }

    g_hash_table.table_size = table_size;

    return false;
}



static bool find(const uint8_t *key, uint8_t *data,
        mtag_ptr_t *head, mtag_ptr_t **prev,
        mtag_ptr_t *pmark_cur_ptag, mtag_ptr_t *cmark_next_ctag)
{

try_again:
    *prev = head;

    // D1:
    pmark_cur_ptag->ptr = (*prev)->ptr;
    pmark_cur_ptag->tag = (*prev)->tag;

    while (true) { // D2:
        if(GET_PTR(pmark_cur_ptag->ptr) == NULL) {
            return false;
        }

        cmark_next_ctag->ptr = GET_PTR(pmark_cur_ptag->ptr)->next.ptr; // D3:
        cmark_next_ctag->tag = GET_PTR(pmark_cur_ptag->ptr)->next.tag;

        if ((*prev)->tag != pmark_cur_ptag->tag || // D5:
            (*prev)->ptr != GET_PTR(pmark_cur_ptag->ptr) ||
            IS_MARKED_PTR((*prev)->ptr)) {

            DEBUG(atomic_load(prev)->tag, pmark_cur_ptag->tag);

            printf("%p %p %d\n", atomic_load(prev)->ptr, GET_PTR(pmark_cur_ptag->ptr), IS_MARKED_PTR((*prev)->ptr));

            goto try_again;
        }

        if(!IS_MARKED_PTR(cmark_next_ctag->ptr)) {
            int ckey;

            ckey = memcmp(GET_PTR(pmark_cur_ptag->ptr)->key, key, KEY_SIZE);
            if(ckey >= 0) { // D6:
                if(ckey == 0) {
                    if(data) {
                        memcpy(data, GET_PTR(pmark_cur_ptag->ptr)->data, DATA_SIZE);
                    }

                    return true;
                } else {
                    return false;
                };
            }

            *prev = &(GET_PTR(pmark_cur_ptag->ptr))->next; // D7:
        } else {
            // D8:

            mtag_ptr_t expected_cur = {
                    .ptr = SET_MARK(pmark_cur_ptag->ptr, 0),
                    .tag = pmark_cur_ptag->tag
            };

            mtag_ptr_t new_next = {
                    .ptr = SET_MARK(cmark_next_ctag->ptr, 0),
                    .tag = pmark_cur_ptag->tag + 1
            };

            if (atomic_compare_exchange_weak(*prev, &expected_cur, new_next)) {
                // printf("free: %p %lu\n", pmark_cur_ptag->ptr, pmark_cur_ptag->tag);
                freelist_free(GET_PTR(pmark_cur_ptag->ptr));
                cmark_next_ctag->tag = pmark_cur_ptag->tag + 1;

                return true;
            } else {
                DEBUG((*prev)->tag, pmark_cur_ptag->tag);
                goto try_again;
            }
        }

        pmark_cur_ptag->ptr = cmark_next_ctag->ptr; // D9:
        pmark_cur_ptag->tag = cmark_next_ctag->tag;
    }
}

// Find a key
bool hashtable_find(const uint8_t *key, uint8_t *data)
{
    mtag_ptr_t *head, *prev, pmark_cur_ptag, cmark_next_ctag;

    uint64_t index = hash_function(key, KEY_SIZE) % g_hash_table.table_size;

    head = &g_hash_table.head[index];

    return find(key, data, head, &prev, &pmark_cur_ptag, &cmark_next_ctag);
}

int hashtable_insert(const uint8_t *key, uint8_t *data)
{
    node_t *node;
    mtag_ptr_t *prev, *head, pmark_cur_ptag, cmark_next_ctag;

    uint64_t index = hash_function(key, KEY_SIZE) % g_hash_table.table_size;

    head = &g_hash_table.head[index];

    node = freelist_allocate();
    if(node == NULL) {
        return -1;
    }

    // DEBUG("node: %p\n", node);

    memcpy(node->key, key, KEY_SIZE);
    memcpy(node->data, data, DATA_SIZE);

    while (true) {
        // Find the appropriate position to insert
        if (find(key, data, head, &prev, &pmark_cur_ptag, &cmark_next_ctag)) { // A1:
            return false;
        }
        // Prepare the node to be inserted
        node->next.ptr = GET_PTR(pmark_cur_ptag.ptr); // A2:
        node->next.tag = 0;

        // Prepare the expected and new values for the compare-and-swap operation
        mtag_ptr_t expected_cur = {
            .ptr = SET_MARK(pmark_cur_ptag.ptr, 0),
            .tag = pmark_cur_ptag.tag
        };

        mtag_ptr_t new_next = {
            .ptr = SET_MARK(node, 0),
            .tag = pmark_cur_ptag.tag + 1
        };

        // Attempt to insert the new node into the linked list
        if (atomic_compare_exchange_weak(prev, &expected_cur, new_next)) {
            return 0;
        }
        DEBUG(prev->tag, pmark_cur_ptag.tag);
    }


    return 1;
}

bool hashtable_delete(const uint8_t *key)
{
    mtag_ptr_t *prev, *head, pmark_cur_ptag, cmark_next_ctag, expected_cur, new_next;
    uint64_t index = hash_function(key, KEY_SIZE) % g_hash_table.table_size;

    head = &g_hash_table.head[index];

    while(true) { // B1:
        if(!find(key, NULL, head, &prev, &pmark_cur_ptag, &cmark_next_ctag)) {
            return false;
        }

        {
            // B2
            mtag_ptr_t expected_cur = {
                    .ptr = SET_MARK(cmark_next_ctag.ptr, 0),
                    .tag = cmark_next_ctag.tag
            };

            mtag_ptr_t new_next = {
                    .ptr = SET_MARK(cmark_next_ctag.ptr, 1),
                    .tag = cmark_next_ctag.tag + 1
            };


            if (!atomic_compare_exchange_weak(&GET_PTR(pmark_cur_ptag.ptr)->next,
                                              &expected_cur, new_next)) {
                DEBUG(GET_PTR(pmark_cur_ptag.ptr)->next.tag, cmark_next_ctag.tag);
                //printf("!!!%p %p\n", &GET_PTR(pmark_cur_ptag.ptr)->next, cmark_next_ctag.ptr);
                continue;
            }
        }

        {
            // B3
            mtag_ptr_t expected_cur = {
                    .ptr = SET_MARK(pmark_cur_ptag.ptr, 0),
                    .tag = pmark_cur_ptag.tag
            };

            mtag_ptr_t new_next = {
                    .ptr = SET_MARK(cmark_next_ctag.ptr, 0),
                    .tag = pmark_cur_ptag.tag + 1
            };


            if (atomic_compare_exchange_weak(prev, &expected_cur, new_next)) {
                // printf("free: %p %lu\n", pmark_cur_ptag.ptr, pmark_cur_ptag.tag);
                freelist_free(pmark_cur_ptag.ptr);
                return true;
            } else {
                DEBUG(prev->tag, pmark_cur_ptag.tag);

                find(key, NULL, head, &prev, &pmark_cur_ptag, &cmark_next_ctag);
            }
        }
    }

    return false;
}

void hash_table_destroy(void)
{
    free(g_hash_table.head);
}

