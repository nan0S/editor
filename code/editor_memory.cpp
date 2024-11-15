internal arena *
AllocArenaSize(u64 Size, u64 Align)
{
 u64 Header = AlignForwardPow2(SizeOf(arena), Align);
 u64 Capacity = Max(Header + Size, Megabytes(1));
 void *Memory = Platform.AllocVirtualMemory(Capacity, true);
 Assert(Cast(umm)Memory % Align == 0);
 
 arena *Arena = Cast(arena *)Memory;
 Arena->Cur = Arena;
 Arena->Memory = Memory;
 Arena->Used = Header;
 Arena->Capacity = Capacity;
 Arena->Align = Align;
 
 return Arena;
}

internal arena *
AllocArenaAlign(u64 Align)
{
 arena *Arena = AllocArenaSize(0, Align);
 return Arena;
}

internal arena *
AllocArena(void)
{
 arena *Arena = AllocArenaAlign(16);
 return Arena;
}

internal void *
PushSizeNonZero(arena *Arena, u64 Size)
{
 void *Result = 0;
 arena *Last = 0;
 for (arena *Cur = Arena->Cur; Cur; Cur = Cur->Next)
 {
  umm Mem = Cast(umm)Cur->Memory;
  umm Addr = AlignForwardPow2(Mem + Cur->Used, Cur->Align);
  u64 NewUsed = (Addr - Mem) + Size;
  if (NewUsed <= Cur->Capacity)
  {
   Cur->Used = NewUsed;
   Result = Cast(void *)Addr;
   break;
  }
  
  Last = Cur;
 }
 
 if (!Result)
 {
  arena *NewArena = AllocArenaSize(Size, Last->Align);
  Last->Next = NewArena;
  Arena->Cur = NewArena;
  umm Addr = Cast(umm)NewArena->Memory + NewArena->Used;
  umm AddrAligned = AlignForwardPow2(Addr, NewArena->Align);
  Result = Cast(void *)AddrAligned;
  NewArena->Used += Size + (AddrAligned - Addr);
  Assert(NewArena->Used <= NewArena->Capacity);
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
  Platform.DeallocVirtualMemory(Arena->Memory, Arena->Capacity);
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
 Temp.Arena->Cur = Temp.SavedCur;
 Temp.SavedCur->Used = Temp.SavedUsed;
}