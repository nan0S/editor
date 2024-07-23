#if OS_WINDOWS
# include "editor_win32.cpp"
#elif OS_LINUX
# include "editor_linux.cpp"
#endif

internal u64
ReadCPUTimer(void)
{
   return __rdtsc();
}

internal u64
EstimateCPUFrequency(u64 GuessSampleTimeMs)
{
   u64 OSFreq = ReadOSTimerFrequency();
   u64 OSWaitCount = OSFreq * GuessSampleTimeMs / 1000;
   u64 OSElapsed = 0;
   
   u64 CPUBegin = ReadCPUTimer();
   u64 OSBegin = ReadOSTimer();
   while (OSElapsed < OSWaitCount)
   {
      u64 OSEnd = ReadOSTimer();
      OSElapsed = OSEnd - OSBegin;
   }
   u64 CPUEnd = ReadCPUTimer();
   
   u64 CPUElapsed = CPUEnd - CPUBegin;
   u64 CPUFreq = OSFreq * CPUElapsed / OSElapsed;
   
   return CPUFreq;
}

internal string
OS_ReadEntireFile(arena *Arena, string Path)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Read);
   u64 Size = OS_FileSize(File);
   char *Data = PushArrayNonZero(Arena, Size, char);
   u64 Read = OS_ReadFile(File, Data, Size);
   string Result = Str(Data, Read);
   OS_CloseFile(File);
   
   return Result;
}

internal b32
OS_WriteDataToFile(string Path, string Data)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Write);
   u64 Written = OS_WriteFile(File, Data.Data, Data.Count);
   b32 Success = (Written == Data.Count);
   OS_CloseFile(File);
   
   return Success;
}

internal b32
OS_WriteDataListToFile(string Path, string_list DataList)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Write);
   b32 Success = true;
   u64 Offset = 0;
   ListIter(Node, DataList.Head, string_list_node)
   {
      string Data = Node->String;
      u64 Written = OS_WriteFile(File, Data.Data, Data.Count, Offset);
      Offset += Written;
      if (Written != Data.Count)
      {
         Success = false;
         break;
      }
   }
   OS_CloseFile(File);
   
   return Success;
}

internal void OutputFile(file_handle Out, string String) { OS_WriteFile(Out, String.Data, String.Count); }
internal void Output(string String) { OutputFile(StdOutput(), String); }
internal void OutputFV(char const *Format, va_list Args) { OutputFileFV(StdOutput(), Format, Args); }
internal void OutputError(string String) { OutputFile(StdError(), String); }
internal void OutputErrorFV(char const *Format, va_list Args) { OutputFileFV(StdError(), Format, Args); }
internal void OutputFile(file_handle Out, char const *String) { OutputFile(Out, StrFromCStr(String)); }
internal void Output(char const *String) { Output(StrFromCStr(String)); }
internal void OutputError(char const *String) { OutputError(StrFromCStr(String)); }

internal void
OutputFileF(file_handle Out, char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(Out, Format, Args);
   va_end(Args);
}

internal void
OutputFileFV(file_handle Out, char const *Format, va_list Args)
{
   temp_arena Temp = TempArena(0);
   string String = StrFV(Temp.Arena, Format, Args);
   OutputFile(Out, String);
   EndTemp(Temp);
}

internal void
OutputF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(StdOutput(), Format, Args);
   va_end(Args);
}

internal void
OutputErrorF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(StdError(), Format, Args);
   va_end(Args);
}

internal void
OutputDebugF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   OutputDebugFV(Format, Args);
   va_end(Args);
   EndTemp(Temp);
}

internal void
OutputDebugFV(char const *Format, va_list Args)
{
   temp_arena Temp = TempArena(0);
   string String = StrFV(Temp.Arena, Format, Args);
   OutputDebug(String);
   EndTemp(Temp);
}

internal void
PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data)
{
   ++Queue->EntryCount;
   Assert(Queue->EntryCount < ArrayCount(Queue->Entries));
   
   u64 EntryIndex = Queue->NextEntryToWrite;
   work_queue_entry *Entry = Queue->Entries + EntryIndex;
   Entry->Func = Func;
   Entry->Data = Data;
#if TEST_WORK_QUEUE
   Entry->Completed = false;
#endif
   
   CompilerWriteBarrier;
   
   Queue->NextEntryToWrite = (EntryIndex + 1) % ArrayCount(Queue->Entries);
   PostSemaphore(&Queue->Semaphore);
}

internal b32
DoWork(work_queue *Queue)
{
   b32 ShouldSleep = false;
   
   u64 EntryIndex = Queue->NextEntryToRead;
   if (EntryIndex != Queue->NextEntryToWrite)
   {
      u64 Exchange = (EntryIndex + 1) % ArrayCount(Queue->Entries);
      if (InterlockedCmpExch(&Queue->NextEntryToRead, EntryIndex, Exchange) == EntryIndex)
      {
         work_queue_entry *Entry = Queue->Entries + EntryIndex;
         Entry->Func(Entry->Data);
#if TEST_WORK_QUEUE
         Entry->Completed = true;
#endif
         InterlockedIncr(&Queue->CompletionCount);
      }
   }
   else
   {
      ShouldSleep = true;
   }
   
   return ShouldSleep;
}

internal OS_THREAD_FUNC(WorkerThreadProc)
{
   work_queue *Queue = Cast(work_queue *)Data;
   InitThreadCtx();
   for (;;)
   {
      if (DoWork(Queue))
      {
         WaitSemaphore(&Queue->Semaphore);
      }
   }
}

internal void
CompleteAllWork(work_queue *Queue)
{
   while (Queue->CompletionCount != Queue->EntryCount)
   {
      DoWork(Queue);
   }
   
#if TEST_WORK_QUEUE
   for (u64 WorkIndex = 0;
        WorkIndex < Queue->EntryCount;
        ++WorkIndex)
   {
      work_queue_entry *Entry = Queue->Entries + WorkIndex;
      Assert(Entry->Completed);
   }
#endif
   
   Queue->CompletionCount = 0;
   Queue->EntryCount = 0;
}

internal void
InitWorkQueue(work_queue *Queue, u64 ThreadCount)
{
   InitSemaphore(&Queue->Semaphore, 0, ThreadCount);
   for (u64 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
   {
      OS_LaunchThread(WorkerThreadProc, Queue);
   }
}