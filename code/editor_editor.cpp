internal curve_params
DefaultCurveParams(void)
{
 curve_params Params = {};
 
 f32 LineWidth = 0.009f;
 v4 PolylineColor = RGBA_Color(16, 31, 31, 200);
 
 Params.LineColor = RGBA_Color(21, 69, 98);
 Params.LineWidth = LineWidth;
 Params.PointColor = RGBA_Color(0, 138, 138, 148);
 Params.PointRadius = 0.014f;
 Params.PolylineColor = PolylineColor;
 Params.PolylineWidth = LineWidth;
 Params.ConvexHullColor = PolylineColor;
 Params.ConvexHullWidth = LineWidth;
 Params.SamplesPerControlPoint = 50;
 Params.TotalSamples = 1000;
 Params.Parametric.MaxT = 1.0f;
 Params.Parametric.X_Equation = NilExpr;
 Params.Parametric.Y_Equation = NilExpr;
 Params.B_Spline.KnotPointRadius = 0.010f;
 Params.B_Spline.KnotPointColor = RGBA_Color(138, 0, 0, 148);
 
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
 State->MergeEntity = AllocEntity(EntityStore, Entity_Curve, true);
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
 NilExpr = &Editor->NilParametricExpr;
 
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
  entity *Entity = AllocEntity(EntityStore, Entity_Curve, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_Bezier;
  InitCurveEntity2(Entity, StrLit("de-casteljau"), Params);
  
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
  entity *Entity = AllocEntity(EntityStore, Entity_Curve, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_B_Spline;
  InitCurveEntity2(Entity, StrLit("b-spline"), Params);
  
  u32 PointCount = 30;
  curve_points_modify_handle Handle = BeginModifyCurvePoints(&Witness, PointCount, ModifyCurvePointsWhichPoints_JustControlPoints);
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
SelectEntity(editor *Editor, entity *Entity)
{
 entity *SelectedEntity = EntityFromHandle(Editor->SelectedEntityHandle);
 if (SelectedEntity)
 {
  SelectedEntity->Flags &= ~EntityFlag_Selected;
 }
 if (Entity)
 {
  Entity->Flags |= EntityFlag_Selected;
 }
 Editor->SelectedEntityHandle = MakeEntityHandle(Entity);
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
DuplicateEntity(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 entity_store *EntityStore = &Editor->EntityStore;
 entity *Copy = AllocEntity(EntityStore, Entity->Type, false);
 entity_with_modify_witness CopyWitness = BeginEntityModify(Copy);
 string CopyName = StrF(Temp.Arena, "%S(copy)", GetEntityName(Entity));
 
 InitEntityFromEntity(&CopyWitness, Entity);
 SetEntityName(Copy, CopyName);
 SelectEntity(Editor, Copy);
 
 // TODO(hbr): This is not right, translate depending on camera zoom
 f32 SlightTranslationX = 0.2f;
 v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
 Copy->P += SlightTranslation;
 
 EndEntityModify(CopyWitness);
 
 EndTemp(Temp);
}

// TODO(hbr): Optimize this function
internal void
FocusCameraOnEntity(editor *Editor, entity *Entity, f32 AspectRatio)
{
 rect2 AABB = EmptyAABB();
 
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   
   u32 PointCount = Curve->ControlPointCount;
   v2 *Points = Curve->ControlPoints;
   for (u32 PointIndex = 0;
        PointIndex < PointCount;
        ++PointIndex)
   {
    v2 P = LocalEntityPositionToWorld(Entity, Points[PointIndex]);
    // TODO(hbr): Maybe include also point size
    AddPointAABB(&AABB, P);
   }
  } break;
  
  case Entity_Image: {
   NotImplemented;
#if 0
   // TODO(hbr): Use LocalEntityPositionToWorld here instead
   f32 ScaleX = Entity->Scale.X;
   f32 ScaleY = Entity->Scale.Y;
   
   sf::Vector2u TextureSize = Entity->Image.Texture.getSize();
   f32 HalfWidth = 0.5f * GlobalImageScaleFactor * ScaleX * TextureSize.x;
   f32 HalfHeight = 0.5f * GlobalImageScaleFactor * ScaleY * TextureSize.y;
   
   AddPointAABB(&AABB, Entity->P + V2( HalfWidth,  HalfHeight));
   AddPointAABB(&AABB, Entity->P + V2(-HalfWidth,  HalfHeight));
   AddPointAABB(&AABB, Entity->P + V2( HalfWidth, -HalfHeight));
   AddPointAABB(&AABB, Entity->P + V2(-HalfWidth, -HalfHeight));
#endif
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
 
 SetCameraTarget(&Editor->Camera, AABB, AspectRatio);
}

internal void
SplitCurveOnControlPoint(editor *Editor, entity_with_modify_witness *EntityWitness)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 entity_store *EntityStore = &Editor->EntityStore;
 
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  u32 HeadPointCount = Curve->SelectedIndex.Index + 1;
  u32 TailPointCount = Curve->ControlPointCount - Curve->SelectedIndex.Index;
  
  entity *HeadEntity = Entity;
  entity *TailEntity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
  
  entity_with_modify_witness *HeadWitness = EntityWitness;
  entity_with_modify_witness TailWitness = BeginEntityModify(TailEntity);
  
  InitEntityFromEntity(&TailWitness, HeadEntity);
  
  curve *HeadCurve = SafeGetCurve(HeadEntity);
  curve *TailCurve = SafeGetCurve(TailEntity);
  
  string HeadName  = StrF(Temp.Arena, "%S (head)", GetEntityName(Entity));
  string TailName = StrF(Temp.Arena, "%S (tail)", GetEntityName(Entity));
  SetEntityName(HeadEntity, HeadName);
  SetEntityName(TailEntity, TailName);
  
  curve_points_modify_handle HeadPoints = BeginModifyCurvePoints(HeadWitness, HeadPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers);
  curve_points_modify_handle TailPoints = BeginModifyCurvePoints(&TailWitness, TailPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers);
  Assert(HeadPoints.PointCount == HeadPointCount);
  Assert(TailPoints.PointCount == TailPointCount);
  
  u32 SplitAt = Curve->SelectedIndex.Index;
  ArrayCopy(TailPoints.ControlPoints, Curve->ControlPoints + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailPoints.Weights, Curve->ControlPointWeights + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailCurve->CubicBezierPoints, Curve->CubicBezierPoints + SplitAt, TailPoints.PointCount);
  
  EndModifyCurvePoints(HeadPoints);
  EndModifyCurvePoints(TailPoints);
  
  EndEntityModify(TailWitness);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 temp_arena Temp = TempArena(0);
 
 entity_with_modify_witness Witness = BeginEntityModify(Entity);
 
 u32 ControlPointCount = Curve->ControlPointCount;
 v2 *ElevatedControlPoints = PushArrayNonZero(Temp.Arena,
                                              ControlPointCount + 1,
                                              v2);
 f32 *ElevatedControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                     ControlPointCount + 1,
                                                     f32);
 MemoryCopy(ElevatedControlPoints,
            Curve->ControlPoints,
            ControlPointCount * SizeOf(ElevatedControlPoints[0]));
 MemoryCopy(ElevatedControlPointWeights,
            Curve->ControlPointWeights,
            ControlPointCount * SizeOf(ElevatedControlPointWeights[0]));
 
 BezierCurveElevateDegreeWeighted(ElevatedControlPoints,
                                  ElevatedControlPointWeights,
                                  ControlPointCount);
 
 cubic_bezier_point *ElevatedCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                  (ControlPointCount + 1),
                                                                  cubic_bezier_point);
 BezierCubicCalculateAllControlPoints(ControlPointCount + 1,
                                      ElevatedControlPoints,
                                      ElevatedCubicBezierPoints);
 
 SetCurveControlPoints(&Witness,
                       ControlPointCount + 1,
                       ElevatedControlPoints,
                       ElevatedControlPointWeights,
                       ElevatedCubicBezierPoints);
 
 EndEntityModify(Witness);
 
 EndTemp(Temp);
}

internal vertex_array
CopyLineVertices(arena *Arena, vertex_array Vertices)
{
 vertex_array Result = Vertices;
 Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, v2);
 ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
 
 return Result;
}

