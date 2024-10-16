#ifndef LAB1_MANDELBROT_H
#define LAB1_MANDELBROT_H

#define FILENAME "mandelbrot.csv"

#include <stdio.h>

typedef struct {
    FILE *output_file;
    long npoints;
} mandelbrot_args;

int mandelbrot(int argc, char *argv[]);

#endif //LAB1_MANDELBROT_H
