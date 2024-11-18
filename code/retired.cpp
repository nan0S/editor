#if 0
if (Collision.Entity)
{
 curve *Curve = GetCurve(Collision.Entity);
 b32 ClickActionDone = false; 
 
 if (Editor->CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget &&
     Collision.Entity != Editor->CurveAnimation.FromCurveEntity)
 {
  Editor->CurveAnimation.ToCurveEntity = Collision.Entity;
  ClickActionDone = true;
 }
 
 curve_combining_state *Combining = &Editor->CurveCombining;
 if (Combining->SourceEntity)
 {
  if (Collision.Entity != Combining->SourceEntity)
  {
   Combining->WithEntity = Collision.Entity;
  }
  
  // NOTE(hbr): Logic to recognize whether first/last point of
  // combine/target curve was clicked. If so then change which
  // point should be combined with which one (first with last,
  // first with first, ...). On top of that if user clicked
  // on already selected point but in different direction
  // (target->combine rather than combine->target) then swap
  // target and combine curves to combine in the other direction.
  
  // TODO(hbr): Probably simplify this logic
  if (Collision.Type == CurveCollision_ControlPoint)
  {
   if (Collision.Entity == Combining->SourceEntity)
   {
    curve *SourceCurve = GetCurve(Combining->SourceEntity);
    if (Collision.PointIndex == 0)
    {
     if (!Combining->SourceCurveLastControlPoint)
     {
      Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
      Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
      Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
      Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
     }
     else
     {
      Combining->SourceCurveLastControlPoint = false;
     }
    }
    else if (Collision.PointIndex == SourceCurve->ControlPointCount - 1)
    {
     if (Combining->SourceCurveLastControlPoint)
     {
      Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
      Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
      Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
      Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
     }
     else
     {
      Combining->SourceCurveLastControlPoint = true;
     }
    }
   }
   else if (Collision.Entity == Combining->WithEntity)
   {
    curve *WithCurve = GetCurve(Combining->WithEntity);
    if (Collision.PointIndex == 0)
    {
     Combining->WithCurveFirstControlPoint = true;
    }
    else if (Collision.PointIndex == WithCurve->ControlPointCount - 1)
    {
     Combining->WithCurveFirstControlPoint = false;
    }
   }
  }
  
  ClickActionDone = true;
 }
}
else
{
 
}

ImGui::CreateContext();
ImGui_ImplWin32_Init(Window);
ImGui_ImplOpenGL3_Init();

ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();

ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
internal void InitWorkQueue(work_queue *Queue, u32 ThreadCount);
internal void PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data);
internal void CompleteAllWork(work_queue *Queue);

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
 InitThreadCtx(Gigabytes(64));
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
InitWorkQueue(work_queue *Queue, u32 ThreadCount)
{
 InitSemaphore(&Queue->Semaphore, 0, ThreadCount);
 for (u64 ThreadIndex = 0;
      ThreadIndex < ThreadCount;
      ++ThreadIndex)
 {
  OS_LaunchThread(WorkerThreadProc, Queue);
 }
}
internal u64
EstimateCPUFreq(u64 GuessSampleTimeMs)
{
 u64 OSFreq = GetOSTimerFreq();
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

#endif