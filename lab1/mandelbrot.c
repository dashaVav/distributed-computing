#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <complex.h>
#include <stdbool.h>
#include "headers/mandelbrot.h"

pthread_mutex_t mandelbrot_mutex;
long written_points;

int check_mandelbrot_point(double complex c) {
    double complex z = 0 + 0 * I;
    for (int i = 0; i < 1000; ++i) {
        z = z * z + c;
        if (cabs(z) > 2.0) {
            return 0;
        }
    }
    return 1;
}

double getPoint(double min, double max) {
    return min + (max - min) * rand() / (double) RAND_MAX;
}

void *compute_mandelbrot(void *varg) {
    mandelbrot_args *args = (mandelbrot_args *) varg;
    srand(time(NULL));

    while (true) {
        double x = getPoint(-2.0, 1.0);
        double y = getPoint(-1.5, 1.5);
        double complex c = x + y * I;

        if (check_mandelbrot_point(c)) {
            pthread_mutex_lock(&mandelbrot_mutex);
            if (written_points >= args->npoints) {
                pthread_mutex_unlock(&mandelbrot_mutex);
                break;
            }
            fprintf(args->output_file, "%f,%f\n", x, y);
            written_points++;
            pthread_mutex_unlock(&mandelbrot_mutex);
        }
    }
    return NULL;
}

int mandelbrot(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s nthreads npoints\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nthreads = strtoll(argv[1], NULL, 10);
    long npoints = strtoll(argv[2], NULL, 10);

    written_points = 0;
    FILE *output_file = fopen(FILENAME, "w");
    if (!output_file) {
        perror("Failed to open file!");
        return EXIT_FAILURE;
    }

    pthread_mutex_init(&mandelbrot_mutex, NULL);
    pthread_t *threads = malloc(nthreads * sizeof(pthread_t));

    for (int i = 0; i < nthreads; ++i) {
        mandelbrot_args *args = (mandelbrot_args *) malloc(sizeof(mandelbrot_args));
        args->output_file = output_file;
        args->npoints = npoints;
        pthread_create(&threads[i], NULL, compute_mandelbrot, (void *) args);
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    fclose(output_file);
    pthread_mutex_destroy(&mandelbrot_mutex);
    free(threads);
    return EXIT_SUCCESS;
}
