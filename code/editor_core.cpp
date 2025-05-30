internal u32
InternalIndexFromTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 u32 Index = TextureIndexFromHandle(Handle) - 1;
 Assert(Index < Store->TextureCount);
 return Index;
}

internal render_texture_handle
TextureHandleFromInternalIndex(u32 Index)
{
 render_texture_handle Handle = TextureHandleFromIndex(Index + 1);
 return Handle;
}

internal render_texture_handle
AllocTextureHandle(entity_store *Store)
{
 render_texture_handle Result = TextureHandleZero();
 for (u32 Index = 0;
      Index < Store->TextureCount;
      ++Index)
 {
  if (Store->TextureHandleRefCount[Index] == 0)
  {
   Result = TextureHandleFromInternalIndex(Index);
   ++Store->TextureHandleRefCount[Index];
   break;
  }
 }
 return Result;
}

internal void
DeallocTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 if (!TextureHandleMatch(Handle, TextureHandleZero()))
 {
  u32 Index = InternalIndexFromTextureHandle(Store, Handle);
  Assert(Store->TextureHandleRefCount[Index] != 0);
  --Store->TextureHandleRefCount[Index];
 }
}

internal render_texture_handle
CopyTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 u32 Index = InternalIndexFromTextureHandle(Store, Handle);
 ++Store->TextureHandleRefCount[Index];
 return Handle;
}

internal void
InitEntityStore(entity_store *Store,
                u32 MaxTextureCount,
                u32 MaxBufferCount,
                string_cache *StrCache)
{
 arena *Arena = AllocArena(Gigabytes(64));
 Store->Arena = Arena;
 ForEachElement(Index, Store->ByTypeArenas)
 {
  Store->ByTypeArenas[Index] = AllocArena(Megabytes(1));
 }
 Store->TextureCount = MaxTextureCount;
 Store->TextureHandleRefCount = PushArray(Arena, MaxTextureCount, b32);
 Store->BufferCount = MaxBufferCount;
 Store->IsBufferHandleAllocated = PushArray(Arena, MaxBufferCount, b32);
 Store->StrCache = StrCache;
}

internal entity *
AllocEntity(entity_store *Store, b32 DontTrack)
{
 entity *Entity = Store->Free;
 if (Entity)
 {
  StackPop(Store->Free);
 }
 else
 {
  Entity = PushStructNonZero(Store->Arena, entity);
 }
 u32 Generation = Entity->Generation;
 StructZero(Entity);
 
 Entity->Id = Store->IdCounter++;
 Entity->Generation = Generation;
 Entity->InternalFlags = (DontTrack ? 0 : EntityInternalFlag_Tracked);
 Entity->NameBuffer = AllocString(Store->StrCache, 128);
 
 // NOTE(hbr): It shouldn't really matter anyway that we allocate arenas even for
 // entities other than curves.
 curve *Curve = &Entity->Curve;
 Curve->ComputeArena = AllocArena(Megabytes(32));
 Curve->DegreeLowering.Arena = AllocArena(Megabytes(32));
 Curve->ParametricResources.Arena = AllocArena(Megabytes(32));
 
 if (Entity->InternalFlags & EntityInternalFlag_Tracked)
 {
  DLLPushBack(Store->Head, Store->Tail, Entity);
  ++Store->Count;
  ++Store->AllocGeneration;
 }
 
 return Entity;
}

internal void
DeallocEntity(entity_store *Store, entity *Entity)
{
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   DeallocArena(Curve->ComputeArena);
   DeallocArena(Curve->DegreeLowering.Arena);
   DeallocArena(Curve->ParametricResources.Arena);
  }break;
  
  case Entity_Image: {
   image *Image = &Entity->Image;
   DeallocTextureHandle(Store, Image->TextureHandle);
  }break;
  
  case Entity_Count: InvalidPath;
 }
 
 ++Entity->Generation;
 
 if (Entity->InternalFlags & EntityInternalFlag_Tracked)
 {
  DLLRemove(Store->Head, Store->Tail, Entity);
  --Store->Count;
  ++Store->AllocGeneration;
 }
 
 StackPush(Store->Free, Entity);
}

internal entity_array
AllEntityArrayFromStore(entity_store *Store)
{
 entity_array Array = EntityArrayFromType(Store, Entity_Count);
 return Array;
}

internal entity_array
EntityArrayFromType(entity_store *Store, entity_type Type)
{
 if (Store->AllocGeneration != Store->ByTypeGenerations[Type])
 {
  arena *Arena = Store->ByTypeArenas[Type];
  ClearArena(Arena);
  
  entity_array Array = {};
  u32 MaxCount = Store->Count;
  Array.Entities = PushArrayNonZero(Arena, MaxCount, entity *);
  entity **At = Array.Entities;
  ListIter(Entity, Store->Head, entity)
  {
   if (Type == Entity_Count || (Type == Entity->Type))
   {
    *At++ = Entity;
   }
  }
  u32 ActualCount = Cast(u32)(At - Array.Entities);
  Assert(ActualCount <= MaxCount);
  Array.Count = ActualCount;
  
  Store->ByTypeArrays[Type] = Array;
  Store->ByTypeGenerations[Type] = Store->AllocGeneration;
 }
 return Store->ByTypeArrays[Type];
}

internal void
InitThreadTaskMemoryStore(thread_task_memory_store *Store)
{
 Store->Arena = AllocArena(Gigabytes(1));
}

internal thread_task_memory *
BeginThreadTaskMemory(thread_task_memory_store *Store)
{
 thread_task_memory *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, thread_task_memory);
 }
 StructZero(Task);
 Task->Arena = AllocArena(Gigabytes(1));
 return Task;
}

internal void
EndThreadTaskMemory(thread_task_memory_store *Store, thread_task_memory *Task)
{
 DeallocArena(Task->Arena);
 StackPush(Store->Free, Task);
}

internal void
InitImageLoadingStore(image_loading_store *Store)
{
 Store->Arena = AllocArena(Gigabytes(1));
}

internal image_loading_task *
BeginAsyncImageLoadingTask(image_loading_store *Store)
{
 image_loading_task *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, image_loading_task);
 }
 StructZero(Task);
 
 Task->Arena = AllocArena(Gigabytes(1));
 DLLPushBack(Store->Head, Store->Tail, Task);
 
 return Task;
}

internal void
FinishAsyncImageLoadingTask(image_loading_store *Store, image_loading_task *Task)
{
 DeallocArena(Task->Arena);
 DLLRemove(Store->Head, Store->Tail, Task);
 StackPush(Store->Free, Task);
}
