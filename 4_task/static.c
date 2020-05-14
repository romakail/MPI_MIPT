#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

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
    status_t status = FAIL;
    int tasksNumber = 0;
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
        printf("Usage:\n required 2 arguments (> 0) for tasks number\n");
        // Exit with fail status
        status = FAIL;
        MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Finalize();
        return 0;
    }

    // Send status of arguments
    status = OK;
    MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Prepare slave's tasks
    int* term1 = 0;
    int* term2 = 0;
    int nNums = 0;
    readData(argv[1], &term1, &term2, &tasksNumber);

    int localLen = tasksNumber / (size - 1);
    int rest = tasksNumber % (size - 1);
    task_t* taskShedule = malloc(size * sizeof(task_t));
    int taskPointer = 0;
    printf("Shedule : ");
    for (i = 1; i < size; i++)
    {
        taskShedule[i].start = taskPointer;
        taskPointer += localLen;
        if (i < (rest + 1))
        {
            taskPointer++;
        }
        taskShedule[i].end = taskPointer;
        taskShedule[i].tasksNum = tasksNumber;
        printf("[%d] %d(%d..%d) ", i, taskShedule[i].end - taskShedule[i].start\
                                 , taskShedule[i].start, taskShedule[i].end - 1);
    }
    printf("\n");

    // Send tasks for work begin
    startTime = MPI_Wtime();
    MPI_Scatter(taskShedule, 3, MPI_INT, &localTask, 3, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(term1, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(term2, tasksNumber, MPI_INT, 0, MPI_COMM_WORLD);

    int* sum = (int*) calloc(localLen + tasksNumber, sizeof(*sum));
    int* sendBuf = (int*) calloc(localLen, sizeof(*sendBuf));
    MPI_Gather(sendBuf, localLen, MPI_INT, sum, localLen, MPI_INT, 0, MPI_COMM_WORLD);

    printResult(sum+localLen, tasksNumber, argv[2]);

    }
    else
    {
        // Slave branch
        //
        // Check status
        MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (status == FAIL)
        {
            MPI_Finalize();
            return 0;
        }

        // Get tasks
        MPI_Scatter(NULL, 0, 0, &localTask, 3, MPI_INT, 0, MPI_COMM_WORLD);
        //printf("[%d] Get %d..%d\n", rank, localTask.start, localTask.end - 1);
        int* term1 = (int*) malloc(localTask.tasksNum * sizeof(*term1));
        int* term2 = (int*) malloc(localTask.tasksNum * sizeof(*term2));
        MPI_Bcast(term1, localTask.tasksNum, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(term2, localTask.tasksNum, MPI_INT, 0, MPI_COMM_WORLD);
        // Calc task
        startTime = MPI_Wtime();

        int* sum = (int*) calloc (localTask.end - localTask.start, sizeof(*sum));
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

        MPI_Gather(sum, localTask.end - localTask.start, MPI_INT,
                    NULL, localTask.tasksNum, MPI_INT, 0, MPI_COMM_WORLD);

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
