#include "hash_table.h"
#include "freelist.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#define TEST_ITERATIONS 100000000UL
#define HASH_TABLE_SIZE 2000000UL
#define NUM_THREADS 32

#ifdef _DEBUG
    #define DEBUG(...) fprintf (stderr, __VA_ARGS__)
#else
    #define DEBUG(...)
#endif



typedef struct {
    size_t num_keys;
    size_t num_insertions;
    uint8_t **keys;
    uint8_t **data;
} thread_args_t;

static long unsigned int _duplicate_insertions = 0;
static long unsigned int *duplicate_insertions = &_duplicate_insertions;

static long unsigned int _total_insertions = 0;
static long unsigned int *total_insertions = &_total_insertions;

static long unsigned int _mem_full = 0;
static long unsigned int *mem_full = &_mem_full;

static long unsigned int _retry_insert= 0;
static long unsigned int *retry_insert = &_retry_insert;

static long unsigned int _changed_insert= 0;
static long unsigned int *changed_insert = &_changed_insert;

static long unsigned int _notfound_insert= 0;
static long unsigned int *notfound_insert = &_notfound_insert;


void *thread_function(void *arg)
{

    thread_args_t *targs = (thread_args_t *)arg;

    uint8_t data_read[DATA_SIZE];
    for (size_t i = 0; i < targs->num_insertions; i++) {
        size_t key_index = rand() % targs->num_keys;
        size_t data_index = rand() % targs->num_keys;

        __sync_fetch_and_add(total_insertions, 1);

#if 1
        if(hashtable_find(targs->keys[key_index], data_read) == E_FOUND) {
            //printf("found key:[%s] data:[%s], deleting\n", targs->keys[key_index], data_read);

            hashtable_delete(targs->keys[key_index]);
        }
#endif
        // printf("insert: key:[%s], data[%s]\n", targs->keys[key_index], targs->data[data_index]);

        int res = hashtable_insert(targs->keys[key_index], targs->data[data_index]);

        switch(res) {
        case E_MEMFULL:
           DEBUG("failed to insert (%lu elements occupied), memory full!\n", freelist_get_nuber_elements());
            __sync_fetch_and_add(mem_full, 1);
            continue;

        case E_FOUND:
            DEBUG("failed to insert %lu, already there!\n", freelist_get_nuber_elements());
            __sync_fetch_and_add(duplicate_insertions, 1);
            continue;

        case E_RETRY:
            DEBUG("failed to insert %lu, retry!\n", freelist_get_nuber_elements());
            __sync_fetch_and_add(retry_insert, 1);
            continue;
        }

        if(hashtable_find(targs->keys[key_index], data_read) == E_NOTFOUND) {
            DEBUG("key not found!\n");
            __sync_fetch_and_add(notfound_insert, 1);
            continue;
        }

        if(memcmp(targs->data[data_index], data_read, DATA_SIZE)) {
            DEBUG("key: %s added: [%s], found: [%s]\n", targs->keys[key_index], targs->data[data_index], data_read);
            __sync_fetch_and_add(changed_insert, 1);
            continue;
        }

        hashtable_delete(targs->keys[key_index]);
    }

    pthread_exit(NULL);
}

void print_statistics(struct timeval start, struct timeval end)
{
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    double throughput = *total_insertions / elapsed;

    printf("Total_insertions: %lu\n", *total_insertions);
    printf("Mem_full: %lu\n", *mem_full);
    printf("Duplicated insertions: %lu\n", *duplicate_insertions);
    printf("Not found insertions: %lu\n", *notfound_insert);
    printf("Changed insertions: %lu\n", *changed_insert);
    printf("Retry insertions: %lu\n", *retry_insert);


    printf("Total time taken: %.2f seconds\n", elapsed);
    printf("Throughput: %.2f operations/second\n", throughput);


}


