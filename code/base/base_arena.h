#ifndef EDITOR_MEMORY_H
#define EDITOR_MEMORY_H

struct arena
{
 arena *Next;
 arena *Cur;
 
 b32 Freed;
 
 void *Memory;
 u64 Used;
 u64 Capacity;
 u64 Commited;
 u64 Align;
};

struct temp_arena
{
 arena *Arena;
 arena *SavedCur;
 u64 SavedUsed;
};

internal arena *AllocArena(u64 ReserveButNotCommit);
internal void   DeallocArena(arena *Arena);
internal void   ClearArena(arena *Arena);
internal void * PushSize(arena *Arena, u64 Size);
internal void * PushSizeNonZero(arena *Arena, u64 Size);
#define         PushStruct(Arena, Type) Cast(Type *)PushSize(Arena, SizeOf(Type))
#define         PushStructNonZero(Arena, Type) Cast(Type *)PushSizeNonZero(Arena, SizeOf(Type))
#define         PushArray(Arena, Count, Type) Cast(Type *)PushSize(Arena, (Count) * SizeOf(Type))
#define         PushArrayNonZero(Arena, Count, Type) Cast(Type *)PushSizeNonZero(Arena, (Count) * SizeOf(Type))
#define         PushString(Arena, Length) MakeStr(PushArray(Arena, Length, char), Length)
#define         PushStringNonZero(Arena, Length) MakeStr(PushArrayNonZero(Arena, Length, char), Length)
#define         PushArray2D(Arena, Rows, Cols, Type) PushArray(Arena, (Rows) * (Cols), Type)
#define         PushArray2DNonZero(Arena, Rows, Cols, Type) PushArrayNonZero(Arena, (Rows) * (Cols), Type)
#define         PushArrayUntyped(Arena, Count, ElemSize) PushSize(Arena, (Count) * (ElemSize))
#define         PushArrayUntypedNonZero(Arena, Count, ElemSize) PushSizeNonZero(Arena, (Count) * (ElemSize))

internal temp_arena BeginTemp(arena *Arena);
internal void       EndTemp(temp_arena Temp);

#endif //EDITOR_MEMORY_H
