#include "hash_table.h"
#include "freelist.h"

#include <string.h>
#include <stdio.h>
<<<<<<< Updated upstream

// Example usage
int main()
{
    int value;

    hashtable_init(100);
=======
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#define TEST_ITERATIONS 100000000
#define HASH_TABLE_SIZE 2000000
#define NUM_THREADS 32


void *thread_function(void *arg) {
>>>>>>> Stashed changes

    // Insert key-value pairs
    hashtable_insert(1, 100);

<<<<<<< Updated upstream
=======
    for(int i; i < TEST_ITERATIONS; i++) {
        for (size_t i = 0; i < KEY_SIZE; i++) {
            key[i] =  rand() % 256;
        }
>>>>>>> Stashed changes

    hashtable_insert(2, 200);
    if (hashtable_find(2, &value)) {
        printf("Key 2 has value %d\n", value);
    } else {
        printf("Key 2 not found\n");
    }

    hashtable_insert(102, 201);

    if (hashtable_find(2, &value)) {
        printf("Key 2 has value %d\n", value);
    } else {
        printf("Key 2 not found\n");
    }


    if (hashtable_find(102, &value)) {
        printf("Key 102 has value %d\n", value);
    } else {
        printf("Key 102 not found\n");
    }

<<<<<<< Updated upstream
=======
        for (size_t i = 0; i < DATA_SIZE; i++) {
            data[i] =  rand() % 256;
>>>>>>> Stashed changes



<<<<<<< Updated upstream
=======
        if(hashtable_insert(key, data) == -1) {
            printf("failed to insert %lu\n!", freelist_get_nuber_elements());
            continue;
        }
>>>>>>> Stashed changes







    hashtable_insert(2342342, 2342342);

<<<<<<< Updated upstream

    // Find values
=======
    for(size_t j = 0; j < (HASH_TABLE_SIZE * 3) / 4; j++) {
        for (size_t i = 0; i < KEY_SIZE; i++) {
            key[i] = rand() % 256;
        }
        for (size_t i = 0; i < DATA_SIZE; i++) {
            data[i] = rand() % 256;
        }

        if(hashtable_insert(key, data) == -1) {
            printf("failed to insert %lu\n!", freelist_get_nuber_elements());
            return -1;
        }
>>>>>>> Stashed changes

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


    if (hashtable_find(202, &value)) {
        printf("Key 202 has value %d\n", value);
    } else {
        printf("Key 202 not found\n");
    }

    if (hashtable_find(2342342, &value)) {
        printf("Key 2342342 has value %d\n", value);
    } else {
        printf("Key 2342342 not found\n");
    }

<<<<<<< Updated upstream
    hashtable_delete(2342342);

    if (hashtable_find(2342342, &value)) {
        printf("Key 2342342 has value %d\n", value);
    } else {
        printf("Key 2342342 not found\n");
    }

    // Clean up freelist
    hash_table_destroy();

=======
>>>>>>> Stashed changes
    return 0;
}


