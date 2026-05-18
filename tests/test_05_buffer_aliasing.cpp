#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data[10];
    if (rank == 0) {
        MPI_Request req, req2;
        MPI_Isend(data, 10, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Isend(data, 10, MPI_INT, 1, 1, MPI_COMM_WORLD, &req2); // ERROR: Aliasing
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MPI_Wait(&req2, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Recv(data, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(data, 10, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Finalize();
    return 0;
}