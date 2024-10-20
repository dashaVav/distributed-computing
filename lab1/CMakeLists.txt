cmake_minimum_required(VERSION 3.28)
project(lab1 C)

set(CMAKE_C_STANDARD 17)

add_executable(lab1 main_monte_carlo.c
        monte_carlo.c
        mandelbrot.c
        task3/custom_rwlock.c
        task3/my_rand.c
        task3/pth_ll_rwl.c
        main_mandelbrot.c
        main_monte_carlo.c
)
