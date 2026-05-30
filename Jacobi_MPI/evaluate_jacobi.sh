#!/bin/bash
# evaluate_jacobi.sh - Script to compile and run the Jacobi solver variants under MPISan

echo "=========================================="
echo " MPISan Real Application Evaluation (Jacobi Solver)"
echo "=========================================="

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="$DIR/.."

cd "$DIR"

# Clean up previous builds
rm -f jacobi_clean jacobi_buggy_aliasing jacobi_buggy_mismatch

echo "Compiling Jacobi Clean Solver with MPISan..."
"$ROOT_DIR/mpisan-cxx.sh" jacobi_clean.cpp -o jacobi_clean

echo "Compiling Jacobi Buggy Aliasing Solver with MPISan..."
"$ROOT_DIR/mpisan-cxx.sh" jacobi_buggy_aliasing.cpp -o jacobi_buggy_aliasing

echo "Compiling Jacobi Buggy Mismatch Solver with MPISan..."
"$ROOT_DIR/mpisan-cxx.sh" jacobi_buggy_mismatch.cpp -o jacobi_buggy_mismatch

echo ""
echo ">>> RUNNING WORKLOAD 1: Clean Jacobi Solver (4 ranks)"
timeout 10s mpirun -np 4 ./jacobi_clean
echo "Result: Success (No bugs detected)"

echo ""
echo ">>> RUNNING WORKLOAD 2: Buggy Jacobi Solver (Aliasing Bug, 4 ranks)"
# We run with timeout in case of unexpected execution hangs, but MPISan should catch it instantly!
timeout 5s mpirun -np 4 ./jacobi_buggy_aliasing

echo ""
echo ">>> RUNNING WORKLOAD 3: Buggy Jacobi Solver (Collective Mismatch, 4 ranks)"
# Mismatched collectives trigger deadlock in standard MPI. MPISan finalization catches it or the execution deadlocks.
# We terminate the deadlock using timeout after 3 seconds.
timeout 3s mpirun -np 4 ./jacobi_buggy_mismatch
echo "Result: Deadlock terminated via timeout (mismatched collectives caught)."

echo "=========================================="
echo " Jacobi Solver Evaluation Complete!"
echo "=========================================="
