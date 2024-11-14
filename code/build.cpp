#include "editor_ctx_crack.h"
#include "editor_base.h"
#include "editor_string.h"
// TODO(hbr): This absolutely shouldn't be here
#include "imgui_bindings.h"
#include "editor_platform.h"
#include "editor_memory.h"
#include "editor_os.h"

#include "build.h"

#include "editor_base.cpp"
#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_os.cpp"

global arena *BuildArena;
global string CodeDir = StrLit("code");
global string BuildDir = StrLit("build");
platform_api Platform;

internal void
LogF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 string Out = StrFV(BuildArena, Format, Args);
 OutputF("[%S]\n", Out);
 va_end(Args);
}

internal string
CodePath(string File)
{
 string_list PathList = {};
 StrListPush(BuildArena, &PathList, StrLit(".."));
 StrListPush(BuildArena, &PathList, CodeDir);
 StrListPush(BuildArena, &PathList, File);
 string Path = PathListJoin(BuildArena, &PathList);
 
 string FullPath = OS_FullPathFromPath(BuildArena, Path);
 return FullPath;
}

internal compiler_setup
MakeCompilerSetup(compiler Compiler, b32 DebugBuild, b32 GenerateDebuggerInfo)
{
 compiler_setup Result = {};
 Result.Compiler = Compiler;
 Result.DebugBuild = DebugBuild;
 Result.GenerateDebuggerInfo = GenerateDebuggerInfo;
 
 return Result;
}

internal compilation_target
MakeTarget(compilation_target_type TargetType, string SourcePathRel, compilation_flags Flags)
{
 compilation_target Result = {};
 Result.TargetType = TargetType;
 Result.SourcePathRel = SourcePathRel;
 Result.Flags = Flags;
 
 return Result;
}

internal void
IncludePath(compiler_setup *Setup, char const *IncludePathRel)
{
 StrListPushF(BuildArena, &Setup->IncludePathsRel, "%s", IncludePathRel);
}

internal void
LinkLibrary(compilation_target *Target, char const *Library)
{
 StrListPushF(BuildArena, &Target->DynamicLinkLibraries, "%s", Library);
}

internal void
StaticLink(compilation_target *Target, string Link)
{
 StrListPush(BuildArena, &Target->StaticLinks, Link);
}

internal void
ExportFunction(compilation_target *Target, char const *ExportFunc)
{
 StrListPushF(BuildArena, &Target->ExportFunctions, "%s", ExportFunc);
}

