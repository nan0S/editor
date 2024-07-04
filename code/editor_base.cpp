//- memory
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

internal void
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

internal void
ArenaGrowUnaligned(arena *Arena, u64 Grow)
{
   u64 NewUsed = Arena->Used + Grow;
   ArenaSetUsed(Arena, NewUsed);
}

internal void *
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

internal temp_arena
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

internal pool *
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

internal void *
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

internal heap_allocator *
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

internal void *
HeapReallocSize(heap_allocator *Heap, void *Memory, u64 NewSize)
{
   MarkUnused(Heap);
   void *Result = realloc(Memory, NewSize);
   
   return Result;
}

internal void
HeapFree(heap_allocator *Heap, void *Pointer)
{
   MarkUnused(Heap);
   free(Pointer);
}

//- thread context
global thread_local thread_ctx GlobalCtx;

internal void
InitThreadCtx(u64 PerArenaCapacity)
{
   if (!GlobalCtx.Initialized)
   {
      for (u64 ArenaIndex = 0;
           ArenaIndex < ArrayCount(GlobalCtx.Arenas);
           ++ArenaIndex)
      {
         GlobalCtx.Arenas[ArenaIndex] = AllocateArena(PerArenaCapacity);
      }
      
      GlobalCtx.Initialized = true;
   }
}

internal temp_arena
TempArena(arena *Conflict)
{
   temp_arena Result = {};
   for (u64 Index = 0;
        Index < ArrayCount(GlobalCtx.Arenas);
        ++Index)
   {
      arena *Arena = GlobalCtx.Arenas[Index];
      if (Arena != Conflict)
      {
         Result = BeginTemp(Arena);
         break;
      }
   }
   
   return Result;
}

//- format
internal u64
Fmt(u64 BufSize, char *Buf, char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   u64 Result = FmtV(BufSize, Buf, Format, Args);
   va_end(Args);
   
   return Result;
}

struct out_buf
{
   char *At;
   u64 Count;
};

internal void
OutC(out_buf *Out, char C)
{
   if (Out->At)
   {
      *Out->At = C;
      ++Out->At;
   }
   --Out->Count;
}

internal void
OutStrC(out_buf *Out, char const *S)
{
   while (*S)
   {
      OutC(Out, *S);
      ++S;
   }
}

internal void
OutStr(out_buf *Out, string S)
{
   if (Out->At)
   {
      MemoryCopy(Out->At, S.Data, S.Count);
      Out->At += S.Count;
   }
   Out->Count -= S.Count;
}

internal void
FmtNumber(out_buf *Out, u64 N, u64 Base, char const *Digits, char const *BasePrefix,
          b32 Signed, b32 PrecedeWithBasePrefix, b32 ForceSign, b32 SpaceAsSign)
{
   char Sign = 0;
   s64 S = Cast(s64)N;
   if (Signed && S < 0)
   {
      N = -S;
      Sign = '-';
   }
   else
   {
      Sign = '+';
   }
   if (N != 0 && (ForceSign || Sign == '-'))
   {
      OutC(Out, Sign);
   }
   else if (SpaceAsSign)
   {
      OutC(Out, ' ');
   }
   if (PrecedeWithBasePrefix)
   {
      OutStrC(Out, BasePrefix);
   }
   
   char *Start = Out->At;
   do
   {
      u64 DigitIndex = N % Base;
      char Digit = Digits[DigitIndex];
      OutC(Out, Digit);
      N /= Base;
   } while (N);
   char *End = Out->At;
   
   while (Start < End)
   {
      --End;
      char Temp = *End;
      *End = *Start;
      *Start = Temp;
      ++Start;
   }
}

internal u64
ParseU64(char const **AtOut)
{
   u64 N = 0;
   
   char const *At = *AtOut;
   while (IsDigit(*At))
   {
      u64 Digit = *At - '0';
      N = N*10 + Digit;
      ++At;
   }
   *AtOut = At;
   
   return N;
}

