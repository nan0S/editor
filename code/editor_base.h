#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <stdint.h>
#include <stdarg.h>
#include <math.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define U8_MIN  ((u8)0)
#define U16_MIN ((u16)0)
#define U32_MIN ((u32)0)
#define U64_MIN ((u64)0)

#define U8_MAX  ((u8)-1)
#define U16_MAX ((u16)-1)
#define U32_MAX ((u32)-1)
#define U64_MAX ((u64)-1)

#define S8_MIN  ((s8)0x80)
#define S16_MIN ((s16)0x8000)
#define S32_MIN ((s32)0x80000000)
#define S64_MIN ((s64)0x8000000000000000ll)

#define S8_MAX  ((s8)0x7f)
#define S16_MAX ((s16)0x7fff)
#define S32_MAX ((s32)0x7fffffff)
#define S64_MAX ((s64)0x7fffffffffffffffll)

typedef uint32_t b32;
#define true ((b32)1)
#define false ((b32)0)

typedef float f32;
typedef double f64;
union { u32 I; f32 F; } F32Inf = { 0x7f800000 };
union { u64 I; f64 F; } F64Inf = { 0x7ff0000000000000ull };
#define INF_F32 F32Inf.F
#define INF_F64 F64Inf.F
#define EPS_F32 0.0001f
#define EPS_F64 0.000000001

//- context crack
#if defined(_WIN32)
# define OS_WINDOWS 1
#elif defined(__linux__)
# define OS_LINUX 1
#else
# error unsupported OS
#endif

#if defined(_MSC_VER)
# define COMPILER_MSVC 1
#elif defined(__GNUC__) || defined(__GNUG__)
# define COMPILER_GCC 1
#elif defined(__clang__)
# define COMPILER_CLANG 1
#else
# error unsupported compiler
#endif

#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(COMPILER_MSVC)
# define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif

#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 0
#endif

#define function static
#define internal static
#define local static
#define global static

#if OS_WINDOWS
# pragma section(".roglob", read)
# define read_only __declspec(allocate(".roglob"))
# define thread_local __declspec(thread)
#endif

#if OS_LINUX
# define thread_local __thread
// TODO(hbr): read_only for Linux?
#endif

#define Bytes(N)      (((u64)(N)) << 0)
#define Kilobytes(N)  (((u64)(N)) << 10)
#define Megaobytes(N) (((u64)(N)) << 20)
#define Gigabytes(N)  (((u64)(N)) << 30)

#define Thousand(N) ((N) * 1000)
#define Million(N)  ((N) * 1000000)
#define Billion(N)  ((N) * 1000000000ull)

#define ArrayCount(Arr) (SizeOf(Arr)/SizeOf((Arr)[0]))
#define AssertAlways(Expr) do { if (!(Expr)) { Trap(); } } while (0)
#define StaticAssert(Expr, Label) typedef int Static_Assert_Failed_##Label[(Expr) ? 1 : -1]
#define MarkUnused(Var) (void)Var
#define Cast(Type) (Type)
#define OffsetOf(Type, Member) (Cast(u64)(&((Cast(Type *)0)->Member)))
#define SizeOf(Expr) sizeof(Expr)
#define AlignOf(Type) alignof(Type)
#define NotImplemented Assert(!"Not Implemented")
#define InvalidPath Assert(!"Invalid Path");
#define DeferBlock(Start, End) for (int _i_ = ((Start), 0); _i_ == 0; ++_i_, (End))

#if defined(BUILD_DEBUG)
# define Assert(Expr) AssertAlways(Expr)
#else
# define Assert(Expr) (void)(Expr)
#endif

#if COMPILER_MSVC
# define Trap() __debugbreak()
#endif
#if COMPILER_GCC || COMPILER_CLANG
# define Trap() __builtin_trap()
#endif
#if !defined(Trap)
# error trap not defined
#endif

