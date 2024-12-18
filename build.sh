#!/bin/bash

mkdir -p build
pushd build

g++ -I../code ../code/linux/linux_editor.cpp

popd
