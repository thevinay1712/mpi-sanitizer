# MPISan: Compiler-Integrated MPI Usage Sanitizer

[![LLVM 18](https://img.shields.io/badge/LLVM-18.1.3-blue.svg)](https://llvm.org/)
[![MPI](https://img.shields.io/badge/MPI-OpenMPI--4.1-green.svg)](https://www.open-mpi.org/)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

MPISan is a powerful, state-of-the-art **compiler-integrated sanitizer** for MPI (Message Passing Interface) applications. By embedding directly inside the **LLVM/Clang compiler pipeline**, MPISan rewrites code at the Intermediate Representation (IR) level to inject metadata-capturing hooks. 

At runtime, our thread-safe validation engine intercept these hooks to dynamically detect memory overlaps, circular deadlocks, null-pointer passing, and mismatched collective or point-to-point type signatures.

---

## 📬 The Big Picture (A Layman's Analogy)

Imagine **MPI (Message Passing Interface)** is a global postal network. Thousands of letters are moving between different post offices (ranks or computers) at lightning speed. 
* **The Problem:** Programmers make mistakes. They send letters to themselves and wait forever (deadlocks), send large parcels into tiny mailboxes (buffer overflows), or write on the same letter at the exact same time (buffer aliasing). Because supercomputers are complex, these bugs rarely crash explicitly—they just silently corrupt the results.
* **The MPISan Solution:** We build an **invisible inspector** directly into the post offices during the construction phase (compilation). This inspector installs GPS trackers on every letter. If anyone tries to call their own number, overflow a mailbox, or mismatch types, MPISan immediately halts the operation, sounds an alarm, and points directly to the line of code that caused the error.

---

## 🚀 Key Features

* **Buffer Aliasing Detection:** Automatically catches overlapping read-write or write-write asynchronous operations (`MPI_Isend`, `MPI_Irecv`) to prevent silent data corruption.
* **Type & Size Mismatch Matching Engine:** Uses a deadlock-free FIFO matching engine at finalization to verify that base datatypes match and payloads do not overflow receiver buffers.
* **Immediate Deadlock Prevention:** Flags circular blocking calls (like sending to self) before execution stalls.
* **Collective Validation:** Automatically checks that all participating MPI ranks execute the correct sequence of matching collectives (`MPI_Bcast`, `MPI_Reduce`).
* **Null Pointer Protection:** Instantly stops execution if null-pointers are supplied to active send/receive calls.
* **Exascale Scalability:** Extensively validated against **LLNL's LULESH** and a custom **MPI 2D Jacobi Solver**.

---

## 📂 Project Reorganized Layout

We have structured the repository in strict accordance with the submission guidelines:

```
mpi-sanitizer/
├── src/                      # Source Code
│   ├── pass/                 # LLVM Instrumentation Pass (MPISanPass.cpp)
│   └── runtime/              # Thread-Safe Validation C++ Runtime (mpisan_rt.cpp)
├── testcases/                # Automated 15+ Testcases Suite
│   ├── generate_tests.py     # Python automated testcase generator
│   └── test_*.cpp            # Clean & buggy test scripts
├── Jacobi_MPI/               # Extra Evaluation Application
│   ├── jacobi_clean.cpp      # Perfectly safe 2D Jacobi iteration
│   ├── jacobi_buggy_aliasing.cpp # Jacobi Solver with seeded aliasing bug
│   ├── jacobi_buggy_mismatch.cpp # Jacobi Solver with seeded collectives mismatch
│   └── evaluate_jacobi.sh    # Evaluation script for Jacobi solver
├── build.sh                  # Root-level compiler & runtime build script
├── run.sh                    # Master evaluation runner script
├── mpisan-cxx.sh             # Compiler wrapper (replacement for mpicxx)
├── evaluate_lulesh.sh        # Runner for LULESH proxy application
├── README.md                 # Project introduction
├── DESIGN.md                 # Conceptual design & architecture
├── IMPLEMENTATION.md         # LLVM IR pass & runtime details
└── EVALUATION.md             # Baseline benchmarks & evaluation metrics
```

---

## 🛠️ Getting Started (WSL / Ubuntu / Debian)

To build and run this project, you need the LLVM-18 toolchain, CMake, and an MPI implementation.

### 1. Install Dependencies
```bash
sudo apt-get update
sudo apt-get install -y clang-18 llvm-18-dev libomp-dev cmake make openmpi-bin libopenmpi-dev git python3
```

### 2. Build the Sanitizer
Run our root-level build script. This will compile both the LLVM IR Pass (`MPISanPass.so`) and the Runtime Library (`libmpisan_rt.so`) natively inside `build/src/`:
```bash
chmod +x build.sh run.sh mpisan-cxx.sh evaluate_lulesh.sh Jacobi_MPI/evaluate_jacobi.sh
./build.sh
```

### 3. Run the Master Evaluation Suite
To execute the sanitizer against our main test suite (proving clean runs, deadlock detection, buffer aliasing, type mismatches, and the LULESH proxy app):
```bash
./run.sh
```

### 4. Run the Jacobi Solver Workload (Extra Mentor Deliverable)
To run the evaluation on our custom 2D Jacobi Solver:
```bash
./Jacobi_MPI/evaluate_jacobi.sh
```

---

## 💻 Compiling Your Own Custom MPI Code

Compiling your custom application with MPISan is extremely transparent. Simply swap `mpicxx` for our wrapper script `mpisan-cxx.sh`:

```bash
# 1. Compile your custom code with our wrapper
./mpisan-cxx.sh my_application.cpp -o my_application

# 2. Run the binary normally using mpirun
mpirun -np 4 ./my_application
```

If a bug exists, MPISan will automatically abort execution and print the exact rank, bug class, and details!
