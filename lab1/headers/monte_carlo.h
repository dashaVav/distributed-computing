#ifndef LAB1_MONTE_CARLO_H
#define LAB1_MONTE_CARLO_H

#include <pthread.h>

typedef struct {
    long trials_per_thread;
} monte_carlo_args;

int monte_carlo(int argc, char *argv[]);

#endif //LAB1_MONTE_CARLO_H