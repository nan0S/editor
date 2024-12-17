#ifndef BASE_ARENA_H
#define BASE_ARENA_H

struct arena
{
 void *Memory;
 u64 Used;
 u64 Capacity;
 u64 Commited;
};

struct temp_arena
{
 arena *Arena;
 u64 SavedUsed;
};

internal arena *AllocArena(u64 Reserve);
internal void ClearArena(arena *Arena);
internal void *PushSize(arena *Arena, u64 Size);
internal void *PushSizeNonZero(arena *Arena, u64 Size);

#define PushArray(Arena, Count, Type) (Cast(Type *)PushSize(Arena, (Count) * SizeOf(Type)))
#define PushStruct(Arena, Type) PushArray(Arena, 1, Type)
#define PushString(Arena, Length) MakeStr(PushArray(Arena, Length, char), Length)
#define PushArrayNonZero(Arena, Count, Type) (Cast(Type *)PushSizeNonZero(Arena, (Count) * SizeOf(Type)))
#define PushStructNonZero(Arena, Type) PushArrayNonZero(Arena, 1, Type)
#define PushStringNonZero(Arena, Length) MakeStr(PushArrayNonZero(Arena, Length, char), Length)

internal temp_arena BeginTemp(arena *Arena);
internal void EndTemp(temp_arena Temp);

#endif //BASE_ARENA_H
