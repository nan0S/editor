#include "base_ctx_crack.h"
#include "base_core.h"
#include "base_arena.h"
#include "base_string.h"
#include "base_thread_ctx.h"
#include "os_core.h"
#include "base_nobuild.h"
#include "base_compile.h"

#include "base_core.cpp"
#include "base_arena.cpp"
#include "base_string.cpp"
#include "base_thread_ctx.cpp"
#include "base_compile.cpp"
#include "os_core.cpp"
#include "base_nobuild.cpp"

int main(int ArgCount, char *Argv[])
{
 u64 BeginTSC = OS_ReadCPUTimer();
 
 InitThreadCtx(Gigabytes(64));
 OS_EntryPoint(ArgCount, Argv);
 InitBuild(ArgCount, Argv);
 
 if (!RecompileYourselfIfNecessary(ArgCount, Argv))
 {
  arena *Arena = AllocArena(Gigabytes(64));
  
  b32 Debug = false;
  b32 Release = false;
  b32 ForceRecompile = false;
  b32 Verbose = false;
  for (int ArgIndex = 1;
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
   if (StrMatch(Arg, StrLit("force-recompile"), true))
   {
    ForceRecompile = true;
   }
   if (StrMatch(Arg, StrLit("verbose"), true))
   {
    Verbose = true;
   }
  }
  if (!Debug && !Release)
  {
   // NOTE(hbr): By default build debug
   Debug = true;
  }
  
  process_queue ProcessQueue = {};
  compiler_setup Setup = MakeCompilerSetup(Compiler_Default, Debug, true, Verbose);
  IncludePath(&Setup, StrLit("../code/third_party/imgui"));
  
  //- precompile third part code into obj
  compile_result ThirdParty = {};
  {
   compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
   compilation_target Target = MakeTarget(Obj, StrLit("../code/editor_third_party_unity.cpp"), Flags);
   ThirdParty = Compile(Setup, Target);
   OS_ProcessWait(ThirdParty.CompileProcess);
  }
  
  //- compile editor code into library
  compile_result Editor = {};
  {
   compilation_target Target = MakeTarget(Lib, StrLit("../code/editor.cpp"), 0);
   StaticLink(&Target, ThirdParty.OutputTarget);
   Editor = Compile(Setup, Target);
   EnqueueProcess(&ProcessQueue, Editor.CompileProcess);
  }
  
  //- compile renderer into library
  compile_result Renderer = {};
  {
   compilation_target Target = MakeTarget(Lib, StrLit("../code/win32_editor_renderer_opengl.cpp"), 0);
   LinkLibrary(&Target, "Opengl32.lib"); // wgl,glEnable,...
   LinkLibrary(&Target, "Gdi32.lib"); // SwapBuffers,SetPixelFormat,...
   Renderer = Compile(Setup, Target);
   EnqueueProcess(&ProcessQueue, Renderer.CompileProcess);
  }
  
  //- compile platform layer into executable
  {
   compilation_target Target = MakeTarget(Exe, StrLit("../code/win32_editor.cpp"), 0);
   LinkLibrary(&Target, "User32.lib"); // CreateWindowExA,...
   LinkLibrary(&Target, "Comdlg32.lib"); // GetOpenFileName,...
   DefineMacro(&Target, StrLit("EDITOR_DLL"), Editor.OutputTarget);
   DefineMacro(&Target, StrLit("EDITOR_RENDERER_DLL"), Renderer.OutputTarget);
   compile_result Win32PlatformExe = Compile(Setup, Target);
   EnqueueProcess(&ProcessQueue, Win32PlatformExe.CompileProcess);
  }
  
  WaitProcesses(&ProcessQueue);
  
  u64 CPUFreq = OS_CPUTimerFreq();
  u64 EndTSC = OS_ReadCPUTimer();
  u64 DiffTSC = EndTSC - BeginTSC;
  f32 BuildSec = Cast(f32)DiffTSC / CPUFreq;
  OS_PrintF("[build took %.3fs]\n", BuildSec);
 }
 
 return 0;
}