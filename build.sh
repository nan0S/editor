#!/bin/bash

mkdir -p build
pushd build

clang++ -O0 -fno-omit-frame-pointer -ggdb ../code/build.cpp -o build.exe
# clang++ -O0 -fno-omit-frame-pointer -ggdb -I../code ../code/linux/linux_editor.cpp -o linux_editor.exe

popd
