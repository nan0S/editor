#ifndef EDITOR_STRING_H
#define EDITOR_STRING_H

struct string
{
 char *Data;
 u64 Count;
};

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

struct arena;

internal u64 Fmt(char *Buf, u64 BufSize, char const *Format, ...);
internal u64 FmtV(char *Buf, u64 BufSize, char const *Format, va_list Args);

internal b32         CharIsLower(char C);
internal b32         CharIsUpper(char C);
internal char        CharToLower(char C);
internal char        CharToUpper(char C);
internal b32         CharIsDigit(char C);

internal string      MakeStr(char *Data, u64 Count);
#define              StrLit(Lit) MakeStr(Cast(char *)Lit, ArrayCount(Lit) - 1)
internal string      StrCopy(arena *Arena, string Str);
internal string      CStrCopy(arena *Arena, char const *Str);
internal string      StrF(arena *Arena, char const *Format, ...);
internal string      StrFV(arena *Arena, char const *Format, va_list Args);
internal string      StrFromCStr(char const *Str);
internal string      CStrFromStr(arena *Arena, string Str);
internal u64         CStrLen(char const *Str);

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

internal string      PathChopLastPart(string Path);
internal string      PathLastPart(string Path);
internal string      PathConcat(arena *Arena, string A, string B);
internal string      PathListJoin(arena *Arena, string_list *Path);

internal void        StrListPush(arena *Arena, string_list *List, string Str);
internal string      StrListJoin(arena *Arena, string_list *List, string Sep);
internal string_list StrListCopy(arena *Arena, string_list *List);
internal void        StrListConcatInPlace(string_list *List, string_list *ToPush);

#endif //EDITOR_STRING_H
