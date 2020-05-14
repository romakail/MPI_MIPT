#define _USE_MATH_DEFINES

#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>


typedef float calcType; // Change to double if need more precise computation

int strToInt (char* mode);
calcType calcSum(int startNum, int finalNum);

int main (int argc, char** argv)
{
    int rank = 0;
    int size = 0;

    double startTime = 0;
    double endTime   = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    assert(argc == 2);
    int seqLen = strToInt(argv[1]);
    if (seqLen == -1)
        return -1;

    /* Distributing data */
    int* thresholds = NULL;
    if (rank == 0)
    {
        thresholds = (int*) calloc (size + 1, sizeof(int));
        for (int i = 0; i < size + 1; i++)
        {
            thresholds[i] = 1 + seqLen * i / size;
        }
    }

    int startNum = 0;
    int finalNum = 0;

    MPI_Scatter(thresholds    , 1, MPI_INT, &startNum, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(thresholds + 1, 1, MPI_INT, &finalNum, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // printf ("[%d] start : %d, final : %d\n", rank, startNum, finalNum);

    /* Calculaton */
    calcType sum = calcSum(startNum, finalNum);

    /* Gathering data */
    calcType* sums = NULL;
    if (rank == 0)
        sums = (calcType*) calloc (size, sizeof(calcType));

    MPI_Gather(&sum, 1, MPI_FLOAT, sums, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        calcType seqSum = 0.0;
        for (int i = size - 1; i > -1; i--)
            seqSum += sums[i];

        seqSum = seqSum * 6 / M_PI / M_PI;
        printf ("Sequence sum : %f\n", seqSum);
    }

    MPI_Finalize();
    return 0;
}

calcType calcSum(int startNum, int finalNum)
{
    calcType sum = 0.0;
    for (int i = finalNum - 1; i > startNum - 1; i--)
        sum += 1 / (calcType) i / (calcType) i;
    return  sum;
}

int strToInt (char* mode)
{
    int val = 0;
    errno = 0;
    char* endptr = 0;

    val = strtol (mode, &endptr, 10);

    if (errno == ERANGE)
    {
        printf ("overflow of int\n");
        return -1;
    }
    else if (errno == EINVAL)
    {
        printf ("no digits seen\n");
        return -1;
    }
    else if (val < 0)
    {
        printf ("less then zero");
        return -1;
    }
    else if (*endptr != '\0')
    {
        printf ("I found not a digit\n");
        return -1;
    }

    return val;
}
