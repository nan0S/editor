internal b32
DoWork(work_queue *Queue)
{
 b32 ShouldSleep = false;
 
 u32 EntryIndex = Queue->NextEntryToRead;
 u32 NextEntryIndex = EntryIndex + 1;
 if (EntryIndex != Queue->NextEntryToWrite)
 {
  if (OS_AtomicCmpExch32(&Queue->NextEntryToRead, EntryIndex, NextEntryIndex) == EntryIndex)
  {
   work_queue_entry *Entry = Queue->Entries + EntryIndex;
   Entry->Func(Entry->UserData);
   ++Entry->ExecutedCount;
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
 ThreadCtxAlloc();
 
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
 
 work_queue_entry *Entry = Queue->Entries + EntryIndex;
 Entry->Func = Func;
 Entry->UserData = UserData;
 
 Queue->NextEntryToWrite = EntryIndex + 1;
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
