#ifndef EDITOR_STRING_H
#define EDITOR_STRING_H

//- String
typedef char *string;
typedef string error_string;

struct string_header
{
   u64 Size;
};

#define StringHeader(String) (Cast(string_header *)(String) - 1)

// NOTE(hbr): Allocates on heap
function string Str(char const *String, u64 Size);
function string Str(arena *Arena, char const *CString);
function string Str(arena *Arena, char const *String, u64 Size);
function void FreeStr(string String);
function u64 StrLength(string String);
function u64 CStrLength(char const *CString);
function string StrF(arena *Arena, char const *Format, ...);
function string StrFV(arena *Arena, char const *Format, va_list ArgList);
function string DuplicateStr(arena *Arena, string String);
function string DuplicateStr(string String);
function b32 AreStringsEqual(string A, string B);
function void RemoveExtension(string Path);
function b32 HasSuffix(string String, string Suffix);
function string StringChopFileNameWithoutExtension(arena *Arena, string String);
function string Substr(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive);

function char ToUpper(char C);

#define StrLit(Lit) Str(Lit, ArrayCount(Lit)-1)
#define StrLitArena(Arena, Lit) Str(Arena, Lit, ArrayCount(Lit)-1)

//- String List
struct string_list_node
{
   string_list_node *Next;
   string String;
};

struct string_list
{
   string_list_node *Head;
   string_list_node *Tail;
   
   u64 NumNodes;
   u64 TotalSize;
};

function void StringListPush(arena *Arena, string_list *List, string String);

#endif //EDITOR_STRING_H