internal void
FmtFloat(out_buf *Out, f64 F, char const *Inf, char const *Nan, u64 Precision,
         b32 ForceSign, b32 SpaceAsSign)
{
   u64 ExponentLength = 11;
   u64 ExponentMask = ((1ll << ExponentLength) - 1);
   s64 ExponentBias = ExponentMask >> 1;
   u64 MantissaLength = 52;
   u64 MantissaMask = ((1ull << MantissaLength) - 1);
   union f64_bits
   {
      f64 F64;
      u64 U64;
   } Bits = {};
   Bits.F64 = F;
   u64 SignBit = (Bits.U64 >> 63) & 1;
   u64 ExponentRaw = (Bits.U64 >> MantissaLength) & ExponentMask;
   u64 MantissaRaw = Bits.U64 & MantissaMask;
   
   b32 InfOrNan = false;
   b32 IsNan = false;
   if (ExponentRaw == ExponentMask)
   {
      InfOrNan = true;
      IsNan = (MantissaRaw != 0);
   }
   
   if (!IsNan && (ForceSign || (SignBit && F != 0)))
   {
      char Sign = (SignBit ? '-' : '+');
      OutC(Out, Sign);
   }
   else if (SpaceAsSign)
   {
      OutC(Out, ' ');
   }
   
   if (InfOrNan)
   {
      char const *S = (IsNan ? Nan : Inf);
      OutStrC(Out, S);
   }
   else
   {
      s64 Exponent = 0;
      u64 BaseDigit = 0;
      if (ExponentRaw == 0) // subnormal
      {
         BaseDigit = 0;
         Exponent = -(ExponentBias - 1);
      }
      else // normal
      {
         BaseDigit = 1;
         Exponent = ExponentRaw - ExponentBias;
      }
      u64 Mantissa = MantissaRaw << (ExponentLength + 1);
      
      u64 IntegerPart = 0;
      if (Exponent >= 0)
      {
         IntegerPart = (BaseDigit << Exponent) | (Mantissa >> (64 - Exponent));
      }
      
      FmtNumber(Out, IntegerPart, 10, "0123456789", "", false, false, false, false);
      OutC(Out, '.');
      
      // TODO(hbr): extracting decimal part isn't perfect, maybe improve
      F = Abs(F);
      F -= IntegerPart;
      
      u64 DecimalsWritten = 0;
      do {
         F *= 10;
         u64 Digit = Cast(u64)F;
         OutC(Out, Digit + '0');
         F -= Digit;
         ++DecimalsWritten;
      } while (DecimalsWritten < Precision && F != 0);
   }
}

