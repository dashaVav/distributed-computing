cmake_minimum_required(VERSION 3.28)
project(lab2 C)

set(CMAKE_C_STANDARD 17)

add_executable(lab2
        task1/by_block.c
        task1/by_rows.c
        task1/by_cols.c)