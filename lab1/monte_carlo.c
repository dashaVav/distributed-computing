#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "headers/monte_carlo.h"

long monte_carlo_total_hits = 0;
pthread_mutex_t monte_carlo_mutex;

double get_random_point() {
    return (double) rand() / RAND_MAX;
}

void *compute_monte_carlo(void *vargs) {
    srand(time(NULL));
    long local_hits = 0;
    monte_carlo_args *args = (monte_carlo_args *) vargs;

    for (long i = 0; i < args->trials_per_thread; ++i) {
        double x = get_random_point();
        double y = get_random_point();
        if (x * x + y * y <= 1.0) {
            local_hits++;
        }
    }
    pthread_mutex_lock(&monte_carlo_mutex);
    monte_carlo_total_hits += local_hits;
    pthread_mutex_unlock(&monte_carlo_mutex);
    return NULL;
}

int monte_carlo(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s nthreads ntrials\n", argv[0]);
        return EXIT_FAILURE;
    }

    long nthreads = strtoll(argv[1], NULL, 10);
    int ntrials = strtoll(argv[2], NULL, 10);
    long trials_per_thread = ntrials / nthreads;
    pthread_t *threads = malloc(nthreads * sizeof(pthread_t));
    pthread_mutex_init(&monte_carlo_mutex, NULL);

    for (int i = 0; i < nthreads; ++i) {
        monte_carlo_args *args = (monte_carlo_args *) malloc(sizeof(monte_carlo_args));
        args->trials_per_thread = trials_per_thread;
        pthread_create(&threads[i], NULL, compute_monte_carlo, (void *) args);
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    double pi = 4.0 * (double) monte_carlo_total_hits / (double) ntrials;
    printf("pi: %f", pi);

    pthread_mutex_destroy(&monte_carlo_mutex);
    free(threads);
    return EXIT_SUCCESS;
}