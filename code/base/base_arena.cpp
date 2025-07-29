#ifndef AllocVirtualMemory
# define AllocVirtualMemory OS_Reserve
#endif

#ifndef DeallocVirtualMemory
# define DeallocVirtualMemory OS_Release
#endif

#ifndef CommitVirtualMemory
# define CommitVirtualMemory OS_Commit
#endif

internal arena *
AllocArenaSize(u64 ReserveNotCommit)
{
 ReserveNotCommit += SizeOf(arena);
 ReserveNotCommit = AlignForwardPow2(ReserveNotCommit, 4096);
 ReserveNotCommit = ClampBot(ReserveNotCommit, 4096);
 
 void *Memory = AllocVirtualMemory(ReserveNotCommit, false);
 arena *Arena = Cast(arena *)Memory;
 CommitVirtualMemory(Memory, SizeOf(arena));
 
 Arena->Cur = Arena;
 Arena->Memory = Memory;
 Arena->Used = SizeOf(arena);
 Arena->Commited = SizeOf(arena);
 Arena->Capacity = ReserveNotCommit;
 Arena->Align = 16;
 
 return Arena;
}

internal arena *
AllocArena(u64 ReserveButNotCommit)
{
 arena *Arena = AllocArenaSize(ReserveButNotCommit);
 return Arena;
}

struct alloc_from
{
 b32 Fits;
 void *Addr;
};
internal alloc_from
AllocFrom(arena *Arena, u64 Size)
{
 alloc_from Result = {};
 
 umm Mem = Cast(umm)Arena->Memory;
 umm Addr = AlignForwardPow2(Mem + Arena->Used, Arena->Align);
 u64 NewUsed = (Addr - Mem) + Size;
 
 if (NewUsed <= Arena->Capacity)
 {
  Arena->Used = NewUsed;
  
  if (NewUsed > Arena->Commited)
  {
   u64 NewCommited = AlignForwardPow2(NewUsed, 4096);
   umm CommitAt = Mem + Arena->Commited;
   u64 ToCommit = NewCommited - Arena->Commited;
   CommitVirtualMemory(Cast(void *)CommitAt, ToCommit);
   Arena->Commited = NewCommited;
  }
  
  Result.Fits = true;
  Result.Addr = Cast(void *)Addr;
 }
 
 return Result;
}

internal void *
PushSizeNonZero(arena *Arena, u64 Size)
{
 void *Result = 0;
 arena *Tail = 0;
 
 for (arena *Cur = Arena->Cur; Cur; Cur = Cur->Next)
 {
  alloc_from Alloc = AllocFrom(Cur, Size);
  if (Alloc.Fits)
  {
   Result = Alloc.Addr;
   break;
  }
  
  Tail = Cur;
 }
 
 if (!Result)
 {
  arena *NewArena = AllocArenaSize(Size);
  Tail->Next = NewArena;
  Arena->Cur = NewArena;
  
  alloc_from Alloc = AllocFrom(NewArena, Size);
  Result = Alloc.Addr;
  Assert(Alloc.Fits);
 }
 
 return Result;
}

internal void *
PushSize(arena *Arena, u64 Size)
{
 void *Result = PushSizeNonZero(Arena, Size);
 MemoryZero(Result, Size);
 return Result;
}

internal void
DeallocArena(arena *Arena)
{
 arena *Node = Arena;
 while (Node)
 {
  arena *Next = Node->Next;
  StructZero(Node);
  DeallocVirtualMemory(Node->Memory, Node->Capacity);
  Node = Next;
 }
}

internal void
ClearArena(arena *Arena)
{
 for (arena *Node = Arena; Node; Node = Node->Next)
 {
  Node->Used = SizeOf(arena);
 }
 Arena->Cur = Arena;
}

internal temp_arena
BeginTemp(arena *Arena)
{
 temp_arena Temp = {};
 Temp.Arena = Arena;
 Temp.SavedCur = Arena->Cur;
 Temp.SavedUsed = Arena->Cur->Used;
 
 return Temp;
}

internal void
EndTemp(temp_arena Temp)
{
 ClearArena(Temp.Arena);
 Temp.Arena->Used = Temp.SavedUsed;
}