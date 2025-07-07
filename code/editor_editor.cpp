internal curve_params
DefaultCurveParams(void)
{
 curve_params Params = {};
 
 f32 LineWidth = 0.009f;
 v4 PolylineColor = RGBA_Color(16, 31, 31, 200);
 
 Params.Line.Enabled = true;
 Params.Line.Color = RGBA_Color(21, 69, 98);
 Params.Line.Width = LineWidth;
 Params.Points.Enabled = true;
 Params.Points.Color = RGBA_Color(0, 138, 138, 148);
 Params.Points.Radius = 0.014f;
 Params.Polyline.Color = PolylineColor;
 Params.Polyline.Width = LineWidth;
 Params.ConvexHull.Color = PolylineColor;
 Params.ConvexHull.Width = LineWidth;
 Params.SamplesPerControlPoint = 50;
 Params.TotalSamples = 1000;
 Params.Parametric.MaxT = 1.0f;
 Params.Parametric.X_Equation = NilExpr;
 Params.Parametric.Y_Equation = NilExpr;
 Params.BSpline.Knots.Radius = 0.010f;
 Params.BSpline.Knots.Color = RGBA_Color(138, 0, 0, 148);
 Params.BSpline.PartialConvexHull.Color = PolylineColor;
 Params.BSpline.PartialConvexHull.Width = LineWidth;
 
 return Params;
}

