#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

read_only global u64 StringBucketChunkSizes[]
{
 16,
 64,
 256,
 1024,
 4096,
 16384,
 65536,
 U64_MAX,
};
struct string_chunk_node
{
 string_chunk_node *Next;
 u64 Size;
};
struct string_cache
{
 arena *Arena;
 string_chunk_node *FreeStringChunks[ArrayCount(StringBucketChunkSizes)];
};

//- string cache
internal void InitStringCache(string_cache *Cache);
internal char_buffer AllocString(string_cache *Cache, u64 Size);
internal void DeallocString(string_cache *Cache, char_buffer Buffer);

#endif //BASE_MEMORY_H
