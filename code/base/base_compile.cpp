global arena *BuildArena;

internal void
EquipBuild(arena *Arena)
{
 BuildArena = Arena;
}

internal compiler_setup
MakeCompilerSetup(compiler_choice Compiler, compiler_flags Flags)
{
 compiler_setup Result = {};
 switch (Compiler)
 {
  case Compiler_MSVC: {Result.Compiler = MSVC;}break;
  case Compiler_GCC: {Result.Compiler = GCC;}break;
  case Compiler_Clang: {Result.Compiler = Clang;}break;
  case Compiler_Default: {
#if OS_WINDOWS
   Result.Compiler = MSVC;
#elif OS_LINUX
   Result.Compiler = GCC;
#else
   Result.Compiler = GCC;
#endif
  }break;
 }
 Result.Flags = Flags;
 
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
LinkLibrary(compilation_target *Target, string Library)
{
 StrListPush(BuildArena, &Target->DynamicLinkLibraries, Library);
}

internal void
StaticLink(compilation_target *Target, string Link)
{
 StrListPush(BuildArena, &Target->StaticLinks, Link);
}

internal void
DefineMacro(compilation_target *Target, string Name, string Value)
{
 Assert(Target->DefineMacroCount < ArrayCount(Target->DefineMacros));
 define_variable *Define = Target->DefineMacros + Target->DefineMacroCount++;
 Define->Name = Name;
 Define->Value = Value;
}

internal compilation_target_output
ComputeCompilationTargetOutput(compiler_setup Setup, compilation_target Target)
{
 compilation_target_output Result = {};
 
 if (Target.TargetType != Target_None)
 {
  arena *Arena = BuildArena;
  
  //- establish what is the output target of this compilation
  const char *BuildTargetSuffix = 0;
  if (Target.Flags & CompilationFlag_TemporaryTarget)
  {
   BuildTargetSuffix = "tmp";
  }
  else
  {
   BuildTargetSuffix = ((Setup.Flags & CompilerFlag_DebugBuild) ? "debug" : "release");
  }
  
  string TargetFullName = StrAfterLastSlash(Target.SourcePath);
  string TargetNameBase = StrChopLastDot(TargetFullName);
  string TargetName = StrF(Arena, "%S_%s", TargetNameBase, BuildTargetSuffix);
  
  // TODO(hbr): maybe this is true only on OS_WINDOWS - especially dll part, maybe I have to use .so instead, idk
  char const *TargetExt = 0;
  switch (Target.TargetType)
  {
   case Target_Obj: {TargetExt = "obj";}break;
   case Target_Lib: {TargetExt = "dll";}break;
   case Target_Exe: {TargetExt = "exe";}break;
   case Target_None: InvalidPath;
  }
  string OutputTarget = StrF(Arena, "%S.%s", TargetName, TargetExt);
  
  Result.TargetName = TargetName;
  Result.OutputTarget = OutputTarget;
 }
 
 return Result;
}

internal os_process_handle
Compile(compiler_setup Setup, compilation_target Target)
{
 os_process_handle Process = {};
 
 if (Target.TargetType != Target_None)
 {
  arena *Arena = BuildArena;
  string_list _Cmd = {};
  string_list *Cmd = &_Cmd;
  
  compilation_target_output CompilationTargetOutput = ComputeCompilationTargetOutput(Setup, Target);
  string OutputTarget = CompilationTargetOutput.OutputTarget;
  string TargetName = CompilationTargetOutput.TargetName;
  
  //- possibly don't recompile
  if ((Target.Flags & CompilationFlag_DontRecompileIfAlreadyExists) && OS_FileExists(OutputTarget, false))
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
     
     if (Setup.Flags & CompilerFlag_DebugBuild)
     {
      StrListPushF(Arena, Cmd, "-Od");
     }
     else
     {
      StrListPushF(Arena, Cmd, "-O2");
      StrListPushF(Arena, Cmd, "-Oi"); // generate intrinsic instructions (possibly)
     }
     if (Setup.Flags & CompilerFlag_GenerateDebuggerInfo)
     {
      StrListPushF(Arena, Cmd, "-Z7"); // full debug information - also in execs/libraries/objs
      if (!(Setup.Flags & CompilerFlag_DebugBuild))
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
     StrListPushF(Arena, Cmd, "-w44062"); // enable incomplete switch warnings
     StrListPushF(Arena, Cmd, "-wd4201"); // nonstandard extension used: nameless struct/union
     StrListPushF(Arena, Cmd, "-wd4100"); // unreferenced local parameter
     StrListPushF(Arena, Cmd, "-wd4505"); // unreferenced functions
     StrListPushF(Arena, Cmd, "-wd4189"); // unreferenced variables
     StrListPushF(Arena, Cmd, "-wd4456"); // hiding previous local declaration - shadowing
     StrListPushF(Arena, Cmd, "-wd4127"); // constant conditional expression
     StrListPushF(Arena, Cmd, "-EHa-"); // disables exceptions
     
     // TODO(hbr): Investigate this, this is weird, I don't understand something here.
#if 0
     if (Target.TargetType == Target_Exe)
     {
      // NOTE(hbr): I don't know exactly why this is needed. I don't even know if this is needed in
      // general case. I came across this when trying to link GLFW. Not having this caused error:
      // "defaulblib 'MSVCRT' conflicts with use of other libs ..."
      StrListPushF(Arena, Cmd, "-MD");
     }
#endif
    }break;
    
    case GCC:
    case Clang: {
     StrListPushF(Arena, Cmd, (Setup.Compiler == GCC) ? "gcc" : "clang");
     if (Setup.Flags & CompilerFlag_DebugBuild)
     {
      StrListPushF(Arena, Cmd, "-O0");
     }
     else
     {
      StrListPushF(Arena, Cmd, "-Ofast");
     }
     if (Setup.Flags & CompilerFlag_GenerateDebuggerInfo)
     {
      StrListPushF(Arena, Cmd, "-g");
     }
     if (Setup.Compiler == Clang)
     {
      // NOTE(hbr): this options only exists for Clang but we should already
      // feed GCC compiler absolute paths which means that the errors
      // will also be printed out with absolute paths
      StrListPushF(Arena, Cmd, "-fdiagnostics-absolute-paths");
     }
     StrListPushF(Arena, Cmd, "-Wall");
     StrListPushF(Arena, Cmd, "-Wno-missing-braces");
     StrListPushF(Arena, Cmd, "-Wno-unused-but-set-variable");
     StrListPushF(Arena, Cmd, "-Wno-unused-variable");
     StrListPushF(Arena, Cmd, "-Wno-unused-function");
     StrListPushF(Arena, Cmd, "-Wno-char-subscripts");
     StrListPushF(Arena, Cmd, "-Wno-unused-local-typedef");
    }break;
   }
   
   //- define variables
   StrListPushF(Arena, Cmd, (Setup.Flags & CompilerFlag_DebugBuild) ? "-DBUILD_DEBUG=1" : "-DBUILD_DEBUG=0");
   StrListPushF(Arena, Cmd, (Setup.Flags & CompilerFlag_DevBuild) ? "-DBUILD_DEV=1" : "-DBUILD_DEV=0");
   for (u32 MacroIndex = 0;
        MacroIndex < Target.DefineMacroCount;
        ++MacroIndex)
   {
    define_variable *Macro = Target.DefineMacros + MacroIndex;
    StrListPushF(Arena, Cmd, "-D%S=%S", Macro->Name, Macro->Value);
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
      case Target_Lib:
      case Target_Exe: {
       if (Target.TargetType == Target_Lib)
       {
        StrListPushF(Arena, Cmd, "-LD");
       }
       
       StrListPushF(Arena, Cmd, "-link");
       StrListPushF(Arena, Cmd, "-opt:ref"); // eliminates functions and data that are never referenced
       StrListPushF(Arena, Cmd, "-incremental:no"); // don't prepare file for incremental linking
       StrListPushF(Arena, Cmd, "-out:%S", OutputTarget);
       StrListPushF(Arena, Cmd, "-IGNORE:4099"); // ignore PDB was not found with some_library.lib
       
       StrListConcatInPlace(Cmd, &Target.DynamicLinkLibraries);
      }break;
      
      case Target_Obj: {
       StrListPushF(Arena, Cmd, "-c");
      }break;
      
      case Target_None: InvalidPath;
     }
    }break;
    
    case GCC:
    case Clang: {
     switch (Target.TargetType)
     {
      case Target_Lib:
      case Target_Exe: {
       if (Target.TargetType == Target_Lib)
       {
        StrListPushF(Arena, Cmd, "-shared");
       }
       ListIter(Node, Target.DynamicLinkLibraries.Head, string_list_node)
       {
        string LinkLib = Node->Str;
        StrListPushF(Arena, Cmd, "-l%S", LinkLib);
       }
      }break;
      
      case Target_Obj: {
       StrListPushF(Arena, Cmd, "-c");
      }break;
      
      case Target_None: InvalidPath;
     }
     StrListPushF(Arena, Cmd, "-o%S", OutputTarget);
    }break;
   }
   
   if (Setup.Flags & CompilerFlag_Verbose)
   {
    string_list_join_options Opts = {};
    Opts.Sep = StrLit(" ");
    string CmdStr = StrListJoin(Arena, Cmd, Opts);
    OS_PrintF("%S\n", CmdStr);
   }
   
   Process = OS_ProcessLaunch(*Cmd);
  }
 }
 
 return Process;
}

internal void
EnqueueProcess(process_queue *Queue, os_process_handle Process)
{
 Assert(Queue->ProcessCount < PROCESS_QUEUE_MAX_PROCESS_COUNT);
 Queue->Processes[Queue->ProcessCount++] = Process;
}

internal exit_code_int
WaitProcesses(process_queue *Queue)
{
 exit_code_int ExitCode = 0;
 for (u32 ProcessIndex = 0;
      ProcessIndex < Queue->ProcessCount;
      ++ProcessIndex)
 {
  exit_code_int SubProcessExitCode = OS_ProcessWait(Queue->Processes[ProcessIndex]);
  ExitCode = OS_CombineExitCodes(ExitCode, SubProcessExitCode);
 }
 
 return ExitCode;
}