internal arena *
AllocArena(u64 Reserve)
{
 arena *Arena = 0;
 
 u64 PageSize = OS_PageSize();
 u64 ReserveAligned = AlignForwardPow2(Reserve, PageSize);
 void *Memory = OS_Reserve(ReserveAligned);
 
 if (Memory)
 {
  Assert(ReserveAligned >= PageSize);
  Assert(SizeOf(arena) <= PageSize);
  OS_Commit(Memory, PageSize);
  
  Arena = Cast(arena *)Memory;
  Arena->Memory = Memory;
  Arena->Used = SizeOf(arena);
  Arena->Capacity = Reserve;
  Arena->Commited = PageSize;
 }
 
 return Arena;
}

internal void
ClearArena(arena *Arena)
{
 Arena->Used = SizeOf(arena);
}

internal void *
PushSizeNonZero(arena *Arena, u64 Size)
{
 void *Result = 0;
 
 u64 NewUsed = Arena->Used + Size;
 if (NewUsed <= Arena->Capacity)
 {
  Result = Cast(char *)Arena->Memory + Arena->Used;
  Arena->Used = NewUsed;
  
  if (NewUsed > Arena->Commited)
  {
   u64 PageSize = OS_PageSize();
   u64 NewCommited = AlignForwardPow2(NewUsed, PageSize);
   u64 ToCommit = SafeSubtractU64(NewCommited, Arena->Commited);
   void *CommitedAt = Cast(char *)Arena->Memory + Arena->Commited;
   OS_Commit(CommitedAt, ToCommit);
   Arena->Commited = NewCommited;
  }
 }
 
 return Result;
}

internal void *
PushSize(arena *Arena, u64 Size)
{
 void *Result = PushSizeNonZero(Arena, Size);
 if (Result)
 {
  MemoryZero(Result, Size);
 }
 
 return Result;
}

internal temp_arena
BeginTemp(arena *Arena)
{
 temp_arena Temp = {};
 Temp.Arena = Arena;
 Temp.SavedUsed = Arena->Used;
 
 return Temp;
}

internal void
EndTemp(temp_arena Temp)
{
 Temp.Arena->Used = Temp.SavedUsed;
}
