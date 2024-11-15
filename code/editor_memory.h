#ifndef EDITOR_MEMORY_H
#define EDITOR_MEMORY_H

struct arena
{
 arena *Next;
 arena *Cur;
 
 void *Memory;
 u64 Used;
 u64 Capacity;
 u64 Align;
};

struct temp_arena
{
 arena *Arena;
 arena *SavedCur;
 u64 SavedUsed;
};

internal arena *AllocArena(void);
internal void   DeallocArena(arena *Arena);
internal void   ClearArena(arena *Arena);
internal void * PushSize(arena *Arena, u64 Size);
internal void * PushSizeNonZero(arena *Arena, u64 Size);
#define         PushStruct(Arena, Type) Cast(Type *)PushSize(Arena, SizeOf(Type))
#define         PushStructNonZero(Arena, Type) Cast(Type *)PushSizeNonZero(Arena, SizeOf(Type))
#define         PushArray(Arena, Count, Type) Cast(Type *)PushSize(Arena, (Count) * SizeOf(Type))
#define         PushArrayNonZero(Arena, Count, Type) Cast(Type *)PushSizeNonZero(Arena, (Count) * SizeOf(Type))

internal temp_arena BeginTemp(arena *Arena);
internal void       EndTemp(temp_arena Temp);

#endif //EDITOR_MEMORY_H
