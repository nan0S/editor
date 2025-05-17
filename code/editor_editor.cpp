internal render_texture_handle
AllocTextureHandle(editor_assets *Assets)
{
 render_texture_handle Result = TextureHandleZero();
 for (u32 Index = 0;
      Index < Assets->TextureCount;
      ++Index)
 {
  if (!Assets->IsTextureHandleAllocated[Index])
  {
   Result = TextureHandleFromIndex(Index + 1);
   Assets->IsTextureHandleAllocated[Index] = true;
   break;
  }
 }
 return Result;
}

internal void
DeallocTextureHandle(editor_assets *Assets, render_texture_handle Handle)
{
 if (!TextureHandleMatch(Handle, TextureHandleZero()))
 {
  u32 Index = TextureIndexFromHandle(Handle) - 1;
  Assert(Index < Assets->TextureCount);
  Assert(Assets->IsTextureHandleAllocated[Index]);
  Assets->IsTextureHandleAllocated[Index] = false;
 }
}

internal void
InitEntityStore(entity_store *Store)
{
 Store->Arena = AllocArena(Gigabytes(64));
 ForEachElement(Index, Store->ByTypeArenas)
 {
  Store->ByTypeArenas[Index] = AllocArena(Megabytes(1));
 }
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
 
 curve *Curve = &Entity->Curve;
 Entity->Generation = Generation;
 Entity->Arena = AllocArena(Megabytes(32));
 Entity->Flags = (DontTrack ? 0 : EntityFlag_Tracked);
 Curve->DegreeLowering.Arena = AllocArena(Megabytes(32));
 Curve->ParametricResources.Arena = AllocArena(Megabytes(32));
 
 if (Entity->Flags & EntityFlag_Tracked)
 {
  DLLPushBack(Store->Head, Store->Tail, Entity);
  ++Store->Count;
  ++Store->AllocGeneration;
 }
 
 return Entity;
}

internal void
DeallocEntity(entity_store *Store, editor_assets *Assets, entity *Entity)
{
 if (Entity->Type == Entity_Image)
 {
  image *Image = &Entity->Image;
  DeallocTextureHandle(Assets, Image->TextureHandle);
 }
 
 curve *Curve = &Entity->Curve;
 DeallocArena(Entity->Arena);
 DeallocArena(Curve->DegreeLowering.Arena);
 DeallocArena(Curve->ParametricResources.Arena);
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
InitTaskWithMemoryStore(task_with_memory_store *Store)
{
 Store->Arena = AllocArena(Gigabytes(1));
}

internal task_with_memory *
BeginTaskWithMemory(task_with_memory_store *Store)
{
 task_with_memory *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, task_with_memory);
 }
 StructZero(Task);
 Task->Arena = AllocArena(Gigabytes(1));
 return Task;
}

internal void
EndTaskWithMemory(task_with_memory_store *Store, task_with_memory *Task)
{
 DeallocArena(Task->Arena);
 StackPush(Store->Free, Task);
}

internal void
InitAsyncTaskStore(async_task_store *Store)
{
 Store->Arena = AllocArena(Gigabytes(1));
}

internal async_task *
AllocAsyncTask(async_task_store *Store)
{
 async_task *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, async_task);
 }
 StructZero(Task);
 
 Task->Arena = AllocArena(Gigabytes(1));
 DLLPushBack(Store->Head, Store->Tail, Task);
 
 return Task;
}

internal void
DeallocAsyncTask(async_task_store *Store, async_task *Task)
{
 DeallocArena(Task->Arena);
 DLLRemove(Store->Head, Store->Tail, Task);
 StackPush(Store->Free, Task);
}