#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    pthread_mutex_t mutex;

    pthread_cond_t readers_cond;
    pthread_cond_t writers_cond;

    int readers_count;

    int waiting_writers_count;
    int waiting_readers_count;

    int writer_active;
} custom_rwlock_t;

void custom_rwlock_init(custom_rwlock_t* lock);

void custom_rwlock_destroy(custom_rwlock_t* lock);

void custom_rdlock(custom_rwlock_t* lock);

void custom_wrlock(custom_rwlock_t* lock);

void custom_unlock(custom_rwlock_t* lock);
