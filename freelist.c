#include <stdatomic.h>
#include <stdlib.h>

#include "freelist.h"

typedef struct tag_ptr_s tag_ptr_t;
typedef struct node_s node_t;

struct tag_ptr_s {
    node_t *ptr;
    uint64_t tag;
};

typedef struct node_s {
    tag_ptr_t next;
} node_t;

typedef struct {
    tag_ptr_t head;
    size_t object_size;
    size_t capacity;
    void* memory_pool;
    _Atomic(size_t) occupied;
} lockfree_list_t;


static lockfree_list_t g_freelist;

// Initialize the freelist
bool freelist_init(size_t object_size, size_t capacity)
{
    node_t* node;

    g_freelist.object_size = object_size;
    g_freelist.capacity = capacity;
    g_freelist.memory_pool = aligned_alloc(64, capacity * (object_size + sizeof(node_t)));

    if (g_freelist.memory_pool == 0) {
        return false;
    }

    g_freelist.head.ptr = g_freelist.memory_pool;
    // Push all nodes onto the freelist

    for (size_t i = 0; i < capacity - 1; i++) {

        node = (void *)g_freelist.head.ptr + i * (object_size + sizeof(node_t));
        node->next.ptr  =  (void *)g_freelist.head.ptr + (i + 1) * (object_size + sizeof(node_t));
        node->next.tag = 0;
    }

    node = (void *)g_freelist.head.ptr + (capacity - 1) * (object_size + sizeof(node_t));

    node->next.ptr = 0;
    node->next.tag = 0;

        
    return true;
}

// Allocate an object
void* freelist_allocate(void)
{
    tag_ptr_t old_head, new_head;

    do {
        old_head = atomic_load(&g_freelist.head);

        if (old_head.ptr == 0) {
            return NULL;  // The freelist is empty
        }

        node_t* node = (old_head.ptr);
        new_head.ptr = node->next.ptr;

        new_head.tag = old_head.tag + 1;
    } while (!atomic_compare_exchange_weak(&g_freelist.head, &old_head, new_head));

    g_freelist.occupied++;

    return (void*)old_head.ptr + sizeof(node_t);
}

size_t freelist_get_nuber_elements(void)
{
    return g_freelist.occupied;
}

// Free an object
void freelist_free(void* ptr)
{
    node_t* node = ptr - sizeof(node_t);
    tag_ptr_t old_head, new_head;

    do {
        old_head = atomic_load(&g_freelist.head);
        node->next.ptr = old_head.ptr;
        new_head.ptr = node;
        new_head.tag = old_head.tag + 1;
    } while (!atomic_compare_exchange_weak(&g_freelist.head, &old_head, new_head));

    g_freelist.occupied--;
}

// Clean up the freelist
void freelist_destroy()
{
    free((void *)g_freelist.memory_pool);
}



