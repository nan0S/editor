internal void
InitLeftClickState(editor_left_click_state *LeftClick, arena_store *ArenaStore)
{
 LeftClick->OriginalVerticesArena = AllocArenaFromStore(ArenaStore, Megabytes(128));
}

internal void
InitFrameStats(frame_stats *FrameStats)
{
 FrameStats->Calculation.MinFrameTime = +F32_INF;
 FrameStats->Calculation.MaxFrameTime = -F32_INF;
}

internal void
InitMergingCurvesState(merging_curves_state *State,
                       entity_store *EntityStore)
{
 State->MergeEntity = AllocEntity(EntityStore, true);
}

internal void
InitVisualProfiler(visual_profiler_state *VisualProfiler, profiler *Profiler)
{
 VisualProfiler->ReferenceMs = VisualProfiler->DefaultReferenceMs = 1000.0f / 120;
 VisualProfiler->Profiler = Profiler;
}

internal void
InitAnimatingCurvesState(animating_curves_state *State,
                         entity_store *EntityStore)
{
 State->ExtractEntity = AllocEntity(EntityStore, true);
}

internal string
GetBaseProjectTitle(editor *Editor)
{
 string BaseTitle = {};
 if (Editor->IsProjectFileBacked)
 {  
  BaseTitle = StrAfterLastSlash(Editor->ProjectFilePath);
 }
 else
 {
  BaseTitle = StrLit("Untitled");
 }
 return BaseTitle;
}

internal void
UpdateWindowTitle(editor *Editor)
{
 temp_arena Temp = TempArena(0);
 string BaseTitle = GetBaseProjectTitle(Editor);
 string ModifiedIndicator = {};
 if (Editor->ProjectModified)
 {
  ModifiedIndicator = StrLit("*");
 }
 string WindowTitle = StrF(Temp.Arena,
                           "%S%S - %S",
                           BaseTitle,
                           ModifiedIndicator,
                           EditorAppName);
 Platform.SetWindowTitle(WindowTitle);
 EndTemp(Temp);
}

internal void
ProcessImageLoadingTasks(editor *Editor)
{
 image_loading_store *ImageLoadingStore = Editor->ImageLoadingStore;
 entity_store *EntityStore = Editor->EntityStore;
 string_store *StrStore = Editor->StrStore;
 
 ListIter(ImageLoading,
          ImageLoadingStore->Head,
          image_loading_task)
 {
  if (ImageLoading->State == Image_Loading)
  {
   // NOTE(hbr): nothing to do
  }
  else
  {
   image_instantiation_spec InstantiationSpec = ImageLoading->ImageInstantiationSpec;
   if (ImageLoading->State == Image_Loaded)
   {
    entity *Entity = 0;
    switch (InstantiationSpec.Type)
    {
     case ImageInstantiationSpec_EntityProvided: {
      Entity = EntityFromHandle(InstantiationSpec.ImageEntity);
      if (Entity)
      {
       image *Image = &Entity->Image;
       InitEntityImagePart(Image,
                           ImageLoading->ImageInfo.Width,
                           ImageLoading->ImageInfo.Height,
                           ImageLoading->ImageFilePath,
                           ImageLoading->LoadingTexture,
                           StrStore);
      }
     }break;
     
     case ImageInstantiationSpec_AtP: {
      Entity = AddEntity(Editor);
      InitEntityAsImage(Entity,
                        InstantiationSpec.InstantiateImageAtP,
                        ImageLoading->ImageInfo.Width,
                        ImageLoading->ImageInfo.Height,
                        ImageLoading->ImageFilePath,
                        ImageLoading->LoadingTexture,
                        StrStore);
     }break;
    }
    if (Entity)
    {
     SelectEntity(Editor, Entity);
    }
   }
   else
   {
    Assert(ImageLoading->State == Image_Failed);
    DeallocTextureHandle(EntityStore, ImageLoading->LoadingTexture);
    
    if (InstantiationSpec.Type == ImageInstantiationSpec_EntityProvided)
    {
     entity *Entity = EntityFromHandle(InstantiationSpec.ImageEntity);
     if (Entity)
     {
      DeallocEntity(EntityStore, Entity);
     }
    }
    AddNotificationF(Editor, Notification_Error,
                     "failed to load image from %S",
                     ImageLoading->ImageFilePath);
   }
   
   FinishAsyncImageLoadingTask(ImageLoadingStore, ImageLoading);
  }
 }
}

struct load_image_work
{
 thread_task_memory_store *Store;
 thread_task_memory *TaskMemory;
 renderer_transfer_op *TextureOp;
 string ImagePath;
 image_loading_task *ImageLoading;
};

internal void
LoadImageWork(void *UserData)
{
 load_image_work *Work = Cast(load_image_work *)UserData;
 
 thread_task_memory_store *Store = Work->Store;
 thread_task_memory *Task = Work->TaskMemory;
 renderer_transfer_op *TextureOp = Work->TextureOp;
 string ImagePath = Work->ImagePath;
 image_loading_task *ImageLoading = Work->ImageLoading;
 arena *Arena = Task->Arena;
 
 string ImageData = Platform.ReadEntireFile(Arena, Work->ImagePath);
 loaded_image LoadedImage = LoadImageFromMemory(Arena, ImageData.Data, ImageData.Count);
 
 image_loading_state AsyncTaskState;
 renderer_transfer_op_state OpState;
 if (LoadedImage.Success)
 {
  // TODO(hbr): Don't do memory copy, just read into that memory directly
  MemoryCopy(TextureOp->Pixels, LoadedImage.Pixels, LoadedImage.Info.SizeInBytesUponLoad);
  OpState = RendererOp_ReadyToTransfer;
  AsyncTaskState = Image_Loaded;
 }
 else
 {
  OpState = RendererOp_Empty;
  AsyncTaskState = Image_Failed;
 }
 
 CompilerWriteBarrier;
 TextureOp->State = OpState;
 ImageLoading->State = AsyncTaskState;
 
 EndThreadTaskMemory(Store, Task);
}

internal void
TryLoadImage(editor *Editor, string FilePath, image_instantiation_spec Spec)
{
 work_queue *WorkQueue = Editor->LowPriorityQueue;
 entity_store *EntityStore = Editor->EntityStore;
 thread_task_memory_store *ThreadTaskMemoryStore = Editor->ThreadTaskMemoryStore;
 image_loading_store *ImageLoadingStore = Editor->ImageLoadingStore;
 
 image_info ImageInfo = LoadImageInfo(FilePath);
 render_texture_handle TextureHandle = AllocTextureHandle(EntityStore);
 renderer_transfer_op *TextureOp = PushTextureTransfer(Editor->RendererQueue, ImageInfo.Width, ImageInfo.Height, ImageInfo.SizeInBytesUponLoad, TextureHandle);
 thread_task_memory *TaskMemory = BeginThreadTaskMemory(ThreadTaskMemoryStore);
 
 image_loading_task *ImageLoading = BeginAsyncImageLoadingTask(ImageLoadingStore);
 ImageLoading->ImageInstantiationSpec = Spec;
 ImageLoading->ImageInfo = ImageInfo;
 ImageLoading->ImageFilePath = StrCopy(ImageLoading->Arena, FilePath);
 ImageLoading->LoadingTexture = TextureHandle;
 
 load_image_work *Work = PushStruct(TaskMemory->Arena, load_image_work);
 Work->Store = ThreadTaskMemoryStore;
 Work->TaskMemory = TaskMemory;
 Work->TextureOp = TextureOp;
 Work->ImagePath = StrCopy(TaskMemory->Arena, FilePath);
 Work->ImageLoading = ImageLoading;
 
 Platform.WorkQueueAddEntry(WorkQueue, LoadImageWork, Work);
}

internal void
TryLoadImages(editor *Editor, u32 FileCount, string *FilePaths, v2 AtP)
{
 ForEachIndex(FileIndex, FileCount)
 {
  string FilePath = FilePaths[FileIndex];
  image_instantiation_spec Spec = MakeImageInstantiationSpec_AtP(AtP);
  TryLoadImage(Editor, FilePath, Spec);
 }
}

internal arena_store *
AllocArenaStore(void)
{
 arena *Arena = AllocArena(Megabytes(1));
 arena_store *ArenaStore = PushStruct(Arena, arena_store);
 ArenaStore->Arena = Arena;
 return ArenaStore;
}

internal void
DeallocArenaStoreAndAllArenas(arena_store *ArenaStore)
{
 ListIter(Node, ArenaStore->Head, arena_node)
 {
  if (!Node->Deallocated)
  {
   DeallocArena(Node->Arena);
   Node->Deallocated = true;
  }
 }
 DeallocArena(ArenaStore->Arena);
}

internal arena *
AllocArenaFromStore(arena_store *ArenaStore, u64 ReserveButNotCommit)
{
 arena_node *Node = PushStruct(ArenaStore->Arena, arena_node);
 arena *Arena = AllocArena(ReserveButNotCommit);
 Node->Arena = Arena;
 QueuePush(ArenaStore->Head, ArenaStore->Tail, Node);
 return Arena;
}

internal void
DeallocArenaFromStore(arena_store *ArenaStore, arena *Arena)
{
 b32 Deallocated = false;
 ListIter(Node, ArenaStore->Head, arena_node)
 {
  if (Node->Arena == Arena)
  {
   DeallocArena(Arena);
   Node->Deallocated = true;
   Deallocated = true;
   break;
  }
 }
 Assert(Deallocated);
}

internal void
AllocEditorResources(editor *Editor)
{
 editor_memory *Memory = Editor->EditorMemory;
 
 if (Editor->ArenaStore)
 {
  DeallocArenaStoreAndAllArenas(Editor->ArenaStore);
  StructZero(Editor);
 }
 
 arena_store *ArenaStore = AllocArenaStore();
 Editor->EditorMemory = Memory;
 Editor->ArenaStore = ArenaStore;
 Editor->Arena = AllocArenaFromStore(ArenaStore, Gigabytes(64));
 Editor->LowPriorityQueue = Memory->LowPriorityQueue;
 Editor->HighPriorityQueue = Memory->HighPriorityQueue;
 Editor->RendererQueue = Memory->RendererQueue;
 Editor->StrStore = AllocStringStore(ArenaStore);
 Editor->EntityStore = AllocEntityStore(ArenaStore, Memory->MaxTextureCount, Memory->MaxBufferCount, Editor->StrStore);
 Editor->ThreadTaskMemoryStore = AllocThreadTaskMemoryStore(ArenaStore);
 Editor->ImageLoadingStore = AllocImageLoadingStore(ArenaStore);
 Editor->ProjectFilePathArena = AllocArenaFromStore(ArenaStore, Megabytes(1));
 Editor->NotificationsArena = AllocArenaFromStore(ArenaStore, Megabytes(1));
 InitLeftClickState(&Editor->LeftClick, ArenaStore);
 InitVisualProfiler(&Editor->Profiler, Memory->Profiler);
 InitAnimatingCurvesState(&Editor->AnimatingCurves, Editor->EntityStore);
 InitMergingCurvesState(&Editor->MergingCurves, Editor->EntityStore);
 InitFrameStats(&Editor->FrameStats);
}

internal void
SetProjectFilePath(editor *Editor, b32 IsFileBacked, string FilePath)
{
 Editor->IsProjectFileBacked = IsFileBacked;
 ClearArena(Editor->ProjectFilePathArena);
 Editor->ProjectFilePath = StrCopy(Editor->ProjectFilePathArena, FilePath);
 UpdateWindowTitle(Editor);
}

internal void
InitEditor(editor *Editor,
           editor_serializable_state SerializableState,
           b32 IsFileBacked,
           string FilePath)
{
 Editor->SerializableState = SerializableState;
 SetProjectFilePath(Editor, IsFileBacked, FilePath);
}

