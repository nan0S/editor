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
   Result = TextureHandleFromIndex(Index);
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
  u32 Index = TextureIndexFromHandle(Handle);
  Assert(Index < Assets->TextureCount);
  Assert(Assets->IsTextureHandleAllocated[Index]);
  Assets->IsTextureHandleAllocated[Index] = false;
 }
}

internal void
InitEntityStore(entity_store *Store)
{
 for (u32 EntityIndex = 0;
      EntityIndex < ArrayCount(Store->EntityBuffer);
      ++EntityIndex)
 {
  entity *Entity = Store->EntityBuffer + EntityIndex;
  
  Entity->Arena = AllocArena(Megabytes(32));
  Entity->Curve.DegreeLowering.Arena = AllocArena(Megabytes(32));
  
  curve *Curve = &Entity->Curve;
  Curve->ParametricResources.Arena = AllocArena(Megabytes(32));
 }
}

internal entity *
AllocEntity(entity_store *Store)
{
 entity *Entity = Store->Free;
 entity *Entity = 0;
 
 for (u32 EntityIndex = 0;
      EntityIndex < ArrayCount(Store->EntityBuffer);
      ++EntityIndex)
 {
  entity *Current = Store->EntityBuffer + EntityIndex;
  if (!(Current->Flags & EntityFlag_Active))
  {
   Entity = Current;
   
   arena *EntityArena = Entity->Arena;
   arena *DegreeLoweringArena = Entity->Curve.DegreeLowering.Arena;
   arena *ParametricArena = Entity->Curve.ParametricResources.Arena;
   u32 Generation = Entity->Generation;
   
   StructZero(Entity);
   
   Entity->Arena = EntityArena;
   Entity->Curve.DegreeLowering.Arena = DegreeLoweringArena;
   Entity->Curve.ParametricResources.Arena = ParametricArena;
   Entity->Generation = Generation;
   
   Entity->Flags |= EntityFlag_Active;
   ClearArena(Entity->Arena);
   
   break;
  }
 }
 
 return Entity;
}

internal void
DeallocEntity(editor_assets *Assets, entity *Entity)
{
 if (Entity->Type == Entity_Image)
 {
  image *Image = &Entity->Image;
  DeallocTextureHandle(Assets, Image->TextureHandle);
 }
 
 Entity->Flags &= ~EntityFlag_Active;
 ++Entity->Generation;
}

internal entity_array
EntityArrayFromStore(entity_store *Store)
{
 entity_array Array = {};
 Array.Entities = Store->EntityBuffer;
 Array.Count = ArrayCount(Store->EntityBuffer);
 return Array;
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