#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_thread_ctx.h"
#include "base/base_os.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_os.cpp"

#ifndef EDITOR_WORK_QUEUE_H
#define EDITOR_WORK_QUEUE_H

typedef void work_queue_func(void *UserData);

struct work_queue_entry
{
 work_queue_func *Func;
 void *UserData;
};

struct work_queue
{
 u32 volatile EntryCount;
 u32 volatile CompletionCount;
 u32 volatile NextEntryToRead;
 u32 volatile NextEntryToWrite;
 os_semaphore_handle Semaphore;
 work_queue_entry Entries[4096];
};

internal void WorkQueueInit(work_queue *Queue, u32 ThreadCount);
internal void WorkQueueAddEntry(work_queue *Queue, work_queue_func *Func, void *UserData);
internal void WorkQueueCompleteAllWork(work_queue *Queue);

#endif //EDITOR_WORK_QUEUE_H
internal b32
DoWork(work_queue *Queue)
{
 b32 ShouldSleep = false;
 
 u32 EntryIndex = Queue->NextEntryToRead;
 u32 NextEntryIndex = (EntryIndex + 1) % ArrayCount(Queue->Entries);
 u32 NextEntryToWrite = Queue->NextEntryToWrite;
 if (EntryIndex != NextEntryToWrite)
 {
  if (OS_AtomicCmpExch32(&Queue->NextEntryToRead, EntryIndex, NextEntryIndex) == EntryIndex)
  {
   work_queue_entry Entry = Queue->Entries[EntryIndex];
   Entry.Func(Entry.UserData);
   OS_AtomicIncr32(&Queue->CompletionCount);
  }
 }
 else
 {
  ShouldSleep = true;
 }
 
 return ShouldSleep;
}

OS_THREAD_FUNC(WorkQueueThreadEntry)
{
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0;
      ArenaIndex < ArrayCount(Arenas);
      ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena(Gigabytes(10));
 }
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));
 
 work_queue *Queue = Cast(work_queue *)ThreadEntryDataPtr;
 for (;;)
 {
  if (DoWork(Queue))
  {
   OS_SemaphoreWait(&Queue->Semaphore);
  }
 }
 
 // NOTE(hbr): compiler complains about unreachable code
#if 0
 ThreadCtxDealloc();
 return 0;
#endif
}

internal void
WorkQueueAddEntry(work_queue *Queue, work_queue_func *Func, void *UserData)
{
 u32 EntryIndex = Queue->NextEntryToWrite;
 u32 NextEntryIndex = (EntryIndex + 1) % ArrayCount(Queue->Entries);
 
 work_queue_entry *Entry = Queue->Entries + EntryIndex;
 Entry->Func = Func;
 Entry->UserData = UserData;
 
 CompilerWriteBarrier;
 
 Queue->NextEntryToWrite = NextEntryIndex;
 ++Queue->EntryCount;
 OS_SemaphorePost(&Queue->Semaphore);
}

internal void
WorkQueueCompleteAllWork(work_queue *Queue)
{
 while (Queue->CompletionCount != Queue->EntryCount)
 {
  DoWork(Queue);
 }
 
 Queue->CompletionCount = 0;
 Queue->EntryCount = 0;
}

internal void
WorkQueueInit(work_queue *Queue, u32 ThreadCount)
{
 OS_SemaphoreAlloc(&Queue->Semaphore, 0, ThreadCount);
 for (u32 ThreadIndex = 0;
      ThreadIndex < ThreadCount;
      ++ThreadIndex)
 {
  OS_ThreadLaunch(WorkQueueThreadEntry, Queue);
 }
}

internal void
PrintStringWork(void *Data)
{
 string *StringToPrint = Cast(string *)Data;
 OS_PrintF("%S\n", *StringToPrint);
}

struct thread_input
{
 os_barrier_handle *Barrier;
 b32 Sleep;
};

OS_THREAD_FUNC(WaitAndPrint)
{
 thread_input *Input = Cast(thread_input *)ThreadEntryDataPtr;
 OS_Print(StrLit("before\n"));
 if (Input->Sleep) sleep(2);
 OS_BarrierWait(Input->Barrier);
 OS_Print(StrLit("after\n"));

 return 0;
}

global volatile u64 GlobalCounter;

OS_THREAD_FUNC(IncrementPlain)
{
 for (u32 I = 0; I < 1024; ++I)
 {
  ++GlobalCounter;
 }
 return 0;
}