int main(int argc, char **argv)
{
    pthread_t *threads;
    int        num_threads = NUM_THREADS;
    struct     timeval start, end;
    size_t     num_keys = HASH_TABLE_SIZE;
    size_t     table_size = HASH_TABLE_SIZE;
    size_t     num_insertions = TEST_ITERATIONS;
    double     prefill_ratio = 0.75;

    int opt;
    while ((opt = getopt(argc, argv, "t:k:s:i:p:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                if(num_threads == 0) {
                    fprintf(stderr, "num_threads can't be zero\n");
                    exit(EXIT_FAILURE);
                }

                break;

            case 'k':
                num_keys = atoi(optarg);
                break;

            case 's':
                table_size = atoi(optarg);
                if(table_size == 0) {
                    fprintf(stderr, "table_size can't be zero\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'i':
                num_insertions = atoi(optarg);
                break;

            case 'p':
                prefill_ratio = atof(optarg);
                if(prefill_ratio > 1) {
                    fprintf(stderr, "prefill_ratio can't be greater than one\n");
                    exit(EXIT_FAILURE);
                }
                break;

            default:
                fprintf(stderr, "Usage: %s [-t num_threads] [-k num_keys] [-s table_size] [-i num_insertions] [-p prefill_ratio]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    srand(time(NULL));

    printf("hash table size: %lu elements, key size: %d, data size: %d\n",
            table_size, KEY_SIZE, DATA_SIZE);

    if(!freelist_init(sizeof(node_t), table_size)) {
        printf("failed to initialize freelist\n");
        exit(EXIT_FAILURE);
    }

    if(!hashtable_init(table_size)) {
        printf("failed to initialize hashtable\n");
        exit(EXIT_FAILURE);
    }

    // Generate random keys
    uint8_t **keys = (uint8_t **)malloc(num_keys * sizeof(uint8_t *));
    if(keys == NULL) {
        printf("malloc failed!\n");
        exit(EXIT_FAILURE);
    }

    uint8_t **data = (uint8_t **)malloc(num_keys * sizeof(uint8_t *));
    if(data == NULL) {
        printf("malloc failed!\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < num_keys; i++) {
        keys[i] = (uint8_t *)malloc(KEY_SIZE);
        if(keys[i] == NULL) {
            printf("malloc failed!\n");
            exit(EXIT_FAILURE);
        }

        data[i] = (uint8_t *)malloc(DATA_SIZE);
        if(data[i] == NULL) {
            printf("malloc failed!\n");
            exit(EXIT_FAILURE);
        }

        for (size_t j = 0; j < KEY_SIZE - 1; j++) {
            keys[i][j] = 'a' + rand() % 26;
        }

        keys[i][KEY_SIZE - 1] = '\0';

        for (size_t j = 0; j < DATA_SIZE; j++) {
            data[i][j] = 'A' + rand() % 26;
        }

        data[i][DATA_SIZE - 1] = '\0';
    }

    printf("start prefill %lu elements...\n", (size_t)(prefill_ratio * table_size));

    for(size_t j = 0; j < (size_t)(table_size * prefill_ratio); j++) {
        size_t key_index = rand() % num_keys;
        size_t data_index = rand() % num_keys;

        if(hashtable_insert(keys[key_index], data[data_index]) == -1) {
            printf("failed to insert %lu\n!", freelist_get_nuber_elements());
            return -1;
        }
    }

    printf("prefill done!\n");

    threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t thread_args;

    if(!threads) {
        printf("failed to create treads\n");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start, NULL);
    for (int i = 0; i < num_threads; ++i) {
        thread_args.num_keys = num_keys;
        thread_args.num_insertions = num_insertions;
        thread_args.keys = keys;
        thread_args.data = data;

        if(pthread_create(&threads[i], NULL, thread_function, &thread_args) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);

    print_statistics(start, end);

    printf("test done!\n\n");

    hash_table_destroy();
    freelist_destroy();

    exit(EXIT_SUCCESS);
}
