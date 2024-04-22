#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <stdint.h>
#include <assert.h>
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
#define S64_MIN ((s64)0x8000800080008000ll)

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
#define Assert(Expr) assert(Expr)
#define StaticAssert(Expr, Label) typedef int Static_Assert_Failed_##Label[(Expr) ? 1 : -1]
#define MarkUnused(Var) (void)Var
#define Cast(Type) (Type)
#define OffsetOf(Type, Member) (Cast(u64)(&((Cast(Type *)0)->Member)))
#define SizeOf(Expr) sizeof(Expr)
#define AlignOf(Type) alignof(Type)
#define NotImplemented Assert(!"Not Implemented")
#define InvalidPath Assert(!"Invalid Path");

#if COMPILER_MSVC
# define Trap __debugbreak()
#endif
#if COMPILER_GCC || COMPILER_CLANG
# define Trap __builtin_trap()
#endif
#ifndef Trap
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

#define MemoryCopy(Dest, Src, NumBytes) memcpy(Dest, Src, NumBytes)
#define MemoryMove(Dest, Src, NumBytes) memmove(Dest, Src, NumBytes)
#define MemorySet(Ptr, Byte, NumBytes) memset(Ptr, Byte, NumBytes)
#define MemoryZero(Ptr, NumBytes) MemorySet(Ptr, 0, NumBytes)
#define MemoryEqual(Ptr1, Ptr2, NumBytes) (memcmp(Ptr1, Ptr2, NumBytes) == 0)

#define QuickSort(Array, Count, Type, CompareFunction) \
qsort((Array), \
(Count), \
SizeOf(Type), \
Cast(int(*)(const void *, const void *))(CompareFunction))

#define Swap(A, B) \
do { \
auto Temp = (A); \
(A) = (B); \
(B) = Temp; \
} while (0)

#define DeferBlock(Start, End) \
for (int NameConcat(_i_, __LINE__) = ((Start), 0); \
NameConcat(_i_, __LINE__) == 0; \
++NameConcat(_i_, __LINE__), (End))

#define StackPush(Head, Node) \
(Node)->Next = (Head); \
(Head) = (Node)
#define StackPop(Head) \
(Head) = ((Head) ? (Head)->Next : 0)

#define QueuePush(Head, Tail, Node) \
if (Tail) (Tail)->Next = (Node); \
else (Head) = (Node); \
(Node)->Next = 0; \
(Tail) = (Node)
#define QueuePop(Head, Tail) \
(Head) = ((Head) ? (Head)->Next : 0); \
(Tail) = ((Head) ? (Tail) : 0)

#define DLLPushFront(Head, Tail, Node) \
(Node)->Next = (Head); \
(Node)->Prev = 0; \
if ((Head)) (Head)->Prev = (Node); \
else (Tail) = (Node); \
(Head) = (Node)
#define DLLPushBack(Head, Tail, Node) \
(Node)->Next = 0; \
(Node)->Prev = (Tail); \
if ((Tail)) (Tail)->Next = (Node); \
else (Head) = (Node); \
(Tail) = (Node)
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

#define ListIter(VarName, Head, Type) \
for (Type *VarName = (Head), *__Next = ((Head) ? (Head)->Next : 0); \
VarName; \
VarName = __Next, __Next = (__Next ? __Next->Next : 0))
#define ListIterRev(VarName, Tail, Type) \
for (Type *VarName = (Tail), *__Prev = ((Tail) ? (Tail)->Prev : 0); \
VarName; \
VarName = __Prev, __Prev = (__Prev ? __Prev->Prev : 0))

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

#define ArrayReverse(Array, Count) \
do { \
for (u64 Index = 0; Index < ((Count)>>1); ++Index) \
{ \
Swap((Array)[Index], (Array)[(Count) - 1 - Index]); \
} \
} while (0)

// TODO(hbr): Remvoe defer
#if 1
template<typename lambda>
struct scope_guard
{
   lambda Lambda;
   scope_guard(lambda &&Lambda) : Lambda(std::forward<lambda>(Lambda)) {}
   ~scope_guard() { Lambda(); }
};
#define DEFER_1(X, Y) X##Y
#define DEFER_2(X, Y) DEFER_1(X, Y)
#define DEFER_3(X) DEFER_2(X, __LINE__)
#define defer scope_guard DEFER_3(_defer_) = [&]()
#endif

#endif //EDITOR_BASE_H