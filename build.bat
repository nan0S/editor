@echo off

set CommonCompilerFlags=/Zi /std:c++20 /nologo /WX /W4 /wd4201 /wd4996 /wd4100 /wd4505 /I..\code /I..\code\third_party\sfml\include /I..\code\third_party\imgui
set CommonDebugCompilerFlags=/Od /DEDITOR_DEBUG=1 %CommonCompilerFlags%
set CommonReleaseCompilerFlags=/O2 /DEDITOR_DEBUG=0 %CommonCompilerFlags%
set CommonLinkerFlags=/LIBPATH:..\code\third_party\sfml\lib Opengl32.lib

set CompilationMode=%1
IF NOT DEFINED CompilationMode set CompilationMode="debug"

IF NOT EXIST build mkdir build
pushd build

IF %CompilationMode%==release (

	IF NOT EXIST imgui_unity_release.obj (
		ECHO imgui release build:
		cl %CommonReleaseCompilerFlags% /c ..\code\imgui_unity.cpp /Fo:imgui_unity_release.obj
	)

	ECHO editor release build:
	cl %CommonReleaseCompilerFlags% ..\code\editor.cpp imgui_unity_release.obj /link %CommonLinkerFlags% sfml-graphics.lib sfml-system.lib sfml-window.lib /out:editor_release.exe

	copy ..\code\third_party\sfml\bin\sfml-system-2.dll .
	copy ..\code\third_party\sfml\bin\sfml-graphics-2.dll .
	copy ..\code\third_party\sfml\bin\sfml-window-2.dll .

) ELSE (

	IF NOT EXIST imgui_unity.obj (
		ECHO imgui debug build:
		cl %CommonDebugCompilerFlags% /c ..\code\imgui_unity.cpp
	)

	ECHO editor debug build:
	cl %CommonDebugCompilerFlags% ..\code\editor.cpp imgui_unity.obj /link %CommonLinkerFlags% sfml-graphics.lib sfml-system.lib sfml-window.lib

	copy ..\code\third_party\sfml\bin\sfml-system-2.dll .
	copy ..\code\third_party\sfml\bin\sfml-graphics-2.dll .
	copy ..\code\third_party\sfml\bin\sfml-window-2.dll .

)

popd