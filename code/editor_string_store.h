#ifndef EDITOR_STRING_STORE_H
#define EDITOR_STRING_STORE_H

struct string_id
{
 u32 Index;
};

struct string_store
{
 // TODO(hbr): Keep a pointer instead
 string_cache StrCache;
 arena *Arena;
 u32 StrCount;
 u32 StrCapacity;
 char_buffer *Strs;
};

internal string_store *AllocStringStore(arena *PermamentArena);
internal string_id AllocStringOfSize(string_store *Store, u64 Size);
internal string_id AllocStringFromString(string_store *Store, string Str);
internal char_buffer *CharBufferFromStringId(string_store *Store, string_id Id);
internal string StringFromStringId(string_store *Store, string_id Id);

#endif //EDITOR_STRING_STORE_H
