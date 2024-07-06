#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <malloc.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t umm;
typedef intptr_t smm;

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

#define internal static
#define local    static
#define global   static

#if OS_WINDOWS
# pragma section(".roglob", read)
# define read_only __declspec(allocate(".roglob"))
#elif (OS_LINUX && COMPILER_CLANG)
# define read_only __attribute__((section(".rodata")))
#else
# define read_only
#endif

#if OS_WINDOWS
# define thread_local __declspec(thread)
#endif
#if OS_LINUX
# define thread_local __thread
#endif
#if !defined(thread_local)
# error thread_local not defined
#endif

#define Bytes(N)      ((Cast(u64)(N)) << 0)
#define Kilobytes(N)  ((Cast(u64)(N)) << 10)
#define Megabytes(N) ((Cast(u64)(N)) << 20)
#define Gigabytes(N)  ((Cast(u64)(N)) << 30)

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

#define Max(A, B) ((A) < (B) ? (B) : (A))
#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Abs(X) ((X) < 0 ? -(X) : (X))
#define ClampTop(X, Maxi) Min(X, Maxi)
#define ClampBot(X, Mini) Max(X, Mini)
#define Clamp(X, Mini, Maxi) ClampTop(ClampBot(X, Mini), Maxi)
#define Idx(Row, Col, NCols) ((Row)*(NCols) + (Col))
#define ApproxEq32(X, Y) (Abs(X - Y) <= EPS_F32)
#define ApproxEq64(X, Y) (Abs(X - Y) <= EPS_F64)
#define CmpInt(X, Y) (((X) < (Y)) ? -1 : (((X) == (Y)) ? 0 : 1))
#define Swap(A, B, Type) do { Type NameConcat(Temp, __LINE__) = (A); (A) = (B); (B) = NameConcat(Temp, __LINE__); } while (0)
#define AlignPow2(Align, Pow2) (((Align)+((Pow2)-1)) & (~((Pow2)-1)))
#define IsPow2(X) (((X) & (X-1)) == 0)

#define MemoryCopy(Dest, Src, NumBytes) memcpy(Dest, Src, NumBytes)
#define MemoryMove(Dest, Src, NumBytes) memmove(Dest, Src, NumBytes)
#define MemorySet(Ptr, Byte, NumBytes) memset(Ptr, Byte, NumBytes)
#define MemoryZero(Ptr, NumBytes) MemorySet(Ptr, 0, NumBytes)
#define MemoryEqual(Ptr1, Ptr2, NumBytes) (memcmp(Ptr1, Ptr2, NumBytes) == 0)
#define ZeroStruct(Ptr) MemoryZero(Ptr, SizeOf(*(Ptr)))

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
#define ArrayAlloc(Count, Type) Cast(Type *)HeapAllocNonZero((Count) * SizeOf(Type))
#define ArrayFree(Array) HeapDealloc(Array)
#define ArrayReserve(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
(Capacity) = Max(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
HeapDealloc(Array); \
*(Cast(void **)&(Array)) = HeapAllocNonZero((Capacity) * SizeOf((Array)[0])); \
} \
Assert((Capacity) >= (Reserve)); \
} while (0)
#define ArrayReserveCopy(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
(Capacity) = Max(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
*(Cast(void **)&(Array)) = HeapRealloc((Array), (Capacity) * SizeOf((Array)[0])); \
} \
Assert((Capacity) >= (Reserve)); \
} while (0)
#define ArrayReserveCopyZero(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
auto OldCapacity = (Capacity); \
(Capacity) = Max(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
*(Cast(void **)&(Array)) = HeapRealloc((Array), (Capacity) * SizeOf((Array)[0])); \
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
   arena *Next;
   arena *Cur;
   
   void *Memory;
   u64 Used;
   u64 Capacity;
   u64 Align;
};

struct temp_arena
{
   arena *Arena;
   arena *SavedCur;
   u64 SavedUsed;
};

