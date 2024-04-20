#ifndef EDITOR_TYPES_H
#define EDITOR_TYPES_H

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
#define F32_INF F32Inf.F
#define F64_INF F64Inf.F

#define function static
#define internal static
#define local static
#define global static

#define ArrayCount(Arr) (SizeOf(Arr)/SizeOf((Arr)[0]))

#define Bytes(N)      (((u64)(N))<<0)
#define Kilobytes(N)  (((u64)(N))<<10)
#define Megaobytes(N) (((u64)(N))<<20)
#define Gigabytes(N)  (((u64)(N))<<30)

#define Thousand(N) ((N) * 1000)
#define Million(N)  ((N) * 1000000)
#define Billion(N)  ((N) * 1000000000ull)

#define Assert(Expr) assert(Expr)
#define AssertStmt(Expr, Stmt) Assert(Expr), Stmt
#define StaticAssert(Expr, Label) typedef int Static_Assert_Failed_##Label[(Expr) ? 1 : -1]
#define MarkUnused(Var) (void)Var
#define Cast(Type) (Type)
#define SizeOf(Expr) sizeof(Expr)

#define Maximum(A, B) ((A) < (B) ? (B) : (A))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Abs(X) ((X) < 0 ? -(X) : (X))
#define ClampTop(X, Max) Minimum(X, Max)
#define ClampBot(X, Min) Maximum(X, Min)
#define Clamp(X, Min, Max) ClampTop(ClampBot(X, Min), Max)
#define Idx(Row, Column, NColumns) Row*NColumns + Column
#define ApproxEq32(X, Y) (Abs(X - Y) <= Epsilon32)
#define ApproxEq64(X, Y) (Abs(X - Y) <= Epsilon64)

#define MemoryCopy(Dest, Src, NumBytes) memcpy(Dest, Src, NumBytes)
#define MemoryMove(Dest, Src, NumBytes) memmove(Dest, Src, NumBytes)
#define MemorySet(Ptr, Byte, NumBytes) memset(Ptr, Byte, NumBytes)
#define MemoryZero(Ptr, NumBytes) MemorySet(Ptr, 0, NumBytes)

#define QuickSort(Array, Count, Type, CompareFunction) qsort((Array), (Count), SizeOf(Type), Cast(int(*)(const void *, const void *))(CompareFunction))

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
#define ArrayAlloc(Count, Type) Cast(Type *)HeapAllocSize(HeapAllocator(), (Count) * SizeOf(Type))
#define ArrayFree(Array) HeapFree(HeapAllocator(), Array)
#define ArrayReserve(Array, Capacity, Reserve) \
do { \
if ((Reserve) > (Capacity)) \
{ \
(Capacity) = Maximum(CAPACITY_GROW_FORMULA((Capacity)), (Reserve)); \
HeapFree(HeapAllocator(), (Array)); \
*(Cast(void **)&(Array)) = HeapAllocSize(HeapAllocator(), (Capacity) * SizeOf(*(Array))); \
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

template<typename lambda>
struct scope_guard
{
   lambda Lambda;
   scope_guard(lambda &&Lambda) : Lambda(std::forward<lambda>(Lambda)) {}
   ~scope_guard() { Lambda(); }
};
#define DEFER_1(X, Y) X##Y
#define DEFER_2(X, Y) DEFER_1(X, Y)
#define DEFER_3(X) DEFER_2(X, __COUNTER__)
#define defer scope_guard DEFER_3(_defer_) = [&]()

#endif //EDITOR_TYPES_H
