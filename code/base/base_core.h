#ifndef BASE_CORE_H
#define BASE_CORE_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

//- basic
#define internal static
#define local    static
#define global   static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

#define I8_MIN  (( i8)0x80)
#define I16_MIN ((i16)0x8000)
#define I32_MIN ((i32)0x80000000)
#define I64_MIN ((i64)0x8000000000000000ll)

#define I8_MAX  (( i8)0x7f)
#define I16_MAX ((i16)0x7fff)
#define I32_MAX ((i32)0x7fffffff)
#define I64_MAX ((i64)0x7fffffffffffffffll)

typedef uint32_t b32;

typedef float f32;
typedef double f64;

#define F32_INF _F32Inf()
#define F64_INF _F64Inf()
#define F32_EPS 0.0001f
#define F64_EPS 0.000000001

// NOTE(hbr): Unfortunately this construction is necessary. Putting this outside of function
// can violate one definition rule.
inline f32 _F32Inf(void) { union { u32 I; f32 F; } X = {0x7f800000}; return X.F; }
inline f64 _F64Inf(void) { union { u64 I; f64 F; } X = {0x7ff0000000000000ull}; return X.F; }

union v2
{
 struct { f32 X, Y; };
 f32 E[2];
};

union v2i
{
 struct { i32 X, Y; };
 i32 E[2];
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

union mat3
{
 f32 M[3][3];
 v3 Rows[3];
};

struct mat3_inv
{
 mat3 Forward;
 mat3 Inverse;
};

union mat4
{
 f32 M[4][4];
 v4 Rows[4];
};

#if OS_WINDOWS
# define thread_static __declspec(thread)
#endif
#if OS_LINUX
# define thread_static __thread
#endif
#if !defined(thread_static)
# error thread_static not defined
#endif

#if COMPILER_MSVC
# define DLL_EXPORT extern "C" __declspec(dllexport)
#endif
#if COMPILER_CLANG || COMPILER_GCC
# define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif
#if !defined(DLL_EXPORT)
# error DLL_EXPORT not defined
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
#define MemberOf(Type, Member) ((Cast(Type *)0)->Member)
#define AlignOf(Type) alignof(Type)
#define NotImplemented Assert(!"Not Implemented")
#define InvalidPath Assert(!"Invalid Path");
#define DeferBlock(Start, End) for (int _i_ = ((Start), 0); _i_ == 0; ++_i_, (End))
#define NameConcat_(A, B) A##B
#define NameConcat(A, B) NameConcat_(A, B)
#define ConvertNameToString_(Name) #Name
#define ConvertNameToString(Name) ConvertNameToString_(Name) // convert CustomName -> "CustomName"

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
# include <intrin.h> 
# define CompilerWriteBarrier _WriteBarrier()
#endif
#if COMPILER_GCC || COMPILER_CLANG
#include <x86intrin.h>
# define CompilerWriteBarrier asm("":::"memory")
#endif
#if !defined(CompilerWriteBarrier)
# error compiler write barrier not defined
#endif

#define Max(A, B) ((A) < (B) ? (B) : (A))
#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Abs(X) ((X) < 0 ? -(X) : (X))
#define ClampTop(X, Maxi) Min(X, Maxi)
#define ClampBot(X, Mini) Max(X, Mini)
#define Clamp(X, Mini, Maxi) ClampTop(ClampBot(X, Mini), Maxi)
#define Clamp01(X) Clamp(X, 0, 1)
#define Index2D(Row, Col, ColCount) ((Row)*(ColCount) + (Col))
#define ApproxEq32(X, Y) (Abs(X - Y) <= F32_EPS)
#define ApproxEq64(X, Y) (Abs(X - Y) <= F64_EPS)
#define Cmp(X, Y) (((X) < (Y)) ? -1 : (((X) == (Y)) ? 0 : 1))
#define Swap(A, B, Type) do { Type NameConcat(Temp, __LINE__) = (A); (A) = (B); (B) = NameConcat(Temp, __LINE__); } while (0)
#define AlignForwardPow2(Align, Pow2) (((Align)+((Pow2)-1)) & (~((Pow2)-1)))
#define AlignForward(Value, Align) ((Value) - (((Value) + (Align) - 1) % (Align)))
#define IsPow2(X) (((X) & (X-1)) == 0)
#define Lerp(A, B, T) (((A)*(1-(T))) + ((B)*(T)))
#define Map(Value, ValueMin, ValueMax, TargetMin, TargetMax) (((Value)-(ValueMin))/((ValueMax)-(ValueMin)) * ((TargetMax)-(TargetMin)) + (TargetMin))
#define Map01(Value, ValueMin, ValueMax) Map(Value, ValueMin, ValueMax, 0, 1)
#define GenericSort(Array, Count, Type, CmpFunc) qsort((Array), (Count), SizeOf(Type), Cast(int(*)(void const *, void const *))(CmpFunc))
#define SafeDiv0(Num, Den) ((Den) == 0 ? 0 : (Num) / (Den))
#define SafeDiv1(Num, Den) ((Den) == 0 ? 1 : (Num) / (Den))
#define NoOp ((void)0)

#define MemoryCopy(Dst, Src, Bytes) memcpy(Dst, Src, Bytes)
#define MemoryMove(Dst, Src, Bytes) memmove(Dst, Src, Bytes)
#define MemorySet(Ptr, Byte, Bytes) memset(Ptr, Byte, Bytes)
#define MemoryZero(Ptr, Bytes) MemorySet(Ptr, 0, Bytes)
#define MemoryCmp(Ptr1, Ptr2, Bytes) memcmp(Ptr1, Ptr2, Bytes)
#define MemoryEqual(Ptr1, Ptr2, Bytes) (MemoryCmp(Ptr1, Ptr2, Bytes) == 0)
#define StructZero(Ptr) MemoryZero(Ptr, SizeOf(*(Ptr)))
#define ArrayCopy(Dst, Src, ElemCount) MemoryCopy(Dst, Src, (ElemCount) * SizeOf((Dst)[0]))
#define ArrayReverse(Array, Count, Type) do { for (u64 _I_ = 0; _I_ < ((Count)>>1); ++_I_) { Swap((Array)[_I_], (Array)[(Count) - 1 - _I_], Type); } } while (0)
#define ArrayMove(Dst, Src, ElemCount) MemoryMove(Dst, Src, (ElemCount) * SizeOf((Dst)[0]))
#define ArrayZero(Array, ElemCount) MemoryZero(Array, (ElemCount) * SizeOf((Array)[0]))

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

inline internal u32 SafeCastU32(u64 X) { Assert(X <= U32_MAX); return Cast(u32)X; }
inline internal u16 SafeCastU16(u64 X) { Assert(X <= U16_MAX); return Cast(u16)X; }
inline internal  u8  SafeCastU8(u64 X) { Assert(X <=  U8_MAX); return Cast( u8)X; }

inline internal u64 SafeSubtractU64(u64 A, u64 B) { Assert(A >= B); return A-B; }
inline internal u32 SafeSubtractU32(u32 A, u32 B) { Assert(A >= B); return A-B; }
inline internal u16 SafeSubtractU16(u16 A, u16 B) { Assert(A >= B); return A-B; }
inline internal  u8  SafeSubtractU8( u8 A,  u8 B) { Assert(A >= B); return A-B; }

inline internal v2  V2(f32 X, f32 Y) { return {X,Y}; }
inline internal v2i V2I(i32 X, i32 Y) { return {X,Y}; }
inline internal v2u V2U(u32 X, u32 Y) { return {X,Y}; }
inline internal v3  V3(f32 X, f32 Y, f32 Z) { return {X,Y,Z}; }
inline internal v3  V3(v2 XY, f32 Z) { return {XY.X,XY.Y,Z}; }
inline internal v4  V4(f32 X, f32 Y, f32 Z, f32 W) { return {X,Y,Z,W}; }
inline internal v4  V4(v3 XYZ, f32 W) { return {XYZ.X,XYZ.Y,XYZ.Z,W}; }

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

#endif //BASE_CORE_H