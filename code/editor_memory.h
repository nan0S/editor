#ifndef EDITOR_MEMORY_H
#define EDITOR_MEMORY_H

//~ Memory Arena
typedef struct
{
   void *Memory;
   u64 Capacity;
   u64 Used;
   u64 Commited;
   u64 Align;
   u64 InitialHeaderSize;
} arena;

typedef struct
{
   arena *Arena;
   u64 SavedUsed;
} temp_arena;

#define ARENA_DEFAULT_ALIGN 16
#define ARENA_DEFAULT_COMMIT_SIZE Kilobytes(4)

#define PushStruct(Arena, Type) (Type *)PushSize(Arena, SizeOf(Type))
#define PushStructNonZero(Arena, Type) (Type *)PushSizeNonZero(Arena, SizeOf(Type))
#define PushArray(Arena, Count, Type) (Type *)PushSize(Arena, (Count) * SizeOf(Type))
#define PushArrayNonZero(Arena, Count, Type) (Type *)PushSizeNonZero(Arena, (Count) * SizeOf(Type))
#define PopArray(Arena, Count, Type) PopSize(Arena, (Count) * SizeOf(Type))

function arena *AllocateArena(u64 Capacity);
function void FreeArena(arena *Arena);
function void ArenaGrowUnaligned(arena *Arena, u64 Grow);
function void *PushSize(arena *Arena, u64 Size);
function void *PushSizeNonZero(arena *Arena, u64 Size);
function void PopSize(arena *Arena, u64 Size);
function void ClearArena(arena *Arena);

function temp_arena BeginTemp(arena *Conflict);
function void EndTemp(temp_arena Temp);

//~ Pool Allocator
struct pool_node
{
   pool_node *Next;
};

struct pool
{
   // NOTE(hbr): Maybe change to expanding arena
   arena BackingArena;
   u64 ChunkSize;
   pool_node *FreeNode;
};

function pool *AllocatePool(u64 Capacity, u64 ChunkSize, u64 Align);
function void FreePool(pool *Pool);
function void *AllocateChunk(pool *Pool);
function void ReleaseChunk(pool *Pool, void *Chunk);

#define AllocatePoolForType(Capacity, Type) AllocatePool(Capacity, SizeOf(Type), AlignOf(Type))
// TODO(hbr): Checking that SizeOf(Type) and aligof(Type) equal Pool->ChunkSize
// and Pool->Align correspondingly would be useful.
#define PoolAllocStruct(Pool, Type) Cast(Type *)AllocateChunk(Pool)
#define PoolAllocStructNonZero(Pool, Type) Cast(Type *)AllocateChunkNonZero(Pool)

//~ Heap Allocator
struct heap_allocator {};

function heap_allocator *HeapAllocator(void);
function void *HeapAllocSize(heap_allocator *Heap, u64 Size);
function void *HeapAllocSizeNonZero(heap_allocator *Heap, u64 Size);
function void *HeapReallocSize(heap_allocator *Heap, void *Memory, u64 NewSize);
function void HeapFree(heap_allocator *Heap, void *Pointer);

#define HeapAllocStruct(Heap, Type) Cast(Type *)HeapAllocSize(Heap, SizeOf(Type))
#define HeapAllocStructNonZero(Heap, Type) Cast(Type *)HeapAllocSizeNonZero(Heap, SizeOf(Type))
#define HeapReallocArray(Heap, Array, NewCount, Type) Cast(Type *)HeapReallocSize(Heap, Cast(void *)Array, NewCount * SizeOf(*Array))

#endif //EDITOR_MEMORY_H