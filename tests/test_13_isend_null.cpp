#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Request req;
        // MPI_Isend with null buffer isn't explicitly checked in our runtime yet,
        // but we could extend __mpisan_check_isend to catch it. Let's seed it.
        MPI_Isend(nullptr, 10, MPI_INT, 1, 0, MPI_COMM_WORLD, &req); 
    }
    MPI_Finalize();
    return 0;
}