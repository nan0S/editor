/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

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

internal
OS_THREAD_FUNC(WorkQueueThreadEntry)
{
 ThreadCtxInit();
 
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

internal u32
WorkQueueFreeEntryCount(work_queue *Queue)
{
 u32 Result = ArrayCount(Queue->Entries) - Queue->EntryCount;
 return Result;
}