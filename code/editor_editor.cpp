internal renderer_index *
AllocTextureIndex(editor_assets *Assets)
{
 renderer_index *Result = Assets->FirstFreeTextureIndex;
 if (Result)
 {
  Assets->FirstFreeTextureIndex = Result->Next;
 }
 return Result;
}

internal void
DeallocTextureIndex(editor_assets *Assets, renderer_index *Index)
{
 if (Index)
 {
  Index->Next = Assets->FirstFreeTextureIndex;
  Assets->FirstFreeTextureIndex = Index;
 }
}

internal void
AllocEntityResources(entity *Entity)
{
 Entity->Arena = AllocArena(Megabytes(32));
 Entity->Curve.DegreeLowering.Arena = AllocArena(Megabytes(32));
 
 curve *Curve = &Entity->Curve;
 Curve->ParametricResources.Arena = AllocArena(Megabytes(32));
}

internal void
InitEntityStore(entity_store *Store)
{
 for (u32 EntityIndex = 0;
      EntityIndex < ArrayCount(Store->EntityBuffer);
      ++EntityIndex)
 {
  entity *Entity = Store->EntityBuffer + EntityIndex;
  AllocEntityResources(Entity);
 }
}

internal entity *
AllocEntity(entity_store *Store)
{
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
  DeallocTextureIndex(Assets, Image->TextureIndex);
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

internal task_with_memory *
BeginTaskWithMemory(task_with_memory_store *Store)
{
 task_with_memory *Result = 0;
 for (u32 TaskIndex = 0;
      TaskIndex < ArrayCount(Store->Tasks);
      ++TaskIndex)
 {
  task_with_memory *Task = Store->Tasks + TaskIndex;
  if (!Task->Allocated)
  {
   Task->Allocated = true;
   if (!Task->Arena)
   {
    Task->Arena = AllocArena(Megabytes(1));
   }
   Result = Task;
   break;
  }
 }
 
 return Result;
}

internal void
EndTaskWithMemory(task_with_memory *Task)
{
 Assert(Task->Allocated);
 Task->Allocated = false;
 ClearArena(Task->Arena);
}

internal async_task *
AllocAsyncTask(async_task_store *Store)
{
 async_task *Result = 0;
 
 for (u32 TaskIndex = 0;
      TaskIndex < ArrayCount(Store->Tasks);
      ++TaskIndex)
 {
  async_task *Task = Store->Tasks + TaskIndex;
  if (!Task->Active)
  {
   Result = Task;
   Task->Active = true;
   if (!Task->Arena)
   {
    Task->Arena = AllocArena(Megabytes(1));
   }
   Task->State = Image_Loading;
   break;
  }
 }
 
 return Result;
}

internal void
DeallocAsyncTask(async_task *Task)
{
 Task->Active = false;
 ClearArena(Task->Arena);
}

internal void
InitTaskWithMemoryStore(task_with_memory_store *Store)
{
 // NOTE(hbr): Initialize just a few, others will probably never be initialized
 // but if they do, then they will be initialized lazily.
 for (u32 TaskIndex = 0;
      TaskIndex < Min(ArrayCount(Store->Tasks), 4);
      ++TaskIndex)
 {
  task_with_memory *Task = Store->Tasks + TaskIndex;
  Task->Arena = AllocArena(Megabytes(1));
 }
}

internal void
InitAsyncTaskStore(async_task_store *Store)
{
 // NOTE(hbr): Initialize just a few, others will probably never be initialized
 // but if they do, then they will be initialized lazily.
 for (u32 TaskIndex = 0;
      TaskIndex < Min(ArrayCount(Store->Tasks), 4);
      ++TaskIndex)
 {
  async_task *Task = Store->Tasks + TaskIndex;
  Task->Arena = AllocArena(Megabytes(1));
 }
}

internal async_task_array
AsyncTaskArrayFromStore(async_task_store *Store)
{
 async_task_array Array = {};
 Array.Tasks = Store->Tasks;
 Array.Count = ArrayCount(Store->Tasks);
 return Array;
}