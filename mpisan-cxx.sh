#!/bin/bash
# Wrapper script for compiling user applications with MPISan
# Usage: ./mpisan-cxx.sh my_app.cpp -o my_app

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Tell the MPI wrapper to use clang++ underneath so our LLVM pass works,
# while automatically linking the correct mpi.h and MPI libraries!
export OMPI_CXX=clang++
export MPICH_CXX=clang++
COMPILER=mpicxx

PASS_SO="$DIR/build/pass/libMPISanPass.so"
RT_LIB="$DIR/build/runtime"

if [ ! -f "$PASS_SO" ]; then
    PASS_SO="$DIR/build/pass/MPISanPass.so"
fi
if [ ! -f "$PASS_SO" ]; then
    PASS_SO="$DIR/build/pass/libMPISanPass.dylib"
fi

if [ ! -f "$PASS_SO" ]; then
    echo "Error: MPISanPass not found. Have you run CMake and built the project?"
    exit 1
fi

# We add -Wl,-rpath,$RT_LIB so the Linux loader knows exactly where to find our runtime library!
$COMPILER -fpass-plugin=$PASS_SO -L$RT_LIB -Wl,-rpath,$RT_LIB -lmpisan_rt "$@"
