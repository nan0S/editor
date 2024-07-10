#include "editor_base.h"
#include "editor_os.h"

#include "editor_base.cpp"
#include "editor_os.cpp"

global arena *Arena;
global string Code = StrLit("code");
global string Build = StrLit("build");

internal void
LogF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   string Out = StrFV(Arena, Format, Args);
   OutputF("[LOG]: %S\n", Out);
   va_end(Args);
}

internal string
CodePath(string File)
{
   string_list PathList = {};
   StrListPush(Arena, &PathList, StrLit(".."));
   StrListPush(Arena, &PathList, Code);
   StrListPush(Arena, &PathList, File);
   string Path = PathListJoin(Arena, &PathList);
   
   string FullPath = OS_FullPathFromPath(Arena, Path);
   return FullPath;
}

internal process_handle
CompileProgram(b32 Debug)
{
   char const *Mode = (Debug ? "debug" : "release");
   
   string_list BasicCompileCmd = {};
   StrListPush(Arena, &BasicCompileCmd, StrLit("cl"));
   StrListPush(Arena, &BasicCompileCmd, Debug ? StrLit("/Od") : StrLit("/O2"));
   StrListPush(Arena, &BasicCompileCmd, Debug ? StrLit("/DBUILD_DEBUG=1") : StrLit("/DBUILD_DEBUG=0"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/Zi"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/std:c++20"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/nologo"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/WX"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/W4"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4201"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4996"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4100"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4505"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4244"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/EHsc"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4189"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4310"));
   StrListPush(Arena, &BasicCompileCmd, StrLit("/wd4456"));
   StrListPush(Arena, &BasicCompileCmd, StrF(Arena, "/I%S", CodePath(StrLit("."))));
   StrListPush(Arena, &BasicCompileCmd, StrF(Arena, "/I%S", CodePath(StrLit("third_party/sfml/include"))));
   StrListPush(Arena, &BasicCompileCmd, StrF(Arena, "/I%S", CodePath(StrLit("third_party/imgui"))));
   
   string ImguiObj = (Debug ? StrLit("imgui_unity_debug.obj") : StrLit("imgui_unity_release.obj"));
   if (!OS_FileExists(ImguiObj))
   {
      string_list ImguiCmd = StrListCopy(Arena, &BasicCompileCmd);
      StrListPush(Arena, &ImguiCmd, StrLit("/c"));
      StrListPush(Arena, &ImguiCmd, CodePath(StrLit("imgui_unity.cpp")));
      StrListPush(Arena, &ImguiCmd, StrF(Arena, "/Fo:%S", ImguiObj));
      LogF("%s imgui build command: %S", Mode, StrListJoin(Arena, &ImguiCmd, StrLit(" ")));
      process_handle Imgui = OS_LaunchProcess(ImguiCmd);
      OS_WaitForProcessToFinish(Imgui);
   }
   
   string_list MainCmd = BasicCompileCmd;
   StrListPush(Arena, &MainCmd, CodePath(StrLit("editor.cpp")));
   StrListPush(Arena, &MainCmd, ImguiObj);
   StrListPush(Arena, &MainCmd, StrLit("/link"));
   StrListPush(Arena, &MainCmd, StrF(Arena, "/out:editor_%s.exe", Mode));
   StrListPush(Arena, &MainCmd, StrF(Arena, "/LIBPATH:%S", CodePath(StrLit("third_party/sfml/lib"))));
   StrListPush(Arena, &MainCmd, StrLit("Opengl32.lib"));
   StrListPush(Arena, &MainCmd, StrLit("sfml-graphics.lib"));
   StrListPush(Arena, &MainCmd, StrLit("sfml-system.lib"));
   StrListPush(Arena, &MainCmd, StrLit("sfml-window.lib"));
   LogF("%s build command: %S\n", (Debug ? "debug" : "release"), StrListJoin(Arena, &MainCmd, StrLit(" ")));
   process_handle Build = OS_LaunchProcess(MainCmd);
   
   OS_CopyFile(CodePath(StrLit("third_party/sfml/bin/sfml-graphics-2.dll")), StrLit("sfml-graphics-2.dll"));
   OS_CopyFile(CodePath(StrLit("third_party/sfml/bin/sfml-system-2.dll")),   StrLit("sfml-system-2.dll"));
   OS_CopyFile(CodePath(StrLit("third_party/sfml/bin/sfml-window-2.dll")),   StrLit("sfml-window-2.dll"));
   
   return Build;
}

int main(int ArgCount, char *Argv[])
{
   u64 BeginTSC = ReadCPUTimer();
   
   InitThreadCtx();
   Arena = TempArena(0).Arena;
   
   string ExePath = OS_FullPathFromPath(Arena, StrFromCStr(Argv[0]));
   b32 Debug = false;
   b32 Release = false;
   for (u64 ArgIndex = 1;
        ArgIndex < ArgCount;
        ++ArgIndex)
   {
      string Arg = StrFromCStr(Argv[ArgIndex]);
      if (StrMatch(Arg, StrLit("release"), true))
      {
         Release = true;
      }
      if (StrMatch(Arg, StrLit("debug"), true))
      {
         Debug = true;
      }
   }
   if (!Debug && !Release)
   {
      Debug = true;
   }
   
   b32 Error = false;
   string CurDir = OS_CurrentDir(Arena);
   if (StrEndsWith(CurDir, Build) ||
       StrEndsWith(CurDir, Code))
   {
      OS_ChangeDir(StrLit(".."));
   }
   else if (StrEndsWith(CurDir, StrLit("editor")))
   {
      // NOTE(hbr): Nothing to do
   }
   else
   {
      Error = true;
   }
   
   if (!Error)
   {
      OS_MakeDir(Build);
      if (OS_ChangeDir(Build))
      {
         string ExePathNoExt = StrChopLastDot(ExePath);
         string BaseName = PathLastPart(ExePathNoExt);
         string CppName = StrF(Arena, "%S.cpp", BaseName);
         string CppPath = CodePath(CppName);
         
         string_list BuildProgramDependecies = {};
         StrListPush(Arena, &BuildProgramDependecies, CppPath);
         StrListPush(Arena, &BuildProgramDependecies, CodePath(StrLit("editor_base.h")));
         StrListPush(Arena, &BuildProgramDependecies, CodePath(StrLit("editor_base.cpp")));
         StrListPush(Arena, &BuildProgramDependecies, CodePath(StrLit("editor_os.h")));
         StrListPush(Arena, &BuildProgramDependecies, CodePath(StrLit("editor_os.cpp")));
         
         u64 LatestModifyTime = 0;
         ListIter(DependencyFile, BuildProgramDependecies.Head, string_list_node)
         {
            file_attrs Attrs = OS_FileAttributes(DependencyFile->String);
            LatestModifyTime = Max(LatestModifyTime, Attrs.ModifyTime);
         }
         
         file_attrs ExeAttrs = OS_FileAttributes(ExePath);
         if (ExeAttrs.ModifyTime < LatestModifyTime)
         {
            LogF("recompiling build program itself");
            
            string TmpName = StrF(Arena, "%S.tmp.exe", BaseName);
            string_list CompileCmd = {};
            StrListPush(Arena, &CompileCmd, StrLit("cl"));
            StrListPush(Arena, &CompileCmd, StrLit("/Od"));
            StrListPush(Arena, &CompileCmd, StrLit("/Zi"));
            StrListPush(Arena, &CompileCmd, StrLit("/nologo"));
            StrListPush(Arena, &CompileCmd, CppPath);
            StrListPush(Arena, &CompileCmd, StrLit("/link"));
            StrListPush(Arena, &CompileCmd, StrF(Arena, "/out:%S", TmpName));
            process_handle Compile = OS_LaunchProcess(CompileCmd);
            OS_WaitForProcessToFinish(Compile);
            
            string_list BuildCmd = {};
            StrListPush(Arena, &BuildCmd, TmpName);
            process_handle Build = OS_LaunchProcess(BuildCmd);
            OS_WaitForProcessToFinish(Build);
         }
         else
         {
            process_handle Processes[2] = {};
            u64 ProcessCount = 0;
            if (Debug)
            {
               Processes[ProcessCount++] = CompileProgram(true);
            }
            if (Release)
            {
               Processes[ProcessCount++] = CompileProgram(false);
            }
            for (u64 ProcessIndex = 0;
                 ProcessIndex < ProcessCount;
                 ++ProcessIndex)
            {
               OS_WaitForProcessToFinish(Processes[ProcessIndex]);
            }
            
            u64 CPUFreq = EstimateCPUFrequency(3);
            u64 EndTSC = ReadCPUTimer();
            u64 DiffTSC = EndTSC - BeginTSC;
            f64 BuildSec = Cast(f64)DiffTSC / CPUFreq;
            LogF("build took: %.3lfs", BuildSec);
         }
      }
      else
      {
         OutputError("error: cannot change into build directory\n");
      }
   }
   else
   {
      OutputError("error: build called from unexpected directory\n");
   }
   
   return 0;
}