#include <stdatomic.h>
#include <stdlib.h>

#include "freelist.h"

typedef struct {
    uintptr_t ptr;
    uint64_t tag;
} tag_ptr_t;

typedef struct {
    _Atomic(tag_ptr_t) head;
    void* memory_pool;  // Contiguous block of memory for objects
    size_t object_size;
    size_t capacity;
} lockfree_list_t;


typedef struct node_t {
    struct node_t* next;
} node_t;



static lockfree_list_t g_freelist;

// Initialize the freelist
int freelist_init(size_t object_size, size_t capacity)
{
    tag_ptr_t old_head, new_head;
    node_t* node;

    g_freelist.object_size = object_size;
    g_freelist.capacity = capacity;
    g_freelist.memory_pool = calloc(object_size, capacity);
    if (g_freelist.memory_pool == NULL) {
        return -1;
    }

    // Initialize the freelist
    tag_ptr_t init_head = { .ptr = 0, .tag = 0 };
    atomic_store(&g_freelist.head, init_head);

    // Push all nodes onto the freelist
    for (size_t i = 0; i < capacity; ++i) {
        node_t* node = (node_t*)((uintptr_t)g_freelist.memory_pool + i * object_size);

        node->next = (i < capacity - 1) ?
                (node_t*)((uintptr_t)g_freelist.memory_pool + (i + 1) * object_size) :
                NULL;

        do {
            old_head = atomic_load(&g_freelist.head);
            node->next = (node_t*)(old_head.ptr);
            new_head.ptr = (uintptr_t)node;
            new_head.tag = old_head.tag + 1;
        } while (!atomic_compare_exchange_weak(&g_freelist.head, &old_head, new_head));
    }
        
    return 0;
}

// Allocate an object
void* freelist_allocate()
{
    tag_ptr_t old_head, new_head;

    do {
        old_head = atomic_load(&g_freelist.head);

        if (old_head.ptr == 0) {
            return NULL;  // The freelist is empty
        }

        node_t* node = (node_t*)(old_head.ptr);
        new_head.ptr = (uintptr_t)node->next;
        new_head.tag = old_head.tag + 1;
    } while (!atomic_compare_exchange_weak(&g_freelist.head, &old_head, new_head));

    return (void*)old_head.ptr;
}

// Free an object
void freelist_free(void* ptr)
{
    node_t* node = (node_t*)ptr;
    tag_ptr_t old_head, new_head;

    do {
        old_head = atomic_load(&g_freelist.head);
        node->next = (node_t*)(old_head.ptr);
        new_head.ptr = (uintptr_t)node;
        new_head.tag = old_head.tag + 1;
    } while (!atomic_compare_exchange_weak(&g_freelist.head, &old_head, new_head));
}

// Clean up the freelist
void freelist_destroy()
{
    free(g_freelist.memory_pool);
}


