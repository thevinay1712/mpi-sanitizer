#include "mpisan_rt.h"
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <execinfo.h>
#include <unistd.h>

namespace {
    int g_my_rank = -1;
    int g_comm_size = -1;
    bool g_is_initialized = false;
    std::mutex g_rt_mutex;
    MPI_Comm g_sanitizer_comm;

    struct ActiveBuffer {
        const void* ptr;
        size_t size;
        bool is_write;
    };
    std::map<MPI_Request*, ActiveBuffer> g_active_requests;

    std::vector<int> g_collective_history;
    const int COLL_BCAST = 1;
    const int COLL_REDUCE = 2;

    struct CommEvent {
        int type;      // 0 = Send, 1 = Recv
        int peer;      // dest or source
        int tag;
        int count;
        int type_size;
    };
    std::vector<CommEvent> g_comm_history;

    struct GlobalCommEvent {
        int origin_rank;
        int type;      // 0 = Send, 1 = Recv
        int peer;
        int tag;
        int count;
        int type_size;
        bool matched;
    };

    void report_error(const std::string &msg) {
        std::cerr << "\n======================================================\n";
        std::cerr << "[MPISan ERROR] Rank " << g_my_rank << ": " << msg << "\n";
        
        std::cerr << "\n--- Call Stack ---\n";
        void* callstack[128];
        int frames = backtrace(callstack, 128);
        char** strs = backtrace_symbols(callstack, frames);
        if (strs) {
            for (int i = 1; i < frames; ++i) {
                std::cerr << strs[i] << "\n";
            }
            free(strs);
        }
        
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
    
    // Force OpenMPI to return errors instead of immediately aborting.
    // This allows the program to reach __mpisan_finalize for type mismatch reporting.
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    
    g_is_initialized = true;
}

void __mpisan_finalize() {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    
    // 1. Mismatched Collectives Check
    int num_colls = g_collective_history.size();
    int* all_counts = new int[g_comm_size];
    
    MPI_Gather(&num_colls, 1, MPI_INT, all_counts, 1, MPI_INT, 0, g_sanitizer_comm);
    
    if (g_my_rank == 0) {
        for (int i = 1; i < g_comm_size; ++i) {
            if (all_counts[i] != all_counts[0]) {
                delete[] all_counts;
                report_error("Mismatched Collectives! Ranks executed a different number of collective operations.");
            }
        }
    }
    delete[] all_counts;

    // 2. Global Point-to-Point Type & Overflow Matching
    if (g_comm_size > 1) {
        if (g_my_rank > 0) {
            int num_events = g_comm_history.size();
            MPI_Send(&num_events, 1, MPI_INT, 0, 999, g_sanitizer_comm);
            if (num_events > 0) {
                MPI_Send(g_comm_history.data(), num_events * sizeof(CommEvent), MPI_BYTE, 0, 999, g_sanitizer_comm);
            }
        } else {
            std::vector<GlobalCommEvent> all_sends;
            std::vector<GlobalCommEvent> all_recvs;

            for (const auto& ev : g_comm_history) {
                if (ev.type == 0) {
                    all_sends.push_back({0, ev.type, ev.peer, ev.tag, ev.count, ev.type_size, false});
                } else {
                    all_recvs.push_back({0, ev.type, ev.peer, ev.tag, ev.count, ev.type_size, false});
                }
            }

            for (int i = 1; i < g_comm_size; ++i) {
                int num_events = 0;
                MPI_Recv(&num_events, 1, MPI_INT, i, 999, g_sanitizer_comm, MPI_STATUS_IGNORE);
                if (num_events > 0) {
                    std::vector<CommEvent> remote_history(num_events);
                    MPI_Recv(remote_history.data(), num_events * sizeof(CommEvent), MPI_BYTE, i, 999, g_sanitizer_comm, MPI_STATUS_IGNORE);
                    for (const auto& ev : remote_history) {
                        if (ev.type == 0) {
                            all_sends.push_back({i, ev.type, ev.peer, ev.tag, ev.count, ev.type_size, false});
                        } else {
                            all_recvs.push_back({i, ev.type, ev.peer, ev.tag, ev.count, ev.type_size, false});
                        }
                    }
                }
            }

            for (auto& snd : all_sends) {
                auto rcv_it = std::find_if(all_recvs.begin(), all_recvs.end(), [&](const GlobalCommEvent& rcv) {
                    if (rcv.matched) return false;
                    bool rank_matches = (rcv.origin_rank == snd.peer) && 
                                        (rcv.peer == snd.origin_rank || rcv.peer == MPI_ANY_SOURCE);
                    bool tag_matches = (rcv.tag == snd.tag) || (rcv.tag == MPI_ANY_TAG);
                    return rank_matches && tag_matches;
                });

                if (rcv_it != all_recvs.end()) {
                    snd.matched = true;
                    rcv_it->matched = true;

                    if (snd.type_size != rcv_it->type_size) {
                        std::string msg = "Type Mismatch! Rank " + std::to_string(snd.origin_rank) +
                                          " sent base type of " + std::to_string(snd.type_size) + " bytes, but Rank " +
                                          std::to_string(rcv_it->origin_rank) + " expected base type of " +
                                          std::to_string(rcv_it->type_size) + " bytes (Tag: " + std::to_string(snd.tag) + ").";
                        report_error(msg);
                    }

                    int total_send_bytes = snd.count * snd.type_size;
                    int total_recv_bytes = rcv_it->count * rcv_it->type_size;
                    if (total_send_bytes > total_recv_bytes) {
                        std::string msg = "Buffer Overflow! Rank " + std::to_string(snd.origin_rank) +
                                          " sent " + std::to_string(total_send_bytes) + " bytes, but Rank " +
                                          std::to_string(rcv_it->origin_rank) + " only prepared a buffer of " +
                                          std::to_string(total_recv_bytes) + " bytes (Tag: " + std::to_string(snd.tag) + ").";
                        report_error(msg);
                    }
                }
            }
        }
    }

    MPI_Comm_free(&g_sanitizer_comm);
    g_is_initialized = false;
}

void __mpisan_check_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    if (dest == g_my_rank) report_error("Blocking MPI_Send to self (potential deadlock)!");
    if (buf == nullptr && count > 0 && datatype != MPI_DATATYPE_NULL) {
        report_error("Null pointer passed to MPI_Send as send buffer!");
    }
    int type_size; MPI_Type_size(datatype, &type_size);
    g_comm_history.push_back({0, dest, tag, count, type_size});
}

