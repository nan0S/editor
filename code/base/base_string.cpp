internal u64
Fmt(char *Buf, u64 BufSize, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 u64 Result = FmtV(Buf, BufSize, Format, Args);
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
 if (Out->At && Out->Count)
 {
  *Out->At++ = C;
 }
 if (!Out->At || Out->Count)
 {
  --Out->Count;
 }
}

internal void
OutStr(out_buf *Out, string Str)
{
 u64 ToCopy = Str.Count;
 if (Out->At)
 {
  ToCopy = Min(ToCopy, Out->Count);
  MemoryCopy(Out->At, Str.Data, ToCopy);
  Out->At += ToCopy;
 }
 Out->Count -= ToCopy;
}

internal void OutStrC(out_buf *Out, char const *S) { OutStr(Out, MakeStr(Cast(char *)S, CStrLen(S))); }

internal void
FmtNumber(out_buf *Out, u64 N, u64 Base, char const *Digits, char const *BasePrefix,
          b32 Signed, b32 PrecedeWithBasePrefix, b32 ForceSign, b32 SpaceAsSign)
{
 char Sign = 0;
 i64 S = Cast(i64)N;
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
 while (CharIsDigit(*At))
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
 i64 ExponentBias = ExponentMask >> 1;
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
  i64 Exponent = 0;
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
   i64 AdjustMantissa = ClampTop(64 - Exponent, 63);
   IntegerPart = (BaseDigit << Exponent) | (Mantissa >> AdjustMantissa);
  }
  
  char DecChars[] = "0123456789";
  FmtNumber(Out, IntegerPart, 10, DecChars, "", false, false, false, false);
  OutC(Out, '.');
  
  // TODO(hbr): extracting decimal part isn't perfect, maybe improve
  F = Abs(F);
  F -= IntegerPart;
  
  u64 DecimalsWritten = 0;
  do {
   F *= 10;
   u32 Int = Cast(u32)F;
   OutC(Out, DecChars[Int]);
   F -= Int;
   ++DecimalsWritten;
  } while (DecimalsWritten < Precision && F != 0);
 }
}

internal u64
FmtV(char *Buf, u64 BufSize, char const *Format, va_list Args)
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
   else if (CharIsDigit(*At))
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
              (Signed ? va_arg(Args, i64) : va_arg(Args, u64)) :
              (Signed ? Cast(i64)va_arg(Args, i32) : va_arg(Args, u32)));
     
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
     f64 F = (LongDouble ? Cast(f64)va_arg(Args, long double) : va_arg(Args, f64));
     FmtFloat(&Out, F, Inf, Nan, Precision, ForceSign, SpaceAsSign);
    } break;
    
    case 's': {
     char *Str = va_arg(Args, char *);
     OutStrC(&Out, Str);
    } break;
    case 'S': {
     string Str = va_arg(Args, string);
     OutStr(&Out, Str);
    } break;
    case 'c': {
     int C = va_arg(Args, int);
     OutC(&Out, Cast(char)C);
    } break;
    
    case 'n': {
     u32 *WrittenSoFar = va_arg(Args, u32 *);
     *WrittenSoFar = Cast(u32)(BufSize - Out.Count);
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
    if (Out.At)
    {
     PadCount = Min(PadCount, Out.Count);
    }
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
    "I8_MAX: % hhd, I16_MAX: % hd, I32_MAX: % d, I64_MAX: % ld, I64_MAX: % lld, "
    "I8_MIN: % hhd, I16_MIN: % hd, I32_MIN: % d, I64_MIN: % ld, I64_MIN: % lld, "
    "cstr: %s, string: %S",
    0xabc, &WrittenSoFar1, 0xabc, &WrittenSoFar2, 10, 10, 027, 0b1001, 0xdeadc0de, 'a', 10.1234f, 10.1234, -10.1024f, 0.0f, 0.1324513f, 0.1324513f, 0.1324513f, 1.0f/FFF, -1.0f/FFF, 1.0f + 0.0f/FFF, 1.0f/FFF, 0.0f/FFF, 1.0f / (1.0f / FFF), -1.0 / (1.0f / FFF), 0.0f,
    0xabc, 0xabc, 10, 10, -10, 0, 027, 0b1001, 0xdeadc0de,
    U8_MAX, U16_MAX, U32_MAX, U64_MAX, U64_MAX,
    I8_MAX, I16_MAX, I32_MAX, I64_MAX, I64_MAX,
    I8_MIN, I16_MIN, I32_MIN, I64_MIN, I64_MIN,
    "test", StrLit("test"));