struct pool_chunk
{
   pool_chunk *Next;
};
struct pool
{
   arena *BackingArena;
   pool_chunk *FirstFreeChunk;
   u64 ChunkSize;
};

internal arena *AllocArena(void);
internal void   DeallocArena(arena *Arena);
internal void   ClearArena(arena *Arena);
internal void * PushSize(arena *Arena, u64 Size);
#define         PushStruct(Arena, Type) (Type *)PushSize(Arena, SizeOf(Type))
#define         PushArray(Arena, Count, Type) (Type *)PushSize(Arena, (Count) * SizeOf(Type))
internal void * PushSizeNonZero(arena *Arena, u64 Size);
#define         PushStructNonZero(Arena, Type) (Type *)PushSizeNonZero(Arena, SizeOf(Type))
#define         PushArrayNonZero(Arena, Count, Type) (Type *)PushSizeNonZero(Arena, (Count) * SizeOf(Type))

internal temp_arena BeginTemp(arena *Conflict);
internal void       EndTemp(temp_arena Temp);

#define        AllocPool(Type) AllocPoolImpl(SizeOf(Type), AlignOf(Type))
internal void  DeallocPool(pool *Pool);
#define        RequestChunk(Pool, Type) Cast(Type *)RequestChunkImpl(Pool)
#define        RequestChunkNonZero(Pool, Type) Cast(Type *)RequestChunkNonZeroImpl(Pool)
internal void  ReleaseChunk(pool *Pool, void *Chunk);

internal void *HeapAlloc(u64 Size);
internal void *HeapAllocNonZero(u64 Size);
internal void *HeapRealloc(void *Memory, u64 NewSize);
internal void  HeapDealloc(void *Pointer);

#define HeapAllocStruct(Type) Cast(Type *)HeapAlloc(SizeOf(Type))
#define HeapAllocStructNonZero(Type) Cast(Type *)HeapAllocNonZero(SizeOf(Type))
#define HeapReallocArray(Array, NewCount, Type) Cast(Type *)HeapRealloc(Cast(void *)(Array), (NewCount) * SizeOf((Array)[0]))

//- thread context
typedef struct
{
   b32 Initialized;
   arena *Arenas[2];
} thread_ctx;

internal void InitThreadCtx(void);
internal temp_arena TempArena(arena *Conflict);

//- format
internal u64 Fmt(u64 BufSize, char *Buf, char const *Format, ...);
internal u64 FmtV(u64 BufSize, char *Buf, char const *Format, va_list Args);

//- strings
struct string
{
   char *Data;
   u64 Count;
};
typedef string error_string;

// TODO(hbr): Probably rewrite stirng "module"
// NOTE(hbr): Allocates on heap
internal string Str(char const *String, u64 Count);
internal string Str(arena *Arena, char const *String, u64 Count);
internal string StrC(arena *Arena, char const *String);
internal string StrF(arena *Arena, char const *Format, ...);
internal string StrFV(arena *Arena, char const *Format, va_list Args);
internal void FreeStr(string *String);
internal string DuplicateStr(arena *Arena, string String);
internal string DuplicateStr(string String);
internal b32 AreStringsEqual(string A, string B);
internal void RemoveExtension(string *Path);
internal b32 HasSuffix(string String, string Suffix);
internal string StringChopFileNameWithoutExtension(arena *Arena, string String);
internal string Substr(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive);
internal b32 IsValid(string String);
internal b32 IsError(error_string String);

internal char ToUpper(char C);
internal b32 IsDigit(char C);

#define StrLit(Lit) Str(Lit, ArrayCount(Lit)-1)
#define StrLitArena(Arena, Lit) Str(Arena, Lit, ArrayCount(Lit)-1)

// TODO(hbr): functions that do not need rewrite because has been written after rewrite remark
internal string CStrFromStr(arena *Arena, string S);
internal string StrFromCStr(char const *CStr);
internal u64 CStrLen(char const *CStr);

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

internal void StringListPush(arena *Arena, string_list *List, string String);

#endif //EDITOR_BASE_H