#include "hash_table.h"
#include "freelist.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#define TEST_ITERATIONS 100000000UL
#define HASH_TABLE_SIZE 2000000UL
#define NUM_THREADS 32

static long unsigned int _failed_insertions = 0;
static long unsigned int *failed_insertions = &_failed_insertions;

static long unsigned int _total_insertions = 0;
static long unsigned int *total_insertions = &_total_insertions;

static long unsigned int _mem_full = 0;
static long unsigned int *mem_full = &_mem_full;

void *thread_function(void *arg)
{
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

        __sync_fetch_and_add(total_insertions, 1);

        data[DATA_SIZE] = 0;
        data_read[DATA_SIZE] = 0;

        int res = hashtable_insert(key, data);
        if(res == E_MEMFULL) {
            __sync_fetch_and_add(mem_full, 1);
            printf("failed to insert %lu, memory full\n!", freelist_get_nuber_elements());
            continue;
        } else if(res == E_NOTFOUND){
            // printf("failed to insert %lu, already there\n!", freelist_get_nuber_elements());
            __sync_fetch_and_add(failed_insertions, 1);
            hashtable_delete(key);
            continue;
        }

        if(!hashtable_find(key, data_read)) {
            printf("key not found\n!");
            continue;
        }

        if(memcmp(data, data_read, DATA_SIZE)) {
            printf("key: %s added: [%s], found: [%s]\n", key, data, data_read);
        }

        hashtable_delete(key);

        // printf("succes!!!\n");
    }

    return NULL;
}

void print_statistics(struct timeval start, struct timeval end,
        size_t total_insertions)
{
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    double throughput = total_insertions / elapsed;

    printf("Total time taken: %.2f seconds\n", elapsed);
    printf("Throughput: %.2f operations/second\n", throughput);
    printf("Failed insertions: %lu\n", *failed_insertions);
}


int main(int argc, char **argv)
{
    uint8_t    key[KEY_SIZE];
    uint8_t    data[DATA_SIZE];
    pthread_t *threads;
    int        num_threads = NUM_THREADS;
    struct     timeval start, end;

    srand(time(NULL));

    printf("hash table size: %lu elements, key size: %d, data size: %d\n",
            HASH_TABLE_SIZE, KEY_SIZE, DATA_SIZE);

    if(!freelist_init(sizeof(node_t), HASH_TABLE_SIZE)) {
        printf("failed to initialize freelist\n");
        exit(EXIT_FAILURE);
    }

    if(!hashtable_init(HASH_TABLE_SIZE)) {
        printf("failed to initialize hashtable\n");
        exit(EXIT_FAILURE);
    }

    printf("start prefill ...\n");
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

    threads = malloc(num_threads * sizeof(pthread_t));
    if(!threads) {
        printf("failed to create treads\n");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start, NULL);

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, thread_function, NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);

    print_statistics(start, end, TEST_ITERATIONS);

    printf("test done!\n");

    hash_table_destroy();
    freelist_destroy();

    exit(EXIT_SUCCESS);
}
