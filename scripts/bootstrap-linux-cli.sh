#!/bin/bash

rm -rf cmake-build-debug
rm -rf cmake-build-relwithdebinfo
rm -rf cmake-build-release

CXX=clang++ CC=clang cmake \
  -Bcmake-build-debug -S. \
  -DPBRLAB_USE_CCACHE=On \
  -DPBRLAB_BUILD_GUI=Off \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DPBRLAB_WITH_EMBREE=On 

mv cmake-build-debug/compile_commands.json .

CXX=clang++ CC=clang cmake \
  -Bcmake-build-relwithdebinfo -S. \
  -DPBRLAB_USE_CCACHE=On \
  -DPBRLAB_BUILD_GUI=Off \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DPBRLAB_WITH_EMBREE=On 

CXX=clang++ CC=clang cmake \
  -Bcmake-build-release -S. \
  -DPBRLAB_USE_CCACHE=On \
  -DPBRLAB_BUILD_GUI=Off \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DPBRLAB_WITH_EMBREE=On 
