# MPISan Design Document

## 1. Approach
MPISan adopts a **Compiler-Integrated dynamic analysis** approach. Unlike traditional MPI debugging tools (such as MUST or STAT) which rely on intercepting MPI calls purely at runtime using the PMPI interface, MPISan embeds directly into the LLVM compilation pipeline. 

By running as an LLVM IR Pass, the sanitizer:
1. Identifies MPI call sites during compilation.
2. Natively injects metadata-capturing hooks directly into the machine code.
3. Forwards this metadata to a specialized runtime library (`mpisan_rt`).

## 2. Alternatives Considered
### A. PMPI Wrapping (The MUST Approach)
* **How it works:** Uses the MPI standard profiling interface (`PMPI_`) to wrap every MPI call at link time.
* **Why it was rejected:** PMPI wrappers are "black boxes" to the compiler. They prevent aggressive compiler optimizations (like inlining) and cannot easily access local variable bounds or type information available during compilation.

### B. Static Analysis
* **How it works:** Analyzes the abstract syntax tree to find bugs without running the code.
* **Why it was rejected:** MPI code is highly dynamic. Ranks, buffer sizes, and network topologies are calculated at runtime, making static analysis prone to high false positive rates and unable to catch input-dependent deadlocks.

## 3. Architecture
The architecture consists of the **Instrumenter** (LLVM Pass) and the **Validator** (Runtime).
The Validator uses a **Shadow Communicator** (a duplicate of `MPI_COMM_WORLD`) to transmit hidden metadata (like buffer sizes and types) between nodes without interfering with the user's application network traffic.
