#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

typedef enum {
    OK = 0,
    FAIL = 1
} status_t;

typedef struct {
    int start;
    int end;
    int tasksNum;
} task_t;


const int NUM_LEN = 9;

int readData (char* inputFileName, int** term1, int** term2, int* nNums);
int printResult (int* sum, int len, char* fileName);
int myPow(int x, int p);
int calcExcess (int* term1, int* term2, int idx, int tasksNum);

int main(int argc, char* argv[])
{
    status_t argStatus = FAIL;
    MPI_Status status = {};
    int tasksNumber = 0;
    int blockSize = 0;
    task_t localTask = {};
    int rank = 0;
    int size = 0;
    double startTime = 0;
    double endTime = 0;
    int i = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        // Root branch
        //
        // Check arguments
        if (argc != 3)
        {
            printf("Usage:\n required 2 arguments (> 0)\
              \n for tasks number and block size\n");
            // Exit with fail status
            argStatus = FAIL;
            MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Finalize();
        return 0;
        }

        // Send status of arguments
        argStatus = OK;
        MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);


        int* term1 = 0;
        int* term2 = 0;
        readData(argv[1], &term1, &term2, &tasksNumber);
        blockSize = (int) sqrt(tasksNumber);
        printf ("BlockSize = %d\n", blockSize);
        // Prepare slave's tasks
        int blocksNumber = tasksNumber / blockSize;
        int rest = tasksNumber % blockSize;
        printf("Shedule : %d blocks(%d)", blocksNumber, blockSize);
        if (rest != 0)
        {
            blocksNumber++;
            printf(", 1 block(%d)", rest);
        } else {
            // size of first block
            rest = blockSize;
        }
        printf("\n");

        task_t* taskShedule = malloc(blocksNumber * sizeof(task_t));

        // Add first block
        taskShedule[0].start = 0;
        taskShedule[0].end = rest;
        taskShedule[0].tasksNum = tasksNumber;


        // Add other blocks
        int taskPtr = rest;
        for (i = 1; i < blocksNumber; i++)
        {
            taskShedule[i].start = taskPtr;
            taskPtr += blockSize;
            taskShedule[i].end = taskPtr;
            taskShedule[i].tasksNum = tasksNumber;
        }

        // Distributing information

        MPI_Bcast(&tasksNumber, 1   , MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&blockSize  , 1   , MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(term1, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(term2, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);


        // Send blockss for work begin
        int blockPtr = 0;
        int start = 0;
        int end = 0;
        int* sum = 0;
        int slave = 0;
        int* answer = (int*)calloc (2 + blockSize, sizeof(*answer));
        int* result = (int*)calloc (tasksNumber,   sizeof(*answer));

        startTime = MPI_Wtime();

        for (blockPtr = 0; blockPtr < blocksNumber; blockPtr++)
        {
            // Find slave
            MPI_Recv(answer, 2 + blockSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            slave = status.MPI_SOURCE;
            start = answer[0];
            end   = answer[1];
            sum   = &answer[2];
            for (int resultPtr = start, i = 0; resultPtr < end; resultPtr++, i++)
            {
                result[resultPtr] = sum[i];
            }
            // Send block
            MPI_Send(&taskShedule[blockPtr], 3, MPI_INT, slave, OK, MPI_COMM_WORLD);
        }

        // End calc
        for (i = 1; i < size; i++)
        {
            MPI_Recv(answer, 2 + blockSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            slave = status.MPI_SOURCE;
            start = answer[0];
            end   = answer[1];
            sum   = &answer[2];
            for (int resultPtr = start, i = 0; resultPtr < end; resultPtr++, i++)
            {
                result[resultPtr] = sum[i];
            }
            MPI_Send(taskShedule, 3, MPI_INT, slave, FAIL, MPI_COMM_WORLD);
        }

        printResult(result, tasksNumber, argv[2]);

    }
    else
    {
        // Slave branch
        //
        // Check status
        MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (argStatus == FAIL)
        {
            MPI_Finalize();
            return 0;
        }

        int blockSize = 0;
        int tasksNumber = 0;
        MPI_Bcast(&tasksNumber, 1, MPI_INT, 0, MPI_COMM_WORLD);
        int* term1 = (int*) malloc(tasksNumber * sizeof(*term1));
        int* term2 = (int*) malloc(tasksNumber * sizeof(*term2));
        MPI_Bcast(&blockSize, 1     , MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(term1, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(term2, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);


        int* msg  = (int*)calloc(2 + blockSize, sizeof(*msg));
        int* sum = &msg[2];
        // Calc loop
        startTime = MPI_Wtime();

        localTask.start = 0;
        localTask.end = 0;
        localTask.tasksNum = 0;

        while (1)
        {
            // Ready for tasks
            msg[0] = localTask.start;
            msg[1] = localTask.end;
            MPI_Send(msg, 2 + blockSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
            for (int i = 0; i < blockSize; i++)
                sum[i] = 0;
            // Get block
            MPI_Recv(&localTask, 3, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == OK)
            {
                // Calc block
                int sumPointer = localTask.end - localTask.start - 1;
                sum[sumPointer] += calcExcess(term1, term2, localTask.end, localTask.tasksNum);

                for (i = localTask.end - 1; i >= localTask.start; i--, sumPointer--)
                {
                    sum[sumPointer] += term1[i] + term2[i];
                    if (sum[sumPointer] >= myPow(10, NUM_LEN))
                    {
                        if (sumPointer != 0)
                        {
                            sum[sumPointer-1] += 1;
                        }
                        if (i != 0)
                        {
                            sum[sumPointer] = sum[sumPointer] % myPow(10, NUM_LEN);
                        }
                    }
                    // sleep(rank);
                }
            } else {
                break;
            }
        }
        endTime = MPI_Wtime();
        // printf("[TIME %d] %lf\n", rank, endTime - startTime);

    }

  // Finish
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        endTime = MPI_Wtime();
        printf("[TIME RES] %lf\n", endTime - startTime);
    }

    MPI_Finalize();
    return 0;
}

int printResult (int* sum, int len, char* fileName)
{
    FILE* outputFile = fopen(fileName, "w");
    assert(outputFile);

    for (int i = 0; i < len; i++)
    {
        fprintf(outputFile, "%09d", sum[i]);
    }

    fclose(outputFile);
    return 0;
}

int readData (char* inputFileName, int** term1, int** term2, int* nNums)
{
    FILE* inputFile = fopen(inputFileName, "r");
    assert(inputFile);

    int nDigits = 0;
    fscanf(inputFile, "%d\n", &nDigits);

    assert((nDigits % NUM_LEN) == 0);
    *nNums = nDigits / NUM_LEN;

    *term1 = (int*)calloc(*nNums, sizeof(*term1));
    *term2 = (int*)calloc(*nNums, sizeof(*term2));

    for (int i = 0; i < *nNums; i++)
    {
        fscanf (inputFile, "%9d", *term1+i);
    }
    for (int i = 0; i < *nNums; i++)
    {
        fscanf (inputFile, "%9d", *term2+i);
    }

    fclose(inputFile);
    return 0;
}


int myPow(int x, int p)
{
    if (p == 0) return 1;
    if (p == 1) return x;
    return x * myPow(x, p-1);
}

int calcExcess (int* term1, int* term2, int idx, int tasksNum)
{
    if (idx == -1)
        return 0;

    int sum = term1[idx] + term2[idx];

    if      (sum >= myPow(10, NUM_LEN))
        return 1;
    else if ((sum <  myPow(10, NUM_LEN) - 1))
        return 0;
    else
        return calcExcess(term1, term2, idx+1, tasksNum);
}
