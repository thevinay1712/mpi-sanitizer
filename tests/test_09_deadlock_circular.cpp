#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data[100000]; // Large buffer to bypass eager limit
    if (rank == 0) {
        MPI_Send(data, 100000, MPI_INT, 1, 0, MPI_COMM_WORLD); // Will block
        MPI_Recv(data, 100000, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Send(data, 100000, MPI_INT, 0, 1, MPI_COMM_WORLD); // Will block -> deadlock
        MPI_Recv(data, 100000, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Finalize();
    return 0;
}