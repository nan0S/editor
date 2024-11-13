@echo off

IF NOT EXIST build mkdir build
pushd build

REM cl /nologo ..\code\build.cpp /link /out:build.exe
REM build.exe %*

REM cl /Z7 /Od /nologo ..\code\build.cpp /link /out:build.exe

IF NOT EXIST build.exe cl /Od /nologo ..\code\build.cpp /link /out:build.exe
build.exe %*
IF EXIST build_tmp.exe move build_tmp.exe build.exe

popd