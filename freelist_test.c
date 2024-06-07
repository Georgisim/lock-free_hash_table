#include "freelist.h"

#include <string.h>
#include <stdio.h>

// Example usage
typedef struct {
    char data[48];
} MyObject;

int main() {    
    size_t object_size = sizeof(MyObject);  // Size of MyObject is 48 bytes
    size_t capacity = 100;  // Number of objects

    if (freelist_init(object_size, capacity) != 0) {
        fprintf(stderr, "Failed to initialize freelist\n");
        return 1;
    }

    // Allocate objects
    MyObject* objects[capacity];
    for (size_t i = 0; i < capacity; ++i) {
        objects[i] = (MyObject*)freelist_allocate();
        if (objects[i] == NULL) {
            fprintf(stderr, "Failed to allocate object %zu\n", i);
            break;
        }
        
        memset(objects[i], 'a' + i%20  , 47);
        
        objects[i]->data[47] = 0;
        
        printf("Allocated object %zu at %p\n", i, (void*)objects[i]);
    }

    // Free all objects
    for (size_t i = 0; i < capacity; ++i) {
        printf("obj: %s\n", objects[i]->data);
        if (objects[i] != NULL) {
            freelist_free(objects[i]);
            printf("Freed object %zu at %p\n", i, (void*)objects[i]);
        }
    }

    // Clean up the freelist
    freelist_destroy();

    return 0;
}
