#!/bin/sh

set -e

build="$(dirname "$0")/build"
mkdir -p "$build"
save_dir=$(pwd) # /bin/sh doesn't have pushd/popd and I don't want to use /bin/bash
cd "$build"

if ! [ -e "build.exe" ]; then gcc -O0 ../code/build.cpp -o build.exe; fi
./build.exe "$@"
if [ -e "build_tmp.exe" ]; then mv build_tmp.exe build.exe; fi

cd "$save_dir"