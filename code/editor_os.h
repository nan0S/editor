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
internal void *OS_AllocVirtualMemory(u64 Capacity, b32 Commit);
internal void  OS_CommitVirtualMemory(void *Memory, u64 Size);
internal void  OS_DeallocVirtualMemory(void *Memory, u64 Size);

//- misc
internal u64 ReadOSTimerFreq(void);
internal u64 ReadOSTimer(void);
internal u64 ReadCPUTimer(void);
internal u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);
internal u64 GetProcCount(void);
internal u64 GetPageSize(void);

//- time
internal date_time   TimestampToDateTime(timestamp64 Ts);
internal timestamp64 DateTimeToTimestamp(date_time Dt);

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
internal file_handle StdOutput(void);
internal file_handle StdError(void);

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
internal void OutputDebug(string String);
internal void OutputDebugF(char const *Format, ...);
internal void OutputDebugFV(char const *Format, va_list Args);

//- libraries
internal library_handle OS_LoadLibrary(char const *Name);
internal void *         OS_LibraryLoadProc(library_handle Lib, char const *ProcName);
internal void           OS_UnloadLibrary(library_handle Lib);

//- process, threads
internal process_handle OS_LaunchProcess(string_list CmdList);
internal b32            OS_WaitForProcessToFinish(process_handle Process);
internal void           OS_CleanupAfterProcess(process_handle Handle);

typedef OS_THREAD_FUNC(thread_func);
internal thread_handle OS_LaunchThread(thread_func *Func, void *Data);
internal void          OS_WaitThread(thread_handle Thread);
internal void          OS_ReleaseThreadHandle(thread_handle Thread);

//- synchronization
internal void InitMutex(mutex_handle *Mutex);
internal void LockMutex(mutex_handle *Mutex);
internal void UnlockMutex(mutex_handle *Mutex);
internal void DestroyMutex(mutex_handle *Mutex);

internal void InitSemaphore(semaphore_handle *Sem, u64 InitialCount, u64 MaxCount);
internal void PostSemaphore(semaphore_handle *Sem);
internal void WaitSemaphore(semaphore_handle *Sem);
internal void DestroySemaphore(semaphore_handle *Sem);

internal void InitBarrier(barrier_handle *Barrier, u64 ThreadCount);
internal void DestroyBarrier(barrier_handle *Barrier);
internal void WaitBarrier(barrier_handle *Barrier);

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
   semaphore_handle Semaphore;
};

internal void InitWorkQueue(work_queue *Queue, u64 ThreadCount);
internal void PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data);
internal void CompleteAllWork(work_queue *Queue);

#endif //EDITOR_OS_H
