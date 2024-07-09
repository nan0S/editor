#include <windows.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
   uint64_t A = __rdtsc();
   STARTUPINFOA StartupInfo = {sizeof(StartupInfo)};
   PROCESS_INFORMATION ProcessInfo;
   DWORD CreationFlags = 0;
   char *Cmd =
      "cl /Od /DBUILD_DEBUG=1 /Zi /std:c++20 /nologo /WX /W4 /wd4201 /wd4996 /wd4100 /wd4505 /wd4244 /EHsc /wd4189 /wd4310 /wd4456 /I..\\code  /I..\\code\\third_party\\sfml\\include /I..\\code\\third_party\\imgui "
      "..\\code\\editor.cpp imgui_unity.obj /link /LIBPATH:..\\code\\third_party\\sfml\\lib Opengl32.lib "
      "sfml-graphics.lib sfml-system.lib sfml-window.lib";
   CreateProcessA(0, Cmd, 0, 0, 0, CreationFlags, 0, 0, &StartupInfo, &ProcessInfo);
   WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
   uint64_t B = __rdtsc();
   
   printf("%llu\n", B-A);
   
   return 0;
}