internal void
InitLeftClickState(editor_left_click_state *LeftClick)
{
 LeftClick->OriginalVerticesArena = AllocArena(Megabytes(128));
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
InitAnimatingCurvesState(animating_curves_state *State)
{
 State->Arena = AllocArena(Megabytes(32));
}

internal void
InitEditor(editor *Editor, editor_memory *Memory)
{
 Editor->BackgroundColor = Editor->DefaultBackgroundColor = RGBA_Color(21, 21, 21);
 Editor->CollisionToleranceClip = 0.04f;
 Editor->RotationRadiusClip = 0.1f;
 Editor->CurveDefaultParams = DefaultCurveParams();
 Editor->EntityListWindow = true;
 Editor->SelectedEntityWindow = true;
#if BUILD_DEBUG
 Editor->DiagnosticsWindow = true;
 Editor->ProfilerWindow = true;
#endif
 Editor->LowPriorityQueue = Memory->LowPriorityQueue;
 Editor->HighPriorityQueue = Memory->HighPriorityQueue;
 Editor->RendererQueue = Memory->RendererQueue;
 Editor->Arena = AllocArena(Megabytes(32));
 
 InitStringCache(&Editor->StrCache);
 InitEntityStore(&Editor->EntityStore, Memory->MaxTextureCount, Memory->MaxBufferCount, &Editor->StrCache);
 InitCamera(&Editor->Camera);
 InitFrameStats(&Editor->FrameStats);
 InitLeftClickState(&Editor->LeftClick);
 InitMergingCurvesState(&Editor->MergingCurves, &Editor->EntityStore);
 InitVisualProfiler(&Editor->Profiler, Memory->Profiler);
 InitAnimatingCurvesState(&Editor->AnimatingCurves);
 InitThreadTaskMemoryStore(&Editor->ThreadTaskMemoryStore);
 InitImageLoadingStore(&Editor->ImageLoadingStore);
 
 {
  entity_store *EntityStore = &Editor->EntityStore;
  entity *Entity = AllocEntity(EntityStore, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_Bezier;
  InitEntityAsCurve(Entity, StrLit("de-casteljau"), Params);
  
  AppendControlPoint(&Witness, V2(-0.5f, -0.5f));
  AppendControlPoint(&Witness, V2(+0.5f, -0.5f));
  AppendControlPoint(&Witness, V2(+0.5f, +0.5f));
  AppendControlPoint(&Witness, V2(-0.5f, +0.5f));
  
  Entity->Curve.PointTracking.Active = true;
  SetTrackingPointFraction(&Witness, 0.5f);
  
  EndEntityModify(Witness);
 }
 
 {
  entity_store *EntityStore = &Editor->EntityStore;
  entity *Entity = AllocEntity(EntityStore, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_BSpline;
  InitEntityAsCurve(Entity, StrLit("b-spline"), Params);
  
  u32 PointCount = 30;
  curve_points_static_modify_handle Handle = BeginModifyCurvePoints(&Witness, PointCount, ModifyCurvePointsWhichPoints_ControlPointsOnly);
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

internal void
AddNotificationF(editor *Editor, notification_type Type, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 
 if (Editor->NotificationCount == MAX_NOTIFICATION_COUNT)
 {
  ArrayMove(Editor->Notifications, Editor->Notifications + 1, Editor->NotificationCount - 1);
  --Editor->NotificationCount;
 }
 Assert(Editor->NotificationCount < MAX_NOTIFICATION_COUNT);
 
 notification *Notification = Editor->Notifications + Editor->NotificationCount;
 ++Editor->NotificationCount;
 
 Notification->Type = Type;
 u64 ContentCount = FmtV(Notification->ContentBuffer, ArrayCount(Notification->ContentBuffer), Format, Args);
 Notification->Content = MakeStr(Notification->ContentBuffer, ContentCount);
 Notification->LifeTime = 0.0f;
 Notification->ScreenPosY = 0.0f;
 
 va_end(Args);
}

internal void
InitEntityFromEntity(entity_store *EntityStore, entity_with_modify_witness *DstWitness, entity *Src)
{
 entity *Dst = DstWitness->Entity;
 curve *DstCurve = &Dst->Curve;
 curve *SrcCurve = &Src->Curve;
 image *DstImage = &Dst->Image;
 image *SrcImage = &Src->Image;
 
 InitEntityPart(Dst,
                Src->Type,
                Src->XForm,
                GetEntityName(Src),
                Src->SortingLayer,
                Src->Flags);
 
 switch (Src->Type)
 {
  case Entity_Curve: {
   DstCurve->Params = SrcCurve->Params;
   DstCurve->BSplineKnotParams = SrcCurve->BSplineKnotParams;
   SetCurvePoints(DstWitness, CurvePointsHandleFromCurvePointsStatic(&SrcCurve->Points));
   SelectControlPoint(DstCurve, SrcCurve->SelectedControlPoint);
  }break;
  
  case Entity_Image: {
   DstImage->Dim = SrcImage->Dim;
   DeallocTextureHandle(EntityStore, DstImage->TextureHandle);
   DstImage->TextureHandle = CopyTextureHandle(EntityStore, SrcImage->TextureHandle);
  }break;
  
  case Entity_Count: InvalidPath;
 }
}

internal void
DuplicateEntity(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 entity_store *EntityStore = &Editor->EntityStore;
 entity *Copy = AddEntity(Editor);
 entity_with_modify_witness CopyWitness = BeginEntityModify(Copy);
 string CopyName = StrF(Temp.Arena, "%S(copy)", GetEntityName(Entity));
 InitEntityFromEntity(EntityStore, &CopyWitness, Entity);
 SelectEntity(Editor, Copy);
 
 f32 SlightTranslationX = 5 * Copy->Curve.Params.Line.Width;
 v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
 Copy->XForm.P += SlightTranslation;
 
 EndEntityModify(CopyWitness);
 EndTemp(Temp);
}

internal void
SplitCurveOnControlPoint(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 entity_store *EntityStore = &Editor->EntityStore;
 
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
  
  InitEntityFromEntity(EntityStore, &HeadWitness, Entity);
  InitEntityFromEntity(EntityStore, &TailWitness, Entity);
  
  curve *HeadCurve = SafeGetCurve(HeadEntity);
  curve *TailCurve = SafeGetCurve(TailEntity);
  
  string HeadName = StrF(Temp.Arena, "%S(head)", GetEntityName(Entity));
  string TailName = StrF(Temp.Arena, "%S(tail)", GetEntityName(Entity));
  SetEntityName(HeadEntity, HeadName);
  SetEntityName(TailEntity, TailName);
  
  curve_points_static_modify_handle HeadPoints = BeginModifyCurvePoints(&HeadWitness, HeadPointCount, ModifyCurvePointsWhichPoints_AllPoints);
  curve_points_static_modify_handle TailPoints = BeginModifyCurvePoints(&TailWitness, TailPointCount, ModifyCurvePointsWhichPoints_AllPoints);
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
 curve_points_static_modify_handle ModifyPoints = BeginModify.ModifyPoints;
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
 entity_store *EntityStore = &Editor->EntityStore;
 u32 ControlPointCount = Curve->Points.ControlPointCount;
 
 Assert(Tracking->Active);
 Tracking->Active = false;
 
 entity *LeftEntity = AddEntity(Editor);
 entity *RightEntity = AddEntity(Editor);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 InitEntityFromEntity(EntityStore, &LeftWitness, Entity);
 InitEntityFromEntity(EntityStore, &RightWitness, Entity);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", GetEntityName(Entity));
 string RightName = StrF(Temp.Arena, "%S(right)", GetEntityName(Entity));
 SetEntityName(LeftEntity, LeftName);
 SetEntityName(RightEntity, RightName);
 
 curve_points_static_modify_handle LeftPoints = BeginModifyCurvePoints(&LeftWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 curve_points_static_modify_handle RightPoints = BeginModifyCurvePoints(&RightWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
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
  entity_store *EntityStore = &Editor->EntityStore;
  
  entity *Entity = AddEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  InitEntityFromEntity(EntityStore, &EntityWitness, Merging->MergeEntity);
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
 arena *Arena = Animation->Arena;
 StructZero(Animation);
 Animation->Flags = AnimatingCurves_Active;
 BeginChoosing2Curves(&Animation->Choose2Curves);
 Animation->Bouncing = MakeBouncingParam();
 Animation->Arena = Arena;
}

internal void
EndAnimatingCurves(animating_curves_state *Animation)
{
 Animation->Flags &= ~AnimatingCurves_Active;
 ClearArena(Animation->Arena);
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
  curve_points_static_modify_handle ModifyPoints = Modify.ModifyPoints;
  tracked_action *ModifyAction = Modify.ModifyAction;
  
  curve_points_static *OriginalPoints = AllocCurvePoints(Editor);
  CopyCurvePointsFromCurve(Curve, CurvePointsDynamicFromStatic(OriginalPoints));
  
  bezier_lower_degree LowerDegree = BezierCurveLowerDegree(ModifyPoints.ControlPoints, ModifyPoints.Weights, PointCount);
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
     DeallocEntity(&Editor->EntityStore, Entity);
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
TryLoadImages(editor *Editor, u32 FileCount, string *FilePaths, v2 AtP)
{
 work_queue *WorkQueue = Editor->LowPriorityQueue;
 entity_store *EntityStore = &Editor->EntityStore;
 thread_task_memory_store *ThreadTaskMemoryStore = &Editor->ThreadTaskMemoryStore;
 image_loading_store *ImageLoadingStore = &Editor->ImageLoadingStore;
 
 ForEachIndex(FileIndex, FileCount)
 {
  string FilePath = FilePaths[FileIndex];
  
  image_info ImageInfo = LoadImageInfo(FilePath);
  render_texture_handle TextureHandle = AllocTextureHandle(EntityStore);
  renderer_transfer_op *TextureOp = PushTextureTransfer(Editor->RendererQueue, ImageInfo.Width, ImageInfo.Height, ImageInfo.SizeInBytesUponLoad, TextureHandle);
  thread_task_memory *TaskMemory = BeginThreadTaskMemory(ThreadTaskMemoryStore);
  
  image_loading_task *ImageLoading = BeginAsyncImageLoadingTask(ImageLoadingStore);
  ImageLoading->ImageP = AtP;
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
}

internal void
ExecuteEditorCommand(editor *Editor, platform_input_output *Input, editor_command Cmd)
{
 switch (Cmd)
 {
  case EditorCommand_New: {NotImplemented;}break;
  case EditorCommand_Save: {NotImplemented;}break;
  case EditorCommand_SaveAs: {NotImplemented;}break;
  case EditorCommand_Quit: {Input->QuitRequested = true;}break;
  case EditorCommand_ToggleDevConsole: {Editor->DevConsole = !Editor->DevConsole;}break;
  case EditorCommand_ToggleProfiler: {Editor->Profiler.Stopped = !Editor->Profiler.Stopped;}break;
  case EditorCommand_Undo: {Undo(Editor);}break;
  case EditorCommand_Redo: {Redo(Editor);}break;
  case EditorCommand_ToggleUI: {Editor->HideUI = !Editor->HideUI;}break;
  
  case EditorCommand_Open: {
   temp_arena Temp = TempArena(0);
   
   platform_file_dialog_filter PNG = {};
   PNG.DisplayName = StrLit("PNG");
   string PNG_Extensions[] = { StrLit("png") };
   PNG.ExtensionCount = ArrayCount(PNG_Extensions);
   PNG.Extensions = PNG_Extensions;
   
   platform_file_dialog_filter JPEG = {};
   JPEG.DisplayName = StrLit("JPEG");
   string JPEG_Extensions[] = { StrLit("jpg"), StrLit("jpeg"), StrLit("jpe") };
   JPEG.ExtensionCount = ArrayCount(JPEG_Extensions);
   JPEG.Extensions = JPEG_Extensions;
   
   platform_file_dialog_filter BMP = {};
   BMP.DisplayName = StrLit("Windows BMP File");
   string BMP_Extensions[] = { StrLit("bmp") };
   BMP.ExtensionCount = ArrayCount(BMP_Extensions);
   BMP.Extensions = BMP_Extensions;
   
   platform_file_dialog_filter AllFilters[] = { PNG, JPEG, BMP };
   
   platform_file_dialog_filters Filters = {};
   Filters.AnyFileFilter = true;
   Filters.FilterCount = ArrayCount(AllFilters);
   Filters.Filters = AllFilters;
   
   platform_file_dialog_result OpenDialog = Platform.OpenFileDialog(Temp.Arena, Filters);
   
   camera *Camera = &Editor->Camera;
   TryLoadImages(Editor, OpenDialog.FileCount, OpenDialog.FilePaths, Camera->P);
   
   EndTemp(Temp);
  }break;
  
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
EndEditorFrame(editor *Editor, platform_input_output *Input)
{
 //- end pending action tracking group is all tracked actions are completed
 Assert(Editor->IsPendingActionTrackingGroup);
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
 //- execute all editor commands gathered throughout the frame (mostly from input)
 ListIter(CmdNode, Editor->EditorCommandsHead, editor_command_node)
 {
  ExecuteEditorCommand(Editor, Input, CmdNode->Command);
  StackPush(Editor->FreeEditorCommandNode, CmdNode);
 }
 Editor->EditorCommandsHead = 0;
 Editor->EditorCommandsTail = 0;
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
 curve_points_static_modify_handle Handle = BeginModifyCurvePoints(Entity, RequestedPointCount, Which);
 begin_modify_curve_points_static_tracked_result Result = {};
 Result.ModifyPoints = Handle;
 Result.ModifyAction = Action;
 return Result;
}

internal void
EndModifyCurvePointsTracked(editor *Editor,
                            tracked_action *ModifyAction,
                            curve_points_static_modify_handle ModifyPoints)
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
 curve_points_static_modify_handle ModifyPoints = Modify.ModifyPoints;
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
 DeactiveEntity(&Editor->EntityStore, Entity);
}

internal entity *
AddEntity(editor *Editor)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_AddEntity;
 entity *Entity = AllocEntity(&Editor->EntityStore, false);
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
      ActivateEntity(&Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      DeactiveEntity(&Editor->EntityStore, Entity);
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
      DeactiveEntity(&Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      ActivateEntity(&Editor->EntityStore, Entity);
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