#endif

internal b32
CharIsDigit(char C)
{
 b32 Result = ('0' <= C && C <= '9');
 return Result;
}

internal b32
CharIsWhiteSpace(char C)
{
 b32 Result = false;
 switch (C)
 {
  case '\t':
  case '\n':
  case ' ':
  case '\r': {Result = true;}break;
 }
 return Result;
}

internal b32
CharIsUpper(char C)
{
 b32 Result = ('A' <= C && C <= 'Z');
 return Result;
}

internal b32
CharIsLower(char C)
{
 b32 Result = ('a' <= C && C <= 'z');
 return Result;
}

internal char
CharToLower(char C)
{
 char Result = C;
 if (CharIsUpper(Result))
 {
  Result += 'a' - 'A';
 }
 return Result;
}

internal char
CharToUpper(char C)
{
 char Result = C;
 if (CharIsLower(Result))
 {
  Result += 'A' - 'a';
 }
 return Result;
}

internal string
MakeStr(char *Data, u64 Count)
{
 string Result = {};
 Result.Data = Data;
 Result.Count = Count;
 return Result;
}

internal string
StrCopy(arena *Arena, string Str)
{
 string Result = {};
 Result.Data = PushArrayNonZero(Arena, Str.Count, char);
 MemoryCopy(Result.Data, Str.Data, Str.Count);
 Result.Count = Str.Count;
 return Result;
}

internal string
CStrCopy(arena *Arena, char const *Str)
{
 string Result = {};
 Result.Count = CStrLen(Str);
 Result.Data = PushArrayNonZero(Arena, Result.Count, char);
 MemoryCopy(Result.Data, Str, Result.Count);
 return Result;
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
 string Result = {};
 va_list ArgsCopy;
 va_copy(ArgsCopy, Args);
 Result.Count = FmtV(0, 0, Format, Args);
 Result.Data = PushArrayNonZero(Arena, Result.Count + 1, char);
 u64 ActualCount = FmtV(Result.Data, Result.Count + 1, Format, ArgsCopy);
 Assert(ActualCount == Result.Count);
 Result.Data[Result.Count] = 0;
 va_end(ArgsCopy);
 return Result;
}

