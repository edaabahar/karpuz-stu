#!/bin/bash -xe

mkdir -p build
cd build
cmake .. \
    -DFL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a \
    -DFL_INCLUDE_DIR=/opt/homebrew/opt/flex/include \
    -DBISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison
