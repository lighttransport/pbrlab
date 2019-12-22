#!/bin/bash


rm -rf build
mkdir build
cd build

CXX=clang++ CC=clang cmake \
  -DPBRLAB_USE_CCACHE=On \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DPBRLAB_WITH_EMBREE=On \
  ..

mv compile_commands.json ..
cd ..