internal void
LoadEmptyProject(editor *Editor)
{
 AllocEditorResources(Editor);
 
 editor_serializable_state SerializableState = {};
 SerializableState.BackgroundColor = SerializableState.DefaultBackgroundColor = RGBA_U8(21, 21, 21);
 SerializableState.CollisionToleranceClip = 0.04f;
 SerializableState.RotationRadiusClip = 0.1f;
 SerializableState.CurveDefaultParams = DefaultCurveParams();
 SerializableState.EntityListWindow = true;
 SerializableState.SelectedEntityWindow = true;
#if BUILD_DEBUG
 SerializableState.DiagnosticsWindow = true;
 SerializableState.ProfilerWindow = true;
#endif
 InitCamera(&SerializableState.Camera);
 InitEditor(Editor, SerializableState, false, NilStr);
 
 // TODO(hbr): remove
 {
  entity_store *EntityStore = Editor->EntityStore;
  string_store *StrStore = Editor->StrStore;
  entity *Entity = AllocEntity(EntityStore, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_Bezier;
  InitEntityAsCurve(Entity, StrLit("de-casteljau"), Params, StrStore);
  
  AppendControlPoint(&Witness, V2(-0.5f, -0.5f));
  AppendControlPoint(&Witness, V2(+0.5f, -0.5f));
  AppendControlPoint(&Witness, V2(+0.5f, +0.5f));
  AppendControlPoint(&Witness, V2(-0.5f, +0.5f));
  
  Entity->Curve.PointTracking.Active = true;
  SetTrackingPointFraction(&Witness, 0.5f);
  
  EndEntityModify(Witness);
 }
 
 {
  entity_store *EntityStore = Editor->EntityStore;
  string_store *StrStore = Editor->StrStore;
  entity *Entity = AllocEntity(EntityStore, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_BSpline;
  InitEntityAsCurve(Entity, StrLit("b-spline"), Params, StrStore);
  
  u32 PointCount = 30;
  curve_points_modify_handle Handle = BeginModifyCurvePoints(&Witness, PointCount, ModifyCurvePointsWhichPoints_ControlPointsOnly);
  for (u32 PointIndex = 0;
       PointIndex < PointCount;
       ++PointIndex)
  {
   v2 Point = V2(1.0f * PointIndex / PointCount, 1.0f * PointIndex / PointCount);
   Handle.ControlPoints[PointIndex] = Point;
  }
  EndModifyCurvePoints(Handle);
  
  EndEntityModify(Witness);
 }
}

internal b32
LoadProjectFromFile(editor *Editor, string FilePath)
{
 b32 Success = false;
 temp_arena Temp = TempArena(0);
 
 string Data = OS_ReadEntireFile(Temp.Arena, FilePath);
 deserialize_stream Deserial = MakeDeserializeStream(Data);
 
 editor_save_header Header = {};
 editor_serializable_state SerializableState = {};
 
 DeserializeStruct(&Deserial, &Header);
 if (Header.MagicValue == EditorSaveFileMagicValue &&
     Header.Version == EditorVersion)
 {
  DeserializeStruct(&Deserial, &SerializableState);
  
  AllocEditorResources(Editor);
  InitEditor(Editor, SerializableState, true, FilePath);
  
  entity_store *EntityStore = Editor->EntityStore;
  string_store *StrStore = Editor->StrStore;
  
  for (u32 StrIndex = 0;
       StrIndex < Header.StringCount;
       ++StrIndex)
  {
   string String = DeserializeString(Temp.Arena, &Deserial);
   string_id Id = StringIdFromIndex(StrIndex);
   SetOrAllocStringOfId(StrStore, String, Id);
  }
  
  ForEachIndex(EntityIndex, Header.EntityCount)
  {
   entity Serialized = {};
   DeserializeStruct(&Deserial, &Serialized);
   
   entity *Entity = AllocEntity(EntityStore, false);
   switch (Serialized.Type)
   {
    case Entity_Curve: {
     entity_with_modify_witness Witness = BeginEntityModify(Entity);
     InitEntityFromEntity(&Witness, &Serialized, EntityStore);
     EndEntityModify(Witness);
    }break;
    
    case Entity_Image: {
     InitEntityPartFromEntity(Entity, &Serialized, EntityStore);
     string_id ImageFilePathId = Serialized.Image.FilePath;
     string ImageFilePath = StringFromStringId(StrStore, ImageFilePathId);
     image_instantiation_spec Spec = MakeImageInstantiationSpec_EntityProvided(Entity);
     TryLoadImage(Editor, ImageFilePath, Spec);
    }break;
    
    case Entity_Count: InvalidPath;
   }
  }
  
  Success = true;
 }
 
 EndTemp(Temp);
 
 return Success;
}

internal b32
SaveProjectIntoFile(editor *Editor, string FilePath)
{
 b32 Success = false;
 temp_arena Temp = TempArena(0);
 string_list Data = {};
 string_store *StrStore = Editor->StrStore;
 entity_array Entities = AllEntityArrayFromStore(Editor->EntityStore);
 
 if (!StrEndsWith(FilePath, EditorSessionFileExtension))
 {
  FilePath = StrF(Temp.Arena, "%S.%S", FilePath, EditorSessionFileExtension);
 }
 
 editor_save_header Header = {};
 Header.MagicValue = EditorSaveFileMagicValue;
 Header.Version = EditorVersion;
 Header.StringCount = StrStore->StrCount;
 Header.EntityCount = Entities.Count;
 
 SerializeStruct(Temp.Arena, &Data, &Header);
 SerializeStruct(Temp.Arena, &Data, &Editor->SerializableState);
 
 for (u32 StrIndex = 0;
      StrIndex < Header.StringCount;
      ++StrIndex)
 {
  string_id Id = StringIdFromIndex(StrIndex);
  string String = StringFromStringId(StrStore, Id);
  SerializeString(Temp.Arena, &Data, String);
 }
 
 ForEachIndex(EntityIndex, Entities.Count)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  SerializeStruct(Temp.Arena, &Data, Entity);
 }
 
 // TODO(hbr): don't require to join this
 string_list_join_options Opts = {};
 string Joined = StrListJoin(Temp.Arena, &Data, Opts);
 Success = OS_WriteDataToFile(FilePath, Joined);
 
 if (Success)
 {
  SetProjectFilePath(Editor, true, FilePath);
 }
 
 EndTemp(Temp);
 
 return Success;
}

internal void
AddNotification(editor *Editor, notification_type Type, string Content)
{
 if (Editor->NotificationCount == MAX_NOTIFICATION_COUNT)
 {
  ArrayMove(Editor->Notifications, Editor->Notifications + 1, Editor->NotificationCount - 1);
  --Editor->NotificationCount;
 }
 Assert(Editor->NotificationCount < MAX_NOTIFICATION_COUNT);
 
 notification *Notification = Editor->Notifications + Editor->NotificationCount;
 ++Editor->NotificationCount;
 
 Notification->Type = Type;
 Notification->Content = StrCopy(Editor->NotificationsArena, Content);
 Notification->LifeTime = 0.0f;
 Notification->ScreenPosY = 0.0f;
}

internal void
AddNotificationF(editor *Editor, notification_type Type, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 temp_arena Temp = TempArena(0);
 string Content = StrFV(Temp.Arena, Format, Args);
 AddNotification(Editor, Type, Content);
 EndTemp(Temp);
 va_end(Args);
}

internal void
DuplicateEntity(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 entity_store *EntityStore = Editor->EntityStore;
 string_store *StrStore = Editor->StrStore;
 entity *Copy = AddEntity(Editor);
 entity_with_modify_witness CopyWitness = BeginEntityModify(Copy);
 string CopyName = StrF(Temp.Arena, "%S(copy)", GetEntityName(Entity, StrStore));
 InitEntityFromEntity(&CopyWitness, Entity, EntityStore);
 SelectEntity(Editor, Copy);
 
 f32 SlightTranslationX = 5 * Copy->Curve.Params.DrawParams.Line.Width;
 v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
 Copy->XForm.P += SlightTranslation;
 
 EndEntityModify(CopyWitness);
 EndTemp(Temp);
}

internal void
SplitCurveOnControlPoint(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 entity_store *EntityStore = Editor->EntityStore;
 string_store *StrStore = Editor->StrStore;
 
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
  u32 HeadPointCount = SelectedIndex + 1;
  u32 TailPointCount = Curve->Points.ControlPointCount - SelectedIndex;
  
  entity *HeadEntity = AddEntity(Editor);
  entity *TailEntity = AddEntity(Editor);
  
  entity_with_modify_witness HeadWitness = BeginEntityModify(HeadEntity);
  entity_with_modify_witness TailWitness = BeginEntityModify(TailEntity);
  
  InitEntityFromEntity(&HeadWitness, Entity, EntityStore);
  InitEntityFromEntity(&TailWitness, Entity, EntityStore);
  
  curve *HeadCurve = SafeGetCurve(HeadEntity);
  curve *TailCurve = SafeGetCurve(TailEntity);
  
  string HeadName = StrF(Temp.Arena, "%S(head)", GetEntityName(Entity, StrStore));
  string TailName = StrF(Temp.Arena, "%S(tail)", GetEntityName(Entity, StrStore));
  SetEntityName(HeadEntity, HeadName, StrStore);
  SetEntityName(TailEntity, TailName, StrStore);
  
  curve_points_modify_handle HeadPoints = BeginModifyCurvePoints(&HeadWitness, HeadPointCount, ModifyCurvePointsWhichPoints_AllPoints);
  curve_points_modify_handle TailPoints = BeginModifyCurvePoints(&TailWitness, TailPointCount, ModifyCurvePointsWhichPoints_AllPoints);
  Assert(HeadPoints.PointCount == HeadPointCount);
  Assert(TailPoints.PointCount == TailPointCount);
  
  ArrayCopy(HeadPoints.ControlPoints, Curve->Points.ControlPoints, HeadPoints.PointCount);
  ArrayCopy(HeadPoints.Weights, Curve->Points.ControlPointWeights, HeadPoints.PointCount);
  ArrayCopy(HeadPoints.CubicBeziers, Curve->Points.CubicBezierPoints, HeadPoints.PointCount);
  
  u32 SplitAt = SelectedIndex;
  ArrayCopy(TailPoints.ControlPoints, Curve->Points.ControlPoints + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailPoints.Weights, Curve->Points.ControlPointWeights + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailPoints.CubicBeziers, Curve->Points.CubicBezierPoints + SplitAt, TailPoints.PointCount);
  
  EndModifyCurvePoints(HeadPoints);
  EndModifyCurvePoints(TailPoints);
  
  EndEntityModify(HeadWitness);
  EndEntityModify(TailWitness);
  
  RemoveEntity(Editor, Entity);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 entity_with_modify_witness Witness = BeginEntityModify(Entity);
 u32 PointCount = Curve->Points.ControlPointCount;
 begin_modify_curve_points_static_tracked_result BeginModify = BeginModifyCurvePointsTracked(Editor, &Witness, PointCount + 1,
                                                                                             ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 curve_points_modify_handle ModifyPoints = BeginModify.ModifyPoints;
 tracked_action *ModifyAction = BeginModify.ModifyAction;
 if (ModifyPoints.PointCount == PointCount + 1)
 {
  BezierCurveElevateDegreeWeighted(ModifyPoints.ControlPoints,
                                   ModifyPoints.Weights,
                                   PointCount);
 }
 EndModifyCurvePointsTracked(Editor, ModifyAction, ModifyPoints);
 EndEntityModify(Witness);
}

internal vertex_array
CopyLineVertices(arena *Arena, vertex_array Vertices)
{
 vertex_array Result = Vertices;
 Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, v2);
 ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
 
 return Result;
}

internal curve_points_static *
AllocCurvePoints(editor *Editor)
{
 curve_points_static_node *Node = Editor->FreeCurvePointsNode;
 if (Node)
 {
  StackPop(Editor->FreeCurvePointsNode);
 }
 else
 {
  Node = PushStructNonZero(Editor->Arena, curve_points_static_node);
 }
 curve_points_static *Points = &Node->Points;
 // NOTE(hbr): Don't [StructZero] because it is a big struct and it's not necessary
 Points->ControlPointCount = 0;
 return Points;
}

internal void
DeallocCurvePoints(editor *Editor, curve_points_static *Points)
{
 curve_points_static_node *Node = ContainerOf(Points, curve_points_static_node, Points);
 StackPush(Editor->FreeCurvePointsNode, Node);
}

internal entity *
GetSelectedEntity(editor *Editor)
{
 entity *Entity = EntityFromHandle(Editor->SelectedEntity);
 return Entity;
}

internal void
PerformBezierCurveSplit(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 entity_store *EntityStore = Editor->EntityStore;
 u32 ControlPointCount = Curve->Points.ControlPointCount;
 string_store *StrStore = Editor->StrStore;
 
 Assert(Tracking->Active);
 Tracking->Active = false;
 
 entity *LeftEntity = AddEntity(Editor);
 entity *RightEntity = AddEntity(Editor);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 InitEntityFromEntity(&LeftWitness, Entity, EntityStore);
 InitEntityFromEntity(&RightWitness, Entity, EntityStore);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", GetEntityName(Entity, StrStore));
 string RightName = StrF(Temp.Arena, "%S(right)", GetEntityName(Entity, StrStore));
 SetEntityName(LeftEntity, LeftName, StrStore);
 SetEntityName(RightEntity, RightName, StrStore);
 
 curve_points_modify_handle LeftPoints = BeginModifyCurvePoints(&LeftWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 curve_points_modify_handle RightPoints = BeginModifyCurvePoints(&RightWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 Assert(LeftPoints.PointCount == ControlPointCount);
 Assert(RightPoints.PointCount == ControlPointCount);
 
 BezierCurveSplit(Tracking->Fraction,
                  ControlPointCount, Curve->Points.ControlPoints, Curve->Points.ControlPointWeights,
                  LeftPoints.ControlPoints, LeftPoints.Weights,
                  RightPoints.ControlPoints, RightPoints.Weights);
 
 EndModifyCurvePoints(RightPoints);
 EndModifyCurvePoints(LeftPoints);
 
 EndEntityModify(LeftWitness);
 EndEntityModify(RightWitness);
 
 RemoveEntity(Editor, Entity);
 
 EndTemp(Temp);
}

internal void
BeginMergingCurves(merging_curves_state *Merging)
{
 Merging->Active = true;
 BeginChoosing2Curves(&Merging->Choose2Curves);
}

internal void
EndMergingCurves(editor *Editor, b32 Merged)
{
 merging_curves_state *Merging = &Editor->MergingCurves;
 if (Merged)
 {
  entity_store *EntityStore = Editor->EntityStore;
  
  entity *Entity = AddEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  InitEntityFromEntity(&EntityWitness, Merging->MergeEntity, EntityStore);
  EndEntityModify(EntityWitness);
  
  entity *Entity0 = EntityFromHandle(Merging->Choose2Curves.Curves[0]);
  if (Entity0)
  {
   RemoveEntity(Editor, Entity0);
  }
  entity *Entity1 = EntityFromHandle(Merging->Choose2Curves.Curves[1]);
  if (Entity1)
  {
   RemoveEntity(Editor, Entity1);
  }
 }
 Merging->Active = false;
}

internal b32
MergingWantsInput(merging_curves_state *Merging)
{
 b32 WantsInput = (Merging->Active && Merging->Choose2Curves.WaitingForChoice);
 return WantsInput;
}

internal void
MaybeReverseCurvePoints(entity *Entity)
{
 if (IsCurveReversed(Entity))
 {
  curve *Curve = SafeGetCurve(Entity);
  ArrayReverse(Curve->Points.ControlPoints,       Curve->Points.ControlPointCount, v2);
  ArrayReverse(Curve->Points.ControlPointWeights, Curve->Points.ControlPointCount, f32);
  ArrayReverse(Curve->Points.CubicBezierPoints,   Curve->Points.ControlPointCount, cubic_bezier_point);
 }
}

internal bouncing_parameter
MakeBouncingParam(void)
{
 bouncing_parameter Result = {};
 Result.Speed = 0.5f;
 Result.Sign = 1.0f;
 
 return Result;
}

internal void
BeginAnimatingCurves(animating_curves_state *Animation)
{
 entity *ExtractEntity = Animation->ExtractEntity;
 StructZero(Animation);
 Animation->Flags = AnimatingCurves_Active;
 BeginChoosing2Curves(&Animation->Choose2Curves);
 Animation->Bouncing = MakeBouncingParam();
 Animation->ExtractEntity = ExtractEntity;
}

internal void
EndAnimatingCurves(animating_curves_state *Animation)
{
 Animation->Flags &= ~AnimatingCurves_Active;
}

internal b32
AnimationWantsInput(animating_curves_state *Animation)
{
 b32 WantsInput = ((Animation->Flags & AnimatingCurves_Active) && Animation->Choose2Curves.WaitingForChoice);
 return WantsInput;
}

internal void
BeginChoosing2Curves(choose_2_curves_state *Choosing)
{
 Choosing->WaitingForChoice = true;
 Choosing->ChoosingCurveIndex = 0;
 Choosing->Curves[0] = Choosing->Curves[1] = {};
}

internal b32
SupplyCurve(choose_2_curves_state *Choosing, entity *Curve)
{
 b32 AllSupplied = false;
 
 Choosing->Curves[Choosing->ChoosingCurveIndex] = MakeEntityHandle(Curve);
 if (EntityFromHandle(Choosing->Curves[0]) == 0)
 {
  Choosing->ChoosingCurveIndex = 0;
 }
 else if (EntityFromHandle(Choosing->Curves[1]) == 0)
 {
  Choosing->ChoosingCurveIndex = 1;
 }
 else
 {
  Choosing->WaitingForChoice = false;
  AllSupplied = true;
 }
 
 return AllSupplied;
}

internal void
BeginLoweringBezierCurveDegree(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 
 u32 PointCount = Curve->Points.ControlPointCount;
 if (PointCount > 0)
 {
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  begin_modify_curve_points_static_tracked_result Modify = BeginModifyCurvePointsTracked(Editor, &Witness, PointCount - 1, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
  curve_points_modify_handle ModifyPoints = Modify.ModifyPoints;
  tracked_action *ModifyAction = Modify.ModifyAction;
  
  curve_points_static *OriginalPoints = AllocCurvePoints(Editor);
  CopyCurvePointsFromCurve(Curve, CurvePointsDynamicFromStatic(OriginalPoints));
  
  // TODO(hbr): implement this
#if 1
  bezier_lower_degree LowerDegree = BezierCurveLowerDegree(ModifyPoints.ControlPoints, ModifyPoints.Weights, PointCount);
#else
  bezier_lower_degree LowerDegree = {};
  BezierCurveLowerDegreeUniformNormOptimal(ModifyPoints.ControlPoints, ModifyPoints.Weights, PointCount);
#endif
  
  if (LowerDegree.Failure)
  {
   Lowering->Active = true;
   Lowering->OriginalCurvePoints = OriginalPoints;
   Lowering->OriginalCurveVertices = CopyLineVertices(Lowering->Arena, Curve->CurveVertices);
   f32 T = 0.5f;
   Lowering->LowerDegree = LowerDegree;
   Lowering->MixParameter = T;
   ModifyPoints.ControlPoints[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.P_I, LowerDegree.P_II, T);
   ModifyPoints.Weights[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.W_I, LowerDegree.W_II, T);
  }
  else
  {
   DeallocCurvePoints(Editor, OriginalPoints);
  }
  
  EndModifyCurvePointsTracked(Editor, ModifyAction, ModifyPoints);
  EndEntityModify(Witness);
 }
}

internal void
EndLoweringBezierCurveDegree(editor *Editor, curve_degree_lowering_state *Lowering)
{
 Lowering->Active = false;
 ClearArena(Lowering->Arena);
 Assert(Lowering->OriginalCurvePoints);
 DeallocCurvePoints(Editor, Lowering->OriginalCurvePoints);
}

internal void
BeginEditorFrame(editor *Editor)
{
 if (!Editor->IsPendingActionTrackingGroup)
 {
  StructZero(&Editor->PendingActionTrackingGroup);
  Editor->IsPendingActionTrackingGroup = true;
 }
}

internal tracked_action *
AllocTrackedAction(editor *Editor)
{
 tracked_action *Action = Editor->FreeTrackedAction;
 if (Action)
 {
  StackPop(Editor->FreeTrackedAction);
 }
 else
 {
  Action = PushStructNonZero(Editor->Arena, tracked_action);
 }
 StructZero(Action);
 return Action;
}

internal void
DeallocTrackedAction(editor *Editor,
                     action_tracking_group *Group,
                     tracked_action *Action)
{
 DLLRemove(Group->ActionsHead, Group->ActionsTail, Action);
 StackPush(Editor->FreeTrackedAction, Action);
 if (Action->CurvePoints)
 {
  DeallocCurvePoints(Editor, Action->CurvePoints);
 }
 if (Action->FinalCurvePoints)
 {
  DeallocCurvePoints(Editor, Action->FinalCurvePoints);
 }
}

internal void
DeallocWholeActionTrackingGroup(editor *Editor, action_tracking_group *Group)
{
 ListIter(Action, Group->ActionsHead, tracked_action)
 {
  DeallocTrackedAction(Editor, Group, Action);
 }
}

internal void
EndActionTrackingGroup(editor *Editor, action_tracking_group *Group)
{
 if (Group->ActionsHead &&
     (Editor->ActionTrackingGroupIndex < ArrayCount(Editor->ActionTrackingGroups)))
 {  
  // NOTE(hbr): Iterate through every action from the future (aka. actions that have been undo'ed)
  // and deallocate all AddEntity entities. Because they live in that future and that future only.
  // And we ditch that future and create a new path.
  for (u32 GroupIndex = Editor->ActionTrackingGroupIndex;
       GroupIndex < Editor->ActionTrackingGroupCount;
       ++GroupIndex)
  {
   action_tracking_group *Remove = Editor->ActionTrackingGroups + GroupIndex;
   ListIter(Action, Remove->ActionsHead, tracked_action)
   {
    if (Action->Type == TrackedAction_AddEntity)
    {
     entity *Entity = EntityFromHandle(Action->Entity, true);
     Assert(Entity);
     DeallocEntity(Editor->EntityStore, Entity);
    }
   }
   DeallocWholeActionTrackingGroup(Editor, Remove);
  }
  Editor->ActionTrackingGroups[Editor->ActionTrackingGroupIndex] = *Group;
  ++Editor->ActionTrackingGroupIndex;
  Editor->ActionTrackingGroupCount = Editor->ActionTrackingGroupIndex;
 }
 else
 {
  DeallocWholeActionTrackingGroup(Editor, Group);
 }
}

internal void
RequestProjectChange(editor *Editor, project_change_request Request)
{
 project_change_request_state *State = &Editor->ProjectChange;
 StructZero(State);
 State->Request = Request;
}

internal void
ExecuteEditorCommand(editor *Editor, editor_command Cmd)
{
 switch (Cmd)
 {
  case EditorCommand_New: {RequestProjectChange(Editor, ProjectChangeRequest_NewProject);}break;
  case EditorCommand_Open: {RequestProjectChange(Editor, ProjectChangeRequest_OpenFile);}break;
  case EditorCommand_Save: {RequestProjectChange(Editor, ProjectChangeRequest_SaveProject);}break;
  case EditorCommand_SaveAs: {RequestProjectChange(Editor, ProjectChangeRequest_SaveProjectAs);}break;
  case EditorCommand_Quit: {RequestProjectChange(Editor, ProjectChangeRequest_Quit);}break;
  case EditorCommand_ToggleDevConsole: {Editor->DevConsole = !Editor->DevConsole;}break;
  case EditorCommand_ToggleProfiler: {Editor->Profiler.Stopped = !Editor->Profiler.Stopped;}break;
  case EditorCommand_Undo: {Undo(Editor);}break;
  case EditorCommand_Redo: {Redo(Editor);}break;
  case EditorCommand_ToggleUI: {Editor->HideUI = !Editor->HideUI;}break;
  
  case EditorCommand_Delete: {
   entity *Entity = GetSelectedEntity(Editor);
   entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
   if (Entity)
   {
    switch (Entity->Type)
    {
     case Entity_Curve: {
      curve *Curve = SafeGetCurve(Entity);
      if (IsControlPointSelected(Curve))
      {
       RemoveControlPoint(Editor, &EntityWitness, Curve->SelectedControlPoint);
      }
      else
      {
       RemoveEntity(Editor, Entity);
      }
     }break;
     
     case Entity_Image: {
      RemoveEntity(Editor, Entity);
     }break;
     
     case Entity_Count: InvalidPath;
    }
   }
   EndEntityModify(EntityWitness);
  }break;
  
  case EditorCommand_Duplicate: {
   entity *Entity = GetSelectedEntity(Editor);
   if (Entity)
   {
    DuplicateEntity(Editor, Entity);
   }
  }break;
  
  case EditorCommand_Count: InvalidPath;
 }
}

internal void
PushEditorCmd(editor *Editor, editor_command Command)
{
 editor_command_node *Node = Editor->FreeEditorCommandNode;
 if (Node)
 {
  StackPop(Editor->FreeEditorCommandNode);
 }
 else
 {
  Node = PushStructNonZero(Editor->Arena, editor_command_node);
 }
 StructZero(Node);
 Node->Command = Command;
 QueuePush(Editor->EditorCommandsHead, Editor->EditorCommandsTail, Node);
}

internal void
EndEditorFrame(editor *Editor)
{
 //- execute all editor commands gathered throughout the frame (mostly from input)
 ListIter(CmdNode, Editor->EditorCommandsHead, editor_command_node)
 {
  ExecuteEditorCommand(Editor, CmdNode->Command);
  StackPush(Editor->FreeEditorCommandNode, CmdNode);
 }
 Editor->EditorCommandsHead = 0;
 Editor->EditorCommandsTail = 0;
 //- end pending action tracking group is all tracked actions are completed
 // TODO(hbr): restore
 //Assert(Editor->IsPendingActionTrackingGroup);
 action_tracking_group *Group = &Editor->PendingActionTrackingGroup;
 b32 HasPending = false;
 ListIter(Action, Group->ActionsHead, tracked_action)
 {
  if (Action->IsPending)
  {
   HasPending = true;
   break;
  }
 }
 if (!HasPending)
 {
  EndActionTrackingGroup(Editor, Group);
  Editor->IsPendingActionTrackingGroup = false;
 }
}

internal action_tracking_group *
GetPendingActionTrackingGroup(editor *Editor)
{
 Assert(Editor->IsPendingActionTrackingGroup);
 return &Editor->PendingActionTrackingGroup;
}

internal void
BeginMovingEntity(editor_left_click_state *Left, editor *Editor, entity *Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingEntity;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->PendingTrackedAction = BeginEntityTransform(Editor, Target);
}

internal void
BeginMovingCurvePoint(editor_left_click_state *Left,
                      editor *Editor,
                      entity *Target,
                      curve_point_handle CurvePoint)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingCurvePoint;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->CurvePoint = CurvePoint;
 control_point_handle ControlPoint = ControlPointFromCurvePoint(CurvePoint);
 Left->PendingTrackedAction = BeginControlPointMove(Editor, Target, ControlPoint);
}

internal void
BeginMovingTrackingPoint(editor_left_click_state *Left, entity *Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingTrackingPoint;
 Left->TargetEntity = MakeEntityHandle(Target);
}

internal void
BeginMovingBSplineKnot(editor_left_click_state *Left,
                       entity *Target,
                       b_spline_knot_handle BSplineKnot)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingBSplineKnot;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->BSplineKnot = BSplineKnot;
}

internal void
EndLeftClick(editor *Editor, editor_left_click_state *Left)
{
 if (Left->Mode == EditorLeftClick_MovingCurvePoint)
 {
  EndControlPointMove(Editor, Left->PendingTrackedAction);
 }
 else if (Left->Mode == EditorLeftClick_MovingEntity)
 {
  EndEntityTransform(Editor, Left->PendingTrackedAction);
 }
 
 arena *OriginalVerticesArena = Left->OriginalVerticesArena;
 StructZero(Left);
 Left->OriginalVerticesArena = OriginalVerticesArena;
}

internal void
BeginMiddleClick(editor_middle_click_state *Middle, b32 Rotate, v2 ClipSpaceLastMouseP)
{
 Middle->Active = true;
 Middle->Rotate = Rotate;
 Middle->ClipSpaceLastMouseP = ClipSpaceLastMouseP;
}

internal void
EndMiddleClick(editor_middle_click_state *Middle)
{
 StructZero(Middle);
}

internal void
BeginRightClick(editor_right_click_state *Right, v2 ClickP, collision CollisionAtP)
{
 Right->Active = true;
 Right->ClickP = ClickP;
 Right->CollisionAtP = CollisionAtP;
}

internal void
EndRightClick(editor_right_click_state *Right)
{
 StructZero(Right);
}

internal void
ProfilerInspectAllFrames(visual_profiler_state *Visual)
{
 Visual->Mode = VisualProfilerMode_AllFrames;
}

internal void
ProfilerInspectSingleFrame(visual_profiler_state *Visual, u32 FrameIndex)
{
 profiler *Profiler = Visual->Profiler;
 Visual->Mode = VisualProfilerMode_SingleFrame;
 Assert(FrameIndex < MAX_PROFILER_FRAME_COUNT);
 Visual->FrameIndex = FrameIndex;
 Visual->FrameSnapshot = Profiler->Frames[FrameIndex];
}

internal tracked_action *
NextTrackedActionFromGroup(editor *Editor, action_tracking_group *Group, b32 Partial)
{
 tracked_action *Action = AllocTrackedAction(Editor);
 Action->IsPending = Partial;
 DLLPushBack(Group->ActionsHead, Group->ActionsTail, Action);
 return Action;
}

internal void
FinishPendingAction(editor *Editor, action_tracking_group *Group, tracked_action *Action, b32 Cancel)
{
 if (Action != &NilTrackedAction)
 {
  Assert(Action->IsPending);
  Action->IsPending = false;
  if (Cancel)
  {
   DeallocTrackedAction(Editor, Group, Action);
  }
 }
}

internal void
RemoveControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   control_point_handle Point)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_RemoveControlPoint;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 Action->ControlPoint = GetCurveControlPointInWorldSpace(Entity->Entity, Point);
 Action->ControlPointHandle = Point;
 RemoveControlPoint(Entity, Point);
}

