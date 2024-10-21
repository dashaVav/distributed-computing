#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "headers/pth_ll_rwl.h"
#include "headers/my_rand.h"
#include "headers/timer.h"

#define MAX_KEY 100000000

extern struct list_node_s *head;
extern int thread_count;
int total_ops;
extern double insert_percent;
extern double search_percent;
extern double delete_percent;
pthread_rwlock_t rwlock;
pthread_mutex_t count_mutex;
extern int member_count, insert_count, delete_count;

custom_rwlock_t custom_rwlock;

void run_experiment(int num_threads, int num_ops, FILE *file) {
    long i;
    pthread_t *thread_handles;
    unsigned seed = 1;
    double start, finish;
    thread_count = num_threads;
    total_ops = num_ops;
    pthread_rwlock_init(&rwlock, NULL);
    pthread_mutex_init(&count_mutex, NULL);
    thread_handles = malloc(thread_count * sizeof(pthread_t));
    GET_TIME(start);
    for (i = 0; i < thread_count; i++) {
        pthread_create(&thread_handles[i], NULL, Thread_work, (void *) i);
    }
    for (i = 0; i < thread_count; i++) {
        pthread_join(thread_handles[i], NULL);
    }
    GET_TIME(finish);
    double standard_time = finish - start;
    printf("Standard RWLock - Threads: %d, Ops: %d, Time: %.6f seconds\n", thread_count, total_ops, standard_time);
    pthread_rwlock_destroy(&rwlock);
    free(thread_handles);
    custom_rwlock_init(&custom_rwlock);
    thread_handles = malloc(thread_count * sizeof(pthread_t));
    GET_TIME(start);
    for (i = 0; i < thread_count; i++) {
        pthread_create(&thread_handles[i], NULL, Custom_Thread_work, (void *) i);
    }
    for (i = 0; i < thread_count; i++) {
        pthread_join(thread_handles[i], NULL);
    }
    GET_TIME(finish);
    double custom_time = finish - start;
    printf("Custom RWLock - Threads: %d, Ops: %d, Time: %.6f seconds\n", thread_count, total_ops, custom_time);
    custom_rwlock_destroy(&custom_rwlock);
    free(thread_handles);
    fprintf(file, "%d,%d,%.6f,%.6f\n", num_threads, num_ops, standard_time, custom_time);
}

int main() {
    int thread_values[] = {1, 4, 16, 32, 64, 100};
    int ops_values[] = {10, 100, 1000, 10000};
    int num_thread_tests = sizeof(thread_values) / sizeof(thread_values[0]);
    int num_ops_tests = sizeof(ops_values) / sizeof(ops_values[0]);
    FILE *file = fopen("rwlock_results.csv", "w");
    if (file == NULL) {
        perror("Unable to open file");
        return EXIT_FAILURE;
    }
    fprintf(file, "Threads,Ops,Standard RWLock Time (s),Custom RWLock Time (s)\n");
    for (int i = 0; i < num_thread_tests; i++) {
        for (int j = 0; j < num_ops_tests; j++) {
            run_experiment(thread_values[i], ops_values[j], file);
        }
    }
    fclose(file);
    return 0;
}