#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
# include "editor_win32.h"
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
internal u64 ReadOSTimerFreq(void);
internal u64 ReadOSTimer(void);
internal u64 ReadCPUTimer(void);
internal u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);

//- time

//- files
enum
{
   FileAccess_Read  = (1<<0),
   FileAccess_Write = (1<<1),
};
typedef u64 file_access_flags;

struct file_attrs
{
   timestamp64 CreateTime;
   timestamp64 ModifyTime;
   u64 FileSize;
   b32 Dir;
};

internal file_handle OS_OpenFile(string Path, file_access_flags Access);
internal void        OS_CloseFile(file_handle File);
internal u64         OS_ReadFile(file_handle File, char *Buf, u64 Read, u64 Offset = 0);
internal u64         OS_WriteFile(file_handle File, char *Buf, u64 Write, u64 Offset = 0);
internal u64         OS_FileSize(file_handle File);
internal file_attrs  OS_FileAttributes(string Path);

internal void        OS_DeleteFile(string Path);
internal b32         OS_MoveFile(string Src, string Dest);
internal b32         OS_CopyFile(string Src, string Dest);
internal b32         OS_FileExists(string Path);
internal b32         OS_MakeDir(string Path);
internal b32         OS_RemoveDir(string Path);
internal b32         OS_ChangeDir(string Path);
internal string      OS_CurrentDir(arena *Arena);
internal string      OS_FullPathFromPath(arena *Arena, string Path);

typedef b32 success_b32;
internal string      OS_ReadEntireFile(arena *Arena, string Path);
internal success_b32 OS_WriteDataToFile(string Path, string Data);
internal success_b32 OS_WriteDataListToFile(string Path, string_list DataList);

//- output, streams
// TODO(hbr): rename
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

//- process
struct process_handle
{
   STARTUPINFOA StartupInfo;
   PROCESS_INFORMATION ProcessInfo;
};

// TODO(hbr): Move to cpp file
internal process_handle
OS_LaunchProcess(string_list CmdList)
{
   temp_arena Temp = TempArena(0);
   
   process_handle Handle = {};
   Handle.StartupInfo.cb = SizeOf(Handle.StartupInfo);
   DWORD CreationFlags = 0;
   string Cmd = StrListJoin(Temp.Arena, &CmdList, StrLit(" "));
   CreateProcessA(0, Cmd.Data, 0, 0, 0, CreationFlags, 0, 0,
                  &Handle.StartupInfo, &Handle.ProcessInfo);
   EndTemp(Temp);
   
   return Handle;
}

internal b32
OS_WaitForProcessToFinish(process_handle Process)
{
   b32 Success = (WaitForSingleObject(Process.ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0);
   return Success;
}

internal void
OS_CleanupAfterProcess(process_handle Handle)
{
   CloseHandle(Handle.StartupInfo.hStdInput);
   CloseHandle(Handle.StartupInfo.hStdOutput);
   CloseHandle(Handle.StartupInfo.hStdError);
   CloseHandle(Handle.ProcessInfo.hProcess);
   CloseHandle(Handle.ProcessInfo.hThread);
}

#endif //EDITOR_OS_H
