@echo off

IF NOT EXIST build mkdir build
pushd build

IF NOT EXIST build.exe cl -nologo -FC -Od ..\code\build.cpp -link -out:build.exe
build.exe %*
IF EXIST build_tmp.exe move build_tmp.exe build.exe

popd