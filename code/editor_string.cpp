//- String
internal string
Str(string_header *Header, char const *String, u64 Size)
{
   string Result = 0;
   
   if (Header)
   {
      Header->Size = Size;
      
      Result = Cast(string)(Header + 1);
      MemoryCopy(Result, String, Size * SizeOf(Result[0]));
      Result[Size] = '\0';
   }
   
   return Result;
}

function string
Str(char const *String, u64 Size)
{
   void *Pointer = HeapAllocSize(HeapAllocator(), SizeOf(string_header) + Size + 1);
   string Result = Str(Cast(string_header *)Pointer, String, Size);
   
   return Result;
}

function string
Str(arena *Arena, char const *CString)
{
   string Result = Str(Arena, CString, CStrLength(CString));
   return Result;
}

function string
Str(arena *Arena, char const *String, u64 Size)
{
   void *Pointer = ArenaPushSize(Arena, SizeOf(string_header) + Size + 1);
   string Result = Str(Cast(string_header *)Pointer, String, Size);
   
   return Result;
}

function void
FreeStr(string String)
{
   if (String)
   {
      string_header *Header = StringHeader(String);
      HeapFree(HeapAllocator(), Header);
   }
}

function u64
StrLength(string String)
{
   u64 Result = 0;
   if (String) Result = StringHeader(String)->Size;
   
   return Result;
}

function u64
CStrLength(char const *CString)
{
   char const *At = CString;
   while (*At) ++At;
   u64 Length = At - CString;
   
   return Length;
}

function string
StrF(arena *Arena, char const *Format, ...)
{
   va_list ArgList;
   va_start(ArgList, Format);
   
   string Result = StrFV(Arena, Format, ArgList);
   
   va_end(ArgList);
   
   return Result;
}

function string
StrFV(arena *Arena, char const *Format, va_list ArgList)
{
   va_list ArgListCopy;
   
   va_copy(ArgListCopy, ArgList);
   
   u64 Size = vsnprintf(0, 0, Format, ArgList);
   
   string_header *Header =
      Cast(string_header *)ArenaPushSize(Arena,
                                         SizeOf(string_header)+Size+1);
   Header->Size = Size;
   string String = Cast(string)(Header + 1);
   vsnprintf(String, Size + 1, Format, ArgListCopy);
   
   va_end(ArgListCopy);
   
   return String;
}

function string
DuplicateStr(arena *Arena, string String)
{
   string Result = Str(Arena, String, StrLength(String));
   return Result;
}

function string
DuplicateStr(string String)
{
   string Result = Str(String, StrLength(String));
   return Result;
}

internal b32
AreBytesEqual(char *A, char *B, u64 NumBytes)
{
   b32 Result = true;
   for (u64 I = 0; I < NumBytes; ++I)
   {
      if (A[I] != B[I])
      {
         Result = false;
         break;
      }
   }
   
   return Result;
}

function b32
AreStringsEqual(string A, string B)
{
   Assert(A); Assert(B);
   b32 Result = true;
   
   u64 ASize = StrLength(A);
   u64 BSize = StrLength(B);
   
   if (ASize == BSize) Result = AreBytesEqual(A, B, ASize);
   else Result = false;
   
   return Result;
}

function void
RemoveExtension(string Path)
{
   if (Path)
   {
      u64 Index = StrLength(Path);
      while (Index && Path[Index-1] != '.')
      {
         --Index;
      }
      if (Index) StringHeader(Path)->Size = Index-1;
   }
}

function b32
HasSuffix(string String, string Suffix)
{
   b32 Result = true;
   
   u64 StrSize = StrLength(String);
   u64 SuffixSize = StrLength(Suffix);
   
   if (StrSize >= SuffixSize)
   {
      Result = AreBytesEqual(String + (StrSize-SuffixSize),
                             Suffix, SuffixSize);
   }
   else Result = false;
   
   return Result;
}

function string
StringChopFileNameWithoutExtension(arena *Arena, string String)
{
   s64 LastSlashPosition = -1;
   for (s64 Index = StrLength(String);
        Index >= 0;
        --Index)
   {
      if (String[Index] == '/' ||
          String[Index] == '\\')
      {
         LastSlashPosition = Index;
         break;
      }
   }
   
   s64 LastDotPosition = -1;
   for (s64 Index = StrLength(String)-1;
        Index >= 0;
        --Index)
   {
      if (String[Index] == '.')
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
      ToExclusive = StrLength(String);
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

function string
Substr(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive)
{
   u64 StrSize = StrLength(String);
   if (FromInclusive > StrSize) FromInclusive = StrSize;
   if (ToExclusive > StrSize) ToExclusive = StrSize;
   if (FromInclusive > ToExclusive)
   {
      u64 Swap = FromInclusive;
      FromInclusive = ToExclusive;
      ToExclusive = Swap;
   }
   
   u64 Size = ToExclusive - FromInclusive;
   string Substring = Str(Arena, String + FromInclusive, Size);
   
   return Substring;
}

function char
ToUpper(char C)
{
   char Result = C;
   if ('a' <= C && C <= 'z')
   {
      Result = C-'a' + 'A';
   }
   
   return Result;
}

//- String List
function void
StringListPush(arena *Arena, string_list *List, string String)
{
   string_list_node *Node = PushStruct(Arena, string_list_node);
   Node->String = String;
   QueuePush(List->Head, List->Tail, Node);
   
   ++List->NumNodes;
   List->TotalSize += StrLength(String);
}
