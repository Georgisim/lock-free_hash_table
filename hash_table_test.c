#include "hash_table.h"
#include "freelist.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#define TEST_ITERATIONS 100000000
#define HASH_TABLE_SIZE 2000000
#define NUM_THREADS 32


void *thread_function(void *arg) {

    uint8_t key[KEY_SIZE];
    uint8_t data[DATA_SIZE + 1], data_read[DATA_SIZE + 1];

    for(int i; i < TEST_ITERATIONS; i++) {
        for (size_t i = 0; i < KEY_SIZE; i++) {
            key[i] =  rand() % 256;
        }

        key[DATA_SIZE] = 0;

        for (size_t i = 0; i < DATA_SIZE; i++) {
            data[i] =  rand() % 256;

        }

        data[DATA_SIZE] = 0;
        data_read[DATA_SIZE] = 0;

        if(hashtable_insert(key, data) == -1) {
            printf("failed to insert %lu\n!", freelist_get_nuber_elements());
            continue;
        }

        hashtable_find(key, data_read);

        if(memcmp(data, data_read, DATA_SIZE)) {
            printf("key: %s added: [%s], found: [%s]\n", key, data, data_read);
        }

        hashtable_delete(key);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    uint8_t key[KEY_SIZE];
    uint8_t data[DATA_SIZE], data_read[DATA_SIZE];

     srand(time(NULL));

    freelist_init(sizeof(node_t), HASH_TABLE_SIZE * 2);
    hashtable_init(HASH_TABLE_SIZE);

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

    }

    printf("prefill done!\n");


    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, thread_function, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