internal u64
FmtV(u64 BufSize, char *Buf, char const *Format, va_list Args)
{
   out_buf Out = {};
   Out.At = Buf;
   Out.Count = BufSize;
   
   b32 Formatting = true;
   char const *At = Format;
   while (Formatting)
   {
      if (At[0] == '%')
      {
         ++At;
         
         b32 PrecedeWithBasePrefix = false;
         b32 ForceSign = false;
         b32 SpaceAsSign = false;
         b32 LeftJustify = false;
         b32 PadWithZeros = false;
         b32 ParsingFlags = true;
         while (ParsingFlags)
         {
            switch (At[0])
            {
               case '#': { PrecedeWithBasePrefix = true; } break;
               case '+': { ForceSign = true; } break;
               case ' ': { SpaceAsSign = true; } break;
               case '-': { LeftJustify = true; } break;
               case '0': { PadWithZeros = true; } break;
               default: { ParsingFlags = false; } break;
            }
            if (ParsingFlags)
            {
               ++At;
            }
         }
         
         u64 Width = 0;
         if (At[0] == '*')
         {
            Width = Cast(u64)va_arg(Args, u32);
            ++At;
         }
         else if (IsDigit(*At))
         {
            Width = ParseU64(&At);
         }
         
         u64 Precision = 6;
         if (At[0] == '.')
         {
            ++At;
            Precision = ParseU64(&At);
         }
         
         u64 NumLength = 4;
         b32 LongDouble = false;
         if (At[0] == 'h')
         {
            ++At;
            NumLength = 2;
            if (At[0] == 'h')
            {
               ++At;
               NumLength = 1;
            }
         }
         else if (At[0] == 'l')
         {
            ++At;
            NumLength = 8;
            if (At[0] == 'l')
            {
               ++At;
            }
         }
         else if (At[0] == 'L')
         {
            LongDouble = true;
         }
         
         out_buf SavedOut = Out;
         switch (At[0])
         {
            case '%': { OutC(&Out, '%'); } break;
            
            case 'b':
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'p':
            case 'P': {
               u64 Base = 0;
               char const *Digits = 0;
               b32 Signed = false;
               char const *BasePrefix = "";
               switch (At[0])
               {
                  case 'b': { Base = 2; Digits = "01"; BasePrefix = "0b"; } break;
                  case 'd':
                  case 'i': { Base = 10; Digits = "0123456789"; Signed = true; } break;
                  case 'u': { Base = 10; Digits = "0123456789"; } break;
                  case 'p':
                  case 'x': { Base = 16; Digits = "0123456789abcdef"; BasePrefix = "0x"; } break;
                  case 'P':
                  case 'X': { Base = 16; Digits = "0123456789ABCDEF"; BasePrefix = "0X"; } break;
                  case 'o': { Base = 8; Digits = "01234567"; BasePrefix = "0"; } break;
                  default: InvalidPath;
               }
               
               u64 N = ((NumLength == 8) ?
                        (Signed ? va_arg(Args, s64) : va_arg(Args, u64)) :
                        (Signed ? Cast(s64)va_arg(Args, s32) : va_arg(Args, u32)));
               
               FmtNumber(&Out, N, Base, Digits, BasePrefix, Signed, PrecedeWithBasePrefix, ForceSign, SpaceAsSign);
            } break;
            
            case 'f':
            case 'F': {
               char const *Inf = 0;
               char const *Nan = 0;
               switch (At[0])
               {
                  case 'f': { Inf = "inf"; Nan = "nan"; } break;
                  case 'F': { Inf = "INF"; Nan = "NaN"; } break;
                  default: InvalidPath;
               }
               
               // NOTE(hbr): float gets automatically promoted to double by the compiler
               f64 F = (LongDouble ? va_arg(Args, long double) : va_arg(Args, f64));
               FmtFloat(&Out, F, Inf, Nan, Precision, ForceSign, SpaceAsSign);
            } break;
            
            case 's': {
               char *S = va_arg(Args, char *);
               OutStrC(&Out, S);
            } break;
            case 'S': {
               string S = va_arg(Args, string);
               OutStr(&Out, S);
            } break;
            case 'c': {
               char C = va_arg(Args, char);
               OutC(&Out, C);
            } break;
            
            case 'n': {
               u32 *WrittenSoFar = va_arg(Args, u32 *);
               *WrittenSoFar = BufSize - Out.Count;
            } break;
            
            case 'g':
            case 'G':
            case 'a':
            case 'A':
            case 'e':
            case 'E': { Assert(!"Not supported specifier"); } break;
            default: { Assert(!"Invalid specifier"); } break;
         }
         
         u64 Formatted = SavedOut.Count - Out.Count;
         if (Formatted < Width)
         {
            u64 PadCount = Width - Formatted;
            char PadWith = ((LeftJustify || !PadWithZeros) ? ' ' : '0');
            char *PadAt = (LeftJustify ? Out.At : SavedOut.At);
            if (!LeftJustify && SavedOut.At)
            {
               MemoryMove(SavedOut.At + PadCount, SavedOut.At, Formatted);
            }
            if (PadAt)
            {
               MemorySet(PadAt, PadWith, PadCount);
               Out.At += PadCount;
            }
            Out.Count -= PadCount;
         }
      }
      else if (At[0] == '\0')
      {
         Formatting = false;
      }
      else
      {
         OutC(&Out, At[0]);
      }
      ++At;
   }
   u64 Written = BufSize - Out.Count;
   // NOTE(hbr): End with 0 byte, but don't include in return count
   OutC(&Out, 0);
   
   return Written;
}

