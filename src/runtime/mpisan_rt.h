#ifndef MPISAN_RT_H
#define MPISAN_RT_H

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialization and finalization
void __mpisan_init(int *argc, char ***argv);
void __mpisan_finalize();

// P2P Blocking
void __mpisan_check_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
void __mpisan_check_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm);

// P2P Non-Blocking (Injected AFTER the call)
void __mpisan_check_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void __mpisan_check_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
void __mpisan_check_wait(MPI_Request *request, MPI_Status *status);

// Collectives
void __mpisan_check_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
void __mpisan_check_reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif // MPISAN_RT_H
