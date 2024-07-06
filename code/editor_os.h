#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
# include "editor_windows.h"
#elif OS_LINUX
# include "editor_linux.h"
#else
# error OS layer not implemented
#endif

//- virtual memory
internal void *AllocVirtualMemory(u64 Capacity, b32 Commit);
internal void  CommitVirtualMemory(void *Memory, u64 Size);
internal void  DeallocVirtualMemory(void *Memory, u64 Size);

//- timing
internal u64 ReadOSTimerFrequency(void);
internal u64 ReadOSTimer(void);
internal u64 ReadCPUTimer(void);
internal u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);

//- files
enum
{
   FileAccess_Read  = (1<<0),
   FileAccess_Write = (1<<1),
};
typedef u64 file_access_flags;

internal file_handle OS_OpenFile(string Path, file_access_flags Access);
internal void        OS_CloseFile(file_handle File);
internal u64         OS_ReadFile(file_handle File, char *Buf, u64 Read, u64 Offset = 0);
internal u64         OS_WriteFile(file_handle File, char *Buf, u64 Write, u64 Offset = 0);
internal void        OS_DeleteFile(string Path);
internal u64         OS_FileSize(file_handle File);
internal b32         OS_FileExists(string Path);

typedef b32 success_b32;
internal string      OS_ReadEntireFile(arena *Arena, string Path);
internal success_b32 OS_WriteDataToFile(string Path, string Data);
internal success_b32 OS_WriteDataListToFile(string Path, string_list DataList);

//- output, streams
internal file_handle StdOut(void);
internal file_handle StdErr(void);

internal void OutputFile(file_handle Out, string String);
internal void OutputFile(file_handle Out, char const *String);
internal void OutputFileF(file_handle Out, char const *Format, ...);
internal void OutputFileFV(file_handle Out, char const *Format, va_list Args);
internal void Output(string String);
internal void Output(char const *String);
internal void OutputF(char const *Format, ...);
internal void OutputFV(char const *Format, va_list Args);
internal void OutputError(string String);
internal void OutputError(char const *String);
internal void OutputErrorF(char const *Format, ...);
internal void OutputErrorFV(char const *Format, va_list Args);

//- libraries
internal library_handle OS_LoadLibrary(char const *Name);
internal void *OS_LibraryLoadProc(library_handle Lib, char const *ProcName);
internal void OS_UnloadLibrary(library_handle Lib);

#endif //EDITOR_OS_H