#if 0
Fmt(ArrayCount(Buffer), Buffer,
    "%%x: %x%n, %%X: %X%n, %%u: %u, %%d: %d, %%o: %o, %%b: %b, %%p: %p, %%c: %c, %%f: %f, %%lf: %lf, %%f: %f, %%f: %f, %%.3f: %.3f, %%+.3f: %+.3f, %% .3f: % .3f, inf: %f, -inf: %f, nan: %f, INF: %F, NaN: %F, +0: %+f, -0: %+f, 0: %f, "
    "%%#+x: %#+x, %%#+X: %#+X, %%#+u: %#+u, %%#+d: %#+d, %%#+d: %#+d, %%#+d: %#+d, %%#+o: %#+o, %%#+b: %#+b, %%#+p: %#+p, "
    "U8_MAX: % hhu, U16_MAX: % hu, U32_MAX: % u, U64_MAX: % lu, U64_MAX: % llu, "
    "S8_MAX: % hhd, S16_MAX: % hd, S32_MAX: % d, S64_MAX: % ld, S64_MAX: % lld, "
    "S8_MIN: % hhd, S16_MIN: % hd, S32_MIN: % d, S64_MIN: % ld, S64_MIN: % lld, "
    "cstr: %s, string: %S",
    0xabc, &WrittenSoFar1, 0xabc, &WrittenSoFar2, 10, 10, 027, 0b1001, 0xdeadc0de, 'a', 10.1234f, 10.1234, -10.1024f, 0.0f, 0.1324513f, 0.1324513f, 0.1324513f, 1.0f/FFF, -1.0f/FFF, 1.0f + 0.0f/FFF, 1.0f/FFF, 0.0f/FFF, 1.0f / (1.0f / FFF), -1.0 / (1.0f / FFF), 0.0f,
    0xabc, 0xabc, 10, 10, -10, 0, 027, 0b1001, 0xdeadc0de,
    U8_MAX, U16_MAX, U32_MAX, U64_MAX, U64_MAX,
    S8_MAX, S16_MAX, S32_MAX, S64_MAX, S64_MAX,
    S8_MIN, S16_MIN, S32_MIN, S64_MIN, S64_MIN,
    "test", StrLit("test"));
#endif

//- strings
internal string
CreateAndCopyStr(char *Dst, char const *Src, u64 Count)
{
   string Result = {};
   Result.Data = Dst;
   Result.Count = Count;
   MemoryCopy(Dst, Src, Count);
   Dst[Count] = 0;
   
   return Result;
}

internal string
Str(char const *String, u64 Count)
{
   char *Data = Cast(char *)HeapAllocSizeNonZero(HeapAllocator(), Count + 1);
   string Result = CreateAndCopyStr(Data, String, Count);
   
   return Result;
}

internal string
Str(arena *Arena, char const *String, u64 Count)
{
   char *Data = Cast(char *)PushSize(Arena, Count + 1);
   string Result = CreateAndCopyStr(Data, String, Count);
   
   return Result;
}

internal string
StrC(arena *Arena, char const *String)
{
   string Result = Str(Arena, String, CStrLen(String));
   return Result;
}

// TODO(hbr): Make it accept pointer instead
internal void
FreeStr(string *String)
{
   HeapFree(HeapAllocator(), String->Data);
   String->Data = 0;
   String->Count = 0;
}

internal string
StrF(arena *Arena, char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   string Result = StrFV(Arena, Format, Args);
   va_end(Args);
   
   return Result;
}

internal string
StrFV(arena *Arena, char const *Format, va_list Args)
{
   va_list ArgsCopy;
   va_copy(ArgsCopy, Args);
   u64 Count = FmtV(0, 0, Format, Args);
   char *Data = Cast(char *)PushSize(Arena, Count + 1);
   FmtV(Count + 1, Data, Format, ArgsCopy);
   va_end(ArgsCopy);
   
   string Result = {};
   Result.Data = Data;
   Result.Count = Count; 
   
   return Result;
}

internal string
DuplicateStr(arena *Arena, string String)
{
   string Result = Str(Arena, String.Data, String.Count);
   return Result;
}

