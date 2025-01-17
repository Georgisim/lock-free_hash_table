#if !defined(FREELIST_H)
#define FREELIST_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern bool freelist_init(size_t object_size, size_t capacity);
extern void* freelist_allocate();
extern void freelist_free(void* ptr);
extern void freelist_destroy();
extern size_t freelist_get_nuber_elements(void);
#endif
