# MPISan: Compiler-Integrated MPI Usage Sanitizer

MPISan is a powerful, LLVM-based sanitizer for MPI (Message Passing Interface) applications. Unlike traditional external MPI debugging tools (like MUST) which intercept calls purely at runtime, MPISan integrates directly into the LLVM compilation pipeline to instrument MPI calls at the Intermediate Representation (IR) level.

This project intercepts communication patterns, tracks buffer types, and flags usage errors dynamically at runtime with minimal overhead.

## Features
- **Buffer Aliasing Detection:** Identifies overlapping asynchronous memory access (`MPI_Isend`, `MPI_Irecv`) to prevent silent memory corruption.
- **Collective Validation:** Dynamically verifies that all participating MPI ranks execute the correct matching collective operations (e.g., `MPI_Bcast`, `MPI_Reduce`).
- **Deadlock Prevention:** Catches common deadlock patterns (like blocking `MPI_Send` to oneself).
- **Buffer Overflow Protection:** Tracks underlying `MPI_Datatype` sizes and payload limits.
- **Exascale Ready:** Tested against complex proxy applications like LLNL's LULESH.

## Project Structure
* `docs/` - Detailed documentation on Design, Implementation, and Evaluation metrics.
* `pass/` - The LLVM Transformation Pass (`MPISanPass.cpp`) that injects metadata-capturing hooks.
* `runtime/` - A thread-safe C++ runtime library (`mpisan_rt.cpp`) utilizing shadow communicators.
* `tests/` - A Python-based test generator and 15+ automated MPI programs with seeded bugs.
* `build.sh` - Automated build script for the compiler pass and runtime.
* `run.sh` - Automated script to evaluate the sanitizer against clean code, seeded bugs, and the LULESH Exascale proxy app.

---

## 🚀 Getting Started

### 1. Install Dependencies (Ubuntu/Debian/WSL)
To build and run this project, you need the LLVM toolchain, CMake, and an MPI implementation. Run the following command to install everything required:
```bash
sudo apt-get update
sudo apt-get install -y clang llvm-dev libomp-dev cmake make openmpi-bin libopenmpi-dev git
```

### 2. Clone the Repository
```bash
git clone https://github.com/thevinay1712/mpi-sanitizer.git
cd mpi-sanitizer
```

### 3. Build the Sanitizer
Make the scripts executable and run the automated build script. This will compile the LLVM Pass and the Runtime Library natively.
```bash
chmod +x build.sh run.sh evaluate_lulesh.sh mpisan-cxx.sh
./build.sh
```

### 4. Run the Evaluation Suite
To see the sanitizer in action, run the master evaluation script. This will automatically demonstrate the sanitizer catching memory bugs, deadlocks, and successfully validating the complex LULESH Exascale application.
```bash
./run.sh
```

---

## Compiling Your Own Custom MPI Code
If you want to use MPISan on your own custom MPI applications, use the provided compiler wrapper instead of standard `mpicxx`:

```bash
# Compile your code using our wrapper
./mpisan-cxx.sh my_custom_app.cpp -o my_custom_app

# Run it exactly like you normally would. If a bug is detected, MPISan will automatically abort and print the exact error!
mpirun -np 4 ./my_custom_app
```
