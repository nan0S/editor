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

internal compilation_target
ImGui_Compilation(arena *Arena, b32 ForceRecompile)
{
 compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
 compilation_target Target = MakeTarget(Target_Obj, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/editor_imgui_unity.cpp")), Flags);
 
 return Target;
}

internal compilation_target
STB_Compilation(arena *Arena, b32 ForceRecompile)
{
 compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
 compilation_target Target = MakeTarget(Target_Obj, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/editor_stb_unity.cpp")), Flags);
 
 return Target;
}

internal compilation_target
GLFW_Compilation(arena *Arena, b32 ForceRecompile, build_platform BuildPlatform)
{
 compilation_target Target = {};
 if (BuildPlatform == BuildPlatform_GLFW)
 {
  compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
  Target = MakeTarget(Target_Obj, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/editor_glfw_unity.cpp")), Flags);
 }
 
 return Target;
}

internal void
DefineHotReloadMacro(compilation_target *Target, b32 BuildForHotReloading)
{
 string Value = (BuildForHotReloading ? StrLit("1") : StrLit("0"));
 DefineMacro(Target, StrLit("BUILD_HOT_RELOAD"), Value);
}

internal compilation_target
Editor_Compilation(arena *Arena, b32 BuildForHotReloading, string STB_Output)
{
 compilation_target_type TargetType = (BuildForHotReloading ? Target_Lib : Target_Obj);
 compilation_target Target = MakeTarget(TargetType, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/editor.cpp")), 0);
 StaticLink(&Target, STB_Output);
 DefineHotReloadMacro(&Target, BuildForHotReloading);
 
 return Target;
}

internal compilation_target
Renderer_Compilation(arena *Arena, build_platform BuildPlatform)
{
 compilation_target Target = {};
 
 switch (BuildPlatform)
 {
  case BuildPlatform_Native: {
   operating_system OS = DetectOS();
   switch (OS)
   {
    case OS_Win32: {
     string RendererPath = StrLit("../code/win32/win32_editor_renderer_opengl.cpp");
     Target = MakeTarget(Target_Lib, OS_ExecutableRelativeToFullPath(Arena, RendererPath), 0);
     
     LinkLibrary(&Target, StrLit("User32.lib")); // RegisterClassA,...
     LinkLibrary(&Target, StrLit("Opengl32.lib")); // wgl,glEnable,...
     LinkLibrary(&Target, StrLit("Gdi32.lib")); // SwapBuffers,SetPixelFormat,...
    }break;
    
    case OS_Linux: {
     Assert(!"Not supported");
    }break;
   }
  }break;
  
  case BuildPlatform_GLFW: {}break;
 }
 
 return Target;
}

internal compilation_target
PlatformExe_Compilation(arena *Arena,
                        b32 BuildForHotReloading,
                        string EditorOutput, string RendererOutput,
                        string ImGuiOutput, string GLFW_Output,
                        string StubsOutput, string STB_Output,
                        build_platform BuildPlatform)
{
 operating_system OS = DetectOS();
 
 string PlatformExePath = {};
 switch (BuildPlatform)
 {
  case BuildPlatform_Native: {
   switch (OS)
   {
    case OS_Win32: {
     PlatformExePath = StrLit("../code/win32/win32_editor.cpp");
    }break;
    
    case OS_Linux: {
     PlatformExePath = StrLit("../code/x11/x11_editor.cpp");
    }break;
   }
  }break;
  
  case BuildPlatform_GLFW: {
   PlatformExePath = StrLit("../code/glfw/glfw_editor.cpp");
  }break;
 }
 
 compilation_target Target = MakeTarget(Target_Exe, OS_ExecutableRelativeToFullPath(Arena, PlatformExePath), 0);
 switch (OS)
 {
  case OS_Win32: {
   LinkLibrary(&Target, StrLit("User32.lib")); // CreateWindowExA,...
   LinkLibrary(&Target, StrLit("Comdlg32.lib")); // GetOpenFileName,...
   LinkLibrary(&Target, StrLit("Shell32.lib")); // DragQueryFileA,...
   
   if (BuildPlatform == BuildPlatform_GLFW)
   {
    LinkLibrary(&Target, StrLit("Opengl32.lib")); // wgl,glEnable,...
    LinkLibrary(&Target, StrLit("Gdi32.lib")); // SwapBuffers,SetPixelFormat,...
    LinkLibrary(&Target, OS_ExecutableRelativeToFullPath(Arena, StrLit("../code/third_party/glfw/lib-vc2022/glfw3_mt.lib")));
   }
  }break;
  
  case OS_Linux: {
   LinkLibrary(&Target, StrLit("pthread"));
   LinkLibrary(&Target, StrLit("X11"));
   LinkLibrary(&Target, StrLit("GL"));
  }break;
 }
 
 DefineMacro(&Target, StrLit("EDITOR_DLL"), EditorOutput);
 DefineMacro(&Target, StrLit("EDITOR_RENDERER_DLL"), RendererOutput);
 DefineHotReloadMacro(&Target, BuildForHotReloading);
 
 StaticLink(&Target, ImGuiOutput);
 StaticLink(&Target, GLFW_Output);
 
 if (BuildForHotReloading)
 {
  StaticLink(&Target, StubsOutput);
 }
 else
 {
  StaticLink(&Target, EditorOutput);
 }
 
 StaticLink(&Target, STB_Output);
 
 return Target;
}

internal compilation_target
Stubs_Compilation(arena *Arena, build_platform BuildPlatform)
{
 compilation_target Target = MakeTarget(Target_Obj, StrLit("../code/editor_stubs.cpp"), 0);
 return Target;
}

internal string
SmallU32ToStr(u32 N)
{
 string Result = {};
 switch (N)
 {
  case 0: {Result = StrLit("0");}break;
  case 1: {Result = StrLit("1");}break;
  case 2: {Result = StrLit("2");}break;
  case 3: {Result = StrLit("3");}break;
  case 4: {Result = StrLit("4");}break;
  case 5: {Result = StrLit("5");}break;
  case 6: {Result = StrLit("6");}break;
  case 7: {Result = StrLit("7");}break;
  case 8: {Result = StrLit("8");}break;
  case 9: {Result = StrLit("9");}break;
  
  default: InvalidPath;
 }
 
 return Result;
}
internal void
DefineCompilationUnitMacros(compilation_target *Target, u32 *CompilationUnitIndex, u32 CompilationUnitCount)
{
 u32 UnitIndex = (*CompilationUnitIndex)++;
 Assert(UnitIndex < CompilationUnitCount);
 
 DefineMacro(Target, StrLit("COMPILATION_UNIT_INDEX"), SmallU32ToStr(UnitIndex));
 DefineMacro(Target, StrLit("COMPILATION_UNIT_COUNT"), SmallU32ToStr(CompilationUnitCount));
}

internal exit_code_int
CompileEditor(process_queue *ProcessQueue, compiler_choice Compiler, b32 Debug, b32 ForceRecompile, b32 Verbose)
{
 exit_code_int ExitCode = 0;
 temp_arena Temp = TempArena(0);
 
 build_platform BuildPlatform = {};
 operating_system OS = DetectOS();
 switch (OS)
 {
  case OS_Win32: {BuildPlatform = BuildPlatform_GLFW;}break;
  case OS_Linux: {BuildPlatform = BuildPlatform_Native;}break;
 }
 
 b32 BuildForHotReloading = Debug;
 //BuildForHotReloading = false;
 
 compiler_setup Setup = MakeCompilerSetup(Compiler, Debug, true, Verbose);
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code")));
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/third_party/imgui")));
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/third_party/glfw")));
 IncludePath(&Setup, OS_ExecutableRelativeToFullPath(Temp.Arena, StrLit("../code/third_party/gl3w")));
 
 compilation_target GLFW = GLFW_Compilation(Temp.Arena, ForceRecompile, BuildPlatform);
 compilation_target STB = STB_Compilation(Temp.Arena, ForceRecompile);
 compilation_target ImGui = ImGui_Compilation(Temp.Arena, ForceRecompile);
 compilation_target Renderer = Renderer_Compilation(Temp.Arena, BuildPlatform);
 compilation_target Stubs = Stubs_Compilation(Temp.Arena, BuildPlatform);
 
 string STB_Output = ComputeCompilationTargetOutput(Setup, STB).OutputTarget;
 string ImGuiOutput = ComputeCompilationTargetOutput(Setup, ImGui).OutputTarget;
 string GLFW_Output = ComputeCompilationTargetOutput(Setup, GLFW).OutputTarget;
 string RendererOutput = ComputeCompilationTargetOutput(Setup, Renderer).OutputTarget;
 string StubsOutput = ComputeCompilationTargetOutput(Setup, Stubs).OutputTarget;
 
 compilation_target Editor = Editor_Compilation(Temp.Arena, BuildForHotReloading, STB_Output);
 string EditorOutput = ComputeCompilationTargetOutput(Setup, Editor).OutputTarget;
 
 compilation_target PlatformExe = PlatformExe_Compilation(Temp.Arena, BuildForHotReloading,
                                                          EditorOutput, RendererOutput,
                                                          ImGuiOutput, GLFW_Output,
                                                          StubsOutput, STB_Output,
                                                          BuildPlatform);
 
 u32 CompilationUnitCount = 0;
 if (Renderer.TargetType != Target_None)
 {
  ++CompilationUnitCount;
 }
 if (Editor.TargetType != Target_None)
 {
  ++CompilationUnitCount;
 }
 if (PlatformExe.TargetType != Target_None)
 {
  ++CompilationUnitCount;
 }
 
 u32 CompilationUnitIndex = 0;
 if (Renderer.TargetType != Target_None)
 {
  DefineCompilationUnitMacros(&Renderer, &CompilationUnitIndex, CompilationUnitCount);
 }
 if (Editor.TargetType != Target_None)
 {
  DefineCompilationUnitMacros(&Editor, &CompilationUnitIndex, CompilationUnitCount);
 }
 if (PlatformExe.TargetType != Target_None)
 {
  DefineCompilationUnitMacros(&PlatformExe, &CompilationUnitIndex, CompilationUnitCount);
 }
 Assert(CompilationUnitIndex == CompilationUnitCount);
 
 // NOTE(hbr): First start processes that don't depend on anything. Then run their descendants.
 os_process_handle RendererProcess = Compile(Setup, Renderer);
 os_process_handle STBProcess = Compile(Setup, STB);
 os_process_handle ImGuiProcess = Compile(Setup, ImGui);
 os_process_handle GLFWProcess = Compile(Setup, GLFW);
 os_process_handle StubsProcess = Compile(Setup, Stubs);
 
 ExitCode = OS_CombineExitCodes(ExitCode, OS_ProcessWait(STBProcess));
 os_process_handle EditorProcess = Compile(Setup, Editor);
 
 ExitCode = OS_CombineExitCodes(ExitCode, OS_ProcessWait(ImGuiProcess));
 ExitCode = OS_CombineExitCodes(ExitCode, OS_ProcessWait(StubsProcess));
 if (!BuildForHotReloading)
 {
  ExitCode = OS_CombineExitCodes(ExitCode, OS_ProcessWait(EditorProcess));
 }
 os_process_handle PlatformExeProcess = Compile(Setup, PlatformExe);
 
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
 
 ThreadCtxInit();
 OS_Init(ArgCount, Args);
 arena *Arena = TempArena(0).Arena;
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
   exit_code_int SubProcessExitCode = CompileEditor(&ProcessQueue, Compiler, true, ForceRecompile, Verbose);
   ExitCode = OS_CombineExitCodes(ExitCode, SubProcessExitCode);
  }
  if (Release)
  {
   exit_code_int SubProcessExitCode = CompileEditor(&ProcessQueue, Compiler, false, ForceRecompile, Verbose);
   ExitCode = OS_CombineExitCodes(ExitCode, SubProcessExitCode);
  }
  
  {
   exit_code_int SubProcessesExitCode = WaitProcesses(&ProcessQueue);
   ExitCode = OS_CombineExitCodes(ExitCode, SubProcessesExitCode);
  }
  
  u64 CPUFreq = OS_CPUTimerFreq();
  u64 EndTSC = OS_ReadCPUTimer();
  u64 DiffTSC = EndTSC - BeginTSC;
  f32 BuildSec = Cast(f32)DiffTSC / CPUFreq;
  OS_PrintF("[build took %.3fs]\n", BuildSec);
 }
 
 return ExitCode;
}