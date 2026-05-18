# MPISan: Compiler-Integrated MPI Usage Sanitizer

MPISan is a powerful, LLVM-based sanitizer for MPI (Message Passing Interface) applications. Unlike traditional external MPI debugging tools (like MUST), MPISan integrates directly into the LLVM compilation pipeline to instrument MPI calls at the Intermediate Representation (IR) level.

This project was built to intercept communication patterns, track buffer types, and flag usage errors dynamically at runtime with minimal overhead.

## Features
- **Buffer Aliasing Detection:** Identifies overlapping asynchronous memory access (`MPI_Isend`, `MPI_Irecv`) to prevent silent memory corruption.
- **Collective Validation:** Dynamically verifies that all participating MPI ranks execute the correct matching collective operations (e.g., `MPI_Bcast`, `MPI_Reduce`).
- **Deadlock Prevention:** Catches common deadlock patterns (like blocking `MPI_Send` to oneself).
- **Buffer Overflow Protection:** Tracks underlying `MPI_Datatype` sizes and payload limits.
- **Exascale Ready:** Tested against complex proxy applications like LLNL's LULESH.

## Project Structure
* `pass/` - The LLVM Transformation Pass (`MPISanPass.cpp`) that injects metadata-capturing hooks into the application's IR.
* `runtime/` - A thread-safe C++ runtime library (`mpisan_rt.cpp`) utilizing shadow communicators to orchestrate validation across nodes.
* `tests/` - A Python-based test generator and 15+ automated MPI programs with seeded bugs.
* `mpisan-cxx.sh` - The compiler wrapper that automatically loads the pass and links the runtime library.
* `evaluate_lulesh.sh` - An automated script to deploy and evaluate the sanitizer against the LULESH proxy app.

## Building and Running

### Prerequisites
* LLVM/Clang (13.0+)
* CMake (3.10+)
* MPI Implementation (OpenMPI or MPICH)

### Build the Sanitizer
```bash
mkdir build
cd build
cmake ..
make
```

### Compile an Application
Use the provided compiler wrapper to build your MPI programs:
```bash
./mpisan-cxx.sh my_mpi_app.cpp -o my_mpi_app
```

### Run the Application
Run it exactly like you normally would. If a bug is detected, the sanitizer will automatically abort and print a detailed stack error!
```bash
mpirun -np 4 ./my_mpi_app
```
