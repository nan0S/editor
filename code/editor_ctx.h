#ifndef EDITOR_CTX_H
#define EDITOR_CTX_H

struct editor_ctx
{
 arena_store *ArenaStore;
 renderer_transfer_queue *RendererQueue;
 entity_store *EntityStore;
 thread_task_memory_store *ThreadTaskMemoryStore;
 image_loading_store *ImageLoadingStore;
 string_store *StrStore;
 curve_points_store *CurvePointsStore;
 struct work_queue *LowPriorityQueue;
 struct work_queue *HighPriorityQueue;
};

internal void InitEditorCtx(arena_store *ArenaStore,
                            renderer_transfer_queue *RendererQueue,
                            entity_store *EntityStore,
                            thread_task_memory_store *ThreadTaskMemoryStore,
                            image_loading_store *ImageLoadingStore,
                            string_store *StrStore,
                            curve_points_store *CurvePointsStore,
                            struct work_queue *LowPriorityQueue,
                            struct work_queue *HighPriorityQueue);

internal editor_ctx *GetCtx(void);

//~ helpers
internal curve_points_static *GetCurvePoints(curve *Curve);
internal u32 GetControlPointCount(curve *Curve);
internal void FillCharBuffer(string_id Dst, string_id Src);

#endif //EDITOR_CTX_H
