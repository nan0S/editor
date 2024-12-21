#ifndef BASE_COMPILE_H
#define BASE_COMPILE_H

/*

example code:

compiler_setup Setup = MakeCompilerSetup(Clang, Debug, ...);

compilation_target Target1 = MakeTarget(Obj, PathToSourceCode, Flags);
LinkLibrary(&Target1, StrLit("User32.lib"));
LinkLibrary(&Target1, ...);
DefineMacro(&Target1, ...);
...
compilation_command Command1 = ComputeCompilationCommand(Setup, Target1);
os_process_handle Process1 = OS_ProcessLaunch(Command1.Cmd);

compilation_target Target2 = MakeTarget(Exe, PathToSourceCode, Flags);
LinkLibrary(&Target2, ...);
compilation_command Command2 = ComputeCompilationCommand(Setup, Target2);
os_process_handle Process2 = OS_ProcessLaunch(Command2.Cmd);

OS_ProcessWait(Process1);
OS_ProcessWait(Process2);

*/

enum compiler_choice
{
 Compiler_Default,
 Compiler_MSVC,
 Compiler_GCC,
 Compiler_Clang,
};
enum compiler
{
 MSVC,
 GCC,
 Clang,
};

enum compilation_target_type
{
 Exe,
 Obj,
 Lib,
};

struct compiler_setup
{
 b32 DebugBuild;
 b32 GenerateDebuggerInfo;
 b32 Verbose; // NOTE(hbr): print commands
 compiler Compiler;
 string_list IncludePaths;
};

enum
{
 CompilationFlag_TemporaryTarget = (1<<0),
 CompilationFlag_DontRecompileIfAlreadyExists = (1<<1),
};
typedef u32 compilation_flags;

struct define_variable
{
 string Name;
 string Value;
};

struct compilation_target
{
 compilation_target_type TargetType;
 string SourcePath;
 compilation_flags Flags;
 string_list StaticLinks;
 string_list DynamicLinkLibraries;
 u32 DefineMacroCount;
 define_variable DefineMacros[16];
};

struct compilation_command
{
 string OutputTarget;
 string_list Cmd;
};

struct process_queue
{
#define PROCESS_QUEUE_MAX_PROCESS_COUNT 256
 os_process_handle Processes[PROCESS_QUEUE_MAX_PROCESS_COUNT];
 u32 ProcessCount;
};

internal void EquipBuild(arena *Arena);

internal compiler_setup MakeCompilerSetup(compiler_choice Compiler, b32 DebugBuild, b32 GenerateDebuggerInfo, b32 Verbose);
internal compilation_target MakeTarget(compilation_target_type TargetType, string SourcePathRel, compilation_flags Flags);
internal void IncludePath(compiler_setup *Setup, string IncludePathRel);
internal void LinkLibrary(compilation_target *Target, string Library);
internal void StaticLink(compilation_target *Target, string Link);
internal void DefineVariable(compilation_target *Target, string Name, string Value);

internal compilation_command ComputeCompilationCommand(compiler_setup Setup, compilation_target Target);

internal void EnqueueProcess(process_queue *Queue, os_process_handle Process);
internal void WaitProcesses(process_queue *Queue);

#endif //BASE_COMPILE_H
