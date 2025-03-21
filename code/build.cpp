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

enum build_platform
{
 BuildPlatform_Native,
 BuildPlatform_GLFW,
};

internal compilation_command
Renderer_Compilation(arena *Arena, compiler_setup Setup, build_platform Platform)
{
 compilation_target Target = MakeTarget(Lib, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/win32/win32_editor_renderer_opengl.cpp")), 0);
 LinkLibrary(&Target, StrLit("User32.lib")); // RegisterClassA,...
 LinkLibrary(&Target, StrLit("Opengl32.lib")); // wgl,glEnable,...
 LinkLibrary(&Target, StrLit("Gdi32.lib")); // SwapBuffers,SetPixelFormat,...
 
 compilation_command Renderer = ComputeCompilationCommand(Setup, Target);
 return Renderer;
}

internal compilation_command
PlatformExe_Compilation(arena *Arena, compiler_setup Setup, compilation_command Editor, compilation_command Renderer,
                        build_platform Platform)
{
 operating_system OS = DetectOS();
 string PlatformExePath = {};
 switch (OS)
 {
  case OS_Win32: {
   switch (Platform)
   {
    case BuildPlatform_Native: {
     PlatformExePath = StrLit("../code/win32/win32_editor.cpp");
    }break;
    case BuildPlatform_GLFW: {
     PlatformExePath = StrLit("../code/glfw/glfw_editor.cpp");
    }break;
   }
  }break;
  
  case OS_Linux: {
   AssertMsg(Platform == BuildPlatform_Native, "Only native platform supported on Linux");
   PlatformExePath = StrLit("../code/x11/x11_editor.cpp");
  }break;
 }
 
 compilation_target Target = MakeTarget(Exe, OS_ExecutableRelativeToFullPath(Arena, PlatformExePath), 0);
 
 switch (OS)
 {
  case OS_Win32: {
   LinkLibrary(&Target, StrLit("User32.lib")); // CreateWindowExA,...
   LinkLibrary(&Target, StrLit("Comdlg32.lib")); // GetOpenFileName,...
   LinkLibrary(&Target, StrLit("Shell32.lib")); // DragQueryFileA,...
  }break;
  
  case OS_Linux: {
   LinkLibrary(&Target, StrLit("pthread"));
   LinkLibrary(&Target, StrLit("X11"));
   LinkLibrary(&Target, StrLit("GL"));
  }break;
 }
 
 DefineMacro(&Target, StrLit("EDITOR_DLL"), Editor.OutputTarget);
 DefineMacro(&Target, StrLit("EDITOR_RENDERER_DLL"), Renderer.OutputTarget);
 
 compilation_command PlatformExe = ComputeCompilationCommand(Setup, Target);
 return PlatformExe;
}

internal int
CompileEditor(process_queue *ProcessQueue, compiler_choice Compiler, b32 Debug, b32 ForceRecompile, b32 Verbose)
{
 int ExitCode = 0;
 temp_arena Temp = TempArena(0);
 
 compiler_setup Setup = MakeCompilerSetup(Compiler, Debug, true, Verbose);
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code")));
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/third_party/imgui")));
 
 //- precompile third party code into obj
 compilation_command ThirdParty = {};
 {
  compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
  compilation_target Target = MakeTarget(Obj, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/editor_third_party_unity.cpp")), Flags);
  ThirdParty = ComputeCompilationCommand(Setup, Target);
 }
 
 //- compile editor code into library
 compilation_command Editor = {};
 {
  string TargetPath = {};
  compilation_target Target = MakeTarget(Lib, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/editor.cpp")), 0);
  StaticLink(&Target, ThirdParty.OutputTarget);
  Editor = ComputeCompilationCommand(Setup, Target);
 }
 
 //- compile renderer into library and platform layer into executable
 build_platform Platform = (DetectOS() == OS_Win32 ? BuildPlatform_Native : BuildPlatform_Native);
 
 compilation_command Renderer = Renderer_Compilation(Temp.Arena, Setup, Platform);
 compilation_command PlatformExe = PlatformExe_Compilation(Temp.Arena, Setup, Editor, Renderer, Platform);
 
 // NOTE(hbr): Start up renderer and Win32 compilation processes first because they don't depend on anything.
 // Do them in background when waiting for ThirdParty to compile. Editor on the other hand depends on ThirdParty
 // to compile first.
 os_process_handle RendererProcess = OS_ProcessLaunch(Renderer.Cmd);
 os_process_handle PlatformExeProcess = OS_ProcessLaunch(PlatformExe.Cmd);
 os_process_handle ThirdPartyProcess = OS_ProcessLaunch(ThirdParty.Cmd);
 ExitCode = OS_ProcessWait(ThirdPartyProcess);
 os_process_handle EditorProcess = OS_ProcessLaunch(Editor.Cmd);
 
 EnqueueProcess(ProcessQueue, EditorProcess);
 EnqueueProcess(ProcessQueue, RendererProcess);
 EnqueueProcess(ProcessQueue, PlatformExeProcess);
 
 EndTemp(Temp);
 
 return ExitCode;
}

int main(int ArgCount, char *Args[])
{
 u64 BeginTSC = OS_ReadCPUTimer();
 int ExitCode = 0;
 
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
 
 recompilation_result Recompilation = RecompileYourselfIfNecessary(ArgCount, Args);
 ExitCode = Recompilation.RecompilationExitCode;
 if (!Recompilation.TriedToRecompile)
 {
  b32 Debug = false;
  b32 Release = false;
  b32 ForceRecompile = false;
  b32 Verbose = false;
  compiler_choice Compiler = Compiler_Default;
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
   if (StrMatch(Arg, StrLit("gcc"), true))
   {
    Compiler = Compiler_GCC;
   }
   if (StrMatch(Arg, StrLit("clang"), true))
   {
    Compiler = Compiler_Clang;
   }
   if (StrMatch(Arg, StrLit("msvc"), true))
   {
    Compiler = Compiler_MSVC;
   }
   if (StrMatch(Arg, StrLit("default"), true))
   {
    Compiler = Compiler_Default;
   }
  }
  if (!Debug && !Release)
  {
   // NOTE(hbr): By default build debug
   Debug = true;
  }
  
  process_queue ProcessQueue = {};
  if (Debug)
  {
   int SubProcessExitCode = CompileEditor(&ProcessQueue, Compiler, true, ForceRecompile, Verbose);
   if (SubProcessExitCode) ExitCode = SubProcessExitCode;
  }
  if (Release)
  {
   int SubProcessExitCode = CompileEditor(&ProcessQueue, Compiler, false, ForceRecompile, Verbose);
   if (SubProcessExitCode) ExitCode = SubProcessExitCode;
  }
  
  for (u32 ProcessIndex = 0;
       ProcessIndex < ProcessQueue.ProcessCount;
       ++ProcessIndex)
  {
   int SubProcessExitCode = OS_ProcessWait(ProcessQueue.Processes[ProcessIndex]);
   if (SubProcessExitCode) ExitCode = SubProcessExitCode;
  }
  
  u64 CPUFreq = OS_CPUTimerFreq();
  u64 EndTSC = OS_ReadCPUTimer();
  u64 DiffTSC = EndTSC - BeginTSC;
  f32 BuildSec = Cast(f32)DiffTSC / CPUFreq;
  OS_PrintF("[build took %.3fs]\n", BuildSec);
 }
 
 return ExitCode;
}