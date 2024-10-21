#include <pthread.h>

#include "../headers/custom_rwlock.h"

void custom_rwlock_init(custom_rwlock_t* lock) {
    pthread_mutex_init(&lock->mutex, NULL);
    pthread_cond_init(&lock->readers_cond, NULL);
    pthread_cond_init(&lock->writers_cond, NULL);
    lock->readers_count = 0;
    lock->waiting_writers_count = 0;
    lock->waiting_readers_count = 0;
    lock->writer_active = 0;
}

void custom_rwlock_destroy(custom_rwlock_t* lock) {
    pthread_mutex_destroy(&lock->mutex);
    pthread_cond_destroy(&lock->readers_cond);
    pthread_cond_destroy(&lock->writers_cond);
}

// Нельзя получить блокировку на чтение, пока есть блокировка на запись
// ждем освобождения записи
void custom_rdlock(custom_rwlock_t* lock) {
    pthread_mutex_lock(&lock->mutex);

    while (lock->writer_active) {
        lock->waiting_readers_count++;
        pthread_cond_wait(&lock->writers_cond, &lock->mutex);
        lock->waiting_readers_count--;
    }

    lock->readers_count++;
    pthread_mutex_unlock(&lock->mutex);
}

// Нельзя получить блокировку за запись пока есть блокировка на запись
// ждем освобождения записи
// Нельзя получить блокировку на запись, пока есть читатели
// ждем освобождения всех читателей
void custom_wrlock(custom_rwlock_t* lock) {
    pthread_mutex_lock(&lock->mutex);

    while (lock->writer_active) {
        lock->waiting_writers_count++;
        pthread_cond_wait(&lock->writers_cond, &lock->mutex);
        lock->waiting_writers_count--;
    }

    while (lock->readers_count > 0) {
        lock->waiting_writers_count++;
        pthread_cond_wait(&lock->readers_cond, &lock->mutex);
        lock->waiting_writers_count--;
    }

    lock->writer_active = 1;
    pthread_mutex_unlock(&lock->mutex);
}

void custom_unlock(custom_rwlock_t* lock) {
    pthread_mutex_lock(&lock->mutex);
    if (lock->writer_active) {
        lock->writer_active = 0;
        pthread_cond_broadcast(&lock->writers_cond);
    } else if (lock->readers_count) {
            lock->readers_count--;
            if (lock->readers_count == 0) {
                pthread_cond_broadcast(&lock->readers_cond);
            }
        }

    pthread_mutex_unlock(&lock->mutex);
}