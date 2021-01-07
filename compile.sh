#!/bin/bash
clear
mkdir build/
cd build/
cmake ..
make clean
make -j 8
cd ..
rm -rf build/
