#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


// Функция для вывода матрицы на экран
void PrintMatrix(double* matrix, int Size) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            printf("%6.2f ", matrix[i * Size + j]);
        }
        printf("\n");
    }
}


// Функция для случайной инициализации матриц A и B
void RandomDataInitialization(double* pAMatrix, double* pBMatrix, int Size) {
    srand(time(NULL));
    for (int i = 0; i < Size * Size; ++i) {
        pAMatrix[i] = (rand() % 100) + 1;
        pBMatrix[i] = (rand() % 100) + 1;
    }
}


// Функция для заполнения матрицы нулями
void FillMatrixWithZeros(double* matrix, int Size) {
    for (int i = 0; i < Size * Size; i++) {
        matrix[i] = 0.0;
    }
}


// Функция для умножения матриц A и B (для сравнения этого результата с полученным благодаря алгоритму Кэннона)
void MatrixMultiplication(double* pAMatrix, double* pBMatrix, double* pCMatrix, int Size) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            for (int k = 0; k < Size; k++) {
                pCMatrix[i * Size + j] += pAMatrix[i * Size + k] * pBMatrix[k * Size + j];
            }
        }
    }
}


// Функция для проверки результата умножения
int CheckResult(double* pCMatrix, double* pCParallelMatrix, int Size) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            if (pCMatrix[i * Size + j] != pCParallelMatrix[i * Size + j]) {
                printf("Ошибка в позиции (%d, %d)\n", i, j);
                return 0; 
            }
        }
    }
    return 1; 
}