#define Maximum(A, B) ((A) < (B) ? (B) : (A))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Abs(X) ((X) < 0 ? -(X) : (X))
#define ClampTop(X, Max) Minimum(X, Max)
#define ClampBot(X, Min) Maximum(X, Min)
#define Clamp(X, Min, Max) ClampTop(ClampBot(X, Min), Max)
#define Idx(Row, Column, NColumns) ((Row)*(NColumns) + (Column))
#define ApproxEq32(X, Y) (Abs(X - Y) <= EPS_F32)
#define ApproxEq64(X, Y) (Abs(X - Y) <= EPS_F64)
#define IntCmp(X, Y) (((X) < (Y)) ? -1 : (((X) == (Y)) ? 0 : 1))
#define Swap(A, B, Type) do { Type NameConcat(Temp, __LINE__) = (A); (A) = (B); (B) = NameConcat(Temp, __LINE__); } while (0)

#define MemoryCopy(Dest, Src, NumBytes) memcpy(Dest, Src, NumBytes)
#define MemoryMove(Dest, Src, NumBytes) memmove(Dest, Src, NumBytes)
#define MemorySet(Ptr, Byte, NumBytes) memset(Ptr, Byte, NumBytes)
#define MemoryZero(Ptr, NumBytes) MemorySet(Ptr, 0, NumBytes)
#define MemoryEqual(Ptr1, Ptr2, NumBytes) (memcmp(Ptr1, Ptr2, NumBytes) == 0)
#define QuickSort(Array, Count, Type, CmpFunc) qsort((Array), (Count), SizeOf(Type), Cast(int(*)(void const *, void const *))(CmpFunc))

#define ListIter(Var, Head, Type) for (Type *Var = (Head), *__Next = ((Head) ? (Head)->Next : 0); Var; Var = __Next, __Next = (__Next ? __Next->Next : 0))
#define ListIterRev(Var, Tail, Type) for (Type *Var = (Tail), *__Prev = ((Tail) ? (Tail)->Prev : 0); Var; Var = __Prev, __Prev = (__Prev ? __Prev->Prev : 0))

#define StackPush(Head, Node) (Node)->Next = (Head); (Head) = (Node)
#define StackPop(Head) (Head) = ((Head) ? (Head)->Next : 0)

#define QueuePush(Head, Tail, Node) if (Tail) (Tail)->Next = (Node); else (Head) = (Node); (Node)->Next = 0; (Tail) = (Node)
#define QueuePop(Head, Tail) (Head) = ((Head) ? (Head)->Next : 0); (Tail) = ((Head) ? (Tail) : 0)

#define DLLPushFront(Head, Tail, Node)  (Node)->Next = (Head);  (Node)->Prev = 0; if ((Head)) (Head)->Prev = (Node); else (Tail) = (Node); (Head) = (Node)
#define DLLPushBack(Head, Tail, Node) (Node)->Next = 0;  (Node)->Prev = (Tail); if ((Tail)) (Tail)->Next = (Node); else (Head) = (Node); (Tail) = (Node)
#define DLLInsert(Head, Tail, After, Node) \
do { \
if ((After)) \
{ \
(Node)->Next = (After)->Next; \
(Node)->Prev = (After); \
if ((After)->Next) (After)->Next->Prev = (Node), (After)->Next = (Node); \
else (After)->Next = (Node), (Tail) = (Node); \
} \
else { DLLPushFront((Head), (Tail), (Node)); } \
} while (0)
#define DLLRemove(Head, Tail, Node) \
do { \
if ((Node)) \
{ \
if ((Node)->Prev) (Node)->Prev->Next = (Node)->Next; \
if ((Node)->Next) (Node)->Next->Prev = (Node)->Prev; \
(Head) = ((Node) == (Head) ? (Head)->Next : (Head)); \
(Tail) = ((Node) == (Tail) ? (Tail)->Prev : (Tail)); \
} \
} while (0)