void __mpisan_check_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized) return;
    if (buf == nullptr && count > 0 && datatype != MPI_DATATYPE_NULL) {
        report_error("Null pointer passed to MPI_Recv as receive buffer!");
    }
    int type_size; MPI_Type_size(datatype, &type_size);
    g_comm_history.push_back({1, source, tag, count, type_size});
}

void __mpisan_check_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized || request == nullptr) return;
    if (buf == nullptr && count > 0 && datatype != MPI_DATATYPE_NULL) {
        report_error("Null pointer passed to MPI_Isend as send buffer!");
    }
    int type_size; MPI_Type_size(datatype, &type_size);
    register_async_buffer(request, buf, count * type_size, false);
    g_comm_history.push_back({0, dest, tag, count, type_size});
}

void __mpisan_check_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
    std::lock_guard<std::mutex> lock(g_rt_mutex);
    if (!g_is_initialized || request == nullptr) return;
    if (buf == nullptr && count > 0 && datatype != MPI_DATATYPE_NULL) {
        report_error("Null pointer passed to MPI_Irecv as receive buffer!");
    }
    int type_size; MPI_Type_size(datatype, &type_size);
    register_async_buffer(request, buf, count * type_size, true);
    g_comm_history.push_back({1, source, tag, count, type_size});
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