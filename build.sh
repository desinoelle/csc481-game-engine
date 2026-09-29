#!/bin/bash
cmake --build build
cd build
./coolEngine 1
./coolEngine 2
cd ..
