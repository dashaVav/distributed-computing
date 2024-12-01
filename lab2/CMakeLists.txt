cmake_minimum_required(VERSION 3.28)
project(lab2 C)

set(CMAKE_C_STANDARD 17)

add_executable(lab2 matrix_vector_mpi.c
        by_block.c
        by_rows.c)