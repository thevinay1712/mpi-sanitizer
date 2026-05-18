#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data = 0;
    if (rank == 0) {
        MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        int recv;
        MPI_Reduce(&data, &recv, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); // ERROR
    }
    MPI_Finalize();
    return 0;
}