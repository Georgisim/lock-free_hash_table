#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freelist.h"
#include "hash_table.h"

#define SET_MARK(p, b) ((node_t *)((uintptr_t)(p) | (b)))
#define GET_PTR(p) ((node_t *)((uintptr_t)(p) & ~1))
#define IS_MARKED_PTR(p) ((uintptr_t)(p) & 1)

<<<<<<< Updated upstream
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
=======
#ifdef _DEBUG
    #define DEBUG(tag1, tag2) do { \
        pthread_t         self; \
        self = pthread_self(); \
        printf("tag:<%lu, %lu> %lu, %s:%d\n", tag1, tag2, self, __FUNCTION__, __LINE__); \
    } while(0)
#else
    #define DEBUG(tag1, tag2)
#endif
>>>>>>> Stashed changes

typedef struct {
    _Atomic(mtag_ptr_t) *head;
    size_t hash_table_size;
} hash_table_t;

static hash_table_t g_hash_table;

// Initialize the hash table
int hashtable_init(size_t hash_table_size)
{
    freelist_init(sizeof(node_t), hash_table_size * 2);

    g_hash_table.head = calloc(hash_table_size, sizeof(_Atomic(mtag_ptr_t)));
    if (g_hash_table.head == NULL) {
        return -1; // Memory allocation failed
    }

    for (size_t i = 0; i < hash_table_size; i++) {
        mtag_ptr_t init_head = { .ptr = NULL, .tag = i };
        atomic_store(&g_hash_table.head[i], init_head);
    }

    g_hash_table.hash_table_size = hash_table_size;

    return 0;
}

// A simple hash function
size_t hash(int key)
{
    return key % g_hash_table.hash_table_size;
}

static bool find(int key, _Atomic(mtag_ptr_t) *head, _Atomic(mtag_ptr_t) **prev,
                 mtag_ptr_t *pmark_cur_ptag, mtag_ptr_t *cmark_next_ctag)
{
    int ckey;

try_again:
    *prev = head;

    // D1:
    pmark_cur_ptag->ptr = (*prev)->ptr;
    pmark_cur_ptag->tag = (*prev)->tag;

    while (true) { // D2:
        if(GET_PTR(pmark_cur_ptag->ptr) == NULL) {
            return false;
        }

        cmark_next_ctag->ptr = ((node_t *)GET_PTR(pmark_cur_ptag->ptr))->next.ptr; // D3:
        cmark_next_ctag->tag = ((node_t *)GET_PTR(pmark_cur_ptag->ptr))->next.tag;

<<<<<<< Updated upstream
        ckey = ((node_t *)GET_PTR(pmark_cur_ptag->ptr))->key;
=======
        if ((*prev)->tag != pmark_cur_ptag->tag || // D5:
            (*prev)->ptr != GET_PTR(pmark_cur_ptag->ptr) ||
            IS_MARKED_PTR((*prev)->ptr)) {

            DEBUG((*prev)->tag, pmark_cur_ptag->tag);
>>>>>>> Stashed changes

        if (atomic_load(prev)->tag != pmark_cur_ptag->tag || // D5:
            atomic_load(prev)->ptr != GET_PTR(pmark_cur_ptag->ptr) ||
            IS_MARKED_PTR(atomic_load(prev)->ptr)) {
            goto try_again;
        }

        if(!IS_MARKED_PTR(cmark_next_ctag->ptr)) {
            if(ckey >= key) { // D6:
                return ckey == key;
            }

            *prev = &((node_t *)GET_PTR(pmark_cur_ptag->ptr))->next; // D7:
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
                freelist_free(GET_PTR(pmark_cur_ptag->ptr));
                cmark_next_ctag->tag = pmark_cur_ptag->tag + 1;

                return true;
            } else {
                goto try_again;
            }
        }

        pmark_cur_ptag->ptr = cmark_next_ctag->ptr; // D9:
        pmark_cur_ptag->tag = cmark_next_ctag->tag;
    }
}

// Find a key
bool hashtable_find(int key, int value)
{
    _Atomic(mtag_ptr_t) *head;
    mtag_ptr_t pmark_cur_ptag, cmark_next_ctag;
    _Atomic(mtag_ptr_t) *prev;

    size_t index = hash(key);

    head = &g_hash_table.head[index];

    return find(key, head, &prev, &pmark_cur_ptag, &cmark_next_ctag);
}

bool hashtable_insert(int key)
{
    node_t *node;
    mtag_ptr_t pmark_cur_ptag, cmark_next_ctag;
    _Atomic(mtag_ptr_t) *prev;

    _Atomic(mtag_ptr_t) *head;

    size_t index = hash(key);

    head = &g_hash_table.head[index];

    node = freelist_allocate();
<<<<<<< Updated upstream
    node->key = key;
=======
    if(node == NULL) {
        return -1;
    }

    memcpy(node->key, key, KEY_SIZE);
    memcpy(node->data, data, DATA_SIZE);
>>>>>>> Stashed changes

    while (true) {
        // Find the appropriate position to insert
        if (find(key, head, &prev, &pmark_cur_ptag, &cmark_next_ctag)) { // A1:
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
            return true;
        }
    }

    return false;
}

bool hashtable_delete(int key)
{
    _Atomic(mtag_ptr_t) *prev;
    _Atomic(mtag_ptr_t) *head;

    mtag_ptr_t pmark_cur_ptag, cmark_next_ctag;


    size_t index = hash(key);

    head = &g_hash_table.head[index];

    while(find(key, head, &prev, &pmark_cur_ptag, &cmark_next_ctag)) { // B1:
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
                freelist_free(pmark_cur_ptag.ptr);
                return true;
            } // ???
        }
    }

    return false;
}

void hash_table_destroy(void)
{
    free(g_hash_table.head);
}

