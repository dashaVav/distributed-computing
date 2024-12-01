#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <string.h>

void generateData(float* matrix, float* vector, float* result, int rows, int cols, int vecSize) {
    for (int i = 0; i < rows * cols; ++i) {
        matrix[i] = (float)(rand() % 100) / 10.0f;
    }
    for (int i = 0; i < vecSize; ++i) {
        vector[i] = (float)(rand() % 100) / 10.0f;
    }
    for (int i = 0; i < rows; ++i) {
        result[i] = 0.0f;
    }
}

void clearVector(float* vector, int size) {
    for (int i = 0; i < size; ++i) {
        vector[i] = 0.0f;
    }
}

void gemvByBlocks(const float* matrix, const float* vector, float* result, int numCols, int startRow, int endRow, int startCol, int endCol) {
    for (int i = startRow; i < endRow; ++i) {
        for (int j = startCol; j < endCol; ++j) {
            result[i] += matrix[i * numCols + j] * vector[j];
        }
    }
}

void execUnitTest(const float* matrix, const float* vector, const float* result, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        float expected = 0.0f;
        for (int j = 0; j < cols; ++j) {
            expected += matrix[i * cols + j] * vector[j];
        }
        if (fabs(result[i] - expected) > 1e-3) {
            printf("Error in computation at row %d\n", i);
            return;
        }
    }
}

void saveSerialTimes(double* times, int size) {
    FILE* file = fopen("serial_times.txt", "w");
    if (file == NULL) {
        perror("Failed to save serial times");
        return;
    }
    for (int i = 0; i < size; ++i) {
        fprintf(file, "%lf\n", times[i]);
    }
    fclose(file);
}

void loadSerialTimes(double* times, int size) {
    FILE* file = fopen("serial_times.txt", "r");
    if (file == NULL) {
        perror("Failed to load serial times");
        return;
    }
    for (int i = 0; i < size; ++i) {
        fscanf(file, "%lf", &times[i]);
    }
    fclose(file);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nProc, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);

    int sizes[][2] = {{1000, 1000}, {5000, 5000}, {10000, 10000}};
    int numSizes = 3;

    double T_serial[3] = {0.0, 0.0, 0.0};

    if (rank == 0) {
        if (nProc == 1) {
            printf("Running sequential test...\n");
        } else {
            printf("Running parallel test with %d processes...\n", nProc);
            loadSerialTimes(T_serial, numSizes);
        }
        printf("Processes\tMatrix Size\tTime (s)\tSpeedup\t\tEfficiency\n");
    }

    for (int test = 0; test < numSizes; ++test) {
        int rows = sizes[test][0];
        int cols = sizes[test][1];
        int vecSize = cols;

        float* matrix = (float*)malloc(rows * cols * sizeof(float));
        float* vector = (float*)malloc(vecSize * sizeof(float));
        float* localResult = (float*)malloc(rows * sizeof(float));
        float* globalResult = NULL;

        for (int i = 0; i < rows; ++i) {
            localResult[i] = 0.0f;
        }

        if (rank == 0) {
            globalResult = (float*)malloc(rows * sizeof(float));
            generateData(matrix, vector, globalResult, rows, cols, vecSize);
        }

        MPI_Bcast(vector, cols, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Bcast(matrix, rows * cols, MPI_FLOAT, 0, MPI_COMM_WORLD);

        int blocks1d = (int)sqrt(nProc);
        int blockRows = rows / blocks1d;
        int blockCols = cols / blocks1d;

        int startRow = (rank / blocks1d) * blockRows;
        int endRow = startRow + blockRows;
        int startCol = (rank % blocks1d) * blockCols;
        int endCol = startCol + blockCols;

        if (rank < blocks1d * blocks1d) {
            if (rank == blocks1d * blocks1d - 1) {
                endRow = rows;
                endCol = cols;
            }
        }

        double startTime, endTime, duration = 0.0;
        int iters = 10;

        for (int i = 0; i < iters; ++i) {
            startTime = MPI_Wtime();
            if (rank < blocks1d * blocks1d) {
                gemvByBlocks(matrix, vector, localResult, cols, startRow, endRow, startCol, endCol);
            }
            endTime = MPI_Wtime();
            duration += endTime - startTime;
            clearVector(localResult, rows);
        }
        duration /= iters;

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Reduce(localResult, globalResult, rows, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            execUnitTest(matrix, vector, globalResult, rows, cols);

            if (nProc == 1) {
                T_serial[test] = duration;
            }

            double speedup = (T_serial[test] > 0) ? T_serial[test] / duration : 1.0;
            double efficiency = (T_serial[test] > 0) ? speedup / nProc : 0.0;

            printf("%d\t\t%dx%d\t\t%.6f\t%.2f\t\t%.2f\n", nProc, rows, cols, duration, speedup, efficiency);
        }

        free(matrix);
        free(vector);
        free(localResult);
        if (rank == 0) {
            free(globalResult);
        }
    }

    if (rank == 0 && nProc == 1) {
        saveSerialTimes(T_serial, numSizes);
    }

    MPI_Finalize();
    return 0;
}