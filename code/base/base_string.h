#ifndef BASE_STRING_H
#define BASE_STRING_H

struct string
{
 char *Data;
 u64 Count;
};
read_only global string NilStr;

struct string_list_node
{
 string_list_node *Next;
 string Str;
};
struct string_list
{
 string_list_node *Head;
 string_list_node *Tail;
 u64 NodeCount;
 u64 TotalSize;
};
enum
{
 StringListJoinFlag_SkipEmpty = (1<<0),
};
typedef u64 string_list_join_flags;
struct string_list_join_options
{
 string Pre;
 string Sep;
 string Post;
 string_list_join_flags Flags;
};

struct arena;

struct char_buffer
{
 char *Data;
 u64 Count;
 u64 Capacity;
};
read_only global char_buffer NilCharBuffer;

struct deserialize_stream
{
 string Data;
 u64 At;
};

//- formatting
internal u64 Fmt(char *Buf, u64 BufSize, char const *Format, ...);
internal u64 FmtV(char *Buf, u64 BufSize, char const *Format, va_list Args);

//- char operations
internal b32         CharIsLower(char C);
internal b32         CharIsUpper(char C);
internal char        CharToLower(char C);
internal char        CharToUpper(char C);
internal b32         CharIsDigit(char C);
internal b32         CharIsWhiteSpace(char C);
internal b32         CharIsAlpha(char C);

//- string creation
internal string      MakeStr(char *Data, u64 Count);
#define              StrLit(Lit) MakeStr(Cast(char *)Lit, ArrayCount(Lit) - 1)
#define              StrLitComp(Lit) {Lit, ArrayCount(Lit)-1}
internal string      StrCopy(arena *Arena, string Str);
internal string      CStrCopy(arena *Arena, char const *Str);
internal string      StrF(arena *Arena, char const *Format, ...);
internal string      StrFV(arena *Arena, char const *Format, va_list Args);
internal string      StrFromCStr(char const *Str);
internal string      CStrFromStr(arena *Arena, string Str);
internal string      StrFromCharBuffer(char_buffer Buffer);
internal u64         CStrLen(char const *Str);

//- string operations
internal b32         StrMatch(string A, string B, b32 CaseInsensitive);
internal b32         StrEqual(string A, string B);
internal int         StrCmp(string A, string B);
internal b32         StrStartsWith(string Str, string Start);
internal b32         StrEndsWith(string Str, string End);
internal string      StrPrefix(string Str, u64 Count);
internal string      StrSuffix(string Str, u64 Count);
internal string      StrChop(string Str, u64 Chop);
internal string      StrChopLastSlash(string Str);
internal string      StrAfterLastSlash(string Str);
internal string      StrChopLastDot(string Str);
internal string      StrAfterLastDot(string Str);
internal string_list StrSplit(arena *Arena, string Split, string On);
internal b32         StrContains(string S, string Sub);
internal string      StrSubstr(string S, u64 Pos, u64 Count);
internal b32         StrIsEmpty(string S);

//- path manipulation
internal string      PathChopLastPart(string Path);
internal string      PathLastPart(string Path);
internal string      PathConcat(arena *Arena, string A, string B);
internal string      PathListJoin(arena *Arena, string_list *Path);

//- string list
internal void        StrListPush(arena *Arena, string_list *List, string Str);
internal void        StrListPushF(arena *Arena, string_list *List, char const *Format, ...);
internal void        StrListPushFV(arena *Arena, string_list *List, char const *Format, va_list Args);
internal string      StrListJoin(arena *Arena, string_list *List, string_list_join_options Opts);
internal string_list StrListCopy(arena *Arena, string_list *List);
internal void        StrListConcatInPlace(string_list *List, string_list *ToPush);

//- serialize/deserialize
internal deserialize_stream MakeDeserializeStream(string Data);
internal void DeserializeData(deserialize_stream *Stream, void *Dst, u64 Size);
internal string DeserializeString(arena *Arena, deserialize_stream *Stream);
#define DeserializeStruct(Stream, Ptr) DeserializeData(Stream, Ptr, SizeOf(*(Ptr)))
#define DeserializeArray(Stream, Array, Count) DeserializeData(Stream, Array, (Count) * SizeOf(*(Array)))

internal void SerializeData(arena *Arena, string_list *List, void *Data, u64 Size);
internal void SerializeString(arena *Arena, string_list *List, string Str);
#define SerializeStruct(Arena, List, Ptr) SerializeData(Arena, List, Ptr, SizeOf(*(Ptr)))
#define SerializeArray(Arena, List, Array, Count) SerializeData(Arena, List, Array, (Count) * (SizeOf(*(Array))))

#endif //BASE_STRING_H
