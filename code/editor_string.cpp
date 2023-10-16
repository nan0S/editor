//- String
internal string
StringMake(string_header *Header, char const *String, u64 Size)
{
   string Result = 0;
   
   if (Header)
   {
      Header->Size = Size;
      
      Result = cast(string)(Header + 1);
      MemoryCopy(Result, String, Size * sizeof(Result[0]));
      Result[Size] = '\0';
   }
   
   return Result;
}

function string
StringMake(char const *String, u64 Size)
{
   void *Pointer = HeapAllocSize(HeapAllocator(), sizeof(string_header) + Size + 1);
   string Result = StringMake(cast(string_header *)Pointer, String, Size);
   
   return Result;
}

function string
StringMake(arena *Arena, char const *CString)
{
   string Result = StringMake(Arena, CString, CStringLength(CString));
   return Result;
}

function string
StringMake(arena *Arena, char const *String, u64 Size)
{
   void *Pointer = ArenaPushSize(Arena, sizeof(string_header) + Size + 1);
   string Result = StringMake(cast(string_header *)Pointer, String, Size);
   
   return Result;
}

function void
StringFree(string String)
{
   if (String)
   {
      string_header *Header = StringHeader(String);
      HeapFree(HeapAllocator(), Header);
   }
}

function u64
StringSize(string String)
{
   u64 Result = 0;
   if (String) Result = StringHeader(String)->Size;
   
   return Result;
}

function u64
CStringLength(char const *CString)
{
   char const *At = CString;
   while (*At) ++At;
   u64 Length = At - CString;
   
   return Length;
}

function string
StringMakeFormat(arena *Arena, char const *Format, ...)
{
   va_list ArgList;
   va_start(ArgList, Format);
   
   string Result = StringMakeFormatV(Arena, Format, ArgList);
   
   va_end(ArgList);
   
   return Result;
}

function string
StringMakeFormatV(arena *Arena, char const *Format, va_list ArgList)
{
   va_list ArgListCopy;
   
   va_copy(ArgListCopy, ArgList);
   
   u64 Size = vsnprintf(0, 0, Format, ArgList);
   
   string_header *Header =
      cast(string_header *)ArenaPushSize(Arena,
                                         sizeof(string_header)+Size+1);
   Header->Size = Size;
   string String = cast(string)(Header + 1);
   vsnprintf(String, Size + 1, Format, ArgListCopy);
   
   va_end(ArgListCopy);
   
   return String;
}

function string
StringDuplicate(arena *Arena, string String)
{
   string Result = StringMake(Arena, String, StringSize(String));
   return Result;
}

function string
StringDuplicate(string String)
{
   string Result = StringMake(String, StringSize(String));
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
StringsAreEqual(string A, string B)
{
   Assert(A); Assert(B);
   b32 Result = true;
   
   u64 ASize = StringSize(A);
   u64 BSize = StringSize(B);
   
   if (ASize == BSize) Result = AreBytesEqual(A, B, ASize);
   else Result = false;
   
   return Result;
}

function void
StringRemoveExtension(string Path)
{
   if (Path)
   {
      u64 Index = StringSize(Path);
      while (Index && Path[Index-1] != '.')
      {
         --Index;
      }
      if (Index) StringHeader(Path)->Size = Index-1;
   }
}

function b32
StringHasSuffix(string String, string Suffix)
{
   b32 Result = true;
   
   u64 StrSize = StringSize(String);
   u64 SuffixSize = StringSize(Suffix);
   
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
   for (s64 Index = StringSize(String);
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
   for (s64 Index = StringSize(String)-1;
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
      ToExclusive = StringSize(String);
   }
   else
   {
      ToExclusive = LastDotPosition;
   }
   
   string Result = StringSubstring(Arena, String,
                                   FromInclusive,
                                   ToExclusive);
   
   return Result;
}

function string
StringSubstring(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive)
{
   u64 StrSize = StringSize(String);
   if (FromInclusive > StrSize) FromInclusive = StrSize;
   if (ToExclusive > StrSize) ToExclusive = StrSize;
   if (FromInclusive > ToExclusive)
   {
      u64 Swap = FromInclusive;
      FromInclusive = ToExclusive;
      ToExclusive = Swap;
   }
   
   u64 Size = ToExclusive - FromInclusive;
   string Substring = StringMake(Arena, String + FromInclusive, Size);
   
   return Substring;
}

function char
ToUppercase(char C)
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
   List->TotalSize += StringSize(String);
}
