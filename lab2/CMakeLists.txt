cmake_minimum_required(VERSION 3.30)
project(laba2 C)

set(CMAKE_C_STANDARD 17)

add_executable(laba2 matrix_vector_mpi.c
        by_block.c)
