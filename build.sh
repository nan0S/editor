#!/bin/bash

mkdir -p build
pushd build

if ! [ -e "build.exe" ]; then gcc -O0 ../code/build.cpp -o build.exe; fi
./build.exe "$@"
if [ -e "build_tmp.exe" ]; then mv build_tmp.exe build.exe; fi

popd