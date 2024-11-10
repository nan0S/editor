#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

#include <stdint.h>
#include <stdarg.h>

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

//- basic
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t umm;
typedef intptr_t  smm;

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
//#define true  (Cast(b32)1)
//#define false (Cast(b32)0)

typedef float f32;
typedef double f64;
union { u32 I; f32 F; } F32Inf = { 0x7f800000 };
union { u64 I; f64 F; } F64Inf = { 0x7ff0000000000000ull };
#define INF_F32 F32Inf.F
#define INF_F64 F64Inf.F
#define EPS_F32 0.0001f
#define EPS_F64 0.000000001

union v2
{
 struct { f32 X, Y; };
 f32 E[2];
};

union v2s
{
 struct { s32 X, Y; };
 s32 E[2];
};

struct v2u
{
 struct { u32 X, Y; };
 u32 E[2];
};

union v3
{
 struct { f32 X, Y, Z; };
 struct { f32 R, G, B; };
 f32 E[3];
 struct { v2 XY; f32 _Z; };
};

union v4
{
 struct { f32 X, Y, Z, W; };
 struct { f32 R, G, B, A; };
 f32 E[4];
 struct { v2 XY; v2 ZW; };
 struct { v3 XYZ; f32 _W; };
};

struct rectangle2
{
 v2 Mini;
 v2 Maxi;
};

struct m3x3
{
 f32 M[3][3];
};

struct m3x3_inv
{
 m3x3 Forward;
 m3x3 Inverse;
};

struct m4x4
{
 f32 M[4][4];
};

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
# define thread_static __declspec(thread)
#endif
#if OS_LINUX
# define thread_static __thread
#endif
#if !defined(thread_static)
# error thread_static not defined
#endif

#define Bytes(N)      ((Cast(u64)(N)) << 0)
#define Kilobytes(N)  ((Cast(u64)(N)) << 10)
#define Megabytes(N)  ((Cast(u64)(N)) << 20)
#define Gigabytes(N)  ((Cast(u64)(N)) << 30)

#define Thousand(N) (Cast(u64)(N) * 1000)
#define Million(N)  (Cast(u64)(N) * 1000000)
#define Billion(N)  (Cast(u64)(N) * 1000000000ull)

#define ArrayCount(Arr) (SizeOf(Arr)/SizeOf((Arr)[0]))
#define AssertAlways(Expr) do { if (!(Expr)) { Trap; } } while (0)
#define StaticAssert(Expr, Label) typedef int Static_Assert_Failed_##Label[(Expr) ? 1 : -1]
#define MarkUnused(Var) (void)Var
#define Cast(Type) (Type)
#define OffsetOf(Type, Member) (Cast(u64)(&((Cast(Type *)0)->Member)))
#define SizeOf(Expr) sizeof(Expr)
#define AlignOf(Type) alignof(Type)
#define NotImplemented Assert(!"Not Implemented")
#define InvalidPath Assert(!"Invalid Path");
#define DeferBlock(Start, End) for (int _i_ = ((Start), 0); _i_ == 0; ++_i_, (End))
#define NameConcat_(A, B) A##B
#define NameConcat(A, B) NameConcat_(A, B)

#if defined(BUILD_DEBUG)
# define Assert(Expr) AssertAlways(Expr)
#else
# define Assert(Expr) (void)(Expr)
#endif

#if COMPILER_MSVC
# define Trap __debugbreak()
#endif
#if COMPILER_GCC || COMPILER_CLANG
# define Trap __builtin_trap()
#endif
#if !defined(Trap)
# error trap not defined
#endif

#if COMPILER_MSVC
# define CompilerWriteBarrier _WriteBarrier()
#endif
#if COMPILER_GCC || COMPILER_CLANG
# define CompilerWriteBarrier asm("":::"memory")
#endif
#if !defined(CompilerWriteBarrier)
# error compiler write barrier not defined
#endif

#if COMPILER_MSVC
# define THREAD_API WINAPI
#endif
#if COMPILER_GCC || COMPILER_CLANG
# define THREAD_API
#endif
#if !defined(THREAD_API)
# error THREAD_API not defined
#endif

