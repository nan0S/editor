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
 
 InitEntityStore(&Editor->EntityStore, Memory->MaxTextureCount, Memory->MaxBufferCount);
 InitCamera(&Editor->Camera);
 InitFrameStats(&Editor->FrameStats);
 InitLeftClickState(&Editor->LeftClick);
 InitMergingCurvesState(&Editor->MergingCurves, &Editor->EntityStore);
 InitVisualProfiler(&Editor->Profiler, Memory->Profiler);
 InitAnimatingCurvesState(&Editor->AnimatingCurves);
 InitThreadTaskMemoryStore(&Editor->ThreadTaskMemoryStore);
 InitImageLoadingStore(&Editor->ImageLoadingStore);
 
 {
  entity *Entity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), StrLit("de-casteljau"), 0);
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_Bezier;
  InitEntityCurve(Entity, Params);
  
  AppendControlPoint(&EntityWitness, V2(-0.5f, -0.5f));
  AppendControlPoint(&EntityWitness, V2(+0.5f, -0.5f));
  AppendControlPoint(&EntityWitness, V2(+0.5f, +0.5f));
  AppendControlPoint(&EntityWitness, V2(-0.5f, +0.5f));
  
  Entity->Curve.PointTracking.Active = true;
  SetTrackingPointFraction(&EntityWitness, 0.5f);
  
  EndEntityModify(EntityWitness);
 }
 
 {
  entity *Entity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
  entity_with_modify_witness Witness = BeginEntityModify(Entity);
  
  InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), StrLit("b-spline"), 0);
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_B_Spline;
  InitEntityCurve(Entity, Params);
  
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
 
 entity *Copy = AllocEntity(&Editor->EntityStore, Entity->Type, false);
 entity_with_modify_witness CopyWitness = BeginEntityModify(Copy);
 string CopyName = StrF(Temp.Arena, "%S(copy)", Entity->Name);
 
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
  
  string HeadName  = StrF(Temp.Arena, "%S (head)", Entity->Name);
  string TailName = StrF(Temp.Arena, "%S (tail)", Entity->Name);
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
 Assert(Tracking->Active);
 Tracking->Active = false;
 
 u32 ControlPointCount = Curve->ControlPointCount;
 
 entity *LeftEntity = Entity;
 entity *RightEntity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", Entity->Name);
 string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
 
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
  entity *Entity = AllocEntity(&Editor->EntityStore, Entity_Curve, false);
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