// TODO(hbr): Instead of copying all the verices and control points and weights manually, maybe just
// wrap control points, weights and cubicbezier poiints in a structure and just assign data here.
// In other words - refactor this function
// TODO(hbr): Refactor this function big time!!!
// TODO(hbr): This function is kind of broken - for example, we can modify the curve while we are in the "lowering degree failed" mode. While
// this on its own can be useful, because one can fit the curve more tighly to the original one, when we add some control points due to modyfing this,
// and then go back to tweaking the Middle_T value to tweak the middle point when fixing "failed lowering", the point that we tweak might no longer be
// the point that was originally there. In other words, saved control point index got invalidated.
internal void
LowerBezierCurveDegree(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 
 u32 PointCount = Curve->ControlPointCount;
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  v2 *LowerPoints = PushArrayNonZero(Temp.Arena, PointCount, v2);
  f32 *LowerWeights = PushArrayNonZero(Temp.Arena, PointCount, f32);
  cubic_bezier_point *LowerBeziers = PushArrayNonZero(Temp.Arena, PointCount - 1, cubic_bezier_point);
  ArrayCopy(LowerPoints, Curve->ControlPoints, PointCount);
  ArrayCopy(LowerWeights, Curve->ControlPointWeights, PointCount);
  
  bezier_lower_degree LowerDegree = BezierCurveLowerDegree(LowerPoints, LowerWeights, PointCount);
  if (LowerDegree.Failure)
  {
   Lowering->Active = true;
   
   ClearArena(Lowering->Arena);
   
   Lowering->SavedControlPoints = PushArrayNonZero(Lowering->Arena, PointCount, v2);
   Lowering->SavedControlPointWeights = PushArrayNonZero(Lowering->Arena, PointCount, f32);
   Lowering->SavedCubicBezierPoints = PushArrayNonZero(Lowering->Arena, PointCount, cubic_bezier_point);
   ArrayCopy(Lowering->SavedControlPoints, Curve->ControlPoints, PointCount);
   ArrayCopy(Lowering->SavedControlPointWeights, Curve->ControlPointWeights, PointCount);
   ArrayCopy(Lowering->SavedCubicBezierPoints, Curve->CubicBezierPoints, PointCount);
   
   Lowering->SavedLineVertices = CopyLineVertices(Lowering->Arena, Curve->CurveVertices);
   
   f32 T = 0.5f;
   Lowering->LowerDegree = LowerDegree;
   Lowering->MixParameter = T;
   LowerPoints[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.P_I, LowerDegree.P_II, T);
   LowerWeights[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.W_I, LowerDegree.W_II, T);
  }
  
  // TODO(hbr): refactor this, it only has to be here because we still modify control points above
  BezierCubicCalculateAllControlPoints(PointCount - 1, LowerPoints, LowerBeziers);
  SetCurveControlPoints(&Witness, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
  
  EndEntityModify(Witness);
  
  EndTemp(Temp);
 }
}

