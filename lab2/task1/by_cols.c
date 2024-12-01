#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void multiplyByCols(const float *matrix, const float *vector, float *result, int rows, int cols, int start_col,
                    int end_col) {
    for (int i = 0; i < rows; ++i) {
        for (int j = start_col; j < end_col; ++j) {
            result[i] += matrix[i * cols + j] * vector[j];
        }
    }
}

void generateData(float *matrix, float *vector, float *result, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i) {
        matrix[i] = (float) (rand() % 100) / 10.0f;
    }

    for (int i = 0; i < cols; ++i) {
        vector[i] = (float) (rand() % 100) / 10.0f;
    }

    for (int i = 0; i < rows; ++i) {
        result[i] = 0.0f;
    }
}

void clearVector(float *vector, int size) {
    for (int i = 0; i < size; ++i) {
        vector[i] = 0.0f;
    }
}


void loadSerialTimes(double *times, int size) {
    FILE *file = fopen("serial_times_by_cols.txt", "r");
    if (file == NULL) {
        perror("Failed to load serial times");
        return;
    }
    for (int i = 0; i < size; ++i) {
        fscanf(file, "%lf", &times[i]);
    }
    fclose(file);
}

void saveSerialTimes(double *times, int size) {
    FILE *file = fopen("serial_times_by_cols.txt", "w");
    if (file == NULL) {
        perror("Failed to save serial times");
        return;
    }
    for (int i = 0; i < size; ++i) {
        fprintf(file, "%lf\n", times[i]);
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int sizes[][2] = {{1000,  1000},
                      {5000,  5000},
                      {10000, 10000}};
    int num_sizes = 3;

    double T_serial[3] = {0.0, 0.0, 0.0};

    if (rank == 0) {
        if (size == 1) {
            printf("Running sequential test...\n");
        } else {
            printf("Running parallel test with %d processes...\n", size);
            loadSerialTimes(T_serial, num_sizes);
        }
        printf("Processes\tMatrix Size\tTime (s)\tSpeedup\t\tEfficiency\n");
    }

    for (int test = 0; test < num_sizes; ++test) {
        int rows = sizes[test][0];
        int cols = sizes[test][1];

        double start_time, end_time, duration = 0;
        
        float *matrix = (float *) malloc(rows * cols * sizeof(float));
        float *vector = (float *) malloc(cols * sizeof(float));
        float *local_result = (float *) malloc(rows * sizeof(float));
        float *global_result = NULL;

        for (int i = 0; i < rows; ++i) {
            local_result[i] = 0.0f;
        }

        if (rank == 0) {
            global_result = (float *) malloc(rows * sizeof(float));
            generateData(matrix, vector, global_result, rows, cols);
        }

        MPI_Bcast(vector, cols, MPI_FLOAT, 0, MPI_COMM_WORLD);

//        if (rank != 0) {
//            matrix = (float *) malloc(rows * cols * sizeof(float));
//        }

        MPI_Bcast(matrix, rows * cols, MPI_FLOAT, 0, MPI_COMM_WORLD);

        int current_cols = 1, start_col = 0, end_col = 0;
        if (rank < cols) {
            if (size <= cols) {
                current_cols = cols / size;
            }
            start_col = rank * current_cols;
            end_col = start_col + current_cols;
            if (rank == size - 1) {
                end_col = cols;
            }
        }

        int iters = 10;
        for (size_t i = 0; i < iters; ++i) {
            if (rank < cols) {
                start_time = MPI_Wtime();
                multiplyByCols(matrix, vector, local_result, rows, cols, start_col, end_col);
                end_time = MPI_Wtime();
                duration += end_time - start_time;
                clearVector(local_result, rows);
            }
        }

        duration /= (double) iters;

        if (rank < cols) {
            multiplyByCols(matrix, vector, local_result, rows, cols, start_col, end_col);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Reduce(local_result, global_result, rows, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            if (size == 1) {
                T_serial[test] = duration;
            }

            double speedup = (T_serial[test] > 0) ? T_serial[test] / duration : 1.0;
            double efficiency = (T_serial[test] > 0) ? speedup / size : 0.0;

            printf("%d\t\t%dx%d\t\t%.6f\t%.2f\t\t%.2f\n", size, rows, cols, duration, speedup, efficiency);
        }

        free(local_result);
        free(vector);
        if (rank == 0) {
            free(matrix);
            free(global_result);
        }
    }

    if (rank == 0 && size == 1) {
        saveSerialTimes(T_serial, num_sizes);
    }

    MPI_Finalize();

    return 0;
}