internal tracked_action *
BeginEntityTransform(editor *Editor, entity *Entity)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_TransformEntity;
 Action->OriginalEntityXForm = Entity->XForm;
 Action->Entity = MakeEntityHandle(Entity);
 return Action;
}

internal void
EndEntityTransform(editor *Editor, tracked_action *MoveAction)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 entity *Entity = EntityFromHandle(MoveAction->Entity);
 b32 Cancel = false;
 if (Entity)
 {
  xform2d XForm = Entity->XForm;
  if (StructsEqual(&XForm, &MoveAction->OriginalEntityXForm))
  {
   Cancel = true;
  }
  else
  {
   MoveAction->XFormedToEntityXForm = XForm;
  }
 }
 FinishPendingAction(Editor, Group, MoveAction, Cancel);
}

internal tracked_action *
BeginControlPointMove(editor *Editor,
                      entity *Entity,
                      control_point_handle Point)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_MoveControlPoint;
 Action->Entity = MakeEntityHandle(Entity);
 Action->ControlPointHandle = Point;
 Action->ControlPoint = GetCurveControlPointInWorldSpace(Entity, Point);
 return Action;
}

internal void
EndControlPointMove(editor *Editor, tracked_action *MoveAction)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 entity *Entity = EntityFromHandle(MoveAction->Entity);
 b32 Cancel = false;
 if (Entity)
 {
  control_point MovedToControlPoint = GetCurveControlPointInWorldSpace(Entity, MoveAction->ControlPointHandle);
  if (StructsEqual(&MoveAction->MovedToControlPoint, &MoveAction->ControlPoint))
  {
   Cancel = true;
  }
  else
  {
   MoveAction->MovedToControlPoint = MovedToControlPoint;
  }
 }
 FinishPendingAction(Editor, Group, MoveAction, Cancel);
}

internal begin_modify_curve_points_static_tracked_result
BeginModifyCurvePointsTracked(editor *Editor,
                              entity_with_modify_witness *Entity,
                              u32 RequestedPointCount,
                              modify_curve_points_static_which_points Which)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_ModifyCurvePoints;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 curve_points_static *CurvePoints = AllocCurvePoints(Editor);
 CopyCurvePointsFromCurve(SafeGetCurve(Entity->Entity), CurvePointsDynamicFromStatic(CurvePoints));
 Action->CurvePoints = CurvePoints;
 curve_points_modify_handle Handle = BeginModifyCurvePoints(Entity, RequestedPointCount, Which);
 begin_modify_curve_points_static_tracked_result Result = {};
 Result.ModifyPoints = Handle;
 Result.ModifyAction = Action;
 return Result;
}

internal void
EndModifyCurvePointsTracked(editor *Editor,
                            tracked_action *ModifyAction,
                            curve_points_modify_handle ModifyPoints)
{
 EndModifyCurvePoints(ModifyPoints);
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 b32 Cancel = true;
 entity *Entity = EntityFromHandle(ModifyAction->Entity);
 if (Entity)
 {
  curve_points_static *CurvePoints = AllocCurvePoints(Editor);
  CopyCurvePointsFromCurve(SafeGetCurve(Entity), CurvePointsDynamicFromStatic(CurvePoints));
  ModifyAction->FinalCurvePoints = CurvePoints;
  Cancel = false;
 }
 FinishPendingAction(Editor, Group, ModifyAction, Cancel);
}

internal void
SetCurvePointsTracked(editor *Editor, entity_with_modify_witness *Entity, curve_points_handle Points)
{
 begin_modify_curve_points_static_tracked_result Modify = BeginModifyCurvePointsTracked(Editor, Entity, Points.ControlPointCount,
                                                                                        ModifyCurvePointsWhichPoints_AllPoints);
 curve_points_modify_handle ModifyPoints = Modify.ModifyPoints;
 tracked_action *ModifyAction = Modify.ModifyAction;
 SetCurvePoints(Entity, Points);
 EndModifyCurvePointsTracked(Editor, ModifyAction, ModifyPoints);
}

internal control_point_handle
AddControlPoint(editor *Editor,
                entity_with_modify_witness *Entity,
                v2 P, u32 At,
                b32 Append)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_AddControlPoint;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 control_point ControlPoint = MakeControlPoint(P);
 Action->ControlPoint = ControlPoint;
 control_point_handle Handle = (Append ? AppendControlPoint(Entity, P) : InsertControlPoint(Entity, ControlPoint, At));
 Action->ControlPointHandle = Handle;
 return Handle;
}

internal control_point_handle
InsertControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   v2 P,
                   u32 At)
{
 control_point_handle Handle = AddControlPoint(Editor, Entity, P, At, false);
 return Handle;
}

internal control_point_handle
AppendControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   v2 P)
{
 control_point_handle Result = AddControlPoint(Editor, Entity, P, 0, true);
 return Result;
}

internal void
RemoveEntity(editor *Editor, entity *Entity)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_RemoveEntity;
 Action->Entity = MakeEntityHandle(Entity);
 DeactiveEntity(Editor->EntityStore, Entity);
}

internal entity *
AddEntity(editor *Editor)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_AddEntity;
 entity *Entity = AllocEntity(Editor->EntityStore, false);
 Action->Entity = MakeEntityHandle(Entity);
 return Entity;
}

internal void
SelectEntity(editor *Editor, entity *Entity)
{
 //- mark currently selected entity as no longer selected
 entity *Current = EntityFromHandle(Editor->SelectedEntity);
 if (Current)
 {
  MarkEntityDeselected(Current);
 }
 //- select new entity
 if (Entity)
 {
  MarkEntitySelected(Entity);
 }
 Editor->SelectedEntity = MakeEntityHandle(Entity);
}

