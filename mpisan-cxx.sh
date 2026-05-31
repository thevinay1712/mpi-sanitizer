#!/bin/bash
# Wrapper script for compiling user applications with MPISan
# Usage: ./mpisan-cxx.sh my_app.cpp -o my_app

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

export OMPI_CXX=clang++
export MPICH_CXX=clang++
COMPILER=mpicxx

PASS_SO="$DIR/build/src/pass/libMPISanPass.so"
RT_LIB="$DIR/build/src/runtime"

if [ ! -f "$PASS_SO" ]; then
    PASS_SO="$DIR/build/src/pass/MPISanPass.so"
fi
if [ ! -f "$PASS_SO" ]; then
    PASS_SO="$DIR/build/src/pass/libMPISanPass.dylib"
fi

if [ ! -f "$PASS_SO" ]; then
    echo "Error: MPISanPass not found. Have you run CMake and built the project?"
    exit 1
fi

# Check if this is a compile-only step
COMPILE_ONLY=0
for arg in "$@"; do
    if [ "$arg" == "-c" ] || [ "$arg" == "-E" ] || [ "$arg" == "-S" ]; then
        COMPILE_ONLY=1
        break
    fi
done

if [ $COMPILE_ONLY -eq 1 ]; then
    # Do not pass linker flags during compile-only steps
    $COMPILER -fpass-plugin=$PASS_SO "$@"
else
    # Pass both the plugin and the runtime linking flags
    $COMPILER -fpass-plugin=$PASS_SO -L$RT_LIB -Wl,-rpath,$RT_LIB -lmpisan_rt "$@"
fi