internal string
DuplicateStr(string String)
{
   string Result = Str(String.Data, String.Count);
   return Result;
}

internal b32
AreStringsEqual(string A, string B)
{
   b32 Result = false;
   if (A.Count == B.Count)
   {
      Result = MemoryEqual(A.Data, B.Data, A.Count);
   }
   
   return Result;
}

// TODO(hbr): Make this internal return string instead
internal void
RemoveExtension(string *Path)
{
   u64 Index = Path->Count;
   while (Index && Path->Data[Index-1] != '.')
   {
      --Index;
   }
   if (Index)
   {
      Path->Count = Index - 1;
   }
}

internal b32
HasSuffix(string String, string Suffix)
{
   b32 Result = false;
   if (String.Count >= Suffix.Count)
   {
      u64 Offset = String.Count - Suffix.Count;
      Result = MemoryEqual(String.Data + Offset, Suffix.Data, Suffix.Count);
   }
   
   return Result;
}

internal string
StringChopFileNameWithoutExtension(arena *Arena, string String)
{
   s64 LastSlashPosition = -1;
   for (s64 Index = String.Count;
        Index >= 0;
        --Index)
   {
      if (String.Data[Index] == '/' ||
          String.Data[Index] == '\\')
      {
         LastSlashPosition = Index;
         break;
      }
   }
   
   s64 LastDotPosition = -1;
   for (s64 Index = String.Count - 1;
        Index >= 0;
        --Index)
   {
      if (String.Data[Index] == '.')
      {
         LastDotPosition = Index;
         break;
      }
   }
   
   u64 FromInclusive = LastSlashPosition+1;
   u64 ToExclusive = 0;
   if (LastDotPosition == -1 ||
       LastDotPosition <= LastSlashPosition)
   {
      ToExclusive = String.Count;
   }
   else
   {
      ToExclusive = LastDotPosition;
   }
   
   string Result = Substr(Arena, String,
                          FromInclusive,
                          ToExclusive);
   
   return Result;
}

internal string
Substr(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive)
{
   if (FromInclusive > String.Count) FromInclusive = String.Count;
   if (ToExclusive > String.Count) ToExclusive = String.Count;
   if (FromInclusive > ToExclusive)
   {
      u64 Swap = FromInclusive;
      FromInclusive = ToExclusive;
      ToExclusive = Swap;
   }
   
   u64 Count = ToExclusive - FromInclusive;
   string Substring = Str(Arena, String.Data + FromInclusive, Count);
   
   return Substring;
}

internal b32
IsValid(string String)
{
   b32 Result = (String.Data != 0);
   return Result;
}

// TODO(hbr): remove IsError and IsValid
internal b32 IsError(error_string String) { return IsValid(String); }

internal char
ToUpper(char C)
{
   char Result = C;
   if ('a' <= C && C <= 'z')
   {
      Result = C-'a' + 'A';
   }
   
   return Result;
}

internal b32
IsDigit(char C)
{
   b32 Result = ('0' <= C && C <= '9');
   return Result;
}

internal string
CStrFromStr(arena *Arena, string S)
{
   char *Data = PushArrayNonZero(Arena, S.Count + 1, char);
   MemoryCopy(Data, S.Data, S.Count);
   Data[S.Count] = 0;
   
   string Result = {};
   Result.Data = Data;
   Result.Count = S.Count;
   
   return Result;
}

internal string
StrFromCStr(char const *CStr)
{
   string Result = {};
   Result.Data = Cast(char *)CStr;
   Result.Count = CStrLen(CStr);
   
   return Result;
}

internal u64
CStrLen(char const *CStr)
{
   char const *At = CStr;
   while (*At) ++At;
   u64 Length = At - CStr;
   
   return Length;
}

internal void
StringListPush(arena *Arena, string_list *List, string String)
{
   string_list_node *Node = PushStructNonZero(Arena, string_list_node);
   Node->String = String;
   QueuePush(List->Head, List->Tail, Node);
   ++List->NumNodes;
   List->TotalSize += String.Count;
}