internal void
Undo(editor *Editor)
{
 if (Editor->ActionTrackingGroupIndex > 0)
 {
  --Editor->ActionTrackingGroupIndex;
  action_tracking_group *Group = Editor->ActionTrackingGroups + Editor->ActionTrackingGroupIndex;
  ListIterRev(Action, Group->ActionsTail, tracked_action)
  {
   b32 AllowDeactived = (Action->Type == TrackedAction_RemoveEntity);
   entity *Entity = EntityFromHandle(Action->Entity, AllowDeactived);
   // NOTE(hbr): Deselecting entity if zero
   b32 AllowZero = (Action->Type == TrackedAction_SelectEntity);
   if (Entity || AllowZero)
   {
    entity_with_modify_witness Witness = BeginEntityModify(Entity);
    switch (Action->Type)
    {
     case TrackedAction_RemoveControlPoint: {
      u32 InsertAt = IndexFromControlPointHandle(Action->ControlPointHandle);
      InsertControlPoint(&Witness, Action->ControlPoint, InsertAt);
     }break;
     
     case TrackedAction_AddControlPoint: {
      RemoveControlPoint(&Witness, Action->ControlPointHandle);
     }break;
     
     case TrackedAction_MoveControlPoint: {
      SetControlPoint(&Witness, Action->ControlPointHandle, Action->ControlPoint);
     }break;
     
     case TrackedAction_TransformEntity: {
      Entity->XForm = Action->OriginalEntityXForm;
     }break;
     
     case TrackedAction_RemoveEntity: {
      ActivateEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      DeactiveEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_SelectEntity: {
      entity *Previous = EntityFromHandle(Action->PreviouslySelectedEntity);
      SelectEntity(Editor, Previous);
     }break;
     
     case TrackedAction_ModifyCurvePoints: {
      curve_points_static *Points = Action->CurvePoints;
      SetCurvePoints(&Witness, CurvePointsHandleFromCurvePointsStatic(Points));
     }break;
    }
    EndEntityModify(Witness);
   }
  }
 }
}

internal void
Redo(editor *Editor)
{
 if (Editor->ActionTrackingGroupIndex < Editor->ActionTrackingGroupCount)
 {
  action_tracking_group *Group = Editor->ActionTrackingGroups + Editor->ActionTrackingGroupIndex;
  ++Editor->ActionTrackingGroupIndex;
  ListIter(Action, Group->ActionsHead, tracked_action)
  {
   b32 AllowDeactived = (Action->Type == TrackedAction_AddEntity);
   entity *Entity = EntityFromHandle(Action->Entity, AllowDeactived);
   // NOTE(hbr): Deselecting entity if zero
   b32 AllowZero = (Action->Type == TrackedAction_SelectEntity);
   if (Entity || AllowZero)
   {
    entity_with_modify_witness Witness = BeginEntityModify(Entity);
    switch (Action->Type)
    {
     case TrackedAction_RemoveControlPoint: {
      RemoveControlPoint(&Witness, Action->ControlPointHandle);
     }break;
     
     case TrackedAction_AddControlPoint: {
      u32 InsertAt = IndexFromControlPointHandle(Action->ControlPointHandle);
      InsertControlPoint(&Witness, Action->ControlPoint, InsertAt);
     }break;
     
     case TrackedAction_MoveControlPoint: {
      SetControlPoint(&Witness, Action->ControlPointHandle, Action->MovedToControlPoint);
     }break;
     
     case TrackedAction_TransformEntity: {
      Entity->XForm = Action->XFormedToEntityXForm;
     }break;
     
     case TrackedAction_RemoveEntity: {
      DeactiveEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      ActivateEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_SelectEntity: {
      SelectEntity(Editor, Entity);
     }break;
     
     case TrackedAction_ModifyCurvePoints: {
      curve_points_static *Points = Action->FinalCurvePoints;
      SetCurvePoints(&Witness, CurvePointsHandleFromCurvePointsStatic(Points));
     }break;
    }
    EndEntityModify(Witness);
   }
  }
 }
}

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

internal entity_store *
AllocEntityStore(arena_store *ArenaStore,
                 u32 MaxTextureCount,
                 u32 MaxBufferCount,
                 string_store *StrStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 entity_store *Store = PushStruct(Arena, entity_store);
 Store->Arena = Arena;
 ForEachElement(Index, Store->ByTypeArenas)
 {
  Store->ByTypeArenas[Index] = AllocArenaFromStore(ArenaStore, Megabytes(1));
 }
 Store->TextureCount = MaxTextureCount;
 Store->TextureHandleRefCount = PushArray(Arena, MaxTextureCount, b32);
 Store->BufferCount = MaxBufferCount;
 Store->IsBufferHandleAllocated = PushArray(Arena, MaxBufferCount, b32);
 Store->StrStore = StrStore;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal entity *
AllocEntity(entity_store *Store, b32 DontTrack)
{
 entity_list_node *Node = Store->Free;
 if (Node)
 {
  StackPop(Store->Free);
 }
 else
 {
  Node = PushStructNonZero(Store->Arena, entity_list_node);
 }
 
 entity *Entity = &Node->Entity;
 u32 Generation = Entity->Generation;
 StructZero(Entity);
 
 Entity->Id = Store->IdCounter++;
 Entity->Generation = Generation;
 Entity->Name = AllocStringOfSize(Store->StrStore, 128);
 
 // NOTE(hbr): It shouldn't really matter anyway that we allocate arenas even for
 // entities other than curves.
 curve *Curve = &Entity->Curve;
 Curve->ComputeArena = AllocArenaFromStore(Store->ArenaStore, Megabytes(32));
 Curve->DegreeLowering.Arena = AllocArenaFromStore(Store->ArenaStore, Megabytes(32));
 Curve->ParametricResources.Arena = AllocArenaFromStore(Store->ArenaStore, Megabytes(32));
 
 image *Image = &Entity->Image;
 Image->FilePath = AllocStringOfSize(Store->StrStore, 128);
 
 if (!DontTrack)
 {
  ActivateEntity(Store, Entity);
 }
 
 return Entity;
}

internal void
DeallocEntity(entity_store *Store, entity *Entity)
{
 curve *Curve = &Entity->Curve;
 DeallocArenaFromStore(Store->ArenaStore, Curve->ComputeArena);
 DeallocArenaFromStore(Store->ArenaStore, Curve->DegreeLowering.Arena);
 DeallocArenaFromStore(Store->ArenaStore, Curve->ParametricResources.Arena);
 
 image *Image = &Entity->Image;
 DeallocTextureHandle(Store, Image->TextureHandle);
 
 DeactiveEntity(Store, Entity);
 
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 StackPush(Store->Free, Node);
}

internal void
ActivateEntity(entity_store *Store, entity *Entity)
{
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 DLLPushBack(Store->Head, Store->Tail, Node);
 ++Store->Count;
 ++Store->AllocGeneration;
 Entity->InternalFlags |= EntityInternalFlag_Tracked;
 // NOTE(hbr): If I'm activating entity that was previously deactivated,
 // decremented its previously incremented generation.
 if (Entity->InternalFlags & EntityInternalFlag_Deactivated)
 {
  Assert(Entity->Generation > 0);
  --Entity->Generation;
 }
 Entity->InternalFlags &= ~EntityInternalFlag_Deactivated;
}

internal void
DeactiveEntity(entity_store *Store, entity *Entity)
{
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 if (Entity->InternalFlags & EntityInternalFlag_Tracked)
 {
  DLLRemove(Store->Head, Store->Tail, Node);
  --Store->Count;
  ++Store->AllocGeneration;
  Entity->InternalFlags &= ~EntityInternalFlag_Tracked;
 }
 Entity->InternalFlags |= EntityInternalFlag_Deactivated;
 ++Entity->Generation;
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
  ListIter(Node, Store->Head, entity_list_node)
  {
   entity *Entity = &Node->Entity;
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

internal thread_task_memory_store *
AllocThreadTaskMemoryStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 thread_task_memory_store *Store = PushStruct(Arena, thread_task_memory_store);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
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
 Task->Arena = AllocArenaFromStore(Store->ArenaStore, Gigabytes(1));
 return Task;
}

internal void
EndThreadTaskMemory(thread_task_memory_store *Store, thread_task_memory *Task)
{
 DeallocArenaFromStore(Store->ArenaStore, Task->Arena);
 StackPush(Store->Free, Task);
}

internal image_loading_store *
AllocImageLoadingStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 image_loading_store *Store = PushStruct(Arena, image_loading_store);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal void
DeallocImageLoadingStore(image_loading_store *Store)
{
 DeallocArenaFromStore(Store->ArenaStore, Store->Arena);
 StructZero(Store);
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
 
 Task->Arena = AllocArenaFromStore(Store->ArenaStore, Gigabytes(1));
 DLLPushBack(Store->Head, Store->Tail, Task);
 
 return Task;
}

internal void
FinishAsyncImageLoadingTask(image_loading_store *Store, image_loading_task *Task)
{
 DeallocArenaFromStore(Store->ArenaStore, Task->Arena);
 DLLRemove(Store->Head, Store->Tail, Task);
 StackPush(Store->Free, Task);
}

internal image_instantiation_spec
MakeImageInstantiationSpec_EntityProvided(entity *Entity)
{
 image_instantiation_spec Spec = {};
 Spec.Type = ImageInstantiationSpec_EntityProvided;
 Spec.ImageEntity = MakeEntityHandle(Entity);
 return Spec;
}

internal image_instantiation_spec
MakeImageInstantiationSpec_AtP(v2 AtP)
{
 image_instantiation_spec Spec = {};
 Spec.Type = ImageInstantiationSpec_AtP;
 Spec.InstantiateImageAtP = AtP;
 return Spec;
}

internal multiline_collision
CheckCollisionWithMultiLine(v2 LocalAtP, v2 *CurveSamples, u32 PointCount, f32 Width, f32 Tolerance)
{
 multiline_collision Result = {};
 
 f32 CheckWidth = Width + Tolerance;
 f32 MinDistance = 0.0f;
 u32 MinDistancePointIndex = 0;
 for (u32 PointIndex = 0;
      PointIndex + 1 < PointCount;
      ++PointIndex)
 {
  v2 P1 = CurveSamples[PointIndex + 0];
  v2 P2 = CurveSamples[PointIndex + 1];
  f32 Distance = SegmentSignedDistance(LocalAtP, P1, P2, CheckWidth);
  if (Distance < MinDistance)
  {
   MinDistance = Distance;
   MinDistancePointIndex = PointIndex;
  }
 }
 
 if (MinDistance < 0)
 {
  Result.Collided = true;
  Result.PointIndex = MinDistancePointIndex;
 }
 
 return Result;
}

internal sort_entry_array
SortEntities(arena *Arena, entity_array Entities)
{
 sort_entry_array SortArray = AllocSortEntryArray(Arena, Entities.Count, SortOrder_Descending);
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  // NOTE(hbr): Equal sorting layer images should be below curves
  f32 Offset = 0.0f;
  switch (Entity->Type)
  {
   case Entity_Image: {Offset = 0.5f;} break;
   case Entity_Curve: {Offset = 0.0f;} break;
   case Entity_Count: InvalidPath; break;
  }
  
  AddSortEntry(&SortArray, Entity->SortingLayer + Offset, EntityIndex);
 }
 
 Sort(SortArray.Entries, SortArray.Count, SortFlag_Stable);
 
 return SortArray;
}

internal collision
CheckCollisionWithEntities(editor *Editor, v2 AtP, f32 Tolerance)
{
 ProfileFunctionBegin();
 
 entity_array Entities = AllEntityArrayFromStore(Editor->EntityStore);
 
 collision Result = {};
 temp_arena Temp = TempArena(0);
 sort_entry_array Sorted = SortEntities(Temp.Arena, Entities);
 
 for (u64 SortedIndex = 0;
      SortedIndex < Sorted.Count;
      ++SortedIndex)
 {
  u64 InverseIndex = Sorted.Count-1 - SortedIndex;
  u32 EntityIndex = Sorted.Entries[InverseIndex].Index;
  entity *Entity = Entities.Entities[EntityIndex];
  
  if (IsEntityVisible(Entity))
  {
   v2 LocalAtP = WorldToLocalEntityPosition(Entity, AtP);
   // TODO(hbr): Note that Tolerance becomes wrong when we want to compute everything in
   // local space, fix that
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     u32 ControlPointCount = Curve->Points.ControlPointCount;
     v2 *ControlPoints = Curve->Points.ControlPoints;
     curve_params *Params = &Curve->Params;
     point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
     
     if (AreCurvePointsVisible(Curve))
     {
      //- control points
      {
       f32 MinSignedDistance = F32_INF;
       u32 MinPointIndex = 0;
       for (u32 PointIndex = 0;
            PointIndex < ControlPointCount;
            ++PointIndex)
       {
        // TODO(hbr): Maybe move this function call outside of this loop
        // due to performance reasons.
        point_draw_info PointInfo = GetCurveControlPointDrawInfo(Entity, ControlPointHandleFromIndex(PointIndex));
        f32 CollisionRadius = PointInfo.Radius + PointInfo.OutlineThickness + Tolerance;
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, ControlPoints[PointIndex], CollisionRadius);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPointIndex = PointIndex;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePoint = CurvePointFromControlPoint(ControlPointHandleFromIndex(MinPointIndex));
       }
      }
      
      //- bezier "control" points
      {
       visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
       f32 MinSignedDistance = F32_INF;
       cubic_bezier_point_handle MinPoint = {};
       
       for (u32 Index = 0;
            Index < VisibleBeziers.Count;
            ++Index)
       {
        f32 PointRadius = GetCurveCubicBezierPointRadius(Curve);
        cubic_bezier_point_handle Bezier = VisibleBeziers.Indices[Index];
        v2 BezierPoint = GetCubicBezierPoint(Curve, Bezier);
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, BezierPoint, PointRadius + Tolerance);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPoint = Bezier;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePoint = CurvePointFromCubicBezierPoint(MinPoint);
       }
      }
     }
     
     if (Tracking->Active)
     {
      f32 PointRadius = GetCurveTrackedPointRadius(Curve);
      if (PointCollision(LocalAtP, Tracking->LocalSpaceTrackedPoint, PointRadius + Tolerance))
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_TrackedPoint;
      }
      
      // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
      // anything to Flags.
      if ((Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization) && !Result.Entity)
      {
       u32 IterationCount = Tracking->Intermediate.IterationCount;
       v2 *Points = Tracking->Intermediate.P;
       
       u32 PointIndex = 0;
       for (u32 Iteration = 0;
            Iteration < IterationCount;
            ++Iteration)
       {
        v2 *CurveSamples = Points + PointIndex;
        u32 PointCount = IterationCount - Iteration;
        multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                               CurveSamples,
                                                               PointCount,
                                                               Params->DrawParams.Line.Width,
                                                               Tolerance);
        if (Line.Collided)
        {
         Result.Entity = Entity;
         goto collided_label;
        }
        
        for (u32 I = 0; I < PointCount; ++I)
        {
         if (PointCollision(LocalAtP, Points[PointIndex], Params->DrawParams.Points.Radius + Tolerance))
         {
          Result.Entity = Entity;
          goto collided_label;
         }
         ++PointIndex;
        }
       }
       collided_label:
       NoOp;
      }
     }
     
     if (Params->DrawParams.Line.Enabled)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->CurveSamples,
                                                             Curve->CurveSampleCount,
                                                             Params->DrawParams.Line.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_CurveLine;
       Result.CurveSampleIndex = Line.PointIndex;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsConvexHullVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->ConvexHullPoints,
                                                             Curve->ConvexHullCount,
                                                             Params->DrawParams.ConvexHull.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsPolylineVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             ControlPoints,
                                                             ControlPointCount,
                                                             Params->DrawParams.Polyline.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     if (AreBSplineKnotsVisible(Curve))
     {
      u32 PartitionSize = GetBSplineParams(Curve).KnotParams.PartitionSize;
      v2 *PartitionKnotPoints = Curve->BSplinePartitionKnots;
      point_draw_info KnotPointInfo = GetBSplinePartitionKnotPointDrawInfo(Entity);
      
      // NOTE(hbr): Skip first and last points because they are not moveable
      for (u32 PartitionKnotIndex = 1;
           PartitionKnotIndex + 1 < PartitionSize;
           ++PartitionKnotIndex)
      {
       v2 Knot = PartitionKnotPoints[PartitionKnotIndex];
       if (PointCollision(LocalAtP, Knot, KnotPointInfo.Radius))
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_BSplineKnot;
        Result.BSplineKnot = BSplineKnotHandleFromPartitionKnotIndex(Curve, PartitionKnotIndex);
        break;
       }
      }
     }
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     scale2d Dim = Image->Dim;
     if (-Dim.V.X <= LocalAtP.X && LocalAtP.X <= Dim.V.X &&
         -Dim.V.Y <= LocalAtP.Y && LocalAtP.Y <= Dim.V.Y)
     {
      Result.Entity = Entity;
     }
    } break;
    
    case Entity_Count: InvalidPath; break;
   }
  }
  
  if (Result.Entity)
  {
   break;
  }
 }
 
 EndTemp(Temp);
 
 ProfileEnd();
 
 return Result;
}

