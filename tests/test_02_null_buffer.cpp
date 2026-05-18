#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) MPI_Send(nullptr, 10, MPI_INT, 1, 0, MPI_COMM_WORLD); // ERROR
    MPI_Finalize();
    return 0;
}