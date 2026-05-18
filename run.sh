#!/bin/bash
# run.sh - Script to execute evaluation test cases
echo "========================================"
echo " Running MPISan Evaluation Suite"
echo "========================================"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo "Compiling test cases..."
$DIR/mpisan-cxx.sh tests/test_01_clean.cpp -o test_01
$DIR/mpisan-cxx.sh tests/test_03_send_to_self.cpp -o test_03
$DIR/mpisan-cxx.sh tests/test_05_buffer_aliasing.cpp -o test_05

echo ""
echo ">>> RUNNING WORKING CASE: Clean MPI Code (test_01)"
mpirun -np 2 ./test_01
echo "Result: Success (No bugs detected)"

echo ""
echo ">>> RUNNING FAILURE CASE 1: Deadlock Bug (test_03)"
mpirun -np 2 ./test_03

echo ""
echo ">>> RUNNING FAILURE CASE 2: Buffer Aliasing Bug (test_05)"
mpirun -np 2 ./test_05

echo ""
echo ">>> RUNNING LULESH EXASCALE PROXY APP"
$DIR/evaluate_lulesh.sh
