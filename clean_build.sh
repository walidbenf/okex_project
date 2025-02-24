#!/bin/bash
rm -rf build
mkdir build
cd build

# Configurer avec CMAKE_BUILD_TYPE=Debug
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Compiler
make
