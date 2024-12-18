#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "base/base_thread_ctx.h"
#include "base/base_os.h"
#include "base/base_nobuild.h"
#include "base/base_compile.h"

#include "base/base_core.cpp"
#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_compile.cpp"
#include "base/base_os.cpp"
#include "base/base_nobuild.cpp"

internal void
CompileEditor(process_queue *ProcessQueue, b32 Debug, b32 ForceRecompile, b32 Verbose)
{
 temp_arena Temp = TempArena(0);

 // TODO(hbr): go back to Compiler_Default
 compiler_setup Setup = MakeCompilerSetup(Compiler_Clang, Debug, true, Verbose);
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code")));
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/third_party/imgui")));
 
 //- precompile third part code into obj
 compile_result ThirdParty = {};
 {
  compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
  compilation_target Target = MakeTarget(Obj, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/editor_third_party_unity.cpp")), Flags);
  ThirdParty = Compile(Setup, Target);
  OS_ProcessWait(ThirdParty.CompileProcess);
 }
 
 //- compile editor code into library
 compile_result Editor = {};
 {
  string TargetPath = {};
  compilation_target Target = MakeTarget(Lib, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/editor.cpp")), 0);
  StaticLink(&Target, ThirdParty.OutputTarget);
  Editor = Compile(Setup, Target);
  EnqueueProcess(ProcessQueue, Editor.CompileProcess);
 }
 
 //- compile renderer into library
 compile_result Renderer = {};
 {
  compilation_target Target = MakeTarget(Lib, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/win32/win32_editor_renderer_opengl.cpp")), 0);
  LinkLibrary(&Target, StrLit("User32.lib")); // RegisterClassA,...
  LinkLibrary(&Target, StrLit("Opengl32.lib")); // wgl,glEnable,...
  LinkLibrary(&Target, StrLit("Gdi32.lib")); // SwapBuffers,SetPixelFormat,...
  Renderer = Compile(Setup, Target);
  EnqueueProcess(ProcessQueue, Renderer.CompileProcess);
 }
 
 //- compile platform layer into executable
 {
  compilation_target Target = MakeTarget(Exe, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/win32/win32_editor.cpp")), 0);
  LinkLibrary(&Target, StrLit("User32.lib")); // CreateWindowExA,...
  LinkLibrary(&Target, StrLit("Comdlg32.lib")); // GetOpenFileName,...
  LinkLibrary(&Target, StrLit("Shell32.lib")); // DragQueryFileA,...
  DefineMacro(&Target, StrLit("EDITOR_DLL"), Editor.OutputTarget);
  DefineMacro(&Target, StrLit("EDITOR_RENDERER_DLL"), Renderer.OutputTarget);
  compile_result Win32PlatformExe = Compile(Setup, Target);
  EnqueueProcess(ProcessQueue, Win32PlatformExe.CompileProcess);
 }

 EndTemp(Temp);
}

int main(int ArgCount, char *Args[])
{
 u64 BeginTSC = OS_ReadCPUTimer();
 
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0;
      ArenaIndex < ArrayCount(Arenas);
      ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena(Gigabytes(64));
 }
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));

 OS_Init(ArgCount, Args);

 arena *Arena = Arenas[0];
 EquipBuild(Arena);
 
// if (!RecompileYourselfIfNecessary(ArgCount, Args))
 {
  b32 Debug = false;
  b32 Release = false;
  b32 ForceRecompile = false;
  b32 Verbose = false;
  for (int ArgIndex = 1;
       ArgIndex < ArgCount;
       ++ArgIndex)
  {
   string Arg = StrFromCStr(Args[ArgIndex]);
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
  if (Debug)   CompileEditor(&ProcessQueue, true, ForceRecompile, Verbose);
  if (Release) CompileEditor(&ProcessQueue, false, ForceRecompile, Verbose);
  WaitProcesses(&ProcessQueue);
  
  u64 CPUFreq = OS_CPUTimerFreq();
  u64 EndTSC = OS_ReadCPUTimer();
  u64 DiffTSC = EndTSC - BeginTSC;
  f32 BuildSec = Cast(f32)DiffTSC / CPUFreq;
  OS_PrintF("[build took %.3fs]\n", BuildSec);
 }
 
 return 0;
}