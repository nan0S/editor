#ifndef BASE_BUILD_H
#define BASE_BUILD_H

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
 compiler Compiler;
 string_list IncludePaths;
};

enum
{
 CompilationFlag_TemporaryTarget = (1<<0),
 CompilationFlag_DontRecompileIfAlreadyExists = (1<<1),
};
typedef u32 compilation_flags;

struct compilation_target
{
 compilation_target_type TargetType;
 string SourcePath;
 compilation_flags Flags;
 string_list StaticLinks;
 string_list DynamicLinkLibraries;
 string_list ExportFunctions;
};

struct compile_result
{
 os_process_handle CompileProcess;
 string OutputTarget;
};

struct process_queue
{
#define PROCESS_QUEUE_MAX_PROCESS_COUNT 256
 os_process_handle Processes[PROCESS_QUEUE_MAX_PROCESS_COUNT];
 u32 ProcessCount;
};

internal void InitBuild(void);

internal compiler_setup MakeCompilerSetup(compiler Compiler, b32 DebugBuild, b32 GenerateDebuggerInfo);
internal void IncludePath(compiler_setup *Setup, string IncludePath);

internal compilation_target MakeTarget(compilation_target_type TargetType, string SourcePath, compilation_flags Flags);
internal void ExportFunction(compilation_target *Target, char const *ExportFunc);
internal void LinkLibrary(compilation_target *Target, char const *Library);
internal void StaticLink(compilation_target *Target, string Link);

internal compile_result Compile(compiler_setup Setup, compilation_target Target);

internal void EnqueueProcess(process_queue *Queue, os_process_handle Process);
internal void WaitProcesses(process_queue *Queue);

#endif //BASE_BUILD_H
