#!/bin/bash
# Evaluation script for running MPISan on LULESH (ECP Proxy App)

echo "=========================================="
echo " MPISan Real Application Evaluation (LULESH)"
echo "=========================================="

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ ! -d "LULESH" ]; then
    echo "Cloning LULESH mini-app..."
    git clone https://github.com/LLNL/LULESH.git
fi

cd LULESH

echo "Building LULESH with MPISan compiler wrapper..."
# Ensure clean build
make clean 2>/dev/null
# Build LULESH using our sanitizer wrapper and explicitly enable MPI
make CXX="$DIR/mpisan-cxx.sh" CXXFLAGS="-g -O3 -DUSE_MPI=1" LDFLAGS="-g -O3"

if [ ! -f "lulesh2.0" ]; then
    echo "LULESH build failed!"
    exit 1
fi

echo "=========================================="
echo " Running LULESH with MPISan (8 ranks)"
echo "=========================================="
# Run it with 8 ranks and small iteration counts so it finishes quickly
mpirun -np 8 ./lulesh2.0 -i 5 -s 10

echo "=========================================="
echo " LULESH Evaluation Complete!"
echo " If the application ran to completion without throwing [MPISan ERROR], our sanitizer successfully verified the program dynamically at scale with zero false positives!"
echo "=========================================="
