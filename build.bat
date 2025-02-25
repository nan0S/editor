@echo off

IF NOT EXIST build mkdir build
pushd build

cl -nologo -FC -Od ..\code\build.cpp -link -out:build.exe
IF NOT EXIST build.exe cl -nologo -FC -Od ..\code\build.cpp -link -out:build.exe
build.exe %* verbose
IF EXIST build_tmp.exe move build_tmp.exe build.exe

REM cl  ../code/glfw/glfw_editor.cpp -I../code -I../code/third_party/glfw -out:glfw_editor.exe  glfw3.lib User32.lib Opengl32.lib Gdi32.lib Comdlg32.lib Shell32.lib 
REM cl -I../code -I../code/third_party/glfw ../code/glfw/glfw_editor.cpp -out:glfw_editor.exe

popd