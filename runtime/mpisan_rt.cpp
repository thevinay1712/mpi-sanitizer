#include "mpisan_rt.h"
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <string>
#include <map>
#include <vector>

namespace {
    int g_my_rank = -1;
    int g_comm_size = -1;
    bool g_is_initialized = false;
    std::mutex g_rt_mutex; // Thread safety
    MPI_Comm g_sanitizer_comm;

    // Buffer tracking for async calls
    struct ActiveBuffer {
        const void* ptr;
        size_t size;
        bool is_write;
    };
    // Map from Request to Buffer Info
    std::map<MPI_Request*, ActiveBuffer> g_active_requests;

    // Collective history tracking
    std::vector<int> g_collective_history;
    const int COLL_BCAST = 1;
    const int COLL_REDUCE = 2;

    void report_error(const std::string &msg) {
        std::cerr << "\n======================================================\n";
        std::cerr << "[MPISan ERROR] Rank " << g_my_rank << ": " << msg << "\n";
        std::cerr << "======================================================\n" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    bool check_overlap(const void* ptr1, size_t size1, const void* ptr2, size_t size2) {
        size_t start1 = (size_t)ptr1;
        size_t end1 = start1 + size1;
        size_t start2 = (size_t)ptr2;
        size_t end2 = start2 + size2;
        return (start1 < end2 && start2 < end1);
    }

    void register_async_buffer(MPI_Request* req, const void* buf, size_t size, bool is_write) {
        for (const auto& pair : g_active_requests) {
            if (check_overlap(pair.second.ptr, pair.second.size, buf, size)) {
                // In MPI, overlapping reads (two Isends) are perfectly safe!
                // Overlap is only illegal if at least one operation is writing (Irecv).
                if (is_write || pair.second.is_write) {
                    report_error("Buffer Aliasing! Overlap detected in asynchronous MPI operations.");
                }
            }
        }
        g_active_requests[req] = {buf, size, is_write};
    }
}

extern "C" {

void __mpisan_init(int *argc, char ***argv) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_comm_size);
    MPI_Comm_dup(MPI_COMM_WORLD, &g_sanitizer_comm);
    g_is_initialized = true;
}

void __mpisan_finalize() {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    
    // Collective Verification: Gather total collective calls from all ranks
    int num_colls = g_collective_history.size();
    int* all_counts = new int[g_comm_size];
    
    MPI_Gather(&num_colls, 1, MPI_INT, all_counts, 1, MPI_INT, 0, g_sanitizer_comm);
    
    if (g_my_rank == 0) {
        for (int i = 1; i < g_comm_size; ++i) {
            if (all_counts[i] != all_counts[0]) {
                report_error("Mismatched Collectives! Ranks executed a different number of collective operations.");
            }
        }
    }
    delete[] all_counts;

    MPI_Comm_free(&g_sanitizer_comm);
    g_is_initialized = false;
}

void __mpisan_check_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    if (dest == g_my_rank) report_error("Blocking MPI_Send to self (potential deadlock)!");
    
    // Shadow size checks removed to prevent deadlocks when Isend is matched with Recv
}

void __mpisan_check_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    
    // Shadow size checks removed to prevent deadlocks when Isend is matched with Recv
}

void __mpisan_check_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized || request == nullptr) return;
    int type_size; MPI_Type_size(datatype, &type_size);
    register_async_buffer(request, buf, count * type_size, false);
}

void __mpisan_check_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized || request == nullptr) return;
    int type_size; MPI_Type_size(datatype, &type_size);
    register_async_buffer(request, buf, count * type_size, true);
}

void __mpisan_check_wait(MPI_Request *request, MPI_Status *status) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized || request == nullptr) return;
    g_active_requests.erase(request);
}

void __mpisan_check_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    g_collective_history.push_back(COLL_BCAST);
}

void __mpisan_check_reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    g_collective_history.push_back(COLL_REDUCE);
}

}
