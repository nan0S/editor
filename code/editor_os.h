#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
# include "editor_win32.h"
#elif OS_LINUX
# include "editor_linux.h"
#else
# error OS layer not implemented
#endif

internal void InitOS(void);

//- virtual memory
internal void *OS_AllocVirtualMemory(u64 Capacity, b32 Commit);
internal void  OS_CommitVirtualMemory(void *Memory, u64 Size);
internal void  OS_DeallocVirtualMemory(void *Memory, u64 Size);

//- misc
internal u64 ReadOSTimer(void);
internal u64 ReadCPUTimer(void);
internal u64 GetOSTimerFreq(void);
internal u64 GetCPUTimerFreq(void);
internal u64 GetProcCount(void);
internal u64 GetPageSize(void);

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

struct dir_entry
{
 string FileName;
 file_attrs Attrs;
};

internal file        OS_OpenFile(string Path, file_access_flags Access);
internal void        OS_CloseFile(file File);
internal u64         OS_ReadFile(file File, char *Buf, u64 Read, u64 Offset = 0);
internal u64         OS_WriteFile(file File, char *Buf, u64 Write, u64 Offset = 0);
internal u64         OS_FileSize(file File);
internal file_attrs  OS_FileAttributes(string Path);

internal b32         OS_IterDir(arena *Arena, string Path, dir_iter *Iter, dir_entry *OutEntry);

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
internal file StdOutput(void);
internal file StdError(void);

internal void OutputFile(file Out, string String);
internal void OutputFile(file Out, char const *String);
internal void OutputFileF(file Out, char const *Format, ...);
internal void OutputFileFV(file Out, char const *Format, va_list Args);
internal void Output(string String);
internal void Output(char const *String);
internal void OutputF(char const *Format, ...);
internal void OutputFV(char const *Format, va_list Args);
internal void OutputError(string String);
internal void OutputError(char const *String);
internal void OutputErrorF(char const *Format, ...);
internal void OutputErrorFV(char const *Format, va_list Args);
internal void OutputDebug(string String);
internal void OutputDebugF(char const *Format, ...);
internal void OutputDebugFV(char const *Format, va_list Args);

//- libraries
internal library OS_LoadLibrary(char const *Name);
internal void *  OS_LoadLibraryProc(library Lib, char const *ProcName);
internal void    OS_UnloadLibrary(library Lib);

//- process, threads
internal process OS_LaunchProcess(string_list CmdList);
internal b32     OS_WaitForProcessToFinish(process Process);
internal void    OS_CleanupAfterProcess(process Handle);

typedef OS_THREAD_FUNC(thread_func);
internal thread OS_LaunchThread(thread_func *Func, void *Data);
internal void   OS_WaitThread(thread Thread);
internal void   OS_ReleaseThread(thread Thread);

//- synchronization
internal void InitMutex(mutex *Mutex);
internal void LockMutex(mutex *Mutex);
internal void UnlockMutex(mutex *Mutex);
internal void DestroyMutex(mutex *Mutex);

internal void InitSemaphore(semaphore *Sem, u32 InitialCount, u32 MaxCount);
internal void PostSemaphore(semaphore *Sem);
internal void WaitSemaphore(semaphore *Sem);
internal void DestroySemaphore(semaphore *Sem);

internal void InitBarrier(barrier *Barrier, u64 ThreadCount);
internal void DestroyBarrier(barrier *Barrier);
internal void WaitBarrier(barrier *Barrier);

internal u64 InterlockedIncr(u64 volatile *Value);
internal u64 InterlockedAdd(u64 volatile *Value, u64 Add);
internal u64 InterlockedCmpExch(u64 volatile *Value, u64 Cmp, u64 Exch);

#define TEST_WORK_QUEUE 1
#if !defined(TEST_WORK_QUEUE)
# define TEST_WORK_QUEUE 0
#endif
struct work_queue_entry
{
 void (*Func)(void *Data);
 void *Data;
#if TEST_WORK_QUEUE
 b32 Completed;
#endif
};
struct work_queue
{
 work_queue_entry Entries[1024];
 u64 volatile NextEntryToWrite;
 u64 volatile NextEntryToRead;
 u64 volatile CompletionCount;
 u64 EntryCount;
 semaphore Semaphore;
};
internal void InitWorkQueue(work_queue *Queue, u64 ThreadCount);
internal void PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data);
internal void CompleteAllWork(work_queue *Queue);

#endif //EDITOR_OS_H