inline internal entity_snapshot_for_merging
MakeEntitySnapshotForMerging(entity *Entity)
{
 entity_snapshot_for_merging Result = {};
 Result.Entity = Entity;
 if (Entity)
 {
  Result.XForm = Entity->XForm;
  Result.Flags = Entity->Flags;
  Result.Version = Entity->Version;
 }
 return Result;
}

inline internal b32
EntityModified(entity_snapshot_for_merging Versioned, entity *Entity)
{
 b32 Result = false;
 if (Versioned.Entity != Entity ||
     (Entity && (!StructsEqual(&Entity->XForm, &Versioned.XForm) ||
                 !StructsEqual(&Entity->Flags, &Versioned.Flags) ||
                 !StructsEqual(&Entity->Version, &Versioned.Version))))
 {
  Result = true;
 }
 return Result;
}

inline internal entity_handle
MakeEntityHandle(entity *Entity)
{
 entity_handle Handle = {};
 if (Entity)
 {
  Handle.Entity = Entity;
  Handle.Generation = Entity->Generation;
 }
 return Handle;
}

internal entity_handle
EntityHandleZero(void)
{
 entity_handle Handle = {};
 return Handle;
}

inline internal entity *
EntityFromHandle(entity_handle Handle, b32 AllowDeactived)
{
 entity *Entity = 0;
 if (Handle.Entity &&
     (Handle.Generation == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 if (AllowDeactived &&
     (Entity == 0) &&
     (Handle.Entity->InternalFlags & EntityInternalFlag_Deactivated) &&
     (Handle.Generation + 1 == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 return Entity;
}

internal inline f32
GetCurvePartVisibilityZOffset(curve_part_visibility Part)
{
 Assert(Part < CurvePartVisibility_Count);
 f32 Result = Cast(f32)(CurvePartVisibility_Count-1 - Part) / CurvePartVisibility_Count;
 return Result;
}

inline internal curve *
SafeGetCurve(entity *Entity)
{
 Assert(Entity->Type == Entity_Curve);
 return &Entity->Curve;
}

inline internal image *
SafeGetImage(entity *Entity)
{
 Assert(Entity->Type == Entity_Image);
 return &Entity->Image;
}

inline internal control_point_handle
ControlPointHandleFromIndex(u32 Index)
{
 control_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal u32
IndexFromControlPointHandle(control_point_handle Handle)
{
 Assert(!ControlPointHandleMatch(Handle, ControlPointHandleZero()));
 u32 Result = Handle.Index - 1;
 return Result;
}

inline internal b32
ControlPointHandleMatch(control_point_handle A, control_point_handle B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}

inline internal control_point_handle
ControlPointHandleZero(void)
{
 control_point_handle Index = {};
 return Index;
}

inline internal control_point_handle
ControlPointFromCubicBezierPoint(cubic_bezier_point_handle Bezier)
{
 control_point_handle Result = {};
 if (!CubicBezierPointHandleMatch(Bezier, CubicBezierPointHandleZero()))
 {
  u32 BezierIndex = IndexFromCubicBezierPointHandle(Bezier);
  Result = ControlPointHandleFromIndex(BezierIndex / 3);
 }
 return Result;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleFromIndex(u32 Index)
{
 cubic_bezier_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal cubic_bezier_point_handle
CubicBezierPointFromControlPoint(control_point_handle Handle)
{
 cubic_bezier_point_handle Bezier = {};
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  u32 I = IndexFromControlPointHandle(Handle);
  Bezier = CubicBezierPointHandleFromIndex(3 * I + 1);
 }
 return Bezier;
}

inline internal u32
IndexFromCubicBezierPointHandle(cubic_bezier_point_handle Handle)
{
 Assert(!CubicBezierPointHandleMatch(Handle, CubicBezierPointHandleZero()));
 u32 Index = Handle.Index - 1;
 return Index;
}

inline internal b32
CubicBezierPointHandleMatch(cubic_bezier_point_handle A, cubic_bezier_point_handle B)
{
 b32 Equal = (A.Index == B.Index);
 return Equal;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleZero(void)
{
 cubic_bezier_point_handle Handle = {};
 return Handle;
}

inline internal curve_point_handle
CurvePointFromControlPoint(control_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_ControlPoint;
 Result.Control = Handle;
 return Result;
}

inline internal curve_point_handle
CurvePointFromCubicBezierPoint(cubic_bezier_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_CubicBezierPoint;
 Result.Bezier = Handle;
 return Result;
}

inline internal control_point_handle
ControlPointFromCurvePoint(curve_point_handle Handle)
{
 control_point_handle Result = {};
 switch (Handle.Type)
 {
  case CurvePoint_ControlPoint: {Result = Handle.Control;}break;
  case CurvePoint_CubicBezierPoint: {Result = ControlPointFromCubicBezierPoint(Handle.Bezier);}break;
 }
 return Result;
}

inline internal v2
GetCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromCubicBezierPointHandle(Point);
 if (Index < 3 * Curve->Points.ControlPointCount)
 {
  v2 *Beziers = Cast(v2 *)Curve->Points.CubicBezierPoints;
  Result = Beziers[Index];
 }
 return Result;
}

inline internal v2
GetControlPointP(curve *Curve, control_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromControlPointHandle(Point);
 if (Index < Curve->Points.ControlPointCount)
 {
  Result = Curve->Points.ControlPoints[Index];
 }
 return Result;
}

inline internal v2
GetCenterPointFromCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Bezier)
{
 control_point_handle Control = ControlPointFromCubicBezierPoint(Bezier);
 v2 Point = GetControlPointP(Curve, Control);
 return Point;
}

inline internal control_point_handle
ControlPointIndexFromCurveSampleIndex(curve *Curve, u32 CurveSampleIndex)
{
 Assert(!IsCurveTotalSamplesMode(Curve));
 u32 Index = SafeDiv0(CurveSampleIndex, Curve->Params.SamplesPerControlPoint);
 Assert(Index < Curve->Points.ControlPointCount);
 Index = ClampTop(Index, Curve->Points.ControlPointCount - 1);
 control_point_handle Control = ControlPointHandleFromIndex(Index);
 return Control;
}

internal b32
AreCurvePointsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.DrawParams.Points.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsPolylineVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.Polyline.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsConvexHullVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.ConvexHull.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsBSplineCurve(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->Type == Curve_BSpline);
 return Result;
}

internal b32
AreBSplineConvexHullsVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (IsBSplineCurve(Curve) && CurveParams->DrawParams.BSplinePartialConvexHull.Enabled);
 return Result;
}

internal b32
UsesControlPoints(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_BSpline: {Result = true;}break;
  
  case Curve_Parametric: {}break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

internal b32
IsEntityVisible(entity *Entity)
{
 b32 Result = !(Entity->Flags & EntityFlag_Hidden);
 return Result;
}

internal void
SetEntityVisibility(entity *Entity, b32 Visible)
{
 if (Visible)
 {
  Entity->Flags &= ~EntityFlag_Hidden;
 }
 else
 {
  Entity->Flags |= EntityFlag_Hidden;
 }
}

internal b32
IsEntitySelected(entity *Entity)
{
 b32 Result = (Entity->Flags & EntityFlag_Selected);
 return Result;
}

internal f32
GetCurveTrackedPointRadius(curve *Curve)
{
 f32 Result = 1.5f * Curve->Params.DrawParams.Line.Width;
 return Result;
}

internal void
AddVisibleCubicBezierPoint(visible_cubic_bezier_points *Visible, cubic_bezier_point_handle Point)
{
 Assert(Visible->Count < ArrayCount(Visible->Indices));
 Visible->Indices[Visible->Count] = Point;
 ++Visible->Count;
}

inline internal b32
PrevCubicBezierPoint(cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 if (Index > 0)
 {
  *Point = CubicBezierPointHandleFromIndex(Index - 1);
  Result = true;
 }
 return Result;
}

inline internal b32
NextCubicBezierPoint(curve *Curve, cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 if (Index + 1 < 3 * Curve->Points.ControlPointCount)
 {
  *Point = CubicBezierPointHandleFromIndex(Index + 1);
  Result = true;
 }
 return Result;
}

internal visible_cubic_bezier_points
GetVisibleCubicBezierPoints(entity *Entity)
{
 visible_cubic_bezier_points Result = {};
 
 curve *Curve = SafeGetCurve(Entity);
 if (IsEntitySelected(Entity) &&
     IsControlPointSelected(Curve) &&
     Curve->Params.Type == Curve_Bezier &&
     Curve->Params.Bezier == Bezier_Cubic)
 {
  cubic_bezier_point_handle StartPoint = CubicBezierPointFromControlPoint(Curve->SelectedControlPoint);
  
  cubic_bezier_point_handle PrevPoint = StartPoint;
  if (PrevCubicBezierPoint(&PrevPoint))
  {
   AddVisibleCubicBezierPoint(&Result, PrevPoint);
   if (PrevCubicBezierPoint(&PrevPoint))
   {
    AddVisibleCubicBezierPoint(&Result, PrevPoint);
   }
  }
  
  cubic_bezier_point_handle NextPoint = StartPoint;
  if (NextCubicBezierPoint(Curve, &NextPoint))
  {
   AddVisibleCubicBezierPoint(&Result, NextPoint);
   if (NextCubicBezierPoint(Curve, &NextPoint))
   {
    AddVisibleCubicBezierPoint(&Result, NextPoint);
   }
  }
 }
 
 return Result;
}

internal b32
IsCurveEligibleForPointTracking(curve *Curve)
{
 curve_params *Params = &Curve->Params;
 curve_type Interpolation = Params->Type;
 bezier_type BezierVariant = Params->Bezier;
 b32 Result = (Interpolation == Curve_Bezier && BezierVariant == Bezier_Regular);
 
 return Result;
}

internal b32
CurveHasWeights(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_Bezier &&
               Curve->Params.Bezier == Bezier_Regular);
 return Result;
}

internal b32
IsCurveTotalSamplesMode(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_BSpline: {}break;
  
  case Curve_Parametric: {
   Result = true;
  }break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

inline internal b32
IsCurve(entity *Entity)
{
 b32 Result = (Entity->Type == Entity_Curve);
 return Result;
}

internal b32
IsCurveReversed(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 b32 Result = (UsesControlPoints(Curve) && (Entity->Flags & EntityFlag_CurveAppendFront));
 return Result;
}

internal b32
IsRegularBezierCurve(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_Bezier && Curve->Params.Bezier == Bezier_Regular);
 return Result;
}

internal b32
AreBSplineKnotsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_BSpline && Curve->Params.DrawParams.BSplineKnots.Enabled);
 return Result;
}

internal curve_merge_compatibility
AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method)
{
 b32 Compatible = false;
 string WhyIncompatible = NilStr;
 
 curve_params *Params0 = &Curve0->Params;
 curve_params *Params1 = &Curve1->Params;
 
 string IncompatibleTypes = StrLit("cannot merge curves of different types");
 
#define Concat_C0_G1_Str " can only be merged using [Concat], [C0] or [G1] methods"
#define Concat_Str       " can only be merged using [Concat] method"
 
 b32 Concat_C0_G1 = (Method == CurveMerge_Concat ||
                     Method == CurveMerge_C0 ||
                     Method == CurveMerge_G1);
 b32 Concat = (Method == CurveMerge_Concat);
 
 if (Params0->Type == Params1->Type)
 {
  switch (Params0->Type)
  {
   case Curve_Polynomial: {
    if (Concat_C0_G1) Compatible = true;
    else WhyIncompatible = StrLit("polynomial curves" Concat_C0_G1_Str);
   }break;
   
   case Curve_CubicSpline: {
    if (Params0->CubicSpline == Params1->CubicSpline)
    {
     if (Concat_C0_G1) Compatible = true;
     else WhyIncompatible = StrLit("cubic spline curves" Concat_C0_G1_Str);
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_Bezier: {
    if (Params0->Bezier == Params1->Bezier)
    {
     switch (Params0->Bezier)
     {
      case Bezier_Regular: {Compatible = true;}break;
      case Bezier_Cubic: {
       if (Concat_C0_G1) Compatible = true;
       else WhyIncompatible = StrLit("cubic bezier curves" Concat_C0_G1_Str);
      }break;
      case Bezier_Count: InvalidPath;
     }
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_BSpline: {
    if (Concat) Compatible = true;
    else WhyIncompatible = StrLit("B-spline curves" Concat_Str);
   }break;
   
   case Curve_Parametric: {WhyIncompatible = StrLit("parametric curves cannot be merged");}break;
   
   case Curve_Count: InvalidPath;
  }
 }
 else
 {
  WhyIncompatible = IncompatibleTypes;
 }
 
 curve_merge_compatibility Result = {};
 Result.Compatible = Compatible;
 Result.WhyIncompatible = WhyIncompatible;
 
 return Result;
}

internal void
MarkEntitySelected(entity *Entity)
{
 Entity->Flags |= EntityFlag_Selected;
}

internal void
MarkEntityDeselected(entity *Entity)
{
 Entity->Flags &= ~EntityFlag_Selected;
}

internal point_draw_info
GetEntityPointDrawInfo(entity *Entity, f32 BaseRadius, rgba BaseColor)
{
 point_draw_info Result = {};
 
 Result.Radius = BaseRadius;
 
 f32 OutlineScale = 0.0f;
 if (IsEntitySelected(Entity))
 {
  OutlineScale = 0.55f;
 }
 Result.OutlineThickness = OutlineScale * Result.Radius;
 
 Result.Color = BaseColor;
 Result.OutlineColor = RGBA_Brighten(Result.Color, 0.3f);
 
 return Result;
}

internal point_draw_info
GetCurveControlPointDrawInfo(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 u32 Index = IndexFromControlPointHandle(Point);
 
 point_draw_info Result = GetEntityPointDrawInfo(Entity, Params->DrawParams.Points.Radius, Params->DrawParams.Points.Color);
 
 if (( (Entity->Flags & EntityFlag_CurveAppendFront) && Index == 0) ||
     (!(Entity->Flags & EntityFlag_CurveAppendFront) && Index == Curve->Points.ControlPointCount-1))
 {
  Result.Radius *= 2.0f;
 }
 
 if (ControlPointHandleMatch(Point, Curve->SelectedControlPoint))
 {
  //Result.OutlineColor = RGBA_Brighten(Result.OutlineColor, 0.5f);
  
  Result.Color = RGBA_Opposite(Result.Color);
  Result.OutlineColor = RGBA_Brighten(Result.Color, 0.5f);
 }
 
 return Result;
}

internal point_draw_info
GetBSplinePartitionKnotPointDrawInfo(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 draw_params *BSplineKnots = &Params->DrawParams.BSplineKnots;
 point_draw_info Result = GetEntityPointDrawInfo(Entity, BSplineKnots->Radius, BSplineKnots->Color);
 return Result;
}

internal f32
GetCurveCubicBezierPointRadius(curve *Curve)
{
 // TODO(hbr): Make bezier points smaller
 f32 Result = Curve->Params.DrawParams.Points.Radius;
 return Result;
}

internal string
GetEntityName(entity *Entity, string_store *StrStore)
{
 string Name = StringFromStringId(StrStore, Entity->Name);
 return Name;
}

internal char_buffer *
GetEntityNameBuffer(entity *Entity, string_store *StrStore)
{
 char_buffer *Buffer = CharBufferFromStringId(StrStore, Entity->Name);
 return Buffer;
}

internal control_point
MakeControlPoint(v2 Point)
{
 control_point Result = {};
 Result.P = Point;
 return Result;
}

internal control_point
MakeControlPoint(v2 Point, f32 Weight, cubic_bezier_point Bezier)
{
 control_point Result = {};
 Result.P = Point;
 Result.Weight = Weight;
 Result.Bezier = Bezier;
 Result.IsWeight = Result.IsBezier = true;
 return Result;
}

internal curve_points_handle
MakeCurvePointsHandle(u32 ControlPointCount,
                      v2 *ControlPoints,
                      f32 *ControlPointWeights,
                      cubic_bezier_point *CubicBezierPoints)
{
 curve_points_handle Points = {};
 Points.ControlPointCount = ControlPointCount;
 Points.ControlPoints = ControlPoints;
 Points.ControlPointWeights = ControlPointWeights;
 Points.CubicBezierPoints = CubicBezierPoints;
 return Points;
}

internal curve_points_handle
CurvePointsHandleZero(void)
{
 curve_points_handle Handle = {};
 return Handle;
}

internal curve_points_handle
CurvePointsHandleFromCurvePointsStatic(curve_points_static *Static)
{
 curve_points_handle Handle = MakeCurvePointsHandle(Static->ControlPointCount,
                                                    Static->ControlPoints,
                                                    Static->ControlPointWeights,
                                                    Static->CubicBezierPoints);
 return Handle;
}

internal curve_points_dynamic
MakeCurvePointsDynamic(u32 *ControlPointCount,
                       v2 *ControlPoints,
                       f32 *ControlPointWeights,
                       cubic_bezier_point *CubicBezierPoints,
                       u32 Capacity)
{
 curve_points_dynamic Result = {};
 Result.ControlPointCount = ControlPointCount;
 Result.ControlPoints = ControlPoints;
 Result.ControlPointWeights = ControlPointWeights;
 Result.CubicBezierPoints = CubicBezierPoints;
 Result.Capacity = Capacity;
 return Result;
}

internal curve_points_dynamic
CurvePointsDynamicFromStatic(curve_points_static *Static)
{
 curve_points_dynamic Dynamic = MakeCurvePointsDynamic(&Static->ControlPointCount,
                                                       Static->ControlPoints,
                                                       Static->ControlPointWeights,
                                                       Static->CubicBezierPoints,
                                                       CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT);
 return Dynamic;
}

internal control_point
GetCurveControlPointInWorldSpace(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 
 v2 P = LocalToWorldEntityPosition(Entity, Curve->Points.ControlPoints[Index]);
 f32 Weight = Curve->Points.ControlPointWeights[Index];
 cubic_bezier_point B = Curve->Points.CubicBezierPoints[Index];
 cubic_bezier_point Bezier = {
  LocalToWorldEntityPosition(Entity, B.Ps[0]),
  LocalToWorldEntityPosition(Entity, B.Ps[1]),
  LocalToWorldEntityPosition(Entity, B.Ps[2]),
 };
 
 control_point Result = MakeControlPoint(P, Weight, Bezier);
 return Result;
}

internal void
CopyCurvePointsFromCurve(curve *Curve, curve_points_dynamic Dst)
{
 CopyCurvePoints(Dst, CurvePointsHandleFromCurvePointsStatic(&Curve->Points));
}

internal void
CopyCurvePoints(curve_points_dynamic Dst, curve_points_handle Src)
{
 u32 Copy = Min(Src.ControlPointCount, Dst.Capacity);
 *Dst.ControlPointCount = Copy;
 ArrayCopy(Dst.ControlPoints, Src.ControlPoints, Copy);
 ArrayCopy(Dst.ControlPointWeights, Src.ControlPointWeights, Copy);
 ArrayCopy(Dst.CubicBezierPoints, Src.CubicBezierPoints, Copy);
}

internal rect2
EntityAABB(entity *Entity)
{
 rect2 AABB = EmptyAABB();
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   u32 SampleCount = Curve->CurveSampleCount;
   v2 *Samples = Curve->CurveSamples;
   u32 SampleIndex = 0;
   while (SampleIndex + 8 < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex + 0]);
    AddPointAABB(&AABB, Samples[SampleIndex + 1]);
    AddPointAABB(&AABB, Samples[SampleIndex + 2]);
    AddPointAABB(&AABB, Samples[SampleIndex + 3]);
    AddPointAABB(&AABB, Samples[SampleIndex + 4]);
    AddPointAABB(&AABB, Samples[SampleIndex + 5]);
    AddPointAABB(&AABB, Samples[SampleIndex + 6]);
    AddPointAABB(&AABB, Samples[SampleIndex + 7]);
    SampleIndex += 8;
   }
   while (SampleIndex < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex]);
    ++SampleIndex;
   }
  } break;
  
  case Entity_Image: {
   image *Image = SafeGetImage(Entity);
   scale2d Dim = Image->Dim;
   AddPointAABB(&AABB, V2( Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2( Dim.V.X, -Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X, -Dim.V.Y));
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
 
 rect2_corners AABB_Corners = AABBCorners(AABB);
 rect2 AABB_Transformed = EmptyAABB();
 ForEachEnumVal(Corner, Corner_Count, corner)
 {
  v2 P = LocalToWorldEntityPosition(Entity, AABB_Corners.Corners[Corner]);
  AddPointAABB(&AABB_Transformed, P);
 }
 
 return AABB_Transformed;
}

internal b_spline_params
GetBSplineParams(curve *Curve)
{
 return Curve->ComputedBSplineParams;
}

internal b32
BSplineKnotHandleMatch(b_spline_knot_handle A, b_spline_knot_handle B)
{
 b32 Result = (A.__Index == B.__Index);
 return Result;
}

internal b_spline_knot_handle
BSplineKnotHandleZero(void)
{
 b_spline_knot_handle Handle = {};
 return Handle;
}

internal u32
KnotIndexFromBSplineKnotHandle(b_spline_knot_handle Handle)
{
 Assert(!BSplineKnotHandleMatch(Handle, BSplineKnotHandleZero()));
 u32 Index = Handle.__Index - 1;
 return Index;
}

internal u32
PartitionKnotIndexFromBSplineKnotHandle(curve *Curve, b_spline_knot_handle Handle)
{
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Handle);
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 Assert(KnotIndex >= KnotParams.Degree);
 u32 PartitionKnotIndex = KnotIndex - KnotParams.Degree;
 return PartitionKnotIndex;
}