OS_THREAD_FUNC(IncrementAtomic)
{
 for (u32 I = 0; I < 1024; ++I)
 {
  OS_AtomicIncr64(&GlobalCounter);
 }

 return 0;
}

OS_THREAD_FUNC(IncrementMutex)
{
  for (u32 I = 0; I < 1024; ++I)

{
 os_mutex_handle *Mutex = Cast(os_mutex_handle *)ThreadEntryDataPtr;

 OS_MutexLock(Mutex);
 ++GlobalCounter;
 OS_MutexUnlock(Mutex);

}
 return 0;
}

int main()
{
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0; ArenaIndex < ArrayCount(Arenas); ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena(Gigabytes(64));
 } 
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));

 arena *Arena = Arenas[0];

 OS_FileAttributes(StrLit("data.txt"));

#if 0
 work_queue _Queue = {};
 work_queue *Queue = &_Queue;
 u32 ThreadCount = OS_ProcCount();
 WorkQueueInit(Queue, ThreadCount);

 u32 WorkCount = 4096;
 string *StringsToPrint = PushArray(Arena, WorkCount, string);
 for (u32 WorkIndex = 0; WorkIndex < WorkCount; ++WorkIndex)
 {
  string *StringToPrint = StringsToPrint + WorkIndex;
  *StringToPrint = StrF(Arena, "Hello %u", WorkIndex);
  WorkQueueAddEntry(Queue, PrintStringWork, StringToPrint); 
 }

 WorkQueueCompleteAllWork(Queue);
#endif

#if 0
 {
  os_barrier_handle Barrier = {};
  u32 ThreadCount = 4;
  OS_BarrierAlloc(&Barrier, ThreadCount);
  os_thread_handle *Threads = PushArray(Arena, ThreadCount, os_thread_handle);
  thread_input *Inputs = PushArray(Arena, ThreadCount, thread_input);
  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  {
   thread_input *Input = Inputs + ThreadIndex;
   Input->Barrier = &Barrier;
   Input->Sleep = (ThreadIndex == 0);
   os_thread_handle Thread = OS_ThreadLaunch(WaitAndPrint, Input);
   Threads[ThreadIndex] = Thread; 
  }

  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  {
   os_thread_handle Thread = Threads[ThreadIndex];
   OS_ThreadWait(Thread);
  }

  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  {
   os_thread_handle Thread = Threads[ThreadIndex];
   OS_ThreadRelease(Thread);
  }

 }
#endif

#if 0
{
 u32 ThreadCount = 1024;
 os_thread_handle *Threads = PushArray(Arena, ThreadCount, os_thread_handle);
 for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = OS_ThreadLaunch(IncrementPlain, 0);
  Threads[ThreadIndex] = Thread;
 }
 for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = Threads[ThreadIndex];
  OS_ThreadWait(Thread);
 }
 OS_PrintF("GlobalCounter: %lu\n", GlobalCounter);
 GlobalCounter = 0;
  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = OS_ThreadLaunch(IncrementAtomic, 0);
  Threads[ThreadIndex] = Thread;
 }
 for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = Threads[ThreadIndex];
  OS_ThreadWait(Thread);
 }
 OS_PrintF("GlobalCounter: %lu\n", GlobalCounter);
 GlobalCounter = 0;
 os_mutex_handle Mutex = {};
 OS_MutexAlloc(&Mutex);
  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = OS_ThreadLaunch(IncrementMutex, &Mutex);
  Threads[ThreadIndex] = Thread;
 }
 for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
 {
  os_thread_handle Thread = Threads[ThreadIndex];
  OS_ThreadWait(Thread);
 }
 OS_PrintF("GlobalCounter: %lu\n", GlobalCounter);
 GlobalCounter = 0;
 OS_MutexDealloc(&Mutex);
}
#endif

#if 0
 os_library_handle Library = OS_LibraryLoad(StrLit("build/glfw_library_debug.dll"));
 void *AddVoid = OS_LibraryProc(Library, "add");
 int (*Add)(int, int) = Cast(int(*)(int, int))AddVoid;
 int Result = Add(1, 2);
 Trap;
#endif

#if 0
 string_list Cmd = {};
 StrListPush(Arena, &Cmd, StrLit("ls"));
 StrListPush(Arena, &Cmd, StrLit("-la"));
 os_process_handle Process = OS_ProcessLaunch(Cmd);
 b32 Success = OS_ProcessWait(Process);
 OS_ProcessCleanup(Process);
 Trap;
#endif

 b32 Success = OS_FileCopy(StrLit("a.txt"), StrLit("b.txt"));
 Trap;

 return 0;
}