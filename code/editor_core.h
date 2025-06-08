#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

struct entity_list_node
{
 entity_list_node *Next;
 entity_list_node *Prev;
 entity Entity;
};

struct entity_store
{
 arena *Arena;
 entity_list_node *Free;
 entity_list_node *Head;
 entity_list_node *Tail;
 u32 Count;
 arena *ByTypeArenas[Entity_Count + 1];
 entity_array ByTypeArrays[Entity_Count + 1];
 u32 ByTypeGenerations[Entity_Count + 1];
 u32 AllocGeneration;
 u32 IdCounter;
 u32 TextureCount;
 b32 *TextureHandleRefCount;
 u32 BufferCount;
 b32 *IsBufferHandleAllocated;
 string_cache *StrCache;
};

struct thread_task_memory
{
 thread_task_memory *Next;
 arena *Arena;
};
struct thread_task_memory_store
{
 arena *Arena;
 thread_task_memory *Free;
};

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
struct image_loading_task
{
 image_loading_task *Next;
 image_loading_task *Prev;
 
 arena *Arena;
 image_loading_state State;
 entity *Entity;
 string ImageFilePath;
};
struct image_loading_store
{
 arena *Arena;
 image_loading_task *Head;
 image_loading_task *Tail;
 image_loading_task *Free;
};

//- entity store
internal void InitEntityStore(entity_store *Store, u32 MaxTextureCount, u32 MaxBufferCount, string_cache *StrCache);
internal entity *AllocEntity(entity_store *Store, b32 DontTrack);
internal void DeallocEntity(entity_store *Store, entity *Entity);
internal entity_array AllEntityArrayFromStore(entity_store *Store);
internal entity_array EntityArrayFromType(entity_store *Store, entity_type Type);

internal render_texture_handle AllocTextureHandle(entity_store *Store);
internal void DeallocTextureHandle(entity_store *Store, render_texture_handle Handle);
internal render_texture_handle CopyTextureHandle(entity_store *Store, render_texture_handle Handle);

//- thread task with memory
internal void InitThreadTaskMemoryStore(thread_task_memory_store *Store);
internal thread_task_memory *BeginThreadTaskMemory(thread_task_memory_store *Store);
internal void EndThreadTaskMemory(thread_task_memory_store *Store, thread_task_memory *Task);

//- async store
internal void InitAsyncTaskStore(image_loading_store *Store);
internal image_loading_task *BeginAsyncImageLoadingTask(image_loading_store *Store);
internal void FinishAsyncImageLoadingTask(image_loading_store *Store, image_loading_task *Task);

#endif //EDITOR_CORE_H
