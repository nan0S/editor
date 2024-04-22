internal arena *
AllocateArena(u64 Capacity, u64 InitialHeaderSize, u64 Align)
{
   arena *Arena = 0;
   
   Assert(InitialHeaderSize <= Capacity);
   Assert(InitialHeaderSize <= ARENA_DEFAULT_COMMIT_SIZE);
   
   void *Memory = VirtualMemoryReserve(Capacity);
   VirtualMemoryCommit(Memory, ARENA_DEFAULT_COMMIT_SIZE);
   
   Arena = (arena *)Memory;
   Arena->Memory = Memory;
   Arena->Capacity = Capacity;
   Arena->Used = InitialHeaderSize;
   Arena->Commited = ARENA_DEFAULT_COMMIT_SIZE;
   Arena->Align = Align;
   Arena->InitialHeaderSize = InitialHeaderSize;
   
   return Arena;
}

inline arena *
AllocateArena(u64 Capacity)
{
   arena *Arena = AllocateArena(Capacity, SizeOf(arena), ARENA_DEFAULT_ALIGN);
   return Arena;
}

function void
FreeArena(arena *Arena)
{
   VirtualMemoryRelease(Arena->Memory, Arena->Capacity);
   Arena->Memory = 0;
   Arena->Capacity = 0;
   Arena->Used = 0;
   Arena->Commited = 0;
   Arena->Align = 0;
   Arena->InitialHeaderSize = 0;
}

internal b32
ArenaSetUsed(arena *Arena, u64 NewUsed)
{
   b32 Result = false;
   
   if (NewUsed <= Arena->Capacity)
   {
      Result = true;
      
      Arena->Used = NewUsed;
      
      if (NewUsed > Arena->Commited)
      {
         u64 CommitGrow = NewUsed - Arena->Commited;
         CommitGrow += ARENA_DEFAULT_COMMIT_SIZE-1;
         CommitGrow -= CommitGrow % ARENA_DEFAULT_COMMIT_SIZE;
         VirtualMemoryCommit(Cast(u8 *)Arena->Memory + Arena->Commited, CommitGrow);
         Arena->Commited += CommitGrow;
      }
   }
   
   return Result;
}

function void
ArenaGrowUnaligned(arena *Arena, u64 Grow)
{
   u64 NewUsed = Arena->Used + Grow;
   ArenaSetUsed(Arena, NewUsed);
}

function void *
PushSizeNonZero(arena *Arena, u64 Size)
{
   void *Result = 0;
   
   u64 FreeAddress = (u64)Arena->Memory + Arena->Used;
   FreeAddress += ARENA_DEFAULT_ALIGN-1;
   FreeAddress -= FreeAddress % ARENA_DEFAULT_ALIGN;
   
   u64 NewUsed = FreeAddress - (u64)Arena->Memory + Size;
   if (ArenaSetUsed(Arena, NewUsed))
   {
      Result = (void *)FreeAddress;
   }
   
   return Result;
}

inline void *
PushSize(arena *Arena, u64 Size)
{
   void *Result = PushSizeNonZero(Arena, Size);
   MemoryZero(Result, Size);
   
   return Result;
}

inline void
PopSize(arena *Arena, u64 Size)
{
   Assert(Size <= Arena->Used);
   Arena->Used -= Size;
}

inline void
ClearArena(arena *Arena)
{
   PopSize(Arena, Arena->Used - Arena->InitialHeaderSize);
}

function temp_arena
BeginTemp(arena *Arena)
{
   temp_arena Result = {};
   Result.Arena = Arena;
   Result.SavedUsed = Arena->Used;
   
   return Result;
}

inline void
EndTemp(temp_arena Temp)
{
   Temp.Arena->Used = Temp.SavedUsed;
}

function pool *
AllocatePool(u64 Capacity, u64 ChunkSize, u64 Align)
{
   Assert(ChunkSize >= SizeOf(pool_node));
   
   pool *Pool = (pool *)AllocateArena(Capacity, SizeOf(pool), Align);
   Pool->ChunkSize = ChunkSize;
   Pool->FreeNode = 0;
   
   return Pool;
}

inline void
FreePool(pool *Pool)
{
   FreeArena(Cast(arena *)Pool);
}

function void *
AllocateChunkNonZero(pool *Pool)
{
   void *Result = 0;
   if (Pool->FreeNode)
   {
      Result = Pool->FreeNode;
      StackPop(Pool->FreeNode);
   }
   else
   {
      Result = PushSizeNonZero(&Pool->BackingArena, Pool->ChunkSize);
   }
   
   return Result;
}

inline void *
AllocateChunk(pool *Pool)
{
   void *Result = AllocateChunkNonZero(Pool);
   MemoryZero(Result, Pool->ChunkSize);
   
   return Result;
}

inline void
ReleaseChunk(pool *Pool, void *Chunk)
{
   StackPush(Pool->FreeNode, Cast(pool_node *)Chunk);
}

#include <malloc.h>

function heap_allocator *
HeapAllocator(void)
{
   heap_allocator *Heap = 0;
   return Heap;
}

inline void *
HeapAllocSizeNonZero(heap_allocator *Heap, u64 Size)
{
   MarkUnused(Heap);
   void *Result = malloc(Size);
   
   return Result;
}

inline void *
HeapAllocSize(heap_allocator *Heap, u64 Size)
{
   void *Result = HeapAllocSizeNonZero(Heap, Size);
   MemoryZero(Result, Size);
   
   return Result;
}

function void *
HeapReallocSize(heap_allocator *Heap, void *Memory, u64 NewSize)
{
   MarkUnused(Heap);
   void *Result = realloc(Memory, NewSize);
   
   return Result;
}

function void
HeapFree(heap_allocator *Heap, void *Pointer)
{
   MarkUnused(Heap);
   free(Pointer);
}
