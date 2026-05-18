# MPISan Implementation Details

## 1. LLVM Instrumentation Pass
The LLVM Pass (`MPISanPass.cpp`) operates at the Intermediate Representation (IR) level.
* **Pass Registration:** It uses `registerPipelineStartEPCallback` to automatically inject itself into Clang's optimization pipeline (`-O0` to `-O3`).
* **IR Traversal:** It iterates over every `Instruction` in every `BasicBlock` of every `Function` in the `Module`.
* **CallSite Detection:** It checks if an instruction is a `CallInst`. If the called function's name matches an MPI primitive (e.g., `MPI_Isend`, `MPI_Bcast`), it prepares an injection.
* **IRBuilder Injection:** It uses `IRBuilder<>` to insert calls to our runtime API (`__mpisan_check_...`) immediately before the actual MPI call. Arguments from the original call (buffers, sizes, ranks) are captured and forwarded to the runtime hook.

## 2. Runtime Validation Logic
The runtime library (`mpisan_rt.cpp`) validates the intercepted arguments using the following logic:

### A. Buffer Aliasing
Maintains a `std::map` of active asynchronous requests (`MPI_Request`). When a new asynchronous operation starts, it calculates the memory range `[ptr, ptr + size]`. It checks this range against all active ranges in the map. If an overlap is detected and at least one operation is a write, it forces an `MPI_Abort`.

### B. Deadlock Prevention
Checks the destination rank in blocking calls (e.g., `MPI_Send`). If `dest == g_my_rank`, it immediately throws an error to prevent self-deadlock.

### C. Collective Synchronization
Maintains a local `std::vector<int>` history of every collective operation executed by the rank. During `MPI_Finalize`, rank 0 gathers the total count of collectives from all ranks via the shadow communicator. If there is a mismatch, it signifies a collective order violation.
