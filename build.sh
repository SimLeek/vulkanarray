#!/bin/bash
# need to actually implement this
set -e
set -x

rm -rf build
mkdir build
pushd build

conan install ..
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

bin/md5