// Сборка результатов матрицы C 
void ResultCollection(double* pCMatrix, double* pCblock, int Size, int BlockSize) {
    int rank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    int numBlocksPerLine = Size / BlockSize; 

    if (rank == 0) {

        double* recvBuffer = (double*)malloc(worldSize * BlockSize * BlockSize * sizeof(double));
        int* sendCounts = (int*)malloc(worldSize * sizeof(int));
        int* displacements = (int*)malloc(worldSize * sizeof(int));

        for (int i = 0; i < worldSize; ++i) {
            sendCounts[i] = BlockSize * BlockSize;
            displacements[i] = i * BlockSize * BlockSize;
        }

        MPI_Gatherv(pCblock, BlockSize * BlockSize, MPI_DOUBLE,
                    recvBuffer, sendCounts, displacements, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Размещаем блоки из промежуточного буфера в результирующую матрицу
        for (int i = 0; i < worldSize; ++i) {
            int blockRow = i / numBlocksPerLine;
            int blockCol = i % numBlocksPerLine;

            for (int row = 0; row < BlockSize; ++row) {
                for (int col = 0; col < BlockSize; ++col) {
                    int globalRow = blockRow * BlockSize + row;
                    int globalCol = blockCol * BlockSize + col;

                    pCMatrix[globalRow * Size + globalCol] = recvBuffer[i * BlockSize * BlockSize + row * BlockSize + col];
                }
            }
        }

        free(recvBuffer);
        free(sendCounts);
        free(displacements);
    } else {
        // Остальные процессы отправляют свои блоки
        MPI_Gatherv(pCblock, BlockSize * BlockSize, MPI_DOUBLE,
                    NULL, NULL, NULL, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
}


// Заполняет массив номерами процессов на одно линии в сетке для А
void FillALine(int* line, int numBlocksPerLine, int lineNumber) {
    for(int i = 0; i < numBlocksPerLine; i++) {
        line[i] = i + lineNumber * numBlocksPerLine;
    }
}


// Заполняет массив номерами процессов на одно линии в сетке для B
void FillBLine(int* line, int numBlocksPerLine, int lineNumber) {
    for(int i = 0; i < numBlocksPerLine; i++) {
        line[i] = i * numBlocksPerLine + lineNumber;
    }
}


// Сдвиг блоков (0 - А, 1 - В) 
void BlockCommunication(double* pMBlock, int Size, int BlockSize, int type, int shiftSize) {
    MPI_Status status;
    int numBlocksPerLine = Size / BlockSize;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int lineNumber, myPositionInLine;
    int* line = (int*) malloc(numBlocksPerLine * sizeof(int));

    if (type == 0) {
        lineNumber = rank / numBlocksPerLine;
        FillALine(line, numBlocksPerLine, lineNumber);
        myPositionInLine = rank % numBlocksPerLine;
    }
    else {
        lineNumber = rank % numBlocksPerLine;
        FillBLine(line, numBlocksPerLine, lineNumber);
        myPositionInLine = (rank - lineNumber) / numBlocksPerLine;
    }

    int dest = line[(myPositionInLine - shiftSize + numBlocksPerLine) % numBlocksPerLine];
    int source = line[(myPositionInLine + shiftSize) % numBlocksPerLine];
    MPI_Sendrecv_replace(pMBlock, BlockSize * BlockSize, MPI_DOUBLE, dest, type, source, type, MPI_COMM_WORLD, &status);
}


// Основная функция для алгоритма Кэннона
void CannonAlgorithm(double* pAblock, double* pBblock, double* pCblock, int Size, int BlockSize) {

    int numBlocksPerLine = Size / BlockSize;

    for (int iter = 0; iter < numBlocksPerLine; ++iter) {
        // Умножение блоков A и B
        for (int i = 0; i < BlockSize; ++i) {
            for (int j = 0; j < BlockSize; ++j) {
                for (int k = 0; k < BlockSize; ++k) {
                    pCblock[i * BlockSize + j] += pAblock[i * BlockSize + k] * pBblock[k * BlockSize + j];
                }
            }
        }

        if (iter != numBlocksPerLine - 1) {
            // Коммуникация блоков A и B после умножения
            BlockCommunication(pAblock, Size, BlockSize, 0, 1);
            BlockCommunication(pBblock, Size, BlockSize, 1, 1);
        }
    }
}


int main(int argc, char** argv) {
    int rank, numProcesses, Size, BlockSize;
    double startTime, endTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);

    if (rank == 0) {
        printf("Enter matrix size: \n");
        scanf("%d", &Size);
    }

    MPI_Bcast(&Size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int k = (int) sqrt((double) numProcesses);
    BlockSize = Size / k;

    double* pAMatrix = NULL;
    double* pBMatrix = NULL;
    double* pCMatrix = NULL;

    double* pAblock = (double*) malloc(BlockSize * BlockSize * sizeof(double));
    double* pBblock = (double*) malloc(BlockSize * BlockSize * sizeof(double));
    double* pCblock = (double*) malloc(BlockSize * BlockSize * sizeof(double));

    int numBlocksPerLine = Size / BlockSize;
  
    if (rank == 0) {
        
        pAMatrix = (double*)malloc(Size * Size * sizeof(double));
        pBMatrix = (double*)malloc(Size * Size * sizeof(double));
        pCMatrix = (double*)malloc(Size * Size * sizeof(double));

        RandomDataInitialization(pAMatrix, pBMatrix, Size);
        FillMatrixWithZeros(pCMatrix, Size);

        startTime = MPI_Wtime();

        // Подготовка параметров для Scatterv
        int* sendCounts = (int*)malloc(numProcesses * sizeof(int));
        int* displacements = (int*)malloc(numProcesses * sizeof(int));

        double* tempABuffer = (double*)malloc(numProcesses * BlockSize * BlockSize * sizeof(double));
        double* tempBBuffer = (double*)malloc(numProcesses * BlockSize * BlockSize * sizeof(double));

        for (int i = 0; i < numProcesses; ++i) {
            int blockRow = i / numBlocksPerLine;
            int blockCol = i % numBlocksPerLine;
            sendCounts[i] = BlockSize * BlockSize;
            displacements[i] = i * BlockSize * BlockSize;

            // Заполняем временные буферы для A и B
            for (int row = 0; row < BlockSize; ++row) {
                for (int col = 0; col < BlockSize; ++col) {
                    int globalRow = blockRow * BlockSize + row;
                    int globalCol = blockCol * BlockSize + col;
                    tempABuffer[displacements[i] + row * BlockSize + col] = pAMatrix[globalRow * Size + globalCol];
                    tempBBuffer[displacements[i] + row * BlockSize + col] = pBMatrix[globalRow * Size + globalCol];
                }
            }
        }

        // Рассылка блоков матриц A и B
        MPI_Scatterv(tempABuffer, sendCounts, displacements, MPI_DOUBLE, pAblock, BlockSize * BlockSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(tempBBuffer, sendCounts, displacements, MPI_DOUBLE, pBblock, BlockSize * BlockSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        free(tempABuffer);
        free(tempBBuffer);
    } else {
        MPI_Scatterv(NULL, NULL, NULL, MPI_DOUBLE, pAblock, BlockSize * BlockSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(NULL, NULL, NULL, MPI_DOUBLE, pBblock, BlockSize * BlockSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    FillMatrixWithZeros(pCblock, BlockSize);

    BlockCommunication(pAblock, Size, BlockSize, 0, rank / numBlocksPerLine);
    BlockCommunication(pBblock, Size, BlockSize, 1, rank % numBlocksPerLine);

    CannonAlgorithm(pAblock, pBblock, pCblock, Size, BlockSize);

    ResultCollection(pCMatrix, pCblock, Size, BlockSize);

    if (rank == 0) {

        endTime = MPI_Wtime();

        double* pCorrectCMatrix = (double*) malloc(Size * Size * sizeof(double));
        FillMatrixWithZeros(pCorrectCMatrix, Size);

        MatrixMultiplication(pAMatrix, pBMatrix, pCorrectCMatrix, Size);

        if (CheckResult(pCMatrix, pCorrectCMatrix, Size)) {
            printf("Результаты совпадают!\n");
        } else {
            printf("Результаты не совпадают!\n");
        }

        double durationCannon = endTime - startTime;

        printf("Matrix size,numProcesses,durationCannon\n");
        printf("%d,%d,%.6lf\n", Size, numProcesses, durationCannon);

        free(pCorrectCMatrix);
        free(pAMatrix);
        free(pBMatrix);
        free(pCMatrix);
    }

    MPI_Finalize();

    
    return 0;
}
