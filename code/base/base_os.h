#ifndef BASE_OS_H
#define BASE_OS_H

#if OS_WINDOWS
# include "base_os_win32.h"
#elif OS_LINUX
# include "base_os_linux.h"
#else
# error unsupported OS
#endif

internal void OS_Init(int ArgCount, char *Args[]);

//- memory
internal void *OS_Reserve(u64 Reserve, b32 Commit);
internal void  OS_Release(void *Memory, u64 Size);
internal void  OS_Commit(void *Memory, u64 Size);
internal void  OS_Decommit(void *Memory, u64 Size);

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

internal os_file_handle OS_FileOpen(string Path, file_access_flags Access);
internal void           OS_FileClose(os_file_handle File);
internal u64            OS_FileRead(os_file_handle File, char *Buf, u64 Read, u64 Offset = U64_MAX);
internal u64            OS_FileWrite(os_file_handle File, char *Buf, u64 Write, u64 Offset = U64_MAX);
internal u64            OS_FileSize(os_file_handle File);
internal file_attrs     OS_FileAttributes(string Path);
internal b32            OS_FileValid(os_file_handle File);

struct dir_entry
{
 string FileName;
 file_attrs Attrs;
};

internal string      OS_ReadEntireFile(arena *Arena, string Path);
internal success_b32 OS_WriteDataToFile(string Path, string Data);
internal success_b32 OS_WriteDataListToFile(string Path, string_list DataList);

//- directories
// repeatadly call with initially zero-initialized Iter, if you want to store dir_entry's FileName, you have to copy it
internal success_b32 OS_IterDir(string Path, dir_iter *Iter, dir_entry *OutEntry);

//- stdout, stderr, debug
internal os_file_handle OS_StdOut(void);
internal os_file_handle OS_StdError(void);

internal void OS_Print(string Str);
internal void OS_PrintError(string Str);
internal void OS_PrintDebug(string Str);
internal void OS_PrintFile(os_file_handle File, string Str);

internal void OS_PrintF(char const *Format, ...);
internal void OS_PrintErrorF(char const *Format, ...);
internal void OS_PrintDebugF(char const *Format, ...);
internal void OS_PrintFileF(os_file_handle File, char const *Format, ...);

internal void OS_PrintFV(char const *Format, va_list Args);
internal void OS_PrintErrorFV(char const *Format, va_list Args);
internal void OS_PrintDebugFV(char const *Format, va_list Args);
internal void OS_PrintFileFV(os_file_handle File, char const *Format, va_list Args);

//- file system, paths
internal success_b32 OS_FileDelete(string Path);
internal success_b32 OS_FileMove(string Src, string Dst);
internal success_b32 OS_FileCopy(string Src, string Dst);
internal success_b32 OS_FileExists(string Path);
internal success_b32 OS_DirMake(string Path);
internal success_b32 OS_DirRemove(string Path);
internal success_b32 OS_DirChange(string Path);
internal string      OS_CurrentDir(arena *Arena);
internal string      OS_FullPathFromPath(arena *Arena, string Path);
internal string      OS_BinaryRelativeToFullPath(arena *Arena, string Rel);

//- libraries
internal os_library_handle OS_LibraryLoad(string Path);
internal void *            OS_LibraryProc(os_library_handle Lib, char const *ProcName);
internal void              OS_LibraryUnload(os_library_handle Lib);

//- processes
typedef int exit_code_int;
internal os_process_handle OS_ProcessLaunch(string_list CmdList);
internal exit_code_int     OS_ProcessWait(os_process_handle Process);

//- threads
typedef OS_THREAD_FUNC(os_thread_func);
internal os_thread_handle OS_ThreadLaunch(os_thread_func *Func, void *Data);
internal void             OS_ThreadWait(os_thread_handle Thread);
internal void             OS_ThreadRelease(os_thread_handle Thread);

//- mutex
internal void OS_MutexAlloc(os_mutex_handle *Mutex);
internal void OS_MutexLock(os_mutex_handle *Mutex);
internal void OS_MutexUnlock(os_mutex_handle *Mutex);
internal void OS_MutexDealloc(os_mutex_handle *Mutex);

//- semaphore
internal void OS_SemaphoreAlloc(os_semaphore_handle *Sem, u32 InitialCount, u32 MaxCount);
internal void OS_SemaphorePost(os_semaphore_handle *Sem);
internal void OS_SemaphoreWait(os_semaphore_handle *Sem);
internal void OS_SemaphoreDealloc(os_semaphore_handle *Sem);

//- barrier
internal void OS_BarrierAlloc(os_barrier_handle *Barrier, u32 ThreadCount);
internal void OS_BarrierWait(os_barrier_handle *Barrier);
internal void OS_BarrierDealloc(os_barrier_handle *Barrier);

//- atomics
internal u64 OS_AtomicIncr64(u64 volatile *Value);
internal u64 OS_AtomicAdd64(u64 volatile *Value, u64 Add);
internal u64 OS_AtomicCmpExch64(u64 volatile *Value, u64 Cmp, u64 Exch); // returns incre

internal u32 OS_AtomicIncr32(u32 volatile *Value);
internal u32 OS_AtomicAdd32(u32 volatile *Value, u32 Add);
internal u32 OS_AtomicCmpExch32(u32 volatile *Value, u32 Cmp, u32 Exch);

//- misc
internal u64 OS_ReadCPUTimer(void);
internal u64 OS_CPUTimerFreq(void);
internal u64 OS_ReadOSTimer(void);
internal u64 OS_OSTimerFreq(void);

internal u32 OS_ProcCount(void);
internal u32 OS_PageSize(void);
internal u32 OS_ThreadGetID(void);

internal void OS_Sleep(u64 Milliseconds);

#endif //BASE_OS_H