internal b_spline_knot_handle
BSplineKnotHandleFromKnotIndex(u32 KnotIndex)
{
 b_spline_knot_handle Handle = {};
 Handle.__Index = KnotIndex + 1;
 return Handle;
}

internal b_spline_knot_handle
BSplineKnotHandleFromPartitionKnotIndex(curve *Curve, u32 PartitionKnotIndex)
{
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 u32 KnotIndex = KnotParams.Degree + PartitionKnotIndex;
 b_spline_knot_handle Handle = BSplineKnotHandleFromKnotIndex(KnotIndex);
 return Handle;
}

internal void
ControlPointAndBezierOffsetFromCurvePoint(curve_point_handle CurvePoint,
                                          control_point_handle *ControlPoint,
                                          u32 *BezierOffset)
{
 switch (CurvePoint.Type)
 {
  case CurvePoint_ControlPoint: {
   *ControlPoint = CurvePoint.Control;
   *BezierOffset = 1;
  }break;
  case CurvePoint_CubicBezierPoint: {
   u32 BezierIndex = IndexFromCubicBezierPointHandle(CurvePoint.Bezier);
   *ControlPoint = ControlPointFromCubicBezierPoint(CurvePoint.Bezier);
   *BezierOffset = BezierIndex % 3;
  }break;
 }
 Assert(*BezierOffset < 3);
}

internal void
TranslateCurvePointTo(entity_with_modify_witness *EntityWitness,
                      curve_point_handle CurvePoint,
                      v2 P,
                      translate_curve_point_flags Flags)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 control_point_handle ControlPoint = {};
 u32 BezierOffset = 0;
 ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
 
 u32 ControlPointIndex = IndexFromControlPointHandle(ControlPoint);
 
 if (ControlPointIndex < Curve->Points.ControlPointCount)
 {
  cubic_bezier_point *B = Curve->Points.CubicBezierPoints + ControlPointIndex;
  v2 *TranslatePoint = B->Ps + BezierOffset;
  v2 LocalP = WorldToLocalEntityPosition(Entity, P);
  
  if (BezierOffset == 1)
  {
   v2 LocalTranslation = LocalP - B->P1;
   
   B->P0 += LocalTranslation;
   B->P2 += LocalTranslation;
   
   Curve->Points.ControlPoints[ControlPointIndex] += LocalTranslation;
  }
  else
  {
   Assert(BezierOffset == 0 || BezierOffset == 2);
   v2 *Twin = B->Ps + (2 - BezierOffset);
   
   v2 DesiredTwinDirection;
   if (Flags & TranslateCurvePoint_MatchBezierTwinDirection)
   {
    DesiredTwinDirection = B->P1 - LocalP;
   }
   else
   {
    DesiredTwinDirection = *Twin - B->P1;
   }
   Normalize(&DesiredTwinDirection);
   
   v2 DesiredLengthVector;
   if (Flags & TranslateCurvePoint_MatchBezierTwinLength)
   {
    DesiredLengthVector = B->P1 - LocalP;
   }
   else
   {
    DesiredLengthVector = *Twin - B->P1;
   }
   f32 DesiredTwinLength = Norm(DesiredLengthVector);
   
   *Twin = B->P1 + DesiredTwinLength * DesiredTwinDirection;
  }
  
  *TranslatePoint = LocalP;
 }
 
 MarkEntityModified(EntityWitness);
}

internal void
SelectControlPoint(curve *Curve, control_point_handle ControlPoint)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
SelectControlPointFromCurvePoint(curve *Curve, curve_point_handle CurvePoint)
{
 if (Curve)
 {
  control_point_handle ControlPoint = {};
  u32 BezierOffset = 0;
  ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
MaybeRecomputeCurveBSplineKnots(curve *Curve)
{
 u32 ControlPointCount = Curve->Points.ControlPointCount;
 curve_params *CurveParams = &Curve->Params;
 b_spline_params *RequestedBSplineParams = &CurveParams->BSpline;
 
 b_spline_knot_params FixedBSplineKnotParams = BSplineKnotParamsFromDegree(RequestedBSplineParams->KnotParams.Degree, ControlPointCount);
 Assert(FixedBSplineKnotParams.KnotCount <= MAX_B_SPLINE_KNOT_COUNT);
 b_spline_params FixedBSplineParams = {};
 FixedBSplineParams.Partition = RequestedBSplineParams->Partition;
 FixedBSplineParams.KnotParams = FixedBSplineKnotParams;
 CurveParams->BSpline = FixedBSplineParams;
 
 b_spline_params ComputedBSplineParams = Curve->ComputedBSplineParams;
 
 if (!StructsEqual(&ComputedBSplineParams, &FixedBSplineParams))
 {
  f32 *Knots = Curve->BSplineKnots;
  switch (FixedBSplineParams.Partition)
  {
   case BSplinePartition_Natural: {
    BSplineBaseKnots(FixedBSplineKnotParams, Knots);
    BSplineKnotsNaturalExtension(FixedBSplineParams.KnotParams, Knots);
   }break;
   
   case BSplinePartition_Periodic: {
    BSplineBaseKnots(FixedBSplineKnotParams, Knots);
    BSplineKnotsPeriodicExtension(FixedBSplineParams.KnotParams, Knots);
   }break;
   
   case BSplinePartition_Custom: {
    u32 ComputedKnotCount = ComputedBSplineParams.KnotParams.KnotCount;
    u32 FixedKnotCount = FixedBSplineParams.KnotParams.KnotCount;
    f32 LastKnot = (ComputedKnotCount > 0 ? Knots[ComputedKnotCount - 1] : FixedBSplineParams.KnotParams.B);
    // NOTE(hbr): Just knots with appropriate amount of [B] knots at the end
    for (u32 KnotIndex = ComputedKnotCount;
         KnotIndex < FixedKnotCount;
         ++KnotIndex)
    {
     Knots[KnotIndex] = LastKnot;
    }
   }break;
   case BSplinePartition_Count: InvalidPath;
  }
  Curve->ComputedBSplineParams = FixedBSplineParams;
 }
}

internal void
RemoveControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Point)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 
 if (Index < Curve->Points.ControlPointCount)
 {
#define ArrayRemoveOrdered(Array, At, Count) \
MemoryMove((Array) + (At), \
(Array) + (At) + 1, \
((Count) - (At) - 1) * SizeOf((Array)[0]))
  ArrayRemoveOrdered(Curve->Points.ControlPoints,       Index, Curve->Points.ControlPointCount);
  ArrayRemoveOrdered(Curve->Points.ControlPointWeights, Index, Curve->Points.ControlPointCount);
  ArrayRemoveOrdered(Curve->Points.CubicBezierPoints,   Index, Curve->Points.ControlPointCount);
#undef ArrayRemoveOrdered
  
  --Curve->Points.ControlPointCount;
  
  // NOTE(hbr): Maybe fix selected control point
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (Index == SelectedIndex)
   {
    DeselectControlPoint(Curve);
   }
   else if (Index < SelectedIndex)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex - 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
DeselectControlPoint(curve *Curve)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPointHandleZero();
 }
}

internal b32
IsControlPointSelected(curve *Curve)
{
 b32 Result = false;
 if (UsesControlPoints(Curve) &&
     !ControlPointHandleMatch(Curve->SelectedControlPoint, ControlPointHandleZero()))
 {
  u32 Index = IndexFromControlPointHandle(Curve->SelectedControlPoint);
  Result = (Index < Curve->Points.ControlPointCount);
 }
 return Result;
}

internal control_point_handle
AppendControlPoint(entity_with_modify_witness *EntityWitness, v2 P)
{
 entity *Entity = EntityWitness->Entity;
 u32 InsertAt = 0;
 if (Entity->Flags & EntityFlag_CurveAppendFront)
 {
  InsertAt = 0;
 }
 else
 {
  InsertAt = Entity->Curve.Points.ControlPointCount;
 }
 control_point_handle Result = InsertControlPoint(EntityWitness, MakeControlPoint(P), InsertAt);
 return Result;
}

internal void
SetControlPoint(entity_with_modify_witness *Witness, control_point_handle Handle, control_point Point)
{
 entity *Entity = Witness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 u32 I = IndexFromControlPointHandle(Handle);
 u32 N = Curve->Points.ControlPointCount;
 v2 *P = Curve->Points.ControlPoints;
 f32 *W = Curve->Points.ControlPointWeights;
 cubic_bezier_point *B = Curve->Points.CubicBezierPoints;
 
 P[I] = WorldToLocalEntityPosition(Entity, Point.P);
 W[I] = (Point.IsWeight ? Point.Weight : 1.0f);
 if (Point.IsBezier)
 {
  cubic_bezier_point Bezier = {
   WorldToLocalEntityPosition(Entity, Point.Bezier.P0),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P1),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P2),
  };
  B[I] = Bezier;
 }
 else
 {
  // NOTE(hbr): Cubic bezier point calculation is not really defined
  // for N<=2. At least I tried to "define" it but failed to do something
  // that was super useful. In that case, when the third point is added,
  // just recalculate all bezier points.
  if (Curve->Points.ControlPointCount == 2)
  {
   BezierCubicCalculateAllControlPoints(N+1, P, B);
  }
  else
  {
   CalculateBezierCubicPointAt(N+1, P, B, I);
  }
 }
 
 MarkEntityModified(Witness);
}

internal control_point_handle
InsertControlPoint(entity_with_modify_witness *EntityWitness, control_point Point, u32 At)
{
 control_point_handle Result = {};
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 if (Curve &&
     Curve->Points.ControlPointCount < CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT &&
     At <= Curve->Points.ControlPointCount)
 {
  Assert(UsesControlPoints(Curve));
  
  control_point_handle Handle = ControlPointHandleFromIndex(At);
  u32 N = Curve->Points.ControlPointCount;
  v2 *P = Curve->Points.ControlPoints;
  f32 *W = Curve->Points.ControlPointWeights;
  cubic_bezier_point *B = Curve->Points.CubicBezierPoints;
  
#define ShiftRightArray(Array, ArrayLength, At, ShiftCount) \
MemoryMove((Array) + ((At)+(ShiftCount)), \
(Array) + (At), \
((ArrayLength) - (At)) * SizeOf((Array)[0]))
  ShiftRightArray(P, N, At, 1);
  ShiftRightArray(W, N, At, 1);
  ShiftRightArray(B, N, At, 1);
#undef ShiftRightArray
  
  SetControlPoint(EntityWitness, Handle, Point);
  
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (SelectedIndex >= At)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex + 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  ++Curve->Points.ControlPointCount;
  Result = Handle;
 }
 
 return Result;
}

internal void
SetCurvePoints(entity_with_modify_witness *EntityWitness, curve_points_handle Points)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 CopyCurvePoints(CurvePointsDynamicFromStatic(&Curve->Points), Points);
 DeselectControlPoint(Curve);
 MarkEntityModified(EntityWitness);
}

