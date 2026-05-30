#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 32; // Grid Size
    int local_rows = N / size;
    
    // Grid buffers including ghost cells
    std::vector<std::vector<double>> grid(local_rows + 2, std::vector<double>(N, 0.0));
    std::vector<std::vector<double>> next_grid(local_rows + 2, std::vector<double>(N, 0.0));

    // Initialize boundary conditions
    if (rank == 0) {
        for (int j = 0; j < N; ++j) grid[1][j] = 100.0; // Top boundary
    }
    if (rank == size - 1) {
        for (int j = 0; j < N; ++j) grid[local_rows][j] = 50.0; // Bottom boundary
    }

    double max_diff = 1.0;
    int iter = 0;
    const int MAX_ITER = 100;

    while (max_diff > 1e-4 && iter < MAX_ITER) {
        // Exchange ghost rows
        MPI_Request reqs[4];
        int req_count = 0;

        // Send down to rank+1, receive from rank+1
        if (rank < size - 1) {
            MPI_Isend(&grid[local_rows][0], N, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);
            MPI_Irecv(&grid[local_rows + 1][0], N, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
        }
        // Send up to rank-1, receive from rank-1
        if (rank > 0) {
            MPI_Isend(&grid[1][0], N, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
            MPI_Irecv(&grid[0][0], N, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);
        }

        if (req_count > 0) {
            MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
        }

        // Compute local updates
        double local_max_diff = 0.0;
        for (int i = 1; i <= local_rows; ++i) {
            // Skip boundaries
            if (rank == 0 && i == 1) continue;
            if (rank == size - 1 && i == local_rows) continue;

            for (int j = 1; j < N - 1; ++j) {
                next_grid[i][j] = 0.25 * (grid[i - 1][j] + grid[i + 1][j] + grid[i][j - 1] + grid[i][j + 1]);
                local_max_diff = std::max(local_max_diff, std::abs(next_grid[i][j] - grid[i][j]));
            }
        }

        // Copy back to main grid
        for (int i = 1; i <= local_rows; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                grid[i][j] = next_grid[i][j];
            }
        }

        // Allreduce to find global error delta
        MPI_Allreduce(&local_max_diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        iter++;
    }

    if (rank == 0) {
        std::cout << "Jacobi Solver completed in " << iter << " iterations." << std::endl;
    }

    MPI_Finalize();
    return 0;
}
