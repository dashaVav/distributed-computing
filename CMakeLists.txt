cmake_minimum_required(VERSION 3.29)
project(distributed_computing_lab_1 C)

set(CMAKE_C_STANDARD 11)

include_directories(lab1/headers)

add_executable(distributed_computing_lab_1
        lab1/headers/mandelbrot.h
        lab1/headers/monte_carlo.h
        lab1/main.c
        lab1/mandelbrot.c
        lab1/monte_carlo.c
        lab1/timer.c
        lab1/main2.c)
