#include "editor_base.h"
#include "editor_string.h"
#include "editor_platform.h"
#include "editor_memory.h"
#include "editor_os.h"

#include "editor_base.cpp"
#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_os.cpp"

global arena *GlobalArena;
global string Code = StrLit("code");
global string Build = StrLit("build");
global platform_api Platform;

internal void
LogF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 string Out = StrFV(GlobalArena, Format, Args);
 OutputF("[%S]\n", Out);
 va_end(Args);
}

internal string
CodePath(string File)
{
 string_list PathList = {};
 StrListPush(GlobalArena, &PathList, StrLit(".."));
 StrListPush(GlobalArena, &PathList, Code);
 StrListPush(GlobalArena, &PathList, File);
 string Path = PathListJoin(GlobalArena, &PathList);
 
 string FullPath = OS_FullPathFromPath(GlobalArena, Path);
 return FullPath;
}

internal process
CompileProgram(b32 Debug)
{
 char const *Mode = (Debug ? "debug" : "release");
 
 string_list BasicCompileCmd = {};
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("cl"));
 StrListPush(GlobalArena, &BasicCompileCmd, Debug ? StrLit("/Od") : StrLit("/O2"));
 StrListPush(GlobalArena, &BasicCompileCmd, Debug ? StrLit("/DBUILD_DEBUG=1") : StrLit("/DBUILD_DEBUG=0"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/Zi"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/FS"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/nologo"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/WX"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/W4"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4201"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4996"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4100"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4505"));
 //StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4244"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4189"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/EHsc"));
 
#if 0
 set CommonCompilerFlags=-diagnostics:column
  -WL
  -O2
  -nologo
  -fp:fast
  -fp:except-
  -Gm-
  -GR-
  -EHa-
  -Zo
  -Oi
  -WX
  -W4
  -wd4201 -wd4100 -wd4189 -wd4505 -wd4127
  -FC -Z7 -GS- -Gs9999999
#endif
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4310"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrLit("/wd4456"));
 StrListPush(GlobalArena, &BasicCompileCmd, StrF(GlobalArena, "/I%S", CodePath(StrLit("."))));
 StrListPush(GlobalArena, &BasicCompileCmd, StrF(GlobalArena, "/I%S", CodePath(StrLit("third_party/imgui"))));
 
 string ImguiObj = StrF(GlobalArena, "imgui_unity_%s.obj", Mode);
 process ImGui = {};
 if (!OS_FileExists(ImguiObj))
 {
  string_list ImguiCmd = StrListCopy(GlobalArena, &BasicCompileCmd);
  StrListPush(GlobalArena, &ImguiCmd, StrLit("/c"));
  StrListPush(GlobalArena, &ImguiCmd, CodePath(StrLit("imgui_unity.cpp")));
  StrListPush(GlobalArena, &ImguiCmd, StrF(GlobalArena, "/Fo:%S", ImguiObj));
  LogF("%s imgui build command: %S", Mode, StrListJoin(GlobalArena, &ImguiCmd, StrLit(" ")));
  ImGui = OS_LaunchProcess(ImguiCmd);
 }
 
 string RendererObj = StrF(GlobalArena, "win32_editor_opengl_%s.obj", Mode);
 process Renderer = {};
 {
  string_list RendererCmd = StrListCopy(GlobalArena, &BasicCompileCmd);
  StrListPush(GlobalArena, &RendererCmd, StrLit("/c"));
  StrListPush(GlobalArena, &RendererCmd, CodePath(StrLit("win32_editor_opengl.cpp")));
  StrListPush(GlobalArena, &RendererCmd, StrF(GlobalArena, "/Fo:%S", RendererObj));
  LogF("%s Win32 OpenGL renderer build command: %S", Mode, StrListJoin(GlobalArena, &RendererCmd, StrLit(" ")));
  Renderer = OS_LaunchProcess(RendererCmd);
 }
 
 string_list MainCmd = BasicCompileCmd;
 StrListPush(GlobalArena, &MainCmd, CodePath(StrLit("win32_editor.cpp")));
 StrListPush(GlobalArena, &MainCmd, ImguiObj);
 StrListPush(GlobalArena, &MainCmd, RendererObj);
 StrListPush(GlobalArena, &MainCmd, StrF(GlobalArena, "/Fo:editor_%s.obj", Mode));
 StrListPush(GlobalArena, &MainCmd, StrLit("/link"));
 StrListPush(GlobalArena, &MainCmd, StrF(GlobalArena, "/out:editor_%s.exe", Mode));
 StrListPush(GlobalArena, &MainCmd, StrF(GlobalArena, "/LIBPATH:%S", CodePath(StrLit("third_party/sfml/lib"))));
 StrListPush(GlobalArena, &MainCmd, StrLit("Opengl32.lib"));
 StrListPush(GlobalArena, &MainCmd, StrLit("Comdlg32.lib"));
 StrListPush(GlobalArena, &MainCmd, StrLit("Gdi32.lib"));
 
 OS_WaitForProcessToFinish(ImGui);
 OS_WaitForProcessToFinish(Renderer);
 
 LogF("%s build command: %S", Mode, StrListJoin(GlobalArena, &MainCmd, StrLit(" ")));
 process Build = OS_LaunchProcess(MainCmd);
 
 return Build;
}

PLATFORM_ALLOC_VIRTUAL_MEMORY(BuildAllocVirtualMemory)
{
 void *Result = OS_AllocVirtualMemory(Size, Commit);
 return Result;
}

PLATFORM_DEALLOC_VIRTUAL_MEMORY(BuildDeallocVirtualMemory)
{
 OS_DeallocVirtualMemory(Memory, Size);
}

PLATFORM_COMMIT_VIRTUAL_MEMORY(BuildCommitVirtualMemory)
{
 OS_CommitVirtualMemory(Memory, Size);
}

int main(int ArgCount, char *Argv[])
{
 u64 BeginTSC = ReadCPUTimer();
 
 Platform.AllocVirtualMemory = BuildAllocVirtualMemory;
 Platform.DeallocVirtualMemory = BuildDeallocVirtualMemory;
 Platform.CommitVirtualMemory = BuildCommitVirtualMemory;
 
 InitThreadCtx();
 InitOS();
 GlobalArena = TempArena(0).Arena;
 
 string ExePath = OS_FullPathFromPath(GlobalArena, StrFromCStr(Argv[0]));
 b32 Debug = false;
 b32 Release = false;
 for (u32 ArgIndex = 1;
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
 string CurDir = OS_CurrentDir(GlobalArena);
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
   string CppName = StrF(GlobalArena, "%S.cpp", BaseName);
   string CppPath = CodePath(CppName);
   
   string_list BuildProgramDependecies = {};
   StrListPush(GlobalArena, &BuildProgramDependecies, CppPath);
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_base.h")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_base.cpp")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_memory.h")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_memory.cpp")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_string.h")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_string.cpp")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_os.h")));
   StrListPush(GlobalArena, &BuildProgramDependecies, CodePath(StrLit("editor_os.cpp")));
   
   u64 LatestModifyTime = 0;
   ListIter(DependencyFile, BuildProgramDependecies.Head, string_list_node)
   {
    file_attrs Attrs = OS_FileAttributes(DependencyFile->Str);
    LatestModifyTime = Max(LatestModifyTime, Attrs.ModifyTime);
   }
   
   file_attrs ExeAttrs = OS_FileAttributes(ExePath);
   if (ExeAttrs.ModifyTime < LatestModifyTime)
   {
    LogF("recompiling build program itself");
    
    string TmpName = StrF(GlobalArena, "%S.tmp.exe", BaseName);
    string_list CompileCmd = {};
    StrListPush(GlobalArena, &CompileCmd, StrLit("cl"));
    StrListPush(GlobalArena, &CompileCmd, StrLit("/Od"));
    StrListPush(GlobalArena, &CompileCmd, StrLit("/Zi"));
    StrListPush(GlobalArena, &CompileCmd, StrLit("/nologo"));
    StrListPush(GlobalArena, &CompileCmd, CppPath);
    StrListPush(GlobalArena, &CompileCmd, StrLit("/link"));
    StrListPush(GlobalArena, &CompileCmd, StrF(GlobalArena, "/out:%S", TmpName));
    process Compile = OS_LaunchProcess(CompileCmd);
    OS_WaitForProcessToFinish(Compile);
    
    string_list BuildCmd = {};
    StrListPush(GlobalArena, &BuildCmd, TmpName);
    process Build = OS_LaunchProcess(BuildCmd);
    OS_WaitForProcessToFinish(Build);
   }
   else
   {
    process Processes[2] = {};
    u32 ProcessCount = 0;
    if (Debug)
    {
     Processes[ProcessCount++] = CompileProgram(true);
    }
    if (Release)
    {
     Processes[ProcessCount++] = CompileProgram(false);
    }
    for (u32 ProcessIndex = 0;
         ProcessIndex < ProcessCount;
         ++ProcessIndex)
    {
     OS_WaitForProcessToFinish(Processes[ProcessIndex]);
    }
    
    u64 CPUFreq = EstimateCPUFreq(3);
    u64 EndTSC = ReadCPUTimer();
    u64 DiffTSC = EndTSC - BeginTSC;
    f32 BuildSec = Cast(f32)DiffTSC / CPUFreq;
    LogF("build took %.3fs", BuildSec);
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