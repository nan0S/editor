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
function string StringMake(char const *String, u64 Size);
function string StringMake(arena *Arena, char const *CString);
function string StringMake(arena *Arena, char const *String, u64 Size);
function void StringFree(string String);
function u64 StringSize(string String);
function u64 CStringLength(char const *CString);
function string StringMakeFormat(arena *Arena, char const *Format, ...);
function string StringMakeFormatV(arena *Arena, char const *Format, va_list ArgList);
function string StringDuplicate(arena *Arena, string String);
function string StringDuplicate(string String);
function b32 StringsAreEqual(string A, string B);
function void StringRemoveExtension(string Path);
function b32 StringHasSuffix(string String, string Suffix);
function string StringChopFileNameWithoutExtension(arena *Arena, string String);
function string StringSubstring(arena *Arena, string String, u64 FromInclusive, u64 ToExclusive);

function char ToUppercase(char C);

#define StringMakeLit(Lit) StringMake(Lit, ArrayCount(Lit)-1)
#define StringMakeLitArena(Arena, Lit) StringMake(Arena, Lit, ArrayCount(Lit)-1)

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
