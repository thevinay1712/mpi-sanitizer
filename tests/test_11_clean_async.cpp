#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data1[10], data2[10];
    if (rank == 0) {
        MPI_Request req1, req2;
        MPI_Isend(data1, 10, MPI_INT, 1, 0, MPI_COMM_WORLD, &req1);
        MPI_Isend(data2, 10, MPI_INT, 1, 1, MPI_COMM_WORLD, &req2); // Clean, diff bufs
        MPI_Wait(&req1, MPI_STATUS_IGNORE);
        MPI_Wait(&req2, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Recv(data1, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(data2, 10, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Finalize();
    return 0;
}