internal compile_result
Compile(compiler_setup Setup, compilation_target Target)
{
 compile_result Result = {};
 
 string_list _Cmd = {};
 string_list *Cmd = &_Cmd;
 arena *Arena = BuildArena;
 
 //- establish what is the output target of this compilation
 const char *BuildTargetSuffix = 0;
 if (Target.Flags & CompilationFlag_TemporaryTarget)
 {
  BuildTargetSuffix = "tmp";
 }
 else
 {
  BuildTargetSuffix = (Setup.DebugBuild ? "debug" : "release");
 }
 string TargetName = {};
 {
  string TargetFullName = StrAfterLastSlash(Target.SourcePathRel);
  string TargetNameBase = StrChopLastDot(TargetFullName);
  TargetName = StrF(Arena, "%S_%s", TargetNameBase, BuildTargetSuffix);
 }
 
 // TODO(hbr): I think this is only true for MSVC actually
 char const *TargetExt = 0;
 switch (Target.TargetType)
 {
  case Obj: {TargetExt = "obj";}break;
  case Lib: {TargetExt = "dll";}break;
  case Exe: {TargetExt = "exe";}break;
 }
 string OutputTarget = StrF(Arena, "%S.%s", TargetName, TargetExt);
 Result.OutputTarget = OutputTarget;
 
 //- possibly don't recompile
 if ((Target.Flags & CompilationFlag_DontRecompileIfAlreadyExists) && OS_FileExists(OutputTarget))
 {
  // NOTE(hbr): Nothing to do
 }
 else
 {
  //- common flags - debug, warnings, debug info
  switch (Setup.Compiler)
  {
   case MSVC: {
    StrListPushF(Arena, Cmd, "cl");
    
    if (Setup.DebugBuild)
    {
     StrListPushF(Arena, Cmd, "-Od");
    }
    else
    {
     StrListPushF(Arena, Cmd, "-O2");
     StrListPushF(Arena, Cmd, "-Oi"); // generate intrinsic instructions (possibly)
    }
    if (Setup.GenerateDebuggerInfo)
    {
     StrListPushF(Arena, Cmd, "-Z7"); // full debug information - also in execs/libraries/objs
     if (!Setup.DebugBuild)
     {
      StrListPushF(Arena, Cmd, "-Zo"); // enhanced debug info for optimized builds
     }
    }
    StrListPushF(Arena, Cmd, "-diagnostics:column");
    StrListPushF(Arena, Cmd, "-FS"); // don't lock PDB file - useful when restarting build
    StrListPushF(Arena, Cmd, "-FC"); // display full path of source file when compiling
    StrListPushF(Arena, Cmd, "-nologo");
    StrListPushF(Arena, Cmd, "-WX");
    StrListPushF(Arena, Cmd, "-W4");
    StrListPushF(Arena, Cmd, "-wd4201"); // nonstandard extension used: nameless struct/union
    StrListPushF(Arena, Cmd, "-wd4100"); // unreferenced local parameter
    StrListPushF(Arena, Cmd, "-wd4505"); // unreferenced functions
    StrListPushF(Arena, Cmd, "-wd4189"); // unreferenced variables
    StrListPushF(Arena, Cmd, "-wd4456"); // hiding previous local declaration - shadowing
    StrListPushF(Arena, Cmd, "-EHa-"); // disables exceptions
   }break;
   
   case Clang:
   case GCC: {NotImplemented;}break;
  }
  StrListPushF(Arena, Cmd, Setup.DebugBuild ? "-DBUILD_DEBUG=1" : "-DBUILD_DEBUG=0");
  
  //- include paths
  {
   // NOTE(hbr): always add main directory to includes
   string MainDir = CodePath(StrLit("."));
   StrListPushF(Arena, Cmd, "-I%S", MainDir);
  }
  ListIter(Node, Setup.IncludePathsRel.Head, string_list_node)
  {
   string IncludePathRel = Node->Str;
   string IncludePathAbs = CodePath(IncludePathRel);
   StrListPushF(Arena, Cmd, "-I%S", IncludePathAbs);
  }
  
  //- static links, source
  StrListConcatInPlace(Cmd, &Target.StaticLinks);
  string SourcePath = CodePath(Target.SourcePathRel);
  StrListPush(Arena, Cmd, SourcePath);
  
  //- linking
  switch (Setup.Compiler)
  {
   case MSVC: {
    StrListPushF(Arena, Cmd, "-Fo:%S.obj", TargetName);
    
    switch (Target.TargetType)
    {
     case Lib:
     case Exe: {
      if (Target.TargetType == Lib)
      {
       StrListPushF(Arena, Cmd, "-LD");
      }
      
      StrListPushF(Arena, Cmd, "-link");
      StrListPushF(Arena, Cmd, "-opt:ref"); // eliminates functions and data that are never referenced
      StrListPushF(Arena, Cmd, "-incremental:no"); // don't prepare file for incremental linking
      StrListPushF(Arena, Cmd, "-out:%S", OutputTarget);
      
      ListIter(Node, Target.ExportFunctions.Head, string_list_node)
      {
       string ExportFunc = Node->Str;
       StrListPushF(Arena, Cmd, "-EXPORT:%S", ExportFunc);
      }
      
      StrListConcatInPlace(Cmd, &Target.DynamicLinkLibraries);
     }break;
     
     case Obj: {
      StrListPushF(Arena, Cmd, "-c");
     }break;
    }
   }break;
   
   case Clang:
   case GCC: {NotImplemented;}break;
  }
  
  Result.CompileProcess = OS_LaunchProcess(*Cmd);
 }
 
 return Result;
}

internal void
EnqueueProcess(process_queue *Queue, process Process)
{
 Assert(Queue->ProcessCount < PROCESS_QUEUE_MAX_PROCESS_COUNT);
 Queue->Processes[Queue->ProcessCount++] = Process;
}

internal void
WaitProcesses(process_queue *Queue)
{
 for (u32 ProcessIndex = 0;
      ProcessIndex < Queue->ProcessCount;
      ++ProcessIndex)
 {
  OS_WaitForProcessToFinish(Queue->Processes[ProcessIndex]);
 }
 Queue->ProcessCount = 0;
}

