#include "hash_table.h"
#include "freelist.h"

#include <string.h>
#include <stdio.h>

// Example usage
int main()
{
    hashtable_init(100);

    // Insert key-value pairs
    hashtable_insert(1, 100);
    hashtable_insert(2, 200);
    hashtable_insert(102, 201);
    hashtable_insert(202, 202);
    // Find values
    int value;
    if (hashtable_find(1, &value)) {
        printf("Key 1 has value %d\n", value);
    } else {

        printf("Key 1 not found\n");
    }

    // Remove a key-value pair
     hashtable_delete(1);
    if (hashtable_find(1, &value)) {
        printf("Key 1 has value %d\n", value);
    } else {
        printf("Key 1 not found\n");
    }


    if (hashtable_find(2, &value)) {
        printf("Key 2 has value %d\n", value);
    } else {
        printf("Key 2 not found\n");
    }


    if (hashtable_find1(202, &value)) {
        printf("!Key 2 has value %d\n", value);
    } else {
        printf("!Key 2 not found\n");
    }

    if (hashtable_find(2342342, &value)) {
        printf("Key 2342342 has value %d\n", value);
    } else {
        printf("Key 2342342 not found\n");
    }

    hashtable_delete(2342342);

    if (hashtable_find(2342342, &value)) {
        printf("Key 2342342 has value %d\n", value);
    } else {
        printf("Key 2342342 not found\n");
    }

    // Clean up freelist
    hash_table_destroy();

    return 0;
}


