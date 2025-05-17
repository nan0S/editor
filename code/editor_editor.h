#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

struct entity_store
{
 arena *Arena;
 entity *Free;
 entity *Head;
 entity *Tail;
 u32 Count;
 arena *ByTypeArenas[Entity_Count + 1];
 entity_array ByTypeArrays[Entity_Count + 1];
 u32 ByTypeGenerations[Entity_Count + 1];
 u32 AllocGeneration;
 
 u32 TextureCount;
 b32 *IsTextureHandleAllocated;
 u32 BufferCount;
 b32 *IsBufferHandleAllocated;
};
internal void InitEntityStore(entity_store *Store, u32 MaxTextureCount, u32 MaxBufferCount);
internal entity *AllocEntity(entity_store *Store, entity_type Type, b32 DontTrack);
internal void DeallocEntity(entity *Entity, struct editor_assets *Assets);
internal entity_array AllEntityArrayFromStore(entity_store *Store);
internal entity_array EntityArrayFromType(entity_store *Store, entity_type Type);

struct task_with_memory
{
 task_with_memory *Next;
 arena *Arena;
};
struct task_with_memory_store
{
 arena *Arena;
 task_with_memory *Free;
};
internal void InitTaskWithMemoryStore(task_with_memory_store *Store);
internal task_with_memory *BeginTaskWithMemory(task_with_memory_store *Store);
internal void EndTaskWithMemory(task_with_memory_store *Store, task_with_memory *Task);

enum image_loading_state
{
 Image_Loading,
 Image_Loaded,
 Image_Failed,
};
// NOTE(hbr): Maybe async tasks (that general name) is not the best here.
// It is currently just used for loading images asynchronously (using threads).
// I'm happy to generalize this (for example store function pointers in here),
// but for now I only have images use case and don't expect new stuff. I didn't
// bother renaming this.
struct async_task
{
 async_task *Next;
 async_task *Prev;
 
 arena *Arena;
 image_loading_state State;
 entity *Entity;
 u32 ImageWidth;
 u32 ImageHeight;
 string ImageFilePath;
 v2 AtP;
};
struct async_task_array
{
 async_task *Tasks;
 u32 Count;
};
struct async_task_store
{
 arena *Arena;
 async_task *Head;
 async_task *Tail;
 async_task *Free;
};
internal void InitAsyncTaskStore(async_task_store *Store);
internal async_task *AllocAsyncTask(async_task_store *Store);
internal void DeallocAsyncTask(async_task_store *Store, async_task *Task);

#endif //EDITOR_EDITOR_H