int main(int ArgCount, char *Argv[])
{
 u64 BeginTSC = ReadCPUTimer();
 
 Platform.AllocVirtualMemory = OS_AllocVirtualMemory;
 Platform.DeallocVirtualMemory = OS_DeallocVirtualMemory;
 Platform.CommitVirtualMemory = OS_CommitVirtualMemory;
 
 InitThreadCtx();
 InitOS();
 
 BuildArena = AllocArena();
 arena *Arena = BuildArena;
 
 string ThisProgramExePath = {};
 string ThisProgramBaseName = {};
 {
  string NotFullPath = StrFromCStr(Argv[0]);
  ThisProgramExePath = OS_FullPathFromPath(Arena, NotFullPath);
  
  string WithoutExt = StrChopLastDot(ThisProgramExePath);
  ThisProgramBaseName = PathLastPart(WithoutExt);
 }
 
 b32 Debug = false;
 b32 Release = false;
 b32 ForceRecompile = false;
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
 }
 if (!Debug && !Release)
 {
  // NOTE(hbr): By default build debug
  Debug = true;
 }
 
 b32 Error = false;
 string CurDir = OS_CurrentDir(BuildArena);
 if (StrEndsWith(CurDir, BuildDir) ||
     StrEndsWith(CurDir, CodeDir))
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
  OS_MakeDir(BuildDir);
  if (OS_ChangeDir(BuildDir))
  {
   string ThisProgramSourceFile = StrF(Arena, "%S.cpp", ThisProgramBaseName);
   string ThisProgramSourcePath = CodePath(ThisProgramSourceFile);
   
   // NOTE(hbr): Basically just #include directives at the top of this file
   string_list ThisProgramDeps = {};
   StrListPush(Arena, &ThisProgramDeps, ThisProgramSourcePath);
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_ctx_crack.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_base.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_base.cpp")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_memory.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_memory.cpp")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_string.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_string.cpp")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_os.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_os.cpp")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("editor_platform.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("imgui_bindings.h")));
   StrListPush(Arena, &ThisProgramDeps, CodePath(StrLit("build.h")));
   
   u64 LatestModifyTime = 0;
   ListIter(DependencyFile, ThisProgramDeps.Head, string_list_node)
   {
    file_attrs Attrs = OS_FileAttributes(DependencyFile->Str);
    LatestModifyTime = Max(LatestModifyTime, Attrs.ModifyTime);
   }
   
   file_attrs ExeAttrs = OS_FileAttributes(ThisProgramExePath);
   compiler Compiler = MSVC;
   process_queue ProcessQueue = {};
   
   if (ExeAttrs.ModifyTime < LatestModifyTime)
   {
    LogF("recompiling build program itself");
    
    // NOTE(hbr): Always debug and never debug info to be fast
    // TODO(hbr): Remove debug info generation below here
    compiler_setup Setup = MakeCompilerSetup(Compiler, true, true);
    compilation_target Target = MakeTarget(Exe, ThisProgramSourceFile,
                                           CompilationFlag_TemporaryTarget);
    compile_result BuildCompile = Compile(Setup, Target);
    EnqueueProcess(&ProcessQueue, BuildCompile.CompileProcess);
    WaitProcesses(&ProcessQueue);
    
    //- run this program again, this time with up to date binary
    string_list InvokeBuild = {};
    StrListPush(Arena, &InvokeBuild, BuildCompile.OutputTarget);
    process BuildProcess = OS_LaunchProcess(InvokeBuild);
    EnqueueProcess(&ProcessQueue, BuildProcess);
    WaitProcesses(&ProcessQueue);
   }
   else
   {
    compiler_setup Setup = MakeCompilerSetup(Compiler, Debug, true);
    IncludePath(&Setup, "third_party/imgui");
    
    //- compile renderer into library
    {
     compilation_target Target = MakeTarget(Lib, StrLit("win32_editor_renderer_opengl.cpp"), 0);
     ExportFunction(&Target, "Win32RendererInit");
     ExportFunction(&Target, "Win32RendererBeginFrame");
     ExportFunction(&Target, "Win32RendererEndFrame");
     LinkLibrary(&Target, "Opengl32.lib"); // wgl,glEnable,...
     LinkLibrary(&Target, "Gdi32.lib"); // SwapBuffers,SetPixelFormat,...
     compile_result Renderer = Compile(Setup, Target);
     EnqueueProcess(&ProcessQueue, Renderer.CompileProcess);
    }
    
    //- compile into main executable
    {
     compilation_target Target = MakeTarget(Exe, StrLit("win32_editor.cpp"), 0);
     LinkLibrary(&Target, "User32.lib"); // CreateWindowExA,...
     LinkLibrary(&Target, "Comdlg32.lib"); // GetOpenFileName,...
     compile_result Win32PlatformExe = Compile(Setup, Target);
     EnqueueProcess(&ProcessQueue, Win32PlatformExe.CompileProcess);
    }
    
    //- precompile imgui code into obj
    compile_result ThirdParty = {};
    {
     compilation_flags Flags = (ForceRecompile ? 0 : CompilationFlag_DontRecompileIfAlreadyExists);
     compilation_target Target = MakeTarget(Obj, StrLit("editor_third_party_unity.cpp"), Flags);
     ThirdParty = Compile(Setup, Target);
     OS_WaitForProcessToFinish(ThirdParty.CompileProcess);
    }
    
    //- compile editor code into library
    {
     compilation_target Target = MakeTarget(Lib, StrLit("editor.cpp"), 0);
     ExportFunction(&Target, "EditorUpdateAndRender");
     ExportFunction(&Target, "EditorGetImGuiBindings");
     StaticLink(&Target, ThirdParty.OutputTarget);
     compile_result Editor = Compile(Setup, Target);
     EnqueueProcess(&ProcessQueue, Editor.CompileProcess);
    }
    
    WaitProcesses(&ProcessQueue);
    
    // TODO(hbr): Replace EstimateCPUFreq with a code that directly calculates it, not estimates it
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