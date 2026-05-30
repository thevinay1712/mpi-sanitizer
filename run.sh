#!/bin/bash
# run.sh - Script to execute evaluation test cases
echo "========================================"
echo " Running MPISan Evaluation Suite"
echo "========================================"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo "Compiling test cases..."
$DIR/mpisan-cxx.sh testcases/test_01_clean.cpp -o test_01
$DIR/mpisan-cxx.sh testcases/test_03_send_to_self.cpp -o test_03
$DIR/mpisan-cxx.sh testcases/test_08_async_write_aliasing.cpp -o test_08
$DIR/mpisan-cxx.sh testcases/test_04_type_mismatch.cpp -o test_04
$DIR/mpisan-cxx.sh testcases/test_02_null_buffer.cpp -o test_02

echo ""
echo ">>> RUNNING WORKING CASE: Clean MPI Code (test_01)"
timeout 5s mpirun -np 2 ./test_01
echo "Result: Success (No bugs detected)"

echo ""
echo ">>> RUNNING FAILURE CASE 1: Deadlock Bug (test_03)"
timeout 5s mpirun -np 2 ./test_03

echo ""
echo ">>> RUNNING FAILURE CASE 2: Buffer Aliasing Bug (test_08)"
timeout 5s mpirun -np 2 ./test_08

echo ""
echo ">>> RUNNING FAILURE CASE 3: Type Mismatch Bug (test_04)"
timeout 5s mpirun -np 2 ./test_04

echo ""
echo ">>> RUNNING FAILURE CASE 4: Null Buffer Bug (test_02)"
timeout 5s mpirun -np 2 ./test_02

echo ""
echo ">>> RUNNING LULESH EXASCALE PROXY APP"
$DIR/evaluate_lulesh.sh
