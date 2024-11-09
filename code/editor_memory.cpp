internal arena *
AllocArenaSize(u64 Size, u64 Align)
{
 u64 Header = AlignPow2(SizeOf(arena), Align);
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
  umm Addr = AlignPow2(Mem + Cur->Used, Cur->Align);
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
  umm AddrAligned = AlignPow2(Addr, NewArena->Align);
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

internal pool *
AllocPoolImpl(u64 ChunkSize, u64 Align)
{
 AssertAlways(SizeOf(pool_chunk) <= ChunkSize);
 arena *Arena = AllocArenaAlign(Align);
 pool *Pool = PushStruct(Arena, pool);
 Pool->BackingArena = Arena;
 Pool->ChunkSize = ChunkSize;
 
 return Pool;
}

internal void *
RequestChunkNonZeroImpl(pool *Pool)
{
 void *Result = 0;
 if (Pool->FirstFreeChunk)
 {
  Result = Pool->FirstFreeChunk;
  StackPop(Pool->FirstFreeChunk);
 }
 else
 {
  Result = PushSizeNonZero(Pool->BackingArena, Pool->ChunkSize);
 }
 
 return Result;
}

internal void *
RequestChunkImpl(pool *Pool)
{
 void *Result = RequestChunkNonZeroImpl(Pool);
 MemoryZero(Result, Pool->ChunkSize);
 return Result;
}

internal void
ReleaseChunk(pool *Pool, void *Chunk)
{
 pool_chunk *PoolChunk = Cast(pool_chunk *)Chunk;
 PoolChunk->Next = Pool->FirstFreeChunk;
 Pool->FirstFreeChunk = PoolChunk;
}

internal void
DeallocPool(pool *Pool)
{
 DeallocArena(Pool->BackingArena);
}

struct thread_ctx
{
 b32 Initialized;
 arena *Arenas[2];
};
global thread_static thread_ctx GlobalCtx;

internal void
InitThreadCtx(void)
{
 if (!GlobalCtx.Initialized)
 {
  for (u64 ArenaIndex = 0;
       ArenaIndex < ArrayCount(GlobalCtx.Arenas);
       ++ArenaIndex)
  {
   GlobalCtx.Arenas[ArenaIndex] = AllocArena();
  }
  GlobalCtx.Initialized = true;
 }
}

internal temp_arena
TempArena(arena *Conflict)
{
 temp_arena Result = {};
 for (u64 Index = 0;
      Index < ArrayCount(GlobalCtx.Arenas);
      ++Index)
 {
  arena *Arena = GlobalCtx.Arenas[Index];
  if (Arena != Conflict)
  {
   Result = BeginTemp(Arena);
   break;
  }
 }
 
 return Result;
}