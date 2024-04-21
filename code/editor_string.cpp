//- String
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

function string
Str(char const *String, u64 Count)
{
   char *Data = Cast(char *)HeapAllocSize(HeapAllocator(), Count + 1);
   string Result = CreateAndCopyStr(Data, String, Count);
   
   return Result;
}

function string
Str(arena *Arena, char const *String, u64 Count)
{
   char *Data = Cast(char *)PushSize(Arena, Count + 1);
   string Result = CreateAndCopyStr(Data, String, Count);
   
   return Result;
}

function string
StrC(arena *Arena, char const *String)
{
   string Result = Str(Arena, String, CStrLength(String));
   return Result;
}

// TODO(hbr): Make it accept pointer instead
function void
FreeStr(string *String)
{
   HeapFree(HeapAllocator(), String->Data);
   String->Data = 0;
   String->Count = 0;
}

function u64
CStrLength(char const *String)
{
   char const *At = String;
   while (*At) ++At;
   u64 Length = At - String;
   
   return Length;
}

function string
StrF(arena *Arena, char const *Fmt, ...)
{
   string Result = {};
   
   va_list Args;
   DeferBlock(va_start(Args, Fmt), va_end(Args))
   {
      Result = StrFV(Arena, Fmt, Args);
   }
   
   return Result;
}

function string
StrFV(arena *Arena, char const *Fmt, va_list Args)
{
   string Result = {};
   
   va_list ArgsCopy;
   DeferBlock(va_copy(ArgsCopy, Args), va_end(ArgsCopy))
   {
      u64 Count = vsnprintf(0, 0, Fmt, Args);
      
      char *Data = Cast(char *)PushSize(Arena, Count + 1);
      vsnprintf(Data, Count + 1, Fmt, ArgsCopy);
      Data[Count] = 0;
      
      Result.Data = Data;
      Result.Count = Count; 
   }
   
   return Result;
}

function string
DuplicateStr(arena *Arena, string String)
{
   string Result = Str(Arena, String.Data, String.Count);
   return Result;
}

function string
DuplicateStr(string String)
{
   string Result = Str(String.Data, String.Count);
   return Result;
}

function b32
AreStringsEqual(string A, string B)
{
   b32 Result = false;
   if (A.Count == B.Count)
   {
      Result = MemoryEqual(A.Data, B.Data, A.Count);
   }
   
   return Result;
}

// TODO(hbr): Make this function return string instead
function void
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

function b32
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

function string
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

function string
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

function b32
IsValid(string String)
{
   b32 Result = (String.Data != 0);
   return Result;
}

function b32 IsError(error_string String) { return IsValid(String); }

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
   string_list_node *Node = PushStructNonZero(Arena, string_list_node);
   Node->String = String;
   QueuePush(List->Head, List->Tail, Node);
   
   ++List->NumNodes;
   List->TotalSize += String.Count;
}
