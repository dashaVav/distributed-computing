#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

long total_hits = 0;
long ntrials;
int nthreads;

pthread_mutex_t mutex;

double get_random_point() {
    return (double) rand() / RAND_MAX;
}

void *monte_carlo() {
    srand(time(NULL));
    long local_hits = 0;

    long trials_per_thread = ntrials / nthreads;
    for (long i = 0; i < trials_per_thread; ++i) {
        double x = get_random_point();
        double y = get_random_point();

        if (x * x + y * y <= 1.0) {
            local_hits++;

        }
    }
    printf("%d\n", local_hits);
    pthread_mutex_lock(&mutex);
    total_hits += local_hits;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s nthreads ntrials\n", argv[0]);
        return EXIT_FAILURE;
    }

    nthreads = atoi(argv[1]);
    ntrials = atol(argv[2]);

    pthread_t threads[nthreads];
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < nthreads; ++i) {
        pthread_create(&threads[i], NULL, monte_carlo, (void *) (intptr_t) i);
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    double pi = 4.0 * (double) total_hits / (double) ntrials;

    printf("Estimated value of π: %f\n", pi);

    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}
