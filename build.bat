@echo off

IF NOT EXIST build mkdir build
pushd build

IF NOT EXIST build.exe cl /Od /nologo ..\code\build.cpp /link /out:build.exe
build.exe
IF EXIST build.tmp.exe move build.tmp.exe build.exe

popd