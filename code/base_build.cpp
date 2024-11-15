global arena *BuildArena;

internal void
InitBuild(int ArgCount, char *Argv[])
{
 BuildArena = AllocArena(Megabytes(2));
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
MakeTarget(compilation_target_type TargetType, string SourcePath, compilation_flags Flags)
{
 compilation_target Result = {};
 Result.TargetType = TargetType;
 Result.SourcePath = SourcePath;
 Result.Flags = Flags;
 
 return Result;
}

internal void
IncludePath(compiler_setup *Setup, string IncludePath)
{
 StrListPush(BuildArena, &Setup->IncludePaths, IncludePath);
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
 
 arena *Arena = BuildArena;
 string_list _Cmd = {};
 string_list *Cmd = &_Cmd;
 
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
  string TargetFullName = StrAfterLastSlash(Target.SourcePath);
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
   string MainDir = OS_FullPathFromPath(Arena, StrLit("../code"));
   StrListPushF(Arena, Cmd, "-I%S", MainDir);
  }
  ListIter(Node, Setup.IncludePaths.Head, string_list_node)
  {
   string IncludePath = Node->Str;
   StrListPushF(Arena, Cmd, "-I%S", IncludePath);
  }
  
  //- static links, source
  StrListConcatInPlace(Cmd, &Target.StaticLinks);
  StrListPush(Arena, Cmd, Target.SourcePath);
  
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
  
  Result.CompileProcess = OS_ProcessLaunch(*Cmd);
 }
 
 return Result;
}

internal void
EnqueueProcess(process_queue *Queue, os_process_handle Process)
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
  OS_ProcessWait(Queue->Processes[ProcessIndex]);
 }
 Queue->ProcessCount = 0;
}