#define Max(A, B) ((A) < (B) ? (B) : (A))
#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Abs(X) ((X) < 0 ? -(X) : (X))
#define ClampTop(X, Maxi) Min(X, Maxi)
#define ClampBot(X, Mini) Max(X, Mini)
#define Clamp(X, Mini, Maxi) ClampTop(ClampBot(X, Mini), Maxi)
#define Clamp01(X) Clamp(X, 0, 1)
#define Idx(Row, Col, NCols) ((Row)*(NCols) + (Col))
#define ApproxEq32(X, Y) (Abs(X - Y) <= EPS_F32)
#define ApproxEq64(X, Y) (Abs(X - Y) <= EPS_F64)
#define Cmp(X, Y) (((X) < (Y)) ? -1 : (((X) == (Y)) ? 0 : 1))
#define Swap(A, B, Type) do { Type NameConcat(Temp, __LINE__) = (A); (A) = (B); (B) = NameConcat(Temp, __LINE__); } while (0)
#define AlignPow2(Align, Pow2) (((Align)+((Pow2)-1)) & (~((Pow2)-1)))
#define IsPow2(X) (((X) & (X-1)) == 0)
#define Lerp(A, B, T) (((A)*(1-(T))) + ((B)*(T)))
#define Map(Value, ValueMin, ValueMax, TargetMin, TargetMax) (((Value)-(ValueMin))/((ValueMax)-(ValueMin)) * ((TargetMax)-(TargetMin)) + (TargetMin))
#define Map01(Value, ValueMin, ValueMax) Map(Value, ValueMin, ValueMax, 0, 1)
#define QuickSort(Array, Count, Type, CmpFunc) qsort((Array), (Count), SizeOf(Type), Cast(int(*)(void const *, void const *))(CmpFunc))
#define SafeDiv0(Num, Den) ((Den) == 0 ? 0 : (Num) / (Den))
#define SafeDiv1(Num, Den) ((Den) == 0 ? 1 : (Num) / (Den))

#define MemoryCopy(Dst, Src, NumBytes) memcpy(Dst, Src, NumBytes)
#define MemoryMove(Dst, Src, NumBytes) memmove(Dst, Src, NumBytes)
#define MemorySet(Ptr, Byte, NumBytes) memset(Ptr, Byte, NumBytes)
#define MemoryZero(Ptr, NumBytes) MemorySet(Ptr, 0, NumBytes)
#define MemoryCmp(Ptr1, Ptr2, NumBytes) memcmp(Ptr1, Ptr2, NumBytes)
#define MemoryEqual(Ptr1, Ptr2, NumBytes) (MemoryCmp(Ptr1, Ptr2, NumBytes) == 0)
#define StructZero(Ptr) MemoryZero(Ptr, SizeOf(*(Ptr)))
#define ArrayCopy(Dst, Src, ElemCount) MemoryCopy(Dst, Src, (ElemCount) * SizeOf((Dst)[0]))
#define ArrayReverse(Array, Count, Type) do { for (u64 _I_ = 0; _I_ < ((Count)>>1); ++_I_) { Swap((Array)[_I_], (Array)[(Count) - 1 - _I_], Type); } } while (0)
#define ArrayMove(Dst, Src, ElemCount) MemoryMove(Dst, Src, (ElemCount) * SizeOf((Dst)[0]))

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

inline internal v2  V2(f32 X, f32 Y) { return {X,Y}; }
inline internal v2s V2S(s32 X, s32 Y) { return {X,Y}; }
inline internal v2u V2U(u32 X, u32 Y) { return {X,Y}; }
inline internal v3  V3(f32 X, f32 Y, f32 Z) { return {X,Y,Z}; }
inline internal v3  V3(v2 XY, f32 Z) { return {XY.X,XY.Y,Z}; }
inline internal v4  V4(f32 X, f32 Y, f32 Z, f32 W) { return {X,Y,Z,W}; }

//- time
typedef u64 timestamp64;
struct date_time
{
 u32 Year;  // [0..)
 u16 Ms;    // [0,999]
 u8  Sec;   // [0,59]
 u8  Mins;  // [0,59]
 u8  Hour;  // [0,23]
 u8  Day;   // [0,30]
 u8  Month; // [0,11]
};
internal date_time   TimestampToDateTime(timestamp64 Ts);
internal timestamp64 DateTimeToTimestamp(date_time Dt);

#endif //EDITOR_BASE_H