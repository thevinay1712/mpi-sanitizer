#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data = 1;
    MPI_Send(&data, 1, MPI_INT, rank, 0, MPI_COMM_WORLD); // ERROR
    MPI_Finalize();
    return 0;
}