#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void multiplyByRows(const float *matrix, const float *vector, float *result, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
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
    FILE *file = fopen("serial_times_by_rows.txt", "r");
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
    FILE *file = fopen("serial_times_by_rows.txt", "w");
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

        float start_time, end_time, duration = 0;

        int local_rows = rows / size;
        int local_elements = local_rows * cols;

        float *matrix = NULL;
        float *vector = (float *) malloc(cols * sizeof(float));
        float *local_matrix = (float *) malloc(local_elements * sizeof(float));
        float *local_vector = (float *) malloc(local_rows * sizeof(float));
        float *global_result = NULL;

        if (rank == 0) {
            matrix = (float *) malloc(rows * cols * sizeof(float));
            global_result = (float *) malloc(rows * sizeof(float));
            generateData(matrix, vector, global_result, rows, cols);
        }

        MPI_Scatter(matrix, local_elements, MPI_FLOAT, local_matrix, local_elements, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Bcast(vector, cols, MPI_FLOAT, 0, MPI_COMM_WORLD);

        size_t iters = 10;
        for (size_t i = 0; i < iters; ++i) {
            start_time = MPI_Wtime();
            multiplyByRows(local_matrix, vector, local_vector, local_rows, cols);
            end_time = MPI_Wtime();
            duration += end_time - start_time;
            clearVector(local_vector, local_rows);
        }

        duration /= iters;

        multiplyByRows(local_matrix, vector, local_vector, local_rows, cols);

        MPI_Gather(local_vector, local_rows, MPI_FLOAT, global_result, local_rows, MPI_FLOAT, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            size_t remain_rows = rows % size;
            size_t offset = rows - remain_rows;
            start_time = MPI_Wtime();
            multiplyByRows(matrix + offset * cols, vector, global_result + offset, remain_rows, cols);
            end_time = MPI_Wtime();
            duration += end_time - start_time;
        }

        if (rank == 0) {
            if (size == 1) {
                T_serial[test] = duration;
            }

            double speedup = (T_serial[test] > 0) ? T_serial[test] / duration : 1.0;
            double efficiency = (T_serial[test] > 0) ? speedup / size : 0.0;

            printf("%d\t\t%dx%d\t\t%.6f\t%.2f\t\t%.2f\n", size, rows, cols, duration, speedup, efficiency);
        }

        free(vector);
        free(local_matrix);
        free(local_vector);
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