inline internal void
SetEntityName(entity *Entity, string Name, string_store *StrStore)
{
 char_buffer *CharBuffer = CharBufferFromStringId(StrStore, Entity->Name);
 FillCharBuffer(CharBuffer, Name);
}

internal void
InitEntityPart(entity *Entity,
               entity_type Type,
               xform2d XForm,
               string Name,
               i32 SortingLayer,
               entity_flags Flags,
               string_store *StrStore)
{
 Entity->Type = Type;
 Entity->XForm = XForm;
 SetEntityName(Entity, Name, StrStore);
 Entity->SortingLayer = SortingLayer;
 Entity->Flags = Flags;
}

internal void
InitEntityCurvePart(curve *Curve, curve_params CurveParams)
{
 Curve->Params = CurveParams;
}

internal void
InitEntityAsCurve(entity *Entity, string Name, curve_params CurveParams, string_store *StrStore)
{
 InitEntityPart(Entity, Entity_Curve, XForm2DZero(), Name, 0, 0, StrStore);
 InitEntityCurvePart(&Entity->Curve, CurveParams);
}

internal void
InitEntityImagePart(image *Image,
                    scale2d Dim,
                    string ImageFilePath,
                    render_texture_handle TextureHandle,
                    string_store *StrStore)
{
 Image->Dim = Dim;
 char_buffer *FilePathBuffer = CharBufferFromStringId(StrStore, Image->FilePath);
 FillCharBuffer(FilePathBuffer, ImageFilePath);
 Image->TextureHandle = TextureHandle;
}

internal void
InitEntityImagePart(image *Image,
                    u32 Width, u32 Height,
                    string ImageFilePath,
                    render_texture_handle TextureHandle,
                    string_store *StrStore)
{
 scale2d Dim = Scale2D(Cast(f32)Width / Height, 1.0f);
 InitEntityImagePart(Image, Dim, ImageFilePath, TextureHandle, StrStore);
}

internal void
InitEntityAsImage(entity *Entity,
                  v2 P,
                  u32 Width, u32 Height,
                  string ImageFilePath,
                  render_texture_handle TextureHandle,
                  string_store *StrStore)
{
 string FileName = PathLastPart(ImageFilePath);
 string FileNameNoExt = StrChopLastDot(FileName);
 InitEntityPart(Entity, Entity_Image, XForm2DFromP(P), FileNameNoExt, 0, 0, StrStore);
 InitEntityImagePart(&Entity->Image, Width, Height, ImageFilePath, TextureHandle, StrStore);
}

internal void
InitEntityPartFromEntity(entity *Dst, entity *Src, entity_store *EntityStore)
{
 string_store *StrStore = EntityStore->StrStore;
 InitEntityPart(Dst,
                Src->Type,
                Src->XForm,
                GetEntityName(Src, StrStore),
                Src->SortingLayer,
                Src->Flags,
                StrStore);
}

internal void
InitEntityFromEntity(entity_with_modify_witness *DstWitness, entity *Src, entity_store *EntityStore)
{
 entity *Dst = DstWitness->Entity;
 curve *DstCurve = &Dst->Curve;
 curve *SrcCurve = &Src->Curve;
 image *DstImage = &Dst->Image;
 image *SrcImage = &Src->Image;
 string_store *StrStore = EntityStore->StrStore;
 //- init entity part
 InitEntityPart(Dst,
                Src->Type,
                Src->XForm,
                GetEntityName(Src, StrStore),
                Src->SortingLayer,
                Src->Flags,
                StrStore);
 //- init specific entity type part
 switch (Src->Type)
 {
  case Entity_Curve: {
   InitEntityCurvePart(DstCurve, SrcCurve->Params);
   SetCurvePoints(DstWitness, CurvePointsHandleFromCurvePointsStatic(&SrcCurve->Points));
   SelectControlPoint(DstCurve, SrcCurve->SelectedControlPoint);
  }break;
  
  case Entity_Image: {
   DeallocTextureHandle(EntityStore, DstImage->TextureHandle);
   render_texture_handle TextureHandle = CopyTextureHandle(EntityStore, SrcImage->TextureHandle);
   string SrcFilePath = StringFromStringId(StrStore, SrcImage->FilePath);
   InitEntityImagePart(DstImage, SrcImage->Dim, SrcFilePath, TextureHandle, StrStore);
  }break;
  
  case Entity_Count: InvalidPath;
 }
}

internal void
SetControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Handle, v2 P, f32 Weight)
{
 curve_point_handle CurvePoint = CurvePointFromControlPoint(Handle);
 entity *Entity = EntityWitness->Entity;
 TranslateCurvePointTo(EntityWitness, CurvePoint, P, TranslateCurvePoint_None);
 
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  curve *Curve = SafeGetCurve(Entity);
  u32 Index = IndexFromControlPointHandle(Handle);
  Curve->Points.ControlPointWeights[Index] = Weight;
  MarkEntityModified(EntityWitness);
 }
}

internal curve_points_modify_handle
BeginModifyCurvePoints(entity_with_modify_witness *EntityWitness, u32 RequestedPointCount, modify_curve_points_static_which_points Which)
{
 curve_points_modify_handle Result = {};
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *CurvePoints = &Curve->Points;
 Result.Curve = Curve;
 u32 ActualPointCount = Min(RequestedPointCount, CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT);
 Result.PointCount = ActualPointCount;
 Result.ControlPoints = CurvePoints->ControlPoints;
 Result.Weights = CurvePoints->ControlPointWeights;
 Result.CubicBeziers = CurvePoints->CubicBezierPoints;
 Result.Which = Which;
 // NOTE(hbr): Assume (which is very reasonable) that the curve is always changing here
 MarkEntityModified(EntityWitness);
 
 return Result;
}

internal void
EndModifyCurvePoints(curve_points_modify_handle Handle)
{
 curve *Curve = Handle.Curve;
 curve_points_static *Points = &Curve->Points;
 Points->ControlPointCount = Handle.PointCount;
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsOnly)
 {
  for (u32 PointIndex = 0;
       PointIndex < Points->ControlPointCount;
       ++PointIndex)
  {
   Points->ControlPointWeights[PointIndex] = 1.0f;
  }
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsWithWeights)
 {
  BezierCubicCalculateAllControlPoints(Points->ControlPointCount,
                                       Points->ControlPoints,
                                       Points->CubicBezierPoints);
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_AllPoints)
 {
  // NOTE(hbr): Nothing to do
 }
}

inline internal v2
ProjectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Hadamard(Q, XForm.Scale.V);
 Q = RotateAround(Q, V2(0.0f, 0.0f), XForm.Rotation);
 Q = Q + XForm.P;
 return Q;
}

inline internal v2
UnprojectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Q - XForm.P;
 Q = RotateAround(Q, V2(0, 0), Rotation2DInverse(XForm.Rotation));
 Q = Hadamard(Q, V2(1.0f / XForm.Scale.V.X, 1.0f / XForm.Scale.V.Y));
 return Q;
}

// TODO(hbr): Consider using ModelTransform instead (might be a little slower but probably doesn't matter)
internal v2
WorldToLocalEntityPosition(entity *Entity, v2 P)
{
 v2 Result = UnprojectThroughXForm(Entity->XForm, P);
 return Result;
}

internal v2
LocalToWorldEntityPosition(entity *Entity, v2 P)
{
 v2 Result = ProjectThroughXForm(Entity->XForm, P);
 return Result;
}

internal void
SetTrackingPointFraction(entity_with_modify_witness *EntityWitness, f32 Fraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 Tracking->Fraction = Clamp01(Fraction);
 MarkEntityModified(EntityWitness);
}

internal void
SetBSplineKnotPoint(entity_with_modify_witness *EntityWitness,
                    b_spline_knot_handle Knot,
                    f32 KnotFraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
 f32 *Knots = Curve->BSplineKnots;
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Knot);
 b32 Moveable = ((KnotIndex >= KnotParams.Degree + 1) &&
                 (KnotIndex + 1 < KnotParams.Degree + KnotParams.PartitionSize));
 Assert(Moveable);
 if (Moveable)
 {
  for (u32 Index = KnotIndex;
       Index > 0;
       --Index)
  {
   Knots[Index - 1] = Min(KnotFraction, Knots[Index - 1]);
  }
  for (u32 Index = KnotIndex + 1;
       Index < KnotParams.KnotCount;
       ++Index)
  {
   Knots[Index] = Max(KnotFraction, Knots[Index]);
  }
  Knots[KnotIndex] = KnotFraction;
  MarkEntityModified(EntityWitness);
 }
}

// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->Params.Type == Curve_CubicSpline &&
                 Curve->Params.CubicSpline == CubicSpline_Periodic);
 return IsLooped;
}

struct points_soa
{
 f32 *Xs;
 f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2 *Points, u32 PointCount)
{
 points_soa Result = {};
 Result.Xs = PushArrayNonZero(Arena, PointCount, f32);
 Result.Ys = PushArrayNonZero(Arena, PointCount, f32);
 for (u32 I = 0; I < PointCount; ++I)
 {
  Result.Xs[I] = Points[I].X;
  Result.Ys[I] = Points[I].Y;
 }
 
 return Result;
}

internal void
CalcNormalBezierLinePoints(v2 *ControlPoints,
                           u32 ControlPointCount,
                           u32 OutputLinePointCount,
                           v2 *OutputLinePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (OutputLinePointCount - 1);
 for (u32 OutputIndex = 0;
      OutputIndex < OutputLinePointCount;
      ++OutputIndex)
 {
  
  OutputLinePoints[OutputIndex] = BezierCurveEvaluate(T,
                                                      ControlPoints,
                                                      ControlPointCount);
  
  T += Delta;
 }
}

struct cubic_bezier_curve_segment
{
 u32 PointCount;
 v2 *Points;
};
internal cubic_bezier_curve_segment
GetCubicBezierCurveSegment(cubic_bezier_point *BezierPoints, u32 PointCount, u32 SegmentIndex)
{
 cubic_bezier_curve_segment Result = {};
 if (SegmentIndex < PointCount)
 {
  Result.PointCount = (SegmentIndex < PointCount - 1 ? 4 : 1);
  Result.Points = &BezierPoints[SegmentIndex].P1;
 }
 
 return Result;
}

