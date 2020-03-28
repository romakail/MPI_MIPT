#include <stdio.h>
#include <mpi.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int procNum  = 0;
    int procRank = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &procNum);
    MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

    if (procNum == 1)
    {
        printf ("Hello from process %d\n", procRank);
    }
    else
    {
        MPI_Status status;
        int buf = 0;
        if (procRank == 0)
        {
            printf ("Hello from process %d\n", procRank);
            fflush(stdout);

            for (int i = 1; i < procNum; i++)
            {
                MPI_Send(&i  , 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                //MPI_Recv(&buf, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                assert (buf == i);
            }
        }
        else
        {
            MPI_Recv(&buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            assert(buf == procRank);
            printf ("Hello from process %d\n", buf);
            fflush(stdout);
            MPI_Send(&procRank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
