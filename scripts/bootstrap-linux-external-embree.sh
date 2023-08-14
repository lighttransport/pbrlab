#!/bin/bash

target_dirname="cmake-build-external-embree"

rm -rf ${target_dirname}

CXX=g++ CC=gcc cmake \
  -B${target_dirname} -S. \
  -DPBRLAB_USE_CCACHE=On \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DPBRLAB_USE_EXTERNAL_EMBREE=On \
  -Dembree_DIR=external/embree-3.13.5.x86_64.linux/lib/cmake/embree-3.13.5