internal void
CalcBarycentricPolynomial(v2 *Controls, u32 PointCount, point_spacing PointSpacing, f32 *Ti,
                          u32 SampleCount, v2 *OutSamples,
                          f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  f32 *Omega = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (PointSpacing)
  {
   case PointSpacing_Equidistant: {
    // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
    BarycentricOmega(Omega, Ti, PointCount);
   } break;
   case PointSpacing_Chebychev: {
    BarycentricOmegaChebychev(Omega, PointCount);
   } break;
   
   case PointSpacing_Count: InvalidPath;
  }
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = BarycentricEvaluate(TT, Omega, Ti, SOA.Xs, PointCount);
    f32 Y = BarycentricEvaluate(TT, Omega, Ti, SOA.Ys, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcNewtonPolynomial(v2 *Controls, u32 PointCount, f32 *Ti,
                     u32 SampleCount, v2 *OutSamples,
                     f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Beta_X = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *Beta_Y = PushArrayNonZero(Temp.Arena, PointCount, f32);
  NewtonBetaFast(Beta_X, Ti, SOA.Xs, PointCount);
  NewtonBetaFast(Beta_Y, Ti, SOA.Ys, PointCount);
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = NewtonEvaluate(TT, Beta_X, Ti, PointCount);
    f32 Y = NewtonEvaluate(TT, Beta_Y, Ti, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcPolynomial(v2 *Controls, u32 PointCount,
               polynomial_interpolation_params Polynomial,
               u32 SampleCount, v2 *OutSamples,
               u32 SamplesPerControlPoint)
{
 temp_arena Temp = TempArena(0);
 
 point_spacing PointSpacing = Polynomial.PointSpacing;
 
 f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
 switch (PointSpacing)
 {
  case PointSpacing_Equidistant: { EquidistantPoints(Ti, PointCount, 0.0f, 1.0f); } break;
  case PointSpacing_Chebychev:   { ChebyshevPoints(Ti, PointCount);   } break;
  case PointSpacing_Count: InvalidPath;
 }
 
 f32 Min_T = 0;
 f32 Max_T = 0;
 if (PointCount > 0)
 {
  Min_T = Ti[0];
  Max_T = Ti[PointCount - 1];
 }
 
 switch (Polynomial.Type)
 {
  case PolynomialInterpolation_Barycentric: {CalcBarycentricPolynomial(Controls, PointCount, PointSpacing, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
  case PolynomialInterpolation_Newton:      {CalcNewtonPolynomial(Controls, PointCount, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
  case PolynomialInterpolation_Count: InvalidPath;
 }
 
 EndTemp(Temp);
}

internal void
CalcCubicSpline(v2 *Controls, u32 PointCount,
                cubic_spline_type Spline,
                u32 SampleCount, v2 *OutSamples)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  if (Spline == CubicSpline_Periodic)
  {
   v2 *OriginalControls = Controls;
   
   Controls = PushArrayNonZero(Temp.Arena, PointCount + 1, v2);
   ArrayCopy(Controls, OriginalControls, PointCount);
   Controls[PointCount] = OriginalControls[0];
   
   ++PointCount;
  }
  
  f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
  EquidistantPoints(Ti, PointCount, 0.0f, 1.0f);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Mx = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *My = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (Spline)
  {
   case CubicSpline_Natural: {
    CubicSplineNaturalM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplineNaturalM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Periodic: {
    CubicSplinePeriodicM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplinePeriodicM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Count: InvalidPath;
  }
  
  f32 Begin = Ti[0];
  f32 End = Ti[PointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (SampleCount - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < SampleCount;
       ++OutputIndex)
  {
   
   f32 X = CubicSplineEvaluate(T, Mx, Ti, SOA.Xs, PointCount);
   f32 Y = CubicSplineEvaluate(T, My, Ti, SOA.Ys, PointCount);
   OutSamples[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcRegularBezier(v2 *Controls, f32 *Weights, u32 PointCount,
                  u32 SampleCount, v2 *OutSamples)
{
 f32 T = 0.0f;
 f32 Delta_T = 1.0f / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  OutSamples[SampleIndex] = BezierCurveEvaluateWeighted(T, Controls, Weights, PointCount);
  T += Delta_T;
 }
}

internal void
CalcCubicBezier(cubic_bezier_point *Beziers, u32 PointCount,
                u32 SampleCount, v2 *OutSamples)
{
 if (PointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta_T = 1.0f / (SampleCount - 1);
  
  for (u32 SampleIndex = 0;
       SampleIndex < SampleCount;
       ++SampleIndex)
  {
   f32 Expanded_T = (PointCount - 1) * T;
   u32 SegmentIndex = Cast(u32)Expanded_T;
   f32 Segment_T = Expanded_T - SegmentIndex;
   
   Assert(SegmentIndex < PointCount);
   Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
   
   cubic_bezier_curve_segment Segment = GetCubicBezierCurveSegment(Beziers, PointCount, SegmentIndex);
   OutSamples[SampleIndex] = BezierCurveEvaluate(Segment_T, Segment.Points, Segment.PointCount);
   
   T += Delta_T;
  }
 }
}

inline internal u32
PartitionKnotIndexToKnotIndex(u32 PartitionKnotIndex, b_spline_knot_params KnotParams)
{
 Assert(PartitionKnotIndex < KnotParams.PartitionSize);
 u32 Result = PartitionKnotIndex + KnotParams.Degree;
 return Result;
}

internal void
CalcBSpline(v2 *Controls,
            b_spline_knot_params KnotParams, f32 *Knots,
            u32 SampleCount, v2 *OutSamples)
{
 // TODO(hbr): This slightly breakes moving BSpline along the curve.
 // When we calculate how much to move point forward or backwards, we
 // convert from [0,1] fraction into [SampleIndex], assuming that
 // sample points are sampled uniformly. This isn't the case with the code below.
#if 0
 v2 *Out = OutSamples;
 u32 SamplesPerPartitionKnot = (SampleCount - 1) / (KnotParams.PartitionSize - 1);
 u32 SamplesLeft = SampleCount - 1 - SamplesPerPartitionKnot * (KnotParams.PartitionSize - 1);
 
 for (u32 PartitionKnotIndex = 1;
      PartitionKnotIndex < KnotParams.PartitionSize;
      ++PartitionKnotIndex)
 {
  u32 PrevKnot = PartitionKnotIndexToKnotIndex(PartitionKnotIndex - 1, KnotParams);
  u32 NextKnot = PartitionKnotIndexToKnotIndex(PartitionKnotIndex - 0, KnotParams);
  
  f32 PrevT = Knots[PrevKnot];
  f32 NextT = Knots[NextKnot];
  
  u32 CurrentSampleCount = SamplesPerPartitionKnot;
  if (PartitionKnotIndex == 1) CurrentSampleCount += SamplesLeft;
  
  f32 T = PrevT;
  Assert(NextT >= PrevT);
  f32 DeltaT = (NextT - PrevT) / CurrentSampleCount;
  
  ForEachIndex(Index, CurrentSampleCount)
  {
   *Out++ = BSplineEvaluate(T, Controls, KnotParams, Knots);
   T += DeltaT;
  }
 }
 *Out++ = BSplineEvaluate(KnotParams.B, Controls, KnotParams, Knots);
 Assert(Cast(u64)(Out - OutSamples) == SampleCount);
 
#else
 
 f32 T = KnotParams.A;
 f32 Delta_T = (KnotParams.B - KnotParams.A) / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  OutSamples[SampleIndex] = BSplineEvaluate(T, Controls, KnotParams, Knots);
  T += Delta_T;
 }
 
#endif
}

internal void
CalcParametric(parametric_curve_params Parametric,
               u32 SampleCount, v2 *OutSamples)
{
 f32 MinT = Parametric.MinT;
 f32 MaxT = Max(Parametric.MinT, Parametric.MaxT);
 Assert(MinT <= MaxT);
 
 parametric_equation_expr *X_Equation = Parametric.X_Equation;
 parametric_equation_expr *Y_Equation = Parametric.Y_Equation;
 
 f32 T = MinT;
 f32 DeltaT = (MaxT - MinT) / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex	< SampleCount;
      ++SampleIndex)
 {
  f32 X = ParametricEquationEvalWithT(X_Equation, T);
  f32 Y = ParametricEquationEvalWithT(Y_Equation, T);
  
  v2 Point = V2(X, Y);
  OutSamples[SampleIndex] = Point;
  
  T += DeltaT;
 }
}

internal void
CalcCurve(curve *Curve, u32 SampleCount, v2 *OutSamples)
{
 u32 PointCount = Curve->Points.ControlPointCount;
 v2 *Controls = Curve->Points.ControlPoints;
 f32 *Weights = Curve->Points.ControlPointWeights;
 cubic_bezier_point *Beziers = Curve->Points.CubicBezierPoints;
 curve_params *Params = &Curve->Params;
 f32 *BSplineKnots = Curve->BSplineKnots;
 u32 SamplesPerControlPoint = Params->SamplesPerControlPoint;
 
 switch (Params->Type)
 {
  case Curve_Polynomial: {CalcPolynomial(Controls, PointCount, Params->Polynomial, SampleCount, OutSamples, SamplesPerControlPoint);} break;
  case Curve_CubicSpline: {CalcCubicSpline(Controls, PointCount, Params->CubicSpline, SampleCount, OutSamples);} break;
  
  case Curve_Bezier: {
   switch (Params->Bezier)
   {
    case Bezier_Regular: {CalcRegularBezier(Controls, Weights, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Cubic: {CalcCubicBezier(Beziers, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Count: InvalidPath;
   }
  } break;
  
  case Curve_BSpline: {
   MaybeRecomputeCurveBSplineKnots(Curve);
   b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
   u32 PartitionSize = KnotParams.PartitionSize;
   u32 Degree = KnotParams.Degree;
   v2 *PartitionKnots = Curve->BSplinePartitionKnots;
   for (u32 PartitionKnotIndex = 0;
        PartitionKnotIndex < PartitionSize;
        ++PartitionKnotIndex)
   {
    f32 T = BSplineKnots[Degree + PartitionKnotIndex];
    v2 KnotPoint = BSplineEvaluate(T, Controls, KnotParams, BSplineKnots);
    PartitionKnots[PartitionKnotIndex] = KnotPoint;
   }
   CalcBSpline(Controls, KnotParams, BSplineKnots, SampleCount, OutSamples);
  }break;
  case Curve_Parametric: {CalcParametric(Params->Parametric, SampleCount, OutSamples);}break;
  
  case Curve_Count: InvalidPath;
 }
}

// TODO(hbr): This function needs some serious refactoring.
// First of all, don't break down all the "Compute" functions into hierarchical structure.
// It's very fragile. It's just better (I think) to duplicate some code but have it more independent.
// Second of all just refactor stupid code duplication here.
// Basically inline everything and work your way up from there.
// Then parallelize this heavily. I mean, we can unroll, we can simd, we can multithread this.
internal void
RecomputeCurve(curve *Curve)
{
 ProfileFunctionBegin();
 
 curve_params *Params = &Curve->Params;
 arena *ComputeArena = Curve->ComputeArena;
 u32 ControlCount = Curve->Points.ControlPointCount;
 v2 *Controls = Curve->Points.ControlPoints;
 f32 *Weights = Curve->Points.ControlPointWeights;
 f32 LineWidth = Params->DrawParams.Line.Width;
 
 ClearArena(ComputeArena);
 
 u32 SampleCount = 0;
 if (IsCurveTotalSamplesMode(Curve))
 {
  SampleCount = Curve->Params.TotalSamples;
 }
 else
 {
  u32 ControlCountWithLooped = (IsCurveLooped(Curve) ? ControlCount + 1 : ControlCount);
  if (ControlCountWithLooped > 0)
  {
   SampleCount = (ControlCountWithLooped - 1) * Params->SamplesPerControlPoint + 1;
  }
 }
 v2 *Samples = PushArrayNonZero(ComputeArena, SampleCount, v2);
 CalcCurve(Curve, SampleCount, Samples);
 
 vertex_array CurveVertices = ComputeVerticesOfThickLine(ComputeArena, SampleCount, Samples, LineWidth, IsCurveLooped(Curve));
 vertex_array PolylineVertices = ComputeVerticesOfThickLine(ComputeArena, ControlCount, Controls, Params->DrawParams.Polyline.Width, false);
 
 v2 *ConvexHullPoints = PushArrayNonZero(ComputeArena, ControlCount, v2);
 u32 ConvexHullCount = CalcConvexHull(ControlCount, Controls, ConvexHullPoints);
 vertex_array ConvexHullVertices = ComputeVerticesOfThickLine(ComputeArena, ConvexHullCount, ConvexHullPoints, Params->DrawParams.ConvexHull.Width, true);
 
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 if (IsCurveEligibleForPointTracking(Curve))
 {
  if (Tracking->Active)
  {
   f32 Fraction = Tracking->Fraction;
   
   v2 LocalSpaceTrackedPoint = BezierCurveEvaluateWeighted(Fraction, Controls, Weights, ControlCount);
   Tracking->LocalSpaceTrackedPoint = LocalSpaceTrackedPoint;
   
   if (Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization)
   {
    all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(ComputeArena, Fraction, Controls, Weights, ControlCount);
    vertex_array *LineVerticesPerIteration = PushArray(ComputeArena, Intermediate.IterationCount, vertex_array);
    
    u32 IterationPointsOffset = 0;
    for (u32 Iteration = 0;
         Iteration < Intermediate.IterationCount;
         ++Iteration)
    {
     u32 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
     LineVerticesPerIteration[Iteration] = ComputeVerticesOfThickLine(ComputeArena,
                                                                      CurrentIterationPointCount,
                                                                      Intermediate.P + IterationPointsOffset,
                                                                      LineWidth,
                                                                      false);
     IterationPointsOffset += CurrentIterationPointCount;
    }
    
    Tracking->Intermediate = Intermediate;
    Tracking->LineVerticesPerIteration = LineVerticesPerIteration;
    Tracking->LocalSpaceTrackedPoint = Intermediate.P[Intermediate.TotalPointCount - 1];
   }
  }
 }
 else
 {
  Tracking->Active = false;
 }
 
 u32 BSplineConvexHullCount = 0;
 b_spline_convex_hull *BSplineConvexHulls = 0;
 if (IsBSplineCurve(Curve))
 {
  b_spline_knot_params BSpline = GetBSplineParams(Curve).KnotParams;
  u32 Degree = BSpline.Degree;
  Assert(Degree <= ControlCount);
  u32 ConvexHullCount = ControlCount - Degree;
  b_spline_convex_hull *Hulls = PushArray(ComputeArena, ConvexHullCount, b_spline_convex_hull);
  ForEachIndex(ConvexHullIndex, ConvexHullCount)
  {
   u32 PointCount = Degree + 1;
   v2 *Points = PushArray(ComputeArena, PointCount, v2);
   PointCount = CalcConvexHull(PointCount, Controls + ConvexHullIndex, Points);
   vertex_array Vertices = ComputeVerticesOfThickLine(ComputeArena, PointCount, Points, Params->DrawParams.BSplinePartialConvexHull.Width, true);
   b_spline_convex_hull *Hull = Hulls + ConvexHullIndex;
   Hull->Points = Points;
   Hull->Vertices = Vertices;
  }
  BSplineConvexHullCount = ConvexHullCount;
  BSplineConvexHulls = Hulls;
 }
 
 Curve->CurveSampleCount = SampleCount;
 Curve->CurveSamples = Samples;
 Curve->CurveVertices = CurveVertices;
 Curve->PolylineVertices = PolylineVertices;
 Curve->ConvexHullPoints = ConvexHullPoints;
 Curve->ConvexHullCount = ConvexHullCount;
 Curve->ConvexHullVertices = ConvexHullVertices;
 Curve->BSplineConvexHullCount = BSplineConvexHullCount;
 Curve->BSplineConvexHulls = BSplineConvexHulls;
 
 ProfileEnd();
}

internal void
MarkEntityModified(entity_with_modify_witness *Witness)
{
 Witness->Modified = true;
}

internal entity_with_modify_witness
BeginEntityModify(entity *Entity)
{
 entity_with_modify_witness Result = {};
 Result.Entity = Entity;
 return Result;
}

internal void
EndEntityModify(entity_with_modify_witness Witness)
{
 entity *Entity = Witness.Entity;
 if (Entity && Witness.Modified)
 {
  switch (Entity->Type)
  {
   case Entity_Curve: {RecomputeCurve(&Entity->Curve);}break;
   case Entity_Image: {}break;
   case Entity_Count: InvalidPath;
  }
  
  ++Entity->Version;
 }
}

internal b32
CanAddAdditionalVar(parametric_curve_resources *Resources)
{
 b32 Result = (Resources->FirstFreeAdditionalVar != 0 ||
               Resources->AllocatedAdditionalVarCount < MAX_ADDITIONAL_VAR_COUNT);
 return Result;
}

internal parametric_curve_var *
AddNewAdditionalVar(parametric_curve_resources *Resources)
{
 parametric_curve_var *Var = Resources->FirstFreeAdditionalVar;
 
 if (Var)
 {
  StackPop(Resources->FirstFreeAdditionalVar);
 }
 else if (Resources->AllocatedAdditionalVarCount < MAX_ADDITIONAL_VAR_COUNT)
 {
  Var = Resources->AdditionalVars + Resources->AllocatedAdditionalVarCount;
  Var->Id = Resources->AllocatedAdditionalVarCount;
  ++Resources->AllocatedAdditionalVarCount;
 }
 
 if (Var)
 {
  u32 Id = Var->Id;
  StructZero(Var);
  Var->Id = Id;
  
  DLLPushBack(Resources->AdditionalVarsHead,
              Resources->AdditionalVarsTail,
              Var);
 }
 
 return Var;
}

internal void
RemoveAdditionalVar(parametric_curve_resources *Resources, parametric_curve_var *Var)
{
 DLLRemove(Resources->AdditionalVarsHead,
           Resources->AdditionalVarsTail,
           Var);
 StackPush(Resources->FirstFreeAdditionalVar, Var);
}

internal curve_params
DefaultCurveParams(void)
{
 curve_params Params = {};
 
 f32 LineWidth = 0.009f;
 rgba PolylineColor = RGBA_U8(16, 31, 31, 200);
 
 Params.DrawParams.Line.Enabled = true;
 Params.DrawParams.Line.Color = RGBA_U8(21, 69, 98);
 Params.DrawParams.Line.Width = LineWidth;
 Params.DrawParams.Points.Enabled = true;
 Params.DrawParams.Points.Color = RGBA_U8(0, 138, 138, 148);
 Params.DrawParams.Points.Radius = 0.014f;
 Params.DrawParams.Polyline.Color = PolylineColor;
 Params.DrawParams.Polyline.Width = LineWidth;
 Params.DrawParams.ConvexHull.Color = PolylineColor;
 Params.DrawParams.ConvexHull.Width = LineWidth;
 Params.DrawParams.BSplineKnots.Radius = 0.010f;
 Params.DrawParams.BSplineKnots.Color = RGBA_U8(138, 0, 0, 148);
 Params.DrawParams.BSplinePartialConvexHull.Color = PolylineColor;
 Params.DrawParams.BSplinePartialConvexHull.Width = LineWidth;
 
 Params.SamplesPerControlPoint = 50;
 Params.TotalSamples = 1000;
 Params.Parametric.MaxT = 1.0f;
 Params.Parametric.X_Equation = NilExpr;
 Params.Parametric.Y_Equation = NilExpr;
 
 return Params;
}

internal curve_params
DefaultCubicSplineCurveParams(void)
{
 curve_params Params = DefaultCurveParams();
 Params.Type = Curve_CubicSpline;
 Params.CubicSpline = CubicSpline_Natural;
 return Params;
}

internal string_store *
AllocStringStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 string_store *Store = PushStruct(Arena, string_store);
 Store->StrCache = AllocStringCache(Arena);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal void
DeallocStringStore(string_store *Store)
{
 DeallocArenaFromStore(Store->ArenaStore, Store->Arena);
}

internal string_id
AllocStringFromString(string_store *Store, string Str)
{
 string_id Id = StringIdFromIndex(Store->StrCount);
 SetOrAllocStringOfId(Store, Str, Id);
 return Id;
}

internal void
DeallocString(string_store *Store, string_id Id)
{
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 DeallocString(Store->StrCache, *Buffer);
 StructZero(Buffer);
}

internal void
SetOrAllocStringOfId(string_store *Store, string Str, string_id Id)
{
 u32 Index = IndexFromStringId(Id);
 if (Index >= Store->StrCount)
 {
  string_id AllocatedId = AllocStringOfSize(Store, Str.Count);
  Assert(StringIdMatch(Id, AllocatedId));
 }
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 FillCharBuffer(Buffer, Str);
}

internal string_id
AllocStringOfSize(string_store *Store, u64 Size)
{
 char_buffer Buffer = AllocString(Store->StrCache, Size);
 if (Store->StrCount >= Store->StrCapacity)
 {
  u32 NewCapacity = Max(2 * Store->StrCapacity, Store->StrCount + 1);
  char_buffer *NewStrs = PushArrayNonZero(Store->Arena, NewCapacity, char_buffer);
  ArrayCopy(NewStrs, Store->Strs, Store->StrCount);
  Store->Strs = NewStrs;
  Store->StrCapacity = NewCapacity;
 }
 Assert(Store->StrCount < Store->StrCapacity);
 Store->Strs[Store->StrCount] = Buffer;
 u32 Index = Store->StrCount++;
 string_id Id = {Index};
 return Id;
}

internal char_buffer *
CharBufferFromStringId(string_store *Store, string_id Id)
{
 Assert(Id.Index < Store->StrCount);
 char_buffer *Buffer = Store->Strs + Id.Index;
 return Buffer;
}

internal string
StringFromStringId(string_store *Store, string_id Id)
{
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 string Str = StrFromCharBuffer(*Buffer);
 return Str;
}

internal string_id
StringIdFromIndex(u32 Index)
{
 string_id Id = {};
 Id.Index = Index;
 return Id;
}

internal u32
IndexFromStringId(string_id Id)
{
 u32 Index = Id.Index;
 return Index;
}

internal b32
StringIdMatch(string_id A, string_id B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}