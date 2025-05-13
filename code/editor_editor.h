#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

struct entity_store
{
 entity EntityBuffer[1024];
};

struct task_with_memory
{
 b32 Allocated;
 arena *Arena;
};

struct task_with_memory_store
{
 task_with_memory Tasks[128];
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
struct async_task
{
 b32 Active;
 arena *Arena;
 image_loading_state State;
 u32 ImageWidth;
 u32 ImageHeight;
 string ImageFilePath;
 v2 AtP;
 renderer_index *TextureIndex;
};

struct async_task_array
{
 async_task *Tasks;
 u32 Count;
};

struct async_task_store
{
 async_task Tasks[128];
};

internal void InitEntityStore(entity_store *Store);
internal entity *AllocEntity(entity_store *Store);
internal void DeallocEntity(entity *Entity, struct editor_assets *Assets);
internal entity_array EntityArrayFromStore(entity_store *Store);

#endif //EDITOR_EDITOR_H
