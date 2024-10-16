#include "headers/monte_carlo.h"
#include "headers/mandelbrot.h"

int main(int argc, char *argv[]) {
    mandelbrot(argc, argv);
    monte_carlo(argc, argv);
}
