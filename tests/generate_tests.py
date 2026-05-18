import os

tests = {
    "test_01_clean.cpp": """
#include <mpi.h>
#include <iostream>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data = 42;
    if (rank == 0) MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    else if (rank == 1) MPI_Recv(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Finalize();
    return 0;
}""",
    "test_02_null_buffer.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) MPI_Send(nullptr, 10, MPI_INT, 1, 0, MPI_COMM_WORLD); // ERROR
    MPI_Finalize();
    return 0;
}""",
    "test_03_send_to_self.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data = 1;
    MPI_Send(&data, 1, MPI_INT, rank, 0, MPI_COMM_WORLD); // ERROR
    MPI_Finalize();
    return 0;
}""",
    "test_04_type_mismatch.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        long long data = 42;
        MPI_Send(&data, 1, MPI_LONG_LONG, 1, 0, MPI_COMM_WORLD);
    } else if (rank == 1) {
        int data;
        MPI_Recv(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // ERROR
    }
    MPI_Finalize();
    return 0;
}""",
    "test_05_buffer_aliasing.cpp": """
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
}""",
    "test_06_mismatched_collectives.cpp": """
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
}""",
    "test_07_clean_collectives.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int data = 0;
    MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}""",
    "test_08_async_write_aliasing.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data[10];
    if (rank == 0) {
        MPI_Request req, req2;
        MPI_Irecv(data, 10, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Irecv(data, 10, MPI_INT, 1, 1, MPI_COMM_WORLD, &req2); // ERROR: writing to same buf
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MPI_Wait(&req2, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Send(data, 10, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(data, 10, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}""",
    "test_09_deadlock_circular.cpp": """
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
}""",
    "test_10_recv_null.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        int data = 1;
        MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else if (rank == 1) {
        MPI_Recv(nullptr, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // ERROR
    }
    MPI_Finalize();
    return 0;
}""",
    "test_11_clean_async.cpp": """
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
}""",
    "test_12_type_mismatch_recv_large.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        int data[2];
        MPI_Send(data, 2, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else if (rank == 1) {
        int data;
        MPI_Recv(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Overflow
    }
    MPI_Finalize();
    return 0;
}""",
    "test_13_isend_null.cpp": """
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
}""",
    "test_14_irecv_null.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Request req;
        MPI_Irecv(nullptr, 10, MPI_INT, 1, 0, MPI_COMM_WORLD, &req); 
    }
    MPI_Finalize();
    return 0;
}""",
    "test_15_clean_reduce.cpp": """
#include <mpi.h>
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int send = rank, recv;
    MPI_Reduce(&send, &recv, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}"""
}

# The script runs from the test dir or root
out_dir = os.path.dirname(os.path.abspath(__file__))
for name, content in tests.items():
    with open(os.path.join(out_dir, name), 'w') as f:
        f.write(content.strip())
print(f"Created 15 test files in {out_dir}")
