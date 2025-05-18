internal render_texture_handle
AllocTextureHandle(entity_store *Store)
{
 render_texture_handle Result = TextureHandleZero();
 for (u32 Index = 0;
      Index < Store->TextureCount;
      ++Index)
 {
  if (!Store->IsTextureHandleAllocated[Index])
  {
   Result = TextureHandleFromIndex(Index + 1);
   Store->IsTextureHandleAllocated[Index] = true;
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
  u32 Index = TextureIndexFromHandle(Handle) - 1;
  Assert(Index < Store->TextureCount);
  Assert(Store->IsTextureHandleAllocated[Index]);
  Store->IsTextureHandleAllocated[Index] = false;
 }
}

internal void
InitEntityStore(entity_store *Store,
                u32 MaxTextureCount,
                u32 MaxBufferCount)
{
 arena *Arena = AllocArena(Gigabytes(64));
 Store->Arena = Arena;
 
 ForEachElement(Index, Store->ByTypeArenas)
 {
  Store->ByTypeArenas[Index] = AllocArena(Megabytes(1));
 }
 
 Store->TextureCount = MaxTextureCount;
 Store->IsTextureHandleAllocated = PushArray(Arena, MaxTextureCount, b32);
 Store->BufferCount = MaxBufferCount;
 Store->IsBufferHandleAllocated = PushArray(Arena, MaxBufferCount, b32);
}

internal entity *
AllocEntity(entity_store *Store, entity_type Type, b32 DontTrack)
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
 
 Entity->Generation = Generation;
 Entity->Type = Type;
 Entity->Flags = (DontTrack ? 0 : EntityFlag_Tracked);
 
 switch (Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   Curve->ComputeArena = AllocArena(Megabytes(32));
   Curve->DegreeLowering.Arena = AllocArena(Megabytes(32));
   Curve->ParametricResources.Arena = AllocArena(Megabytes(32));
  }break;
  
  case Entity_Image: {
   image *Image = &Entity->Image;
   render_texture_handle TextureHandle = AllocTextureHandle(Store);
   Image->TextureHandle = TextureHandle;
  }break;
  
  case Entity_Count: InvalidPath;
 }
 
 if (Entity->Flags & EntityFlag_Tracked)
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
 
 if (Entity->Flags & EntityFlag_Tracked)
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