global editor_ctx GlobalEditorCtx;

internal void
InitEditorCtx(arena_store *ArenaStore,
              renderer_transfer_queue *RendererQueue,
              entity_store *EntityStore,
              thread_task_memory_store *ThreadTaskMemoryStore,
              image_loading_store *ImageLoadingStore,
              string_store *StrStore,
              curve_points_store *CurvePointsStore,
              struct work_queue *LowPriorityQueue,
              struct work_queue *HighPriorityQueue)
{
 editor_ctx *Ctx = &GlobalEditorCtx;
 Ctx->ArenaStore = ArenaStore;
 Ctx->RendererQueue = RendererQueue;
 Ctx->EntityStore = EntityStore;
 Ctx->ThreadTaskMemoryStore = ThreadTaskMemoryStore;
 Ctx->ImageLoadingStore = ImageLoadingStore;
 Ctx->StrStore = StrStore;
 Ctx->CurvePointsStore = CurvePointsStore;
 Ctx->LowPriorityQueue = LowPriorityQueue;
 Ctx->HighPriorityQueue = HighPriorityQueue;
}

internal editor_ctx *
GetCtx(void)
{
 return &GlobalEditorCtx;
}

inline internal curve_points_static *
GetCurvePoints(curve *Curve)
{
 curve_points_static *Points = CurvePointsFromId(GetCtx()->CurvePointsStore, Curve->Points);
 return Points;
}

inline internal u32
GetControlPointCount(curve *Curve)
{
 curve_points_static *Points = GetCurvePoints(Curve);
 u32 Count = Points->ControlPointCount;
 return Count;
}

internal void
FillCharBuffer(string_id Dst, string_id Src)
{
 string_store *StrStore = GetCtx()->StrStore;
 char_buffer *DstBuffer = CharBufferFromStringId(StrStore, Dst);
 FillCharBuffer(DstBuffer, StringFromStringId(StrStore, Src));
}