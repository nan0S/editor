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

 return 0;
}