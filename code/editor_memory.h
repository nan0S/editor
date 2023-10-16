#ifndef EDITOR_MEMORY_H
#define EDITOR_MEMORY_H

//- Memory Arena
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

typedef struct
{
   arena *Arenas[2];
} context;

global context GlobalContext;

#define ARENA_DEFAULT_ALIGN 16
#define ARENA_DEFAULT_COMMIT_SIZE Kilobytes(4)

#define PushStruct(Arena, Type) (Type *)ArenaPushSize(Arena, sizeof(Type))
#define PushStructZero(Arena, Type) (Type *)ArenaPushSizeZero(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)ArenaPushSize(Arena, (Count) * sizeof(Type))
#define PushArrayZero(Arena, Count, Type) (Type *)ArenaPushSizeZero(Arena, (Count) * sizeof(Type))
#define PopArray(Arena, Count, Type) ArenaPopSize(Arena, (Count) * sizeof(Type))

function void SetGlobalContext(u64 ArenaCapacity);

function arena *ArenaMake(u64 Capacity);
function void ArenaDealloc(arena *Arena);
function void ArenaGrowUnaligned(arena *Arena, u64 Grow);
function void *ArenaPushSize(arena *Arena, u64 Size);
function void *ArenaPushSizeZero(arena *Arena, u64 Size);
function void ArenaPopSize(arena *Arena, u64 Size);
function void ArenaClear(arena *Arena);

function temp_arena TempArenaBegin(arena *Arena);
function void TempArenaEnd(temp_arena Temp);

#define ReleaseScratch(Scratch) TempArenaEnd(Scratch)
function temp_arena ScratchArena(arena *Conflict);

//- Pool Allocator
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

function pool *PoolMake(u64 Capacity, u64 ChunkSize, u64 Align);
function void *PoolAllocChunk(pool *Pool);
function void PoolFree(pool *Pool, void *Chunk);

#define PoolMakeForType(Capacity, Type) PoolMake(Capacity, sizeof(Type), alignof(Type))
// TODO(hbr): Checking that sizeof(Type) and aligof(Type) equal Pool->ChunkSize
// and Pool->Align correspondingly would be useful.
#define PoolAllocStruct(Pool, Type) cast(Type *)PoolAllocChunk(Pool)

//- Heap Allocator
struct heap_allocator {};

function heap_allocator *HeapAllocator(void);
function void *HeapAllocSize(heap_allocator *Heap, u64 Size);
function void *HeapReallocSize(heap_allocator *Heap, void *Memory, u64 NewSize);
function void HeapFree(heap_allocator *Heap, void *Pointer);

#define HeapAllocStruct(Heap, Type) cast(Type *)HeapAllocSize(Heap, sizeof(Type))
#define HeapReallocArray(Heap, Array, NewCount, Type) cast(Type *)HeapReallocSize(Heap, cast(void *)Array, NewCount * sizeof(*Array))

#endif //EDITOR_MEMORY_H