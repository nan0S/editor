#!/bin/bash

mkdir -p build
pushd build

clang++ -O0 -fno-omit-frame-pointer -ggdb ../code/build.cpp -o build.exe

if ! [ -e "build.exe" ]; then clang++ -O0 -fno-omit-frame-pointer -ggdb ../code/build.cpp -o build.exe; fi
./build.exe "$@"
if [ -e "build_tmp.exe" ]; then mv build_tmp.exe build.exe; fi

#clang++ -O0 -fno-omit-frame-pointer -ggdb -I../code ../code/glfw/glfw_editor.cpp -oglfw_editor.exe

popd