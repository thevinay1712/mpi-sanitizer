#!/bin/bash
# build.sh - Script to build the MPISan project
echo "========================================"
echo " Building MPISan Compiler & Runtime"
echo "========================================"

mkdir -p build
cd build
cmake ..
make -j4

echo "========================================"
echo " Build Complete!"
echo "========================================"
