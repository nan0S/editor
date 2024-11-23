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