internal void
Merge2Curves(entity_with_modify_witness *MergeWitness, entity *Entity0, entity *Entity1, curve_merge_method Method)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve0 = SafeGetCurve(Entity0);
 curve *Curve1 = SafeGetCurve(Entity1);
 
 entity *MergeEntity = MergeWitness->Entity;
 curve *Merge = &MergeEntity->Curve;
 
 string Name = StrF(Temp.Arena, "%S+%S", Entity0->Name, Entity1->Name);
 
 InitEntity(MergeEntity, Entity0->P, Entity0->Scale, Entity0->Rotation, Name, Entity0->SortingLayer);
 InitEntityCurve(MergeEntity, Curve0->Params);
 
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 u32 PointCount0 = Curve0->ControlPointCount;
 u32 PointCount1 = Curve1->ControlPointCount;
 
 v2 *Points0 = Curve0->ControlPoints;
 v2 *Points1 = Curve1->ControlPoints;
 
 f32 *Weights0 = Curve0->ControlPointWeights;
 f32 *Weights1 = Curve1->ControlPointWeights;
 
 cubic_bezier_point *Beziers0 = Curve0->CubicBezierPoints;
 cubic_bezier_point *Beziers1 = Curve1->CubicBezierPoints;
 
 u32 DropCount1 = 0;
 v2 Fix1 = {};
 switch (Method)
 {
  case CurveMerge_Concat: {}break;
  
  case CurveMerge_C0:
  case CurveMerge_C1:
  case CurveMerge_C2:
  case CurveMerge_G1: {
   DropCount1 = 1;
   
   if (PointCount0 > 0 && PointCount1 > 0)
   {
    v2 P0 = Points0[PointCount0 - 1];
    
    v2 P1 = LocalEntityPositionToWorld(Entity1, Points1[0]);
    P1 = WorldToLocalEntityPosition(Entity0, P1);
    
    Fix1 = (P0 - P1);
   }
  }break;
  
  case CurveMerge_Count: InvalidPath;
 }
 u32 PointCount = (PointCount0 + PointCount1);
 if (DropCount1 <= PointCount1)
 {
  PointCount -= DropCount1;
 }
 
 v2 *Points = PushArrayNonZero(Temp.Arena, PointCount, v2);
 f32 *Weights = PushArrayNonZero(Temp.Arena, PointCount, f32);
 cubic_bezier_point *Beziers = PushArrayNonZero(Temp.Arena, PointCount, cubic_bezier_point);
 
 ArrayCopy(Weights,               Weights0,              PointCount0);
 ArrayCopy(Weights + PointCount0, Weights1 + DropCount1, PointCount1 - DropCount1);
 
 ArrayCopy(Points,                Points0,               PointCount0);
 ArrayCopy(Beziers,               Beziers0,              PointCount0);
 
 u32 PointIndex = PointCount0;
 for (u32 PointIndex1 = DropCount1;
      PointIndex1 < PointCount1;
      ++PointIndex1)
 {
  v2 Point1 = Points1[PointIndex1];
  Point1 = LocalEntityPositionToWorld(Entity1, Point1);
  Point1 = WorldToLocalEntityPosition(Entity0, Point1);
  Point1 = Point1 + Fix1;
  Points[PointIndex] = Point1;
  
  cubic_bezier_point Bezier1 = Beziers1[PointIndex1];
  Bezier1.P0 = LocalEntityPositionToWorld(Entity1, Bezier1.P0);
  Bezier1.P0 = WorldToLocalEntityPosition(Entity0, Bezier1.P0);
  Bezier1.P0 = Bezier1.P0 + Fix1;
  
  Bezier1.P1 = LocalEntityPositionToWorld(Entity1, Bezier1.P1);
  Bezier1.P1 = WorldToLocalEntityPosition(Entity0, Bezier1.P1);
  Bezier1.P1 = Bezier1.P1 + Fix1;
  
  Bezier1.P2 = LocalEntityPositionToWorld(Entity1, Bezier1.P2);
  Bezier1.P2 = WorldToLocalEntityPosition(Entity0, Bezier1.P2);
  Bezier1.P2 = Bezier1.P2 + Fix1;
  
  Beziers[PointIndex] = Bezier1;
  
  ++PointIndex;
 }
 
 b32 S_Exists = (SignedCmp(PointCount0 - 3, >=, 0) && SignedCmp(PointCount0 - 3, <, PointCount));
 b32 Q_Exists = (SignedCmp(PointCount0 - 2, >=, 0) && SignedCmp(PointCount0 - 2, <, PointCount));
 b32 P_Exists = (SignedCmp(PointCount0 - 1, >=, 0) && SignedCmp(PointCount0 - 1, <, PointCount));
 b32 R_Exists = (SignedCmp(PointCount0 - 0, >=, 0) && SignedCmp(PointCount0 - 0, <, PointCount));
 b32 T_Exists = (SignedCmp(PointCount0 + 1, >=, 0) && SignedCmp(PointCount0 + 1, <, PointCount));
 
 v2 S = (S_Exists ? Points[PointCount0 - 3] : V2(0,0));
 v2 Q = (Q_Exists ? Points[PointCount0 - 2] : V2(0,0));
 v2 P = (P_Exists ? Points[PointCount0 - 1] : V2(0,0));
 v2 R = (R_Exists ? Points[PointCount0 - 0] : V2(0,0));
 v2 T = (T_Exists ? Points[PointCount0 + 1] : V2(0,0));
 
 v2 R_ = R;
 v2 T_ = T;
 
 f32 n = Cast(f32)PointCount0;
 f32 m = Cast(f32)PointCount1;
 
 switch (Method)
 {
  case CurveMerge_G1: {
   // NOTE(hbr): G1 merge:
   // - C0 and
   // - Q,P,R should be colinear
   // - fix R, but maintain ||R-P|| length
   v2 U = P - Q;
   v2 V = R - P;
   v2 Projected = ProjectOnto(V, U);
   R_ = P + Projected;
  }break;
  
  case CurveMerge_C1:
  case CurveMerge_C2: {
   // NOTE(hbr): C1 merge:
   // - C0 and
   // - n/(b-a) * (P-Q) = m/(c-b) * (R-P)
   // - assuming (b-a) = (c-b) = 1
   // - which gives us: n * (P-Q) = m * (R-P)
   // - solving for R: R = n/m * (P-Q) + P
   R_ = n/m * (P-Q) + P;
   
   if (Method == CurveMerge_C2)
   {
    // NOTE(hbr): C2 merge:
    // - C0 and C1 and
    // - n*(n-1)/(b-a)^2 * (P-2Q+S) = m*(m-1)/(c-b)^2 * (T-2R+P)
    // - assuming (b-a) = (c-b) = 1
    // - which gives us: n*(n-1) * (P-2Q+S) = m*(m-1) * (T-2R+P)
    // - solving for T: T = (n*(n-1))/(m*(m-1)) * (P-2Q+S) + (2R-P)
    T_ = (n*(n-1))/(m*(m-1)) * (P-2*Q+S) + (2*R_-P);
   }
  }break;
  
  case CurveMerge_C0:
  case CurveMerge_Concat: {}break;
  
  case CurveMerge_Count: InvalidPath;
 }
 
 if (Q_Exists && R_Exists)
 {
  v2 Fix_R = R_ - R;
  Points[PointCount0 - 0] += Fix_R;
  Beziers[PointCount0 - 0].P0 += Fix_R;
  Beziers[PointCount0 - 0].P1 += Fix_R;
  Beziers[PointCount0 - 0].P2 += Fix_R;
 }
 
 if (S_Exists && T_Exists)
 {
  v2 Fix_T = T_ - T;
  Points[PointCount0 + 1] += Fix_T;
  Beziers[PointCount0 + 1].P0 += Fix_T;
  Beziers[PointCount0 + 1].P1 += Fix_T;
  Beziers[PointCount0 + 1].P2 += Fix_T;
 }
 
 SetCurveControlPoints(MergeWitness, PointCount, Points, Weights, Beziers);
 
 MaybeReverseCurvePoints(MergeEntity);
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 EndTemp(Temp);
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