internal string
CStrFromStr(arena *Arena, string Str)
{
 char *Data = PushArrayNonZero(Arena, Str.Count + 1, char);
 MemoryCopy(Data, Str.Data, Str.Count);
 Data[Str.Count] = 0;
 
 string Result = {};
 Result.Data = Data;
 Result.Count = Str.Count;
 
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

internal string
StrSuffix(string Str, u64 Count)
{
 string Result = Str;
 if (Result.Count > Count)
 {
  Result.Data = Result.Data + (Result.Count - Count);
  Result.Count = Count;
 }
 
 return Result;
}

internal b32
StrMatch(string A, string B, b32 CaseInsensitive)
{
 b32 Result = true;
 if (A.Count == B.Count)
 {
  for (u64 I = 0; I < A.Count; ++I)
  {
   char Ac = A.Data[I];
   char Bc = B.Data[I];
   if (CaseInsensitive)
   {
    Ac = CharToLower(Ac);
    Bc = CharToLower(Bc);
   }
   if (Ac != Bc)
   {
    Result = false;
    break;
   }
  }
 }
 else
 {
  Result = false;
 }
 
 return Result;
}

internal b32
StrEqual(string A, string B)
{
 b32 Result = StrMatch(A, B, false);
 return Result;
}

internal int
StrCmp(string A, string B)
{
 int Order = MemoryCmp(A.Data, B.Data, Min(A.Count, B.Count));
 int Result = 0;
 if (Order == 0)
 {
  Result = Cmp(A.Count, B.Count);
 }
 else
 {
  Result = Order;
 }
 
 return Result;
}

internal b32
StrEndsWith(string Str, string End)
{
 string Suffix = StrSuffix(Str, End.Count);
 b32 Result = StrEqual(Suffix, End);
 return Result;
}

internal string
StrChop(string Str, u64 Chop)
{
 string Result = Str;
 Result.Count -= ClampTop(Chop, Result.Count);
 return Result;
}

internal string
StrChopLastSlash(string Str)
{
 u64 Left = Str.Count;
 while (Left > 0 && Str.Data[Left-1] != '/' && Str.Data[Left-1] != '\\')
 {
  --Left;
 }
 
 string Result = Str;
 if (Left > 0)
 {
  Result.Count = Left - 1;
 }
 
 return Result;
}

internal string
StrAfterLastSlash(string Str)
{
 u64 Left = Str.Count;
 while (Left > 0 && Str.Data[Left-1] != '/' && Str.Data[Left-1] != '\\')
 {
  --Left;
 }
 
 string Result = Str;
 if (Left > 0)
 {
  Result.Data = Str.Data + Left;
  Result.Count = Str.Count - Left;
 }
 
 return Result;
}

internal string
StrChopLastDot(string Str)
{
 u64 Left = Str.Count;
 while (Left > 0 && Str.Data[Left-1] != '.')
 {
  --Left;
 }
 
 string Result = Str;
 if (Left > 0)
 {
  Result.Count = Left - 1;
 }
 
 return Result;
}

internal string
StrAfterLastDot(string Str)
{
 u64 Left = Str.Count;
 while (Left > 0 && Str.Data[Left-1] != '.')
 {
  --Left;
 }
 
 string Result = Str;
 if (Left > 0)
 {
  Result.Data = Str.Data + Left;
  Result.Count = Str.Count - Left;
 }
 
 return Result;
}

internal string
StrPrefix(string Str, u64 Count)
{
 string Result = {};
 Result.Data = Str.Data;
 Result.Count = Min(Str.Count, Count);
 
 return Result;
}

internal b32
StrStartsWith(string Str, string Start)
{
 string Prefix = StrPrefix(Str, Start.Count);
 b32 Result = StrMatch(Prefix, Start, false);
 
 return Result;
}

internal string_list
StrSplit(arena *Arena, string Split, string On)
{
 string_list Result = {};
 
 char *At = Split.Data;
 u64 Left = Split.Count;
 char *Last = Split.Data;
 b32 Splitting = true;
 while (Splitting)
 {
  Splitting = (Left != 0);
  
  string Cur = MakeStr(At, Left);
  if (StrStartsWith(Cur, On))
  {
   if (At > Last)
   {
    string Add = MakeStr(Last, At - Last);
    StrListPush(Arena, &Result, Add);
   }
   
   At += On.Count;
   Left -= On.Count;
   Last = At;
  }
  else
  {
   ++At;
   --Left;
  }
 }
 
 return Result;
}

internal b32
StrContains(string S, string Sub)
{
 b32 Result = false;
 for (u64 I = Sub.Count; I <= S.Count; ++I)
 {
  string At = StrSubstr(S, I - Sub.Count, Sub.Count);
  if (StrMatch(At, Sub, false))
  {
   Result = true;
   break;
  }
 }
 
 return Result;
}

internal string
StrSubstr(string S, u64 Pos, u64 Count)
{
 string Result = {};
 if (Pos < S.Count)
 {
  Result.Data = S.Data + Pos;
  Result.Count = ClampTop(Count, S.Count - Pos);
 }
 
 return Result;
}

internal string
PathChopLastPart(string Str)
{
 char *At = Str.Data + (Str.Count - 1);
 while (At >= Str.Data && *At != '/' && *At != '\\')
 {
  --At;
 }
 string Result = {};
 Result.Data = Str.Data;
 if (At > Str.Data)
 {
  Result.Count = At - Str.Data;
 }
 if (Result.Count == 0)
 {
  Result = StrLit(".");
 }
 
 return Result;
}

internal string
PathLastPart(string Str)
{
 char *At = Str.Data + (Str.Count - 1);
 while (At >= Str.Data && *At != '/' && *At != '\\')
 {
  --At;
 }
 ++At;
 
 string Result = {};
 Result.Data = At;
 Result.Count = Str.Data + Str.Count - At;
 
 return Result;
}

internal string
PathConcat(arena *Arena, string A, string B)
{
 string_list Path = {};
 StrListPush(Arena, &Path, A);
 StrListPush(Arena, &Path, B);
 string Result = PathListJoin(Arena, &Path);
 
 return Result;
}

internal string
PathListJoin(arena *Arena, string_list *Path)
{
 string Sep = {};
#if OS_WINDOWS
 Sep = StrLit("\\");
#elif OS_LINUX
 Sep = StrLit("/");
#elif
# error unsupported OS
#endif
 string Result = StrListJoin(Arena, Path, Sep, StringListJoinFlag_SkipEmpty);
 return Result;
}

internal void
StrListPush(arena *Arena, string_list *List, string Str)
{
 string_list_node *Node = PushStructNonZero(Arena, string_list_node);
 Node->Str = Str;
 QueuePush(List->Head, List->Tail, Node);
 ++List->NodeCount;
 List->TotalSize += Str.Count;
}

internal void
StrListPushF(arena *Arena, string_list *List, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 StrListPushFV(Arena, List, Format, Args);
 va_end(Args);
}

internal void
StrListPushFV(arena *Arena, string_list *List, char const *Format, va_list Args)
{
 string S = StrFV(Arena, Format, Args);
 StrListPush(Arena, List, S);
}

internal string_list
StrListCopy(arena *Arena, string_list *List)
{
 string_list Copy = {};
 ListIter(Node, List->Head, string_list_node)
 {
  StrListPush(Arena, &Copy, Node->Str);
 }
 
 return Copy;
}

internal void
StrListConcatInPlace(string_list *List, string_list *ToPush)
{
 if (List->Tail)
 {
  List->Tail->Next = ToPush->Head;
  if (ToPush->Tail) List->Tail = ToPush->Tail;
  List->NodeCount += ToPush->NodeCount;
  List->TotalSize += ToPush->TotalSize;
 }
 else
 {
  *List = *ToPush;
 }
 StructZero(ToPush);
}

internal string
StrListJoin(arena *Arena, string_list *List, string Sep, string_list_join_flags Flags)
{
 string Result = {};
 u64 MaxSepTotalSize = 0;
 if (List->NodeCount > 0)
 {
  MaxSepTotalSize = Sep.Count * (List->NodeCount - 1);
 }
 u64 MaxSize = List->TotalSize + MaxSepTotalSize;
 Result.Data = PushArrayNonZero(Arena, MaxSize + 1, char);
 
 char *At = Result.Data;
 string CurSep = {};
 ListIter(Node, List->Head, string_list_node)
 {
  if ((Flags & StringListJoinFlag_SkipEmpty) && Node->Str.Count == 0)
  {
   // nothing to do
  }
  else
  {
   MemoryCopy(At, CurSep.Data, CurSep.Count);
   At += CurSep.Count;
   MemoryCopy(At, Node->Str.Data, Node->Str.Count);
   At += Node->Str.Count;
   CurSep = Sep;
  }
 }
 Assert(At >= Result.Data);
 Result.Count = Cast(u64)(At - Result.Data);
 *At++ = 0;
 
 return Result;
}