// TODO(hbr): I probably should remove these macros
#define CAPACITY_GROW_FORMULA(Capacity) (2 * (Capacity) + 8)
#define array(Type) Type *
#define ArrayAlloc(Count, Type) Cast(Type *)HeapAllocSizeNonZero(HeapAllocator(), (Count) * SizeOf(Type))
#define ArrayFree(Array) HeapFree(HeapAllocator(), Array)
#define ArrayReserve(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
(Capacity) = Maximum(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
HeapFree(HeapAllocator(), (Array)); \
*(Cast(void **)&(Array)) = HeapAllocSizeNonZero(HeapAllocator(), (Capacity) * SizeOf(*(Array))); \
} \
Assert((Capacity) >= (Reserve)); \
} while (0)
#define ArrayReserveCopy(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
(Capacity) = Maximum(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
*(Cast(void **)&(Array)) = HeapReallocSize(HeapAllocator(), (Array), (Capacity) * SizeOf(*(Array))); \
} \
Assert((Capacity) >= (Reserve)); \
} while (0)
#define ArrayReserveCopyZero(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
auto OldCapacity = (Capacity); \
(Capacity) = Maximum(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
*(Cast(void **)&(Array)) = HeapReallocSize(HeapAllocator(), (Array), (Capacity) * SizeOf(*(Array))); \
if ((Capacity) > OldCapacity) { \
MemoryZero((Array) + OldCapacity, ((Capacity)-OldCapacity) * SizeOf(*(Array))); \
} \
} \
Assert((Capacity) >= (Reserve)); \
} while (0)
#define ArrayReverse(Array, Count, Type) \
do { for (u64 _I_ = 0; _I_ < ((Count)>>1); ++_I_) { Swap((Array)[_I_], (Array)[(Count) - 1 - _I_], Type); } } while (0)

//- memory
struct arena
{
   void *Memory;
   u64 Capacity;
   u64 Used;
   u64 Commited;
   u64 Align;
   u64 InitialHeaderSize;
};

struct temp_arena
{
   arena *Arena;
   u64 SavedUsed;
};

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

struct heap_allocator {};

function heap_allocator *HeapAllocator(void);
function void *HeapAllocSize(heap_allocator *Heap, u64 Size);
function void *HeapAllocSizeNonZero(heap_allocator *Heap, u64 Size);
function void *HeapReallocSize(heap_allocator *Heap, void *Memory, u64 NewSize);
function void HeapFree(heap_allocator *Heap, void *Pointer);

#define HeapAllocStruct(Heap, Type) Cast(Type *)HeapAllocSize(Heap, SizeOf(Type))
#define HeapAllocStructNonZero(Heap, Type) Cast(Type *)HeapAllocSizeNonZero(Heap, SizeOf(Type))
#define HeapReallocArray(Heap, Array, NewCount, Type) Cast(Type *)HeapReallocSize(Heap, Cast(void *)Array, NewCount * SizeOf(*Array))

//- thread context
typedef struct
{
   b32 Initialized;
   arena *Arenas[2];
} thread_ctx;

function void InitThreadCtx(u64 PerArenaCapacity);
function temp_arena TempArena(arena *Conflict);

//- format
function u64 Fmt(u64 BufSize, char *Buf, char const *Format, ...);
function u64 FmtV(u64 BufSize, char *Buf, char const *Format, va_list Args);

//- strings
struct string
{
   char *Data;
   u64 Count;
};
typedef string error_string;

// NOTE(hbr): Allocates on heap
function string Str(char const *String, u64 Count);
function string Str(arena *Arena, char const *String, u64 Count);
function string StrC(arena *Arena, char const *String);
function string StrF(arena *Arena, char const *Fmt, ...);
function string StrFV(arena *Arena, char const *Fmt, va_list Args);
function void FreeStr(string *String);
function u64 CStrLength(char const *String);
function string DuplicateStr(arena *Arena, string String);
function string DuplicateStr(string String);
function b32 AreStringsEqual(string A, string B);
function void RemoveExtension(string *Path);
function b32 HasSuffix(string String, string Suffix);
function string StringChopFileNameWithoutExtension(arena *Arena, string String);
function string Substr(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive);
function b32 IsValid(string String);
function b32 IsError(error_string String);

function char ToUpper(char C);
function b32 IsDigit(char C);

#define StrLit(Lit) Str(Lit, ArrayCount(Lit)-1)
#define StrLitArena(Arena, Lit) Str(Arena, Lit, ArrayCount(Lit)-1)

struct string_list_node
{
   string_list_node *Next;
   string String;
};

struct string_list
{
   string_list_node *Head;
   string_list_node *Tail;
   
   u64 NumNodes;
   u64 TotalSize;
};

function void StringListPush(arena *Arena, string_list *List, string String);

#endif //EDITOR_BASE_H