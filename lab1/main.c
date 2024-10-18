#include "headers/monte_carlo.h"
#include "headers/mandelbrot.h"
#include "headers/pth_ll_rwl.h"

int main(int argc, char *argv[]) {
    mandelbrot(argc, argv);
    monte_carlo(argc, argv);
    readWriteLock(argc, argv);
}
