@echo off

IF NOT EXIST build mkdir build
pushd build

cl -nologo -FC -Od -Z7 ..\code\build.cpp -link -out:build.exe
IF NOT EXIST build.exe cl -nologo -FC -Od ..\code\build.cpp -link -out:build.exe
build.exe %* verbose dev debug
IF EXIST build_tmp.exe move build_tmp.exe build.exe

copy glfw_editor_release.exe Apollo.exe

popd