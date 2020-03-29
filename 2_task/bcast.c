#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <limits.h>

int main (int argc, char** argv)
{
    int rank = 0;
    int size = 0;

    double startTime = 0;
    double endTime   = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int package = 666;

    MPI_Barrier (MPI_COMM_WORLD);
    startTime = MPI_Wtime();
    MPI_Bcast(&package, 1, MPI_INT, 0, MPI_COMM_WORLD);
    endTime   = MPI_Wtime();

    double time = endTime - startTime;
    double mean = time;
    double sum = time;
    double squaresSum = 0;
    double error = MPI_Wtick();
    double std  = 0;
    int stepNum = 1;
    char stop   = 0;

    double time_1 = MPI_Wtime();
    double time_2= MPI_Wtime();

    while (!stop)
    {
        MPI_Barrier (MPI_COMM_WORLD);
        startTime = MPI_Wtime();
        MPI_Bcast(&package, 1, MPI_INT, 0, MPI_COMM_WORLD);
        endTime   = MPI_Wtime();

        if (rank == 0)
        {
            stepNum++;
            time = (endTime - startTime);
            sum += time;
            mean = sum / stepNum;
            squaresSum += (mean - time) * (mean - time);

            std = sqrt(squaresSum / stepNum / (stepNum - 1));
            if (std < error)
                stop = 1;

            if (stepNum % 10000 == 0)
            {
                time_1 = MPI_Wtime();
                printf("[%d] std = %lg, time = %lg, mean = %lg\n", stepNum, std, time_1 - time_2, mean);
                time_2 = time_1;
            }
        }
        MPI_Bcast(&stop, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
    {
        printf("[%d] Error = [%lg], STD = [%lg], mean = [%lg]\n", rank, error, std, mean);
        printf("It took %d steps to converge\n", stepNum);
    }

    MPI_Finalize();
    return 0;
}
