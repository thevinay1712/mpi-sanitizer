#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int data = 0;
    MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}