internal void
PerformBezierCurveSplit(editor *Editor, entity_with_modify_witness *EntityWitness)
{
 temp_arena Temp = TempArena(0);
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 entity_store *EntityStore = &Editor->EntityStore;
 
 Assert(Tracking->Active);
 Tracking->Active = false;
 
 u32 ControlPointCount = Curve->ControlPointCount;
 
 entity *LeftEntity = Entity;
 entity *RightEntity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", GetEntityName(Entity));
 string RightName = StrF(Temp.Arena, "%S(right)", GetEntityName(Entity));
 
 curve *LeftCurve = SafeGetCurve(LeftEntity);
 curve *RightCurve = SafeGetCurve(RightEntity);
 
 SetEntityName(LeftEntity, LeftName);
 InitEntityFromEntity(&RightWitness, LeftEntity);
 SetEntityName(RightEntity, RightName);
 
 curve_points_modify_handle LeftPoints = BeginModifyCurvePoints(&LeftWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 curve_points_modify_handle RightPoints = BeginModifyCurvePoints(&RightWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithWeights);
 Assert(LeftPoints.PointCount == ControlPointCount);
 Assert(RightPoints.PointCount == ControlPointCount);
 
 BezierCurveSplit(Tracking->Fraction,
                  ControlPointCount, Curve->ControlPoints, Curve->ControlPointWeights,
                  LeftPoints.ControlPoints, LeftPoints.Weights,
                  RightPoints.ControlPoints, RightPoints.Weights);
 
 EndModifyCurvePoints(RightPoints);
 EndModifyCurvePoints(LeftPoints);
 
 EndEntityModify(LeftWitness);
 EndEntityModify(RightWitness);
 
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
  entity *Entity = AllocEntity(EntityStore, Entity_Curve, false);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  InitEntityFromEntity(&EntityWitness, Merging->MergeEntity);
  EndEntityModify(EntityWitness);
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
  
  ArrayReverse(Curve->ControlPoints,       Curve->ControlPointCount, v2);
  ArrayReverse(Curve->ControlPointWeights, Curve->ControlPointCount, f32);
  ArrayReverse(Curve->CubicBezierPoints,   Curve->ControlPointCount, cubic_bezier_point);
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
BeginMovingEntity(editor_left_click_state *Left, entity_handle Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingEntity;
 Left->TargetEntity = Target;
}

internal void
BeginMovingCurvePoint(editor_left_click_state *Left, entity_handle Target, curve_point_index CurvePoint)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingCurvePoint;
 Left->TargetEntity = Target;
 Left->CurvePointIndex = CurvePoint;
}

internal void
BeginMovingTrackingPoint(editor_left_click_state *Left, entity_handle Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingTrackingPoint;
 Left->TargetEntity = Target;
}

internal void
BeginMovingBSplineKnot(editor_left_click_state *Left, entity_handle Target, u32 B_SplineKnotIndex)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingBSplineKnot;
 Left->TargetEntity = Target;
 Left->B_SplineKnotIndex = B_SplineKnotIndex;
}

internal void
EndLeftClick(editor_left_click_state *Left)
{
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
