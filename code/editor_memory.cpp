//- Memory Arena
function void
SetGlobalContext(u64 ArenaCapacity)
{
   for (u64 ArenaIndex = 0;
        ArenaIndex < ArrayCount(GlobalContext.Arenas);
        ++ArenaIndex)
   {
      GlobalContext.Arenas[ArenaIndex] = ArenaMake(ArenaCapacity);
   }
}

internal arena *
ArenaMake(u64 Capacity, u64 InitialHeaderSize, u64 Align)
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

function arena *
ArenaMake(u64 Capacity)
{
   arena *Arena = ArenaMake(Capacity, SizeOf(arena), ARENA_DEFAULT_ALIGN);
   return Arena;
}

function void
ArenaDealloc(arena *Arena)
{
   VirtualMemoryRelease(Arena->Memory, Arena->Capacity);
   Arena->Memory = 0;
   Arena->Capacity = 0;
   Arena->Used = 0;
   Arena->Commited = 0;
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
ArenaPushSize(arena *Arena, u64 Size)
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

function void *
ArenaPushSizeZero(arena *Arena, u64 Size)
{
   void *Result = ArenaPushSize(Arena, Size);
   MemoryZero(Result, Size);
   
   return Result;
}

function void
ArenaPopSize(arena *Arena, u64 Size)
{
   Assert(Size <= Arena->Used);
   Arena->Used -= Size;
}

function void
ArenaClear(arena *Arena)
{
   ArenaPopSize(Arena, Arena->Used - Arena->InitialHeaderSize);
}

function temp_arena
TempArenaBegin(arena *Arena)
{
   temp_arena Result = {};
   Result.Arena = Arena;
   Result.SavedUsed = Arena->Used;
   
   return Result;
}

function void
TempArenaEnd(temp_arena Temp)
{
   Temp.Arena->Used = Temp.SavedUsed;
}

function temp_arena
ScratchArena(arena *Conflict)
{
   temp_arena Result = {};
   for (u64 Index = 0;
        Index < ArrayCount(GlobalContext.Arenas);
        ++Index)
   {
      arena *Arena = GlobalContext.Arenas[Index];
      if (Arena != Conflict)
      {
         Result = TempArenaBegin(Arena);
         break;
      }
   }
   
   return Result;
}

//- Pool Allocator
function pool *
PoolMake(u64 Capacity, u64 ChunkSize, u64 Align)
{
   Assert(ChunkSize >= SizeOf(pool_node));
   
   pool *Pool = (pool *)ArenaMake(Capacity, SizeOf(pool), Align);
   Pool->ChunkSize = ChunkSize;
   Pool->FreeNode = 0;
   
   return Pool;
}

function void *
PoolAllocChunk(pool *Pool)
{
   void *Result = 0;
   if (Pool->FreeNode)
   {
      Result = Pool->FreeNode;
      StackPop(Pool->FreeNode);
   }
   else
   {
      Result = ArenaPushSize(&Pool->BackingArena, Pool->ChunkSize);
   }
   
   return Result;
}

function void
PoolFree(pool *Pool, void *Chunk)
{
   StackPush(Pool->FreeNode, Cast(pool_node *)Chunk);
}

//- Heap Allocator
#include <malloc.h>

function heap_allocator *
HeapAllocator(void)
{
   heap_allocator *Heap = 0;
   return Heap;
}

function void *
HeapAllocSize(heap_allocator *Heap, u64 Size)
{
   MarkUnused(Heap);
   void *Result = malloc(Size);
   
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
