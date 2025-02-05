#include "editor.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"
#include "editor_renderer.cpp"
#include "editor_parametric_equation.cpp"
#include "editor_entity.cpp"
#include "editor_ui.cpp"
#include "editor_stb.cpp"

platform_api Platform;

// NOTE(hbr): this is code that is

internal void
TranslateCamera(camera *Camera, v2 Translate)
{
 Camera->P += Translate;
 Camera->ReachingTarget = false;
}

internal void
SetCameraTarget(camera *Camera, rectangle2 AABB)
{
 if (IsNonEmpty(&AABB))
 {
  v2 TargetP = 0.5f * (AABB.Mini + AABB.Maxi);
  Camera->TargetP = TargetP;
  v2 Extents = AABB.Maxi - TargetP;
  Camera->TargetZoom = Min(SafeDiv1(1.0f, Extents.X), SafeDiv1(1.0f, Extents.Y));
  Camera->ReachingTarget = true;
 }
}

internal void
SetCameraZoom(camera *Camera, f32 Zoom)
{
 Camera->Zoom = Zoom;
 Camera->ReachingTarget = false;
}

internal void
RotateCameraAround(camera *Camera, v2 Rotate, v2 Around)
{
 Camera->P = RotateAround(Camera->P, Around, Rotate);
 Camera->Rotation = CombineRotations2D(Camera->Rotation, Rotate);
 Camera->ReachingTarget = false;
}

internal void
ZoomCamera(camera *Camera, f32 By)
{
 f32 ZoomDelta = Camera->Zoom * Camera->ZoomSensitivity * By;
 Camera->Zoom += ZoomDelta;
 Camera->ReachingTarget = false;
}

internal void
MovePointAlongCurve(curve *Curve, v2 *TranslateInOut, f32 *PointFractionInOut, b32 Forward)
{
 v2 Translate = *TranslateInOut;
 f32 Fraction = *PointFractionInOut;
 
 u32 LinePointCount = Curve->LinePointCount;
 v2 *LinePoints = Curve->LinePoints;
 f32 DeltaFraction = 1.0f / (LinePointCount - 1);
 
 f32 PointIndexFloat = Fraction * (LinePointCount - 1);
 u32 PointIndex = Cast(u32)(Forward ? FloorF32(PointIndexFloat) : CeilF32(PointIndexFloat));
 PointIndex = ClampTop(PointIndex, LinePointCount - 1);
 
 i32 DirSign = (Forward ? 1 : -1);
 
 b32 Moving = true;
 while (Moving)
 {
  u32 PrevPointIndex = PointIndex;
  PointIndex += DirSign;
  if (PointIndex >= LinePointCount)
  {
   Moving = false;
  }
  
  if (Moving)
  {
   v2 AlongCurve = LinePoints[PointIndex] - LinePoints[PrevPointIndex];
   f32 AlongCurveLength = Norm(AlongCurve);
   f32 InvAlongCurveLength = 1.0f / AlongCurveLength;
   f32 Projection = Clamp01(Dot(AlongCurve, Translate) * InvAlongCurveLength * InvAlongCurveLength);
   Translate -= Projection * AlongCurve;
   Fraction += DirSign * Projection * DeltaFraction;
   
   if (Projection < 1.0f)
   {
    Moving = false;
   }
  }
 }
 
 *TranslateInOut = Translate;
 *PointFractionInOut = Fraction;
}

internal renderer_index *
AllocateTextureIndex(editor_assets *Assets)
{
 renderer_index *Result = Assets->FirstFreeTextureIndex;
 if (Result)
 {
  Assets->FirstFreeTextureIndex = Result->Next;
 }
 return Result;
}

internal void
DeallocateTextureIndex(editor_assets *Assets, renderer_index *Index)
{
 if (Index)
 {
  Index->Next = Assets->FirstFreeTextureIndex;
  Assets->FirstFreeTextureIndex = Index;
 }
}
//////////////////////

struct multiline_collision
{
 b32 Collided;
 u32 PointIndex;
};
internal multiline_collision
CheckCollisionWithMultiLine(v2 LocalAtP, v2 *LinePoints, u32 PointCount, f32 Width, f32 Tolerance)
{
 multiline_collision Result = {};
 
 f32 CheckWidth = Width + Tolerance;
 u32 MinDistancePointIndex = 0;
 f32 MinDistance = 1.0f;
 for (u32 PointIndex = 0;
      PointIndex + 1 < PointCount;
      ++PointIndex)
 {
  v2 P1 = LinePoints[PointIndex + 0];
  v2 P2 = LinePoints[PointIndex + 1];
  
  f32 Distance = SegmentSignedDistance(LocalAtP, P1, P2, CheckWidth);
  if (Distance < MinDistance)
  {
   MinDistance = Distance;
   MinDistancePointIndex = PointIndex;
  }
 }
 
 if (MinDistance <= 0.0f)
 {
  Result.Collided = true;
  Result.PointIndex = MinDistancePointIndex;
 }
 
 return Result;
}

internal collision
CheckCollisionWith(u32 EntityCount, entity *Entities, v2 AtP, f32 Tolerance)
{
 collision Result = {};
 temp_arena Temp = TempArena(0);
 
 entity_array EntityArray = {};
 EntityArray.Count = EntityCount;
 EntityArray.Entities = Entities;
 sorted_entries Sorted = SortEntities(Temp.Arena, EntityArray);
 
 for (u64 SortedIndex = 0;
      SortedIndex < Sorted.Count;
      ++SortedIndex)
 {
  u64 InverseIndex = Sorted.Count-1 - SortedIndex;
  entity *Entity = EntityArray.Entities + Sorted.Entries[InverseIndex].Index;
  
  if (IsEntityVisible(Entity))
  {
   v2 LocalAtP = WorldToLocalEntityPosition(Entity, AtP);
   // TODO(hbr): Note that Tolerance becomes wrong when we want to compute everything in
   // local space, fix that
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     u32 ControlPointCount = Curve->ControlPointCount;
     v2 *ControlPoints = Curve->ControlPoints;
     curve_params *Params = &Curve->Params;
     curve_point_tracking_state *Tracking = &Curve->PointTracking;
     
     if (AreLinePointsVisible(Curve))
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
        point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
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
        Result.CurvePointIndex = CurvePointIndexFromControlPointIndex({MinPointIndex});
       }
      }
      
      //- bezier "control" points
      {
       visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
       f32 MinSignedDistance = F32_INF;
       cubic_bezier_point_index MinPointIndex = {};
       
       for (u32 Index = 0;
            Index < VisibleBeziers.Count;
            ++Index)
       {
        f32 PointRadius = GetCurveCubicBezierPointRadius(Curve);
        cubic_bezier_point_index BezierIndex = VisibleBeziers.Indices[Index];
        v2 BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, BezierPoint, PointRadius + Tolerance);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPointIndex = BezierIndex;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePointIndex = CurvePointIndexFromBezierPointIndex(MinPointIndex);
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
      if (!Tracking->IsSplitting && !Result.Entity)
      {
       u32 IterationCount = Tracking->Intermediate.IterationCount;
       v2 *Points = Tracking->Intermediate.P;
       
       u32 PointIndex = 0;
       for (u32 Iteration = 0;
            Iteration < IterationCount;
            ++Iteration)
       {
        v2 *LinePoints = Points + PointIndex;
        u32 PointCount = IterationCount - Iteration;
        multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP, LinePoints, PointCount,
                                                               Params->LineWidth, Tolerance);
        if (Line.Collided)
        {
         Result.Entity = Entity;
         goto collided_label;
        }
        
        for (u32 I = 0; I < PointCount; ++I)
        {
         if (PointCollision(LocalAtP, Points[PointIndex], Curve->Params.PointRadius + Tolerance))
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
     
     if (!Curve->Params.LineDisabled)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->LinePoints, Curve->LinePointCount,
                                                             Params->LineWidth, Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_CurveLine;
       Result.CurveLinePointIndex = Line.PointIndex;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (Curve->Params.ConvexHullEnabled && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->ConvexHullPoints, Curve->ConvexHullCount,
                                                             Params->ConvexHullWidth, Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (Curve->Params.PolylineEnabled && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             ControlPoints, ControlPointCount,
                                                             Params->PolylineWidth, Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     v2 Dim = Image->Dim;
     if (-Dim.X <= LocalAtP.X && LocalAtP.X <= Dim.X &&
         -Dim.Y <= LocalAtP.Y && LocalAtP.Y <= Dim.Y)
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
 
 return Result;
}

internal entity *
AllocEntity(editor *Editor)
{
 entity *Entity = 0;
 
 entity *Entities = Editor->Entities;
 for (u32 EntityIndex = 0;
      EntityIndex < MAX_ENTITY_COUNT;
      ++EntityIndex)
 {
  entity *Current = Entities + EntityIndex;
  if (!(Current->Flags & EntityFlag_Active))
  {
   Entity = Current;
   
   arena *EntityArena = Entity->Arena;
   arena *DegreeLoweringArena = Entity->Curve.DegreeLowering.Arena;
   arena *ParametricArena = Entity->Curve.ParametricResources.Arena;
   
   StructZero(Entity);
   
   Entity->Arena = EntityArena;
   Entity->Curve.DegreeLowering.Arena = DegreeLoweringArena;
   Entity->Curve.ParametricResources.Arena = ParametricArena;
   
   Entity->Flags |= EntityFlag_Active;
   ClearArena(Entity->Arena);
   
   break;
  }
 }
 
 return Entity;
}

internal void
BeginCurveCombining(curve_combining_state *State, entity *CurveEntity)
{
 StructZero(State);
 State->SourceEntity = CurveEntity;
}

internal void
EndCurveCombining(curve_combining_state *State)
{
 StructZero(State);
}

internal void
DeallocEntity(editor *Editor, entity *Entity)
{
 if (Editor->CurveCombining.SourceEntity == Entity)
 {
  EndCurveCombining(&Editor->CurveCombining);
 }
 if (Editor->CurveCombining.WithEntity == Entity)
 {
  Editor->CurveCombining.WithEntity = 0;
 }
 if (Editor->SelectedEntity == Entity)
 {
  Editor->SelectedEntity = 0;
 }
 
 if (Entity->Type == Entity_Image)
 {
  image *Image = &Entity->Image;
  DeallocateTextureIndex(&Editor->Assets, Image->TextureIndex);
 }
 
 Entity->Flags &= ~EntityFlag_Active;
}

internal void
SelectEntity(editor *Editor, entity *Entity)
{
 if (Editor->SelectedEntity)
 {
  Editor->SelectedEntity->Flags &= ~EntityFlag_Selected;
 }
 if (Entity)
 {
  Entity->Flags |= EntityFlag_Selected;
 }
 Editor->SelectedEntity = Entity;
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

internal vertex_array
CopyLineVertices(arena *Arena, vertex_array Vertices)
{
 vertex_array Result = Vertices;
 Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, v2);
 ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
 
 return Result;
}

internal void
PerformBezierCurveSplit(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve = SafeGetCurve(Entity);
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 Assert(Tracking->Active);
 Tracking->Active = false;
 
 u32 ControlPointCount = Curve->ControlPointCount;
 
 entity *LeftEntity = Entity;
 entity *RightEntity = AllocEntity(Editor);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", Entity->Name);
 string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
 
 curve *LeftCurve = SafeGetCurve(LeftEntity);
 curve *RightCurve = SafeGetCurve(RightEntity);
 
 SetEntityName(LeftEntity, LeftName);
 InitEntityFromEntity(&RightWitness, LeftEntity);
 SetEntityName(RightEntity, RightName);
 
 curve_points LeftPoints = BeginModifyCurvePoints(&LeftWitness, ControlPointCount, ModifyCurvePointsWhichPoints_JustControlPoints);
 curve_points RightPoints = BeginModifyCurvePoints(&RightWitness, ControlPointCount, ModifyCurvePointsWhichPoints_JustControlPoints);
 Assert(LeftPoints.PointCount == ControlPointCount);
 Assert(RightPoints.PointCount == ControlPointCount);
 
 BezierCurveSplit(Tracking->Fraction,
                  ControlPointCount, Curve->ControlPoints, Curve->ControlPointWeights,
                  LeftPoints.ControlPoints, LeftPoints.Weights,
                  RightPoints.ControlPoints, RightPoints.Weights);
 
 EndModifyCurvePoints(RightCurve, &RightPoints);
 EndModifyCurvePoints(LeftCurve, &LeftPoints);
 
 EndEntityModify(LeftWitness);
 EndEntityModify(RightWitness);
 
 EndTemp(Temp);
}

internal void
DuplicateEntity(entity *Entity, editor *Editor)
{
 temp_arena Temp = TempArena(0);
 
 entity *Copy = AllocEntity(Editor);
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

// NOTE(hbr): This should really be some highly local internal, don't expect to reuse.
// It's highly specific.
internal b32
ResetCtxMenu(string Label)
{
 b32 Reset = false;
 if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
 {
  UI_OpenPopup(Label);
 }
 // NOTE(hbr): Passing Label.Data directly here is ok, because Label is actually
 // string literal thus is 0-padded.
 if (ImGui::BeginPopup(Label.Data, ImGuiWindowFlags_NoMove))
 {
  Reset = UI_MenuItemF(0, 0, "Reset");
  UI_EndPopup();
 }
 
 return Reset;
}

// TODO(hbr): Instead of copying all the verices and control points and weights manually, maybe just
// wrap control points, weights and cubicbezier poiints in a structure and just assign data here.
// In other words - refactor this function
// TODO(hbr): Refactor this function big time!!!
internal void
LowerBezierCurveDegree(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 u32 PointCount = Curve->ControlPointCount;
 if (PointCount > 0)
 {
  Assert(Curve->Params.Interpolation == Interpolation_Bezier);
  temp_arena Temp = TempArena(Lowering->Arena);
  
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
   
   Lowering->SavedLineVertices = CopyLineVertices(Lowering->Arena, Curve->LineVertices);
   
   f32 T = 0.5f;
   Lowering->LowerDegree = LowerDegree;
   Lowering->MixParameter = T;
   LowerPoints[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.P_I, LowerDegree.P_II, T);
   LowerWeights[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.W_I, LowerDegree.W_II, T);
  }
  
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  // TODO(hbr): refactor this, it only has to be here because we still modify control points above
  BezierCubicCalculateAllControlPoints(PointCount - 1, LowerPoints, LowerBeziers);
  SetCurveControlPoints(&EntityWitness, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
  EndEntityModify(EntityWitness);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(Curve->Params.Interpolation == Interpolation_Bezier);
 temp_arena Temp = TempArena(0);
 
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
 
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 SetCurveControlPoints(&EntityWitness,
                       ControlPointCount + 1,
                       ElevatedControlPoints,
                       ElevatedControlPointWeights,
                       ElevatedCubicBezierPoints);
 EndEntityModify(EntityWitness);
 
 EndTemp(Temp);
}

// TODO(hbr): Optimize this function
internal void
FocusCameraOnEntity(editor *Editor, entity *Entity)
{
 rectangle2 AABB = EmptyAABB();
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
 
 SetCameraTarget(&Editor->Camera, AABB);
}

internal void
SplitCurveOnControlPoint(entity *Entity, editor *Editor)
{
 curve *Curve = SafeGetCurve(Entity);
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  u32 LeftPointCount = Curve->SelectedIndex.Index + 1;
  u32 RightPointCount = Curve->ControlPointCount - Curve->SelectedIndex.Index;
  
  entity *LeftEntity = Entity;
  entity *RightEntity = AllocEntity(Editor);
  
  entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
  entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
  
  InitEntityFromEntity(&RightWitness, LeftEntity);
  
  curve *LeftCurve = SafeGetCurve(LeftEntity);
  curve *RightCurve = SafeGetCurve(RightEntity);
  
  string LeftName  = StrF(Temp.Arena, "%S(left)", Entity->Name);
  string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
  SetEntityName(LeftEntity, LeftName);
  SetEntityName(RightEntity, RightName);
  
  curve_points LeftPoints = BeginModifyCurvePoints(&LeftWitness, LeftPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers);
  curve_points RightPoints = BeginModifyCurvePoints(&RightWitness, RightPointCount, ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers);
  Assert(LeftPoints.PointCount == LeftPointCount);
  Assert(RightPoints.PointCount == RightPointCount);
  
  ArrayCopy(RightPoints.ControlPoints,
            Curve->ControlPoints + Curve->SelectedIndex.Index,
            RightPoints.PointCount);
  ArrayCopy(RightPoints.Weights,
            Curve->ControlPointWeights + Curve->SelectedIndex.Index,
            RightPoints.PointCount);
  ArrayCopy(RightCurve->CubicBezierPoints,
            Curve->CubicBezierPoints + Curve->SelectedIndex.Index,
            RightPoints.PointCount);
  
  EndModifyCurvePoints(LeftCurve, &LeftPoints);
  EndModifyCurvePoints(RightCurve, &RightPoints);
  
  EndEntityModify(LeftWitness);
  EndEntityModify(RightWitness);
  
  EndTemp(Temp);
 }
}

#if 0
// TODO(hbr): Do a pass oveer this internal to simplify the logic maybe (and simplify how the UI looks like in real life)
// TODO(hbr): Maybe also shorten some labels used to pass to ImGUI
internal void
RenderSelectedEntityUI(editor *Editor)
{
 entity *Entity = Editor->SelectedEntity;
 ui_config *UI_Config = &Editor->UI_Config;
 if (UI_Config->ViewSelectedEntityWindow && Entity)
 {
  UI_LabelF("SelectedEntity")
  {
   if (UI_BeginWindowF(&UI_Config->ViewSelectedEntityWindow, "Selected Entity"))
   {
    b32 SomeCurveParamChanged = false;
    if (Curve)
    {
     curve_params DefaultParams = Editor->CurveDefaultParams;
     curve_params *CurveParams = &Curve->Params;
     
     b32 Delete                        = false;
     b32 Copy                          = false;
     b32 SwitchVisibility              = false;
     b32 Deselect                      = false;
     b32 FocusOn                       = false;
     b32 ElevateBezierCurve            = false;
     b32 LowerBezierCurve              = false;
     b32 SplitBezierCurve              = false;
     b32 AnimateCurve                  = false;
     b32 CombineCurve                  = false;
     b32 SplitOnControlPoint           = false;
     b32 VisualizeDeCasteljau          = false;
     
     UI_NewRow();
     UI_SeparatorTextF("Actions");
     {
      Delete = UI_ButtonF("Delete");
      UI_SameRow();
      Copy = UI_ButtonF("Copy");
      UI_SameRow();
      SwitchVisibility = UI_ButtonF((Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide");
      UI_SameRow();
      Deselect = UI_ButtonF("Deselect");
      UI_SameRow();
      FocusOn = UI_ButtonF("Focus");
      
      if (Curve)
      {
       curve_params *CurveParams = &Curve->Params;
       
       // TODO(hbr): Maybe pick better name than transform
       AnimateCurve = UI_ButtonF("Animate");
       UI_SameRow();
       // TODO(hbr): Maybe pick better name than "Combine"
       CombineCurve = UI_ButtonF("Combine");
       
       UI_Disabled(!IsControlPointSelected(Curve))
       {
        UI_SameRow();
        SplitOnControlPoint = UI_ButtonF("Split on Control Point");
       }
       
       b32 IsBezierRegular = (CurveParams->Interpolation == Interpolation_Bezier &&
                              CurveParams->Bezier == Bezier_Regular);
       UI_Disabled(!IsBezierRegular)
       {
        UI_Disabled(Curve->ControlPointCount < 2)
        {
         UI_SameRow();
         SplitBezierCurve = UI_ButtonF("Split");
        }
        ElevateBezierCurve = UI_ButtonF("Elevate Degree");
        UI_Disabled(Curve->ControlPointCount == 0)
        {
         UI_SameRow();
         LowerBezierCurve = UI_ButtonF("Lower Degree");
        }
        VisualizeDeCasteljau = UI_ButtonF("Visualize De Casteljau");
       }
       
      }
     }
     
     if (Delete)
     {
      DeallocEntity(Editor, Entity);
     }
     
     if (Copy)
     {
      DuplicateEntity(Entity, Editor);
     }
     
     if (SwitchVisibility)
     {
      Entity->Flags ^= EntityFlag_Hidden;
     }
     
     if (Deselect)
     {
      DeselectCurrentEntity(Editor);
     }
     
     if (FocusOn)
     {
      FocusCameraOnEntity(Editor, Entity);
     }
     
     if (ElevateBezierCurve)
     {
      ElevateBezierCurveDegree(Entity);
     }
     
     if (LowerBezierCurve)
     {
      LowerBezierCurveDegree(Entity);
     }
     
     if (SplitBezierCurve)
     {
      BeginCurvePointTracking(Curve, true);
     }
     
     if (AnimateCurve)
     {
      BeginAnimatingCurve(&Editor->CurveAnimation, Entity);
     }
     
     if (CombineCurve)
     {
      BeginCurveCombining(&Editor->CurveCombining, Entity);
     }
     
     if (SplitOnControlPoint)
     {
      SplitCurveOnControlPoint(Entity, Editor);
     }
     
     if (VisualizeDeCasteljau)
     {
      BeginCurvePointTracking(Curve, false);
     }
     
     if (SomeCurveParamChanged)
     {
      MarkCurveForRecomputation(Entity);
     }
    }
    UI_EndWindow();
   }
  }
 }
}
#endif

internal void
UpdateAndRenderMenuBar(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
 local char const *SaveAsLabel = "SaveAsWindow";
 local char const *SaveAsTitle = "Save As";
 local char const *OpenNewProjectLabel = "OpenNewProject";
 local char const *OpenNewProjectTitle = "Open";
 
 local ImGuiWindowFlags FileDialogWindowFlags = ImGuiWindowFlags_NoCollapse;
 
 v2u WindowDim = RenderGroup->Frame->WindowDim;
 ImVec2 HalfWindowDim = ImVec2(0.5f * WindowDim.X, 0.5f * WindowDim.Y);
 ImVec2 FileDialogMinSize = HalfWindowDim;
 ImVec2 FileDialogMaxSize = ImVec2(Cast(f32)WindowDim.X, Cast(f32)WindowDim.Y);
 
#if 0
 auto FileDialog = ImGuiFileDialog::Instance();
 
 string ConfirmCloseProject = StrLit("ConfirmCloseCurrentProject");
 if (NewProject || KeyPressed(Input, Key_N, Modifier_Ctrl))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_NewProject;
 }
 if (OpenProject || KeyPressed(Input, Key_O, Modifier_Ctrl))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_OpenProject;
 }
 if (Quit || KeyPressed(Input, Key_Q, 0) || KeyPressed(Input, Key_Esc, 0))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_Quit;
 }
 
 if (SaveProject || KeyPressed(Input, Key_S, Modifier_Ctrl))
 {
  // TODO(hbr): Implement
  
  if (IsValid(Editor->ProjectSavePath))
  {
   temp_arena Temp = TempArena(0);
   
   error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                               Editor->SaveProjectFormat,
                                                               Editor->ProjectSavePath,
                                                               Editor);
   
   if (IsError(SaveProjectInFormatError))
   {
    AddNotificationF(Editor, Notification_Error, SaveProjectInFormatError.Data);
   }
   else
   {
    AddNotificationF(Editor, Notification_Success, "project successfully saved in %s",
                     Editor->ProjectSavePath.Data);
   }
   
   EndTemp(Temp);
  }
  else
  {
   FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                         SAVE_AS_MODAL_EXTENSION_SELECTION,
                         ".");
  }
 }
 
 if (SaveProjectAs || KeyPressed(Input, Key_S, Modifier_Ctrl|Modifier_Shift))
 {
  FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                        SAVE_AS_MODAL_EXTENSION_SELECTION,
                        ".");
 }
 
 if (LoadImage)
 {
  FileDialog->OpenModal(LoadImageLabel, LoadImageTitle,
                        "Image Files (*.jpg *.png){.jpg,.png}",
                        ".");
 }
 
 
 action_to_do ActionToDo = ActionToDo_Nothing;
 // NOTE(hbr): Open "Are you sure you want to exit?" popup
 {   
  // NOTE(hbr): Center window.
  ImGui::SetNextWindowPos(HalfWindowDim, ImGuiCond_Always, ImVec2(0.5f,0.5f));
  if (UI_BeginPopupModal(ConfirmCloseProject))
  {
   UI_TextF("You are about to discard current project. Save it?");
   ImGui::Separator();
   b32 Yes    = UI_ButtonF("Yes"); UI_SameRow();
   b32 No     = UI_ButtonF("No"); UI_SameRow();
   b32 Cancel = UI_ButtonF("Cancel");
   
   if (Yes || No)
   {
    if (Yes)
    {
     // TODO(hbr): Implement
     if (IsValid(Editor->ProjectSavePath))
     {
      temp_arena Temp = TempArena(0);
      
      string SaveProjectFilePath = Editor->ProjectSavePath;
      save_project_format SaveProjectFormat = Editor->SaveProjectFormat;
      
      error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                                  SaveProjectFormat,
                                                                  SaveProjectFilePath,
                                                                  Editor);
      
      if (IsError(SaveProjectInFormatError))
      {
       AddNotificationF(Editor, Notification_Error, "failed to discard current project: %s",
                        SaveProjectInFormatError.Data);
       
       Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
      }
      else
      {
       AddNotificationF(Editor, Notification_Success, "project sucessfully saved in %s",
                        SaveProjectFilePath.Data);
       
       ActionToDo = Editor->ActionWaitingToBeDone;
       Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
      }
      
      EndTemp(Temp);
     }
     else
     {
      FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                            SAVE_AS_MODAL_EXTENSION_SELECTION,
                            ".");
      // NOTE(hbr): Action is still waiting to be done as soon as save path is chosen.
     }
    }
    else if (No)
    {
     // NOTE(hbr): Do action now.
     ActionToDo = Editor->ActionWaitingToBeDone;
     Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
    }
   }
   
   if (Yes || No || Cancel)
   {
    UI_CloseCurrentPopup();
   }
   
   UI_EndPopup();
  }
 }
 
 // NOTE(hbr): Open dialog to Save Project As 
 if (FileDialog->Display(SaveAsLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   std::string const &SelectedFilter = FileDialog->GetCurrentFilter();
   string SaveProjectFilePath = Str(Temp.Arena,
                                    SelectedPath.c_str(),
                                    SelectedPath.size());
   string SaveProjectExtension = Str(Temp.Arena,
                                     SelectedFilter.c_str(),
                                     SelectedFilter.size());
   
   string Project = StrLitArena(Temp.Arena, PROJECT_FILE_EXTENSION_SELECTION);
   string JPG     = StrLitArena(Temp.Arena, JPG_FILE_EXTENSION_SELECTION);
   string PNG     = StrLitArena(Temp.Arena, PNG_FILE_EXTENSION_SELECTION);
   
   string AddExtension = {};
   save_project_format SaveProjectFormat = SaveProjectFormat_None;
   if (AreStringsEqual(SaveProjectExtension, Project))
   {
    AddExtension = StrLitArena(Temp.Arena, SAVED_PROJECT_FILE_EXTENSION);
    SaveProjectFormat = SaveProjectFormat_ProjectFile;
   }
   else if (AreStringsEqual(SaveProjectExtension, JPG))
   {
    AddExtension = StrLitArena(Temp.Arena, ".jpg");
    SaveProjectFormat = SaveProjectFormat_ImageFile;
   }
   else if (AreStringsEqual(SaveProjectExtension, PNG))
   {
    AddExtension = StrLitArena(Temp.Arena, ".png");
    SaveProjectFormat = SaveProjectFormat_ImageFile;
   }
   
   string SaveProjectFilePathWithExtension = SaveProjectFilePath;
   if (IsValid(AddExtension))
   {
    if (!HasSuffix(SaveProjectFilePathWithExtension, AddExtension))
    {
     SaveProjectFilePathWithExtension = StrF(Temp.Arena,
                                             "%s%s",
                                             SaveProjectFilePathWithExtension,
                                             AddExtension);
    }
   }
   
   error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                               SaveProjectFormat,
                                                               SaveProjectFilePathWithExtension,
                                                               Editor);
   
   if (IsError(SaveProjectInFormatError))
   {
    AddNotificationF(Editor, Notification_Error, SaveProjectInFormatError.Data);
    
    Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
   }
   else
   {
    EditorSetSaveProjectPath(Editor, SaveProjectFormat, SaveProjectFilePathWithExtension);
    
    AddNotificationF(Editor, Notification_Success, "project sucessfully saved in %s",
                     SaveProjectFilePathWithExtension.Data);
    
    ActionToDo = Editor->ActionWaitingToBeDone;
    Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
   }
   
   EndTemp(Temp);
  }
  else
  {
   Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
  }
  
  FileDialog->Close();
 }
 
 switch (ActionToDo)
 {
  case ActionToDo_Nothing: {} break;
  
  case ActionToDo_NewProject: {
   
   // TODO(hbr): Load new project
   
   editor_state NewEditor =
    CreateDefaultEditor(Editor->EntityPool,
                        Editor->DeCasteljauVisual.Arena,
                        Editor->DegreeLowering.Arena,
                        Editor->MovingPointArena,
                        Editor->CurveAnimation.Arena);
   
   DeallocEditor(&Editor);
   Editor = NewEditor;
   
   EditorSetSaveProjectPath(Editor, SaveProjectFormat_None, {});
  } break;
  
  case ActionToDo_OpenProject: {
   Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
   FileDialog->OpenModal(OpenNewProjectLabel, OpenNewProjectTitle,
                         SAVED_PROJECT_FILE_EXTENSION, ".");
  } break;
  
  case ActionToDo_Quit: {
   Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
   Input->QuitRequested = true;
  } break;
 }
 
 // NOTE(hbr): Open dialog to Open New Project
 if (FileDialog->Display(OpenNewProjectLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   // TODO(hbr): Implement this
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   string OpenProjectFilePath = Str(Temp.Arena,
                                    SelectedPath.c_str(),
                                    SelectedPath.size());
   
   
   load_project_result LoadResult = LoadProjectFromFile(Temp.Arena,
                                                        OpenProjectFilePath,
                                                        Editor->EntityPool,
                                                        Editor->DeCasteljauVisual.Arena,
                                                        Editor->DegreeLowering.Arena,
                                                        Editor->MovingPointArena,
                                                        Editor->CurveAnimation.Arena);
   if (IsError(LoadResult.Error))
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Error,
                     "failed to open new project: %s",
                     LoadResult.Error.Data);
   }
   else
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Success,
                     "project successfully loaded from %s",
                     OpenProjectFilePath.Data);
    
    DeallocEditor(&Editor);
    Editor = LoadResult.Editor;
    Editor->Params = LoadResult.EditorParameters;
    Editor->UI_Config = LoadResult.UI_Config;
    
    EditorSetSaveProjectPath(Editor,
                             SaveProjectFormat_ProjectFile,
                             OpenProjectFilePath);
   }
   
   ListIter(Warning, LoadResult.Warnings.Head, string_list_node)
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Warning,
                     Warning->String.Data);
   }
   
   EndTemp(Temp);
   
   
  }
  
  FileDialog->Close();
 }
 
 // NOTE(hbr): Open dialog to Load Image
 if (FileDialog->Display(LoadImageLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   string NewImageFilePath = Str(Temp.Arena,
                                 SelectedPath.c_str(),
                                 SelectedPath.size());
   
   string LoadTextureError = {};
   sf::Texture LoadedTexture = LoadTextureFromFile(Temp.Arena,
                                                   NewImageFilePath,
                                                   &LoadTextureError);
   
   if (IsError(LoadTextureError))
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Error,
                     "failed to load image: %s",
                     LoadTextureError.Data);
   }
   else
   {
    std::string const &SelectedFileName = FileDialog->GetCurrentFileName();
    string ImageFileName = Str(Temp.Arena,
                               SelectedFileName.c_str(),
                               SelectedFileName.size());
    RemoveExtension(&ImageFileName);
    
    entity *Entity = AllocEntity(&Editor);
    InitImageEntity(Entity,
                    V2(0.0f, 0.0f),
                    V2(1.0f, 1.0f),
                    Rotation2DZero(),
                    StrToNameStr(ImageFileName),
                    NewImageFilePath,
                    &LoadedTexture);
    
    SelectEntity(&Editor, Entity);
    
    AddNotificationF(&Editor->Notifications,
                     Notification_Success,
                     "successfully loaded image from %s",
                     NewImageFilePath.Data);
   }
   
   EndTemp(Temp);
  }
  
  FileDialog->Close();
 }
#endif
}

internal void
UpdateAndRenderPointTracking(render_group *Group, editor *Editor, entity *Entity)
{
 curve *Curve = &Entity->Curve;
 curve_params *CurveParams = &Curve->Params;
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 
 if (Tracking->Active)
 {
  if (Tracking->IsSplitting)
  {
   f32 Radius = GetCurveTrackedPointRadius(Curve);
   v4 Color = V4(0.0f, 1.0f, 0.0f, 0.5f);
   f32 OutlineThickness = 0.3f * Radius;
   v4 OutlineColor = DarkenColor(Color, 0.5f);
   
   PushCircle(Group,
              Entity->P + Tracking->LocalSpaceTrackedPoint,
              Entity->Rotation, Entity->Scale,
              Radius, Color,
              GetCurvePartZOffset(CurvePart_BezierSplitPoint),
              OutlineThickness, OutlineColor);
  }
  else
  {
   u32 IterationCount = Tracking->Intermediate.IterationCount;
   f32 P = 0.0f;
   f32 Delta_P = 1.0f / (IterationCount - 1);
   v4 GradientA = RGBA_Color(255, 0, 144);
   v4 GradientB = RGBA_Color(155, 200, 0);
   // TODO(hbr): Shouldn't this use some of the functions that are already used for drwaing curve points to be consistent?
   f32 PointSize = CurveParams->PointRadius;
   
   u32 PointIndex = 0;
   for (u32 Iteration = 0;
        Iteration < IterationCount;
        ++Iteration)
   {
    v4 IterationColor = Lerp(GradientA, GradientB, P);
    
    PushVertexArray(Group,
                    Tracking->LineVerticesPerIteration[Iteration].Vertices,
                    Tracking->LineVerticesPerIteration[Iteration].VertexCount,
                    Tracking->LineVerticesPerIteration[Iteration].Primitive,
                    IterationColor, GetCurvePartZOffset(CurvePart_DeCasteljauAlgorithmLines));
    
    for (u32 I = 0; I < IterationCount - Iteration; ++I)
    {
     v2 Point = Tracking->Intermediate.P[PointIndex];
     PushCircle(Group,
                Entity->P + Point, Entity->Rotation, Entity->Scale,
                PointSize, IterationColor,
                GetCurvePartZOffset(CurvePart_DeCasteljauAlgorithmPoints));
     ++PointIndex;
    }
    
    P += Delta_P;
   }
  }
 }
}

internal void
UpdateAndRenderDegreeLowering(render_group *RenderGroup, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *CurveParams = &Curve->Params;
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 
 if (Lowering->Active)
 {
  Assert(CurveParams->Interpolation == Interpolation_Bezier);
  
  b32 IsDegreeLoweringWindowOpen = true;
  b32 MixChanged = false;
  if (UI_BeginWindowF(&IsDegreeLoweringWindowOpen, 0, "Degree Lowering"))
  {          
   // TODO(hbr): Add wrapping
   UI_TextF("Degree lowering failed (curve has higher degree "
            "than the one you are trying to fit). Tweak parameters"
            "in order to fit curve manually or revert.");
   MixChanged = UI_SliderFloatF(&Lowering->MixParameter, 0.0f, 1.0f, "Middle Point Mix");
  }
  UI_EndWindow();
  
  b32 Ok     = UI_ButtonF("OK"); UI_SameRow();
  b32 Revert = UI_ButtonF("Revert");
  
  //-
  Assert(Lowering->LowerDegree.MiddlePointIndex < Curve->ControlPointCount);
  
  if (MixChanged)
  {
   v2 NewControlPoint = Lerp(Lowering->LowerDegree.P_I, Lowering->LowerDegree.P_II, Lowering->MixParameter);
   f32 NewControlPointWeight = Lerp(Lowering->LowerDegree.W_I, Lowering->LowerDegree.W_II, Lowering->MixParameter);
   
   control_point_index MiddlePointIndex = MakeControlPointIndex(Lowering->LowerDegree.MiddlePointIndex);
   SetCurveControlPoint(&EntityWitness,
                        MiddlePointIndex,
                        NewControlPoint,
                        NewControlPointWeight);
  }
  
  if (Revert)
  {
   SetCurveControlPoints(&EntityWitness,
                         Curve->ControlPointCount + 1,
                         Lowering->SavedControlPoints,
                         Lowering->SavedControlPointWeights,
                         Lowering->SavedCubicBezierPoints);
  }
  
  if (Ok || Revert || !IsDegreeLoweringWindowOpen)
  {
   Lowering->Active = false;
  }
 }
 
 if (Lowering->Active)
 {
  mat3 Model = ModelTransform(Entity->P, Entity->Rotation, Entity->Scale);
  SetTransform(RenderGroup, Model, Cast(f32)Entity->SortingLayer);
  
  v4 Color = Curve->Params.LineColor;
  Color.A *= 0.5f;
  
  PushVertexArray(RenderGroup,
                  Lowering->SavedLineVertices.Vertices,
                  Lowering->SavedLineVertices.VertexCount,
                  Lowering->SavedLineVertices.Primitive,
                  Color,
                  GetCurvePartZOffset(CurvePart_LineShadow));
  
  ResetTransform(RenderGroup);
 }
 
 EndEntityModify(EntityWitness);
}

// TODO(hbr): Refactor this function
internal void
RenderEntityCombo(u32 EntityCount, entity *Entities, entity **InOutEntity, string Label)
{
 entity *Entity = *InOutEntity;
 string Preview = (Entity ? Entity->Name : StrLit(""));
 if (UI_BeginCombo(Preview, Label))
 {
  for (u32 EntityIndex = 0;
       EntityIndex < MAX_ENTITY_COUNT;
       ++EntityIndex)
  {
   entity *Current= Entities + EntityIndex;
   if ((Current->Flags & EntityFlag_Active) &&
       (Current->Type == Entity_Curve) &&
       UI_SelectableItem(Current == Entity, Current->Name))
   {
    *InOutEntity = Current;
    break;
   }
  }
  UI_EndCombo();
 }
}

internal void
UpdateAndRenderCurveCombining(render_group *Group, editor *Editor)
{
 temp_arena Temp = TempArena(0);
 
 curve_combining_state *State = &Editor->CurveCombining;
 if (State->SourceEntity)
 {
  b32 Combine      = false;
  b32 Cancel       = false;
  b32 IsWindowOpen = true;
  if (UI_BeginWindowF(&IsWindowOpen, 0, "Combine Curves"))
  {                 
   if (IsWindowOpen)
   {
    RenderEntityCombo(MAX_ENTITY_COUNT, Editor->Entities, &State->SourceEntity, StrLit("Curve 1"));
    RenderEntityCombo(MAX_ENTITY_COUNT, Editor->Entities, &State->WithEntity,   StrLit("Curve 2"));
    UI_ComboF(Cast(u32 *)&State->CombinationType, CurveCombination_Count, CurveCombinationNames, "Method");
   }
   
   b32 CanCombine = (State->CombinationType != CurveCombination_None);
   if (State->WithEntity)
   {
    curve_params *SourceParams = &SafeGetCurve(State->SourceEntity)->Params;
    curve_params *WithParams = &SafeGetCurve(State->WithEntity)->Params;
    
    if (SourceParams != WithParams &&
        SourceParams->Interpolation == WithParams->Interpolation)
    {
     switch (SourceParams->Interpolation)
     {
      case Interpolation_Polynomial:  {
       CanCombine = (State->CombinationType == CurveCombination_Merge);
      } break;
      case Interpolation_CubicSpline: {
       CanCombine = ((SourceParams->CubicSpline == WithParams->CubicSpline) &&
                     (State->CombinationType == CurveCombination_Merge));
      } break;
      case Interpolation_Bezier: {
       CanCombine = ((SourceParams->Bezier == WithParams->Bezier) &&
                     (SourceParams->Bezier != Bezier_Cubic));
      } break;
      
      case Interpolation_Count: InvalidPath;
     }
    }
   }
   UI_Disabled(!CanCombine)
   {
    Combine = UI_ButtonF("Combine");
    UI_SameRow();
   }
   Cancel = UI_ButtonF("Cancel");
  }
  UI_EndWindow();
  
  if (Combine)
  {
   entity *FromEntity = State->SourceEntity;
   entity *ToEntity   = State->WithEntity;
   curve  *From       = SafeGetCurve(FromEntity);
   curve  *To         = SafeGetCurve(ToEntity);
   u32     FromCount  = From->ControlPointCount;
   u32     ToCount    = To->ControlPointCount;
   
   entity_with_modify_witness ToEntityWitness = BeginEntityModify(ToEntity);
   
   if (State->SourceCurveLastControlPoint)
   {
    ArrayReverse(From->ControlPoints,       FromCount, v2);
    ArrayReverse(From->ControlPointWeights, FromCount, f32);
    ArrayReverse(From->CubicBezierPoints,   FromCount, cubic_bezier_point);
   }
   
   if (State->WithCurveFirstControlPoint)
   {
    ArrayReverse(To->ControlPoints,       ToCount, v2);
    ArrayReverse(To->ControlPointWeights, ToCount, f32);
    ArrayReverse(To->CubicBezierPoints,   ToCount, cubic_bezier_point);
   }
   
   u32 CombinedPointCount = ToCount;
   u32 StartIndex = 0;
   v2 Translation = {};
   switch (State->CombinationType)
   {
    case CurveCombination_Merge: {
     CombinedPointCount += FromCount;
    } break;
    
    case CurveCombination_C0:
    case CurveCombination_C1:
    case CurveCombination_C2:
    case CurveCombination_G1: {
     if (FromCount > 0)
     {
      CombinedPointCount += FromCount - 1;
      StartIndex = 1;
      if (ToCount > 0)
      {
       Translation =
        LocalEntityPositionToWorld(ToEntity, To->ControlPoints[ToCount - 1]) -
        LocalEntityPositionToWorld(FromEntity, From->ControlPoints[0]);
      }
     }
    } break;
    
    case CurveCombination_None:
    case CurveCombination_Count: InvalidPath;
   }
   
   // NOTE(hbr): Allocate buffers and copy control points into them
   v2 *CombinedPoints  = PushArrayNonZero(Temp.Arena, CombinedPointCount, v2);
   f32 *CombinedWeights = PushArrayNonZero(Temp.Arena, CombinedPointCount, f32);
   cubic_bezier_point *CombinedBeziers = PushArrayNonZero(Temp.Arena, CombinedPointCount, cubic_bezier_point);
   ArrayCopy(CombinedPoints, To->ControlPoints, ToCount);
   ArrayCopy(CombinedWeights, To->ControlPointWeights, ToCount);
   ArrayCopy(CombinedBeziers, To->CubicBezierPoints, ToCount);
   
   // TODO(hbr): SIMD?
   for (u32 I = StartIndex; I < FromCount; ++I)
   {
    v2 FromPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
    v2 ToPoint   = WorldToLocalEntityPosition(ToEntity, FromPoint + Translation);
    CombinedPoints[ToCount - StartIndex + I] = ToPoint;
   }
   for (u32 I = StartIndex; I < FromCount; ++I)
   {
    for (u32 J = 0; J < 3; ++J)
    {
     v2 FromBezier = LocalEntityPositionToWorld(FromEntity, From->CubicBezierPoints[I].Ps[J]);
     v2 ToBezier   = WorldToLocalEntityPosition(ToEntity, FromBezier + Translation); 
     CombinedBeziers[(ToCount - StartIndex) + I].Ps[J] = ToBezier;
    }
   }
   ArrayCopy(CombinedWeights + ToCount,
             From->ControlPointWeights + StartIndex,
             (FromCount - StartIndex));
   
   // NOTE(hbr): Combine control points properly on the border
   switch (State->CombinationType)
   {
    // NOTE(hbr): Nothing to do
    case CurveCombination_Merge:
    case CurveCombination_C0: {} break;
    
    case CurveCombination_C1: {
     if (FromCount >= 2 && ToCount >= 2)
     {
      v2 P = CombinedPoints[ToCount - 1];
      v2 Q = CombinedPoints[ToCount - 2];
      
      // NOTE(hbr): First derivative equal
      v2 FixedControlPoint = Cast(f32)ToCount/FromCount * (P - Q) + P;
      v2 Fix = FixedControlPoint - CombinedPoints[ToCount];
      CombinedPoints[ToCount] = FixedControlPoint;
      
      CombinedBeziers[ToCount].P0 += Fix;
      CombinedBeziers[ToCount].P1 += Fix;
      CombinedBeziers[ToCount].P2 += Fix;
     }
    } break;
    
    case CurveCombination_C2: {
     if (FromCount >= 3 && ToCount >= 3)
     {
      // TODO(hbr): Merge C1 with C2, maybe G1.
      v2 R = CombinedPoints[ToCount - 3];
      v2 Q = CombinedPoints[ToCount - 2];
      v2 P = CombinedPoints[ToCount - 1];
      
      // NOTE(hbr): First derivative equal
      v2 Fixed_T = Cast(f32)ToCount/FromCount * (P - Q) + P;
      // NOTE(hbr): Second derivative equal
      v2 Fixed_U = Cast(f32)(FromCount * (FromCount-1))/(ToCount * (ToCount-1)) * (P - 2.0f * Q + R) + 2.0f * Fixed_T - P;
      v2 Fix_T = Fixed_T - CombinedPoints[ToCount];
      v2 Fix_U = Fixed_U - CombinedPoints[ToCount + 1];
      CombinedPoints[ToCount] = Fixed_T;
      CombinedPoints[ToCount + 1] = Fixed_U;
      
      CombinedBeziers[ToCount].P0 += Fix_T;
      CombinedBeziers[ToCount].P1 += Fix_T;
      CombinedBeziers[ToCount].P2 += Fix_T;
      
      CombinedBeziers[ToCount + 1].P0 += Fix_U;
      CombinedBeziers[ToCount + 1].P1 += Fix_U;
      CombinedBeziers[ToCount + 1].P2 += Fix_U;
     }
    } break;
    
    case CurveCombination_G1: {
     if (FromCount >= 2 && ToCount >= 2)
     {
      f32 PreserveLength = Norm(From->ControlPoints[1] - From->ControlPoints[0]);
      
      v2 P = CombinedPoints[ToCount - 2];
      v2 Q = CombinedPoints[ToCount - 1];
      v2 Direction = P - Q;
      Normalize(&Direction);
      
      v2 FixedControlPoint = Q - PreserveLength * Direction;
      v2 Fix = FixedControlPoint - CombinedPoints[ToCount];
      CombinedPoints[ToCount] = FixedControlPoint;
      
      CombinedBeziers[ToCount].P0 += Fix;
      CombinedBeziers[ToCount].P1 += Fix;
      CombinedBeziers[ToCount].P2 += Fix;
     }
    } break;
    
    case CurveCombination_None:
    case CurveCombination_Count: InvalidPath;
   }
   
   SetCurveControlPoints(&ToEntityWitness,
                         CombinedPointCount, CombinedPoints,
                         CombinedWeights, CombinedBeziers);
   
   DeallocEntity(Editor, FromEntity);
   SelectEntity(Editor, ToEntity);
   
   EndEntityModify(ToEntityWitness);
  }
  
  if (Combine || !IsWindowOpen || Cancel)
  {
   EndCurveCombining(State);
  }
 }
 
 curve *SourceCurve = SafeGetCurve(State->SourceEntity);
 curve *WithCurve = SafeGetCurve(State->WithEntity);
 if (SourceCurve && WithCurve && (SourceCurve != WithCurve) &&
     SourceCurve->ControlPointCount > 0 &&
     WithCurve->ControlPointCount > 0)
 {
  v2 SourcePointLocal = (State->SourceCurveLastControlPoint ?
                         SourceCurve->ControlPoints[SourceCurve->ControlPointCount - 1] :
                         SourceCurve->ControlPoints[0]);
  v2 WithPointLocal = (State->WithCurveFirstControlPoint ?
                       WithCurve->ControlPoints[0] :
                       WithCurve->ControlPoints[WithCurve->ControlPointCount - 1]);
  
  v2 SourcePoint = LocalEntityPositionToWorld(State->SourceEntity, SourcePointLocal);
  v2 WithPoint = LocalEntityPositionToWorld(State->WithEntity, WithPointLocal);
  
  f32 LineWidth = 0.5f * SourceCurve->Params.LineWidth;
  v4 Color = SourceCurve->Params.LineColor;
  Color.A = Cast(u8)(0.5f * Color.A);
  
  v2 LineDirection = WithPoint - SourcePoint;
  Normalize(&LineDirection);
  
  f32 TriangleSide = 10.0f * LineWidth;
  f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
  v2 BaseVertex = WithPoint - TriangleHeight * LineDirection;
  PushLine(Group, SourcePoint, BaseVertex, LineWidth, Color, GetCurvePartZOffset(CurvePart_Count));
  
  v2 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
  v2 LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
  v2 RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
  PushTriangle(Group, LeftVertex, RightVertex, WithPoint, Color, GetCurvePartZOffset(CurvePart_Count));
 }
 
 EndTemp(Temp);
}

internal task_with_memory *
BeginTaskWithMemory(editor *Editor)
{
 task_with_memory *Result = 0;
 for (u32 TaskIndex = 0;
      TaskIndex < MAX_TASK_COUNT;
      ++TaskIndex)
 {
  task_with_memory *Task = Editor->Tasks + TaskIndex;
  if (!Task->Allocated)
  {
   Task->Allocated = true;
   if (!Task->Arena)
   {
    Task->Arena = AllocArena();
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

internal void
LoadImageWork(void *UserData)
{
 load_image_work *Work = Cast(load_image_work *)UserData;
 
 task_with_memory *Task = Work->Task;
 renderer_transfer_op *TextureOp = Work->TextureOp;
 string ImagePath = Work->ImagePath;
 async_task *AsyncTask = Work->AsyncTask;
 arena *Arena = Task->Arena;
 
 string ImageData = Platform.ReadEntireFile(Arena, Work->ImagePath);
 loaded_image LoadedImage = LoadImageFromMemory(Arena, ImageData.Data, ImageData.Count);
 
 image_loading_state AsyncTaskState;
 renderer_transfer_op_state OpState;
 if (LoadedImage.Success)
 {
  u64 ImageSize = 4 * LoadedImage.Width * LoadedImage.Height;
  // TODO(hbr): Don't do memory copy, just read into that memory directly
  MemoryCopy(TextureOp->Pixels, LoadedImage.Pixels, ImageSize);
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
 AsyncTask->State = AsyncTaskState;
 
 EndTaskWithMemory(Task);
}

internal async_task *
AllocateAsyncTask(editor *Editor)
{
 async_task *Result = 0;
 
 for (u32 TaskIndex = 0;
      TaskIndex < MAX_TASK_COUNT;
      ++TaskIndex)
 {
  async_task *Task = Editor->AsyncTasks + TaskIndex;
  if (!Task->Active)
  {
   Result = Task;
   Task->Active = true;
   if (!Task->Arena)
   {
    Task->Arena = AllocArena();
   }
   Task->State = Image_Loading;
   break;
  }
 }
 
 return Result;
}

internal void
DeallocateAsyncTask(editor *Editor, async_task *Task)
{
 Task->Active = false;
 ClearArena(Task->Arena);
}

internal void
TryLoadImages(editor *Editor, u32 FileCount, string *FilePaths, v2 AtP)
{
 work_queue *WorkQueue = Editor->LowPriorityQueue;
 
 for (u32 FileIndex = 0;
      FileIndex < FileCount;
      ++FileIndex)
 {
  string FilePath = FilePaths[FileIndex];
  
  image_info ImageInfo = LoadImageInfo(FilePath);
  u64 RequestSize = Cast(u64)4 * ImageInfo.Width * ImageInfo.Height;
  
  editor_assets *Assets = &Editor->Assets;
  // TODO(hbr): Make it always suceeed??????
  renderer_transfer_op *TextureOp = PushTextureTransfer(Assets->RendererQueue, RequestSize);
  // TODO(hbr): Make it always succeed
  renderer_index *TextureIndex = AllocateTextureIndex(Assets);
  async_task *AsyncTask = AllocateAsyncTask(Editor);
  task_with_memory *Task = BeginTaskWithMemory(Editor);
  
  load_image_work *Work = 0;
  if (TextureOp && TextureIndex && Task && AsyncTask)
  {
   TextureOp->Width = ImageInfo.Width;
   TextureOp->Height = ImageInfo.Height;
   TextureOp->TextureIndex = TextureIndex->Index;
   
   AsyncTask->ImageWidth = ImageInfo.Width;
   AsyncTask->ImageHeight = ImageInfo.Height;
   AsyncTask->ImageFilePath = StrCopy(AsyncTask->Arena, FilePath);
   AsyncTask->AtP = AtP;
   AsyncTask->TextureIndex = TextureIndex;
   
   Work = PushStruct(Task->Arena, load_image_work);
   Work->Task = Task;
   Work->TextureOp = TextureOp;
   Work->ImagePath = StrCopy(Task->Arena, FilePath);
   Work->AsyncTask = AsyncTask;
   
   Platform.WorkQueueAddEntry(WorkQueue, LoadImageWork, Work);
  }
  
  //- dellocate things in case of failure
  if (!Work)
  {
   if (TextureOp)
   {
    PopTextureTransfer(Assets->RendererQueue, TextureOp);
   }
   if (TextureIndex)
   {
    DeallocateTextureIndex(Assets, TextureIndex);
   }
   if (AsyncTask)
   {
    DeallocateAsyncTask(Editor, AsyncTask);
   }
   if (Task)
   {
    EndTaskWithMemory(Task);
   }
  }
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

internal f32
UpdateBouncingParam(bouncing_parameter *Bouncing, f32 dt)
{
 f32 T = Bouncing->T + dt * Bouncing->Speed * Bouncing->Sign;
 f32 Sign = Bouncing->Sign;
 if (T < 0.0f || T > 1.0f)
 {
  T = Clamp01(T);
  Sign = -Sign;
 }
 Bouncing->T = T;
 Bouncing->Sign = Sign;
 
 return T;
}

internal void
BeginAnimatingCurves(animating_curves_state *Animation)
{
 arena *Arena = Animation->Arena;
 StructZero(Animation);
 
 Animation->Flags = (AnimatingCurves_Active | AnimatingCurves_ChoosingCurve);
 Animation->ChoosingCurveIndex = 0;
 Animation->Curves[0] = Animation->Curves[1] = 0;
 Animation->Bouncing = MakeBouncingParam();
 Animation->Arena = Arena;
}

internal void
EndAnimatingCurves(animating_curves_state *Animation)
{
 Animation->Flags &= ~AnimatingCurves_Active;
 ClearArena(Animation->Arena);
}

internal void
InitEditor(editor *Editor, editor_memory *Memory)
{
 Editor->BackgroundColor = Editor->DefaultBackgroundColor = RGBA_Color(21, 21, 21);
 Editor->CollisionToleranceClip = 0.02f;
 Editor->RotationRadiusClip = 0.1f;
 
 f32 LineWidth = 0.009f;
 v4 PolylineColor = RGBA_Color(16, 31, 31, 200);
 Editor->CurveDefaultParams.LineColor = RGBA_Color(21, 69, 98);
 Editor->CurveDefaultParams.LineWidth = LineWidth;
 Editor->CurveDefaultParams.PointColor = RGBA_Color(0, 138, 138, 148);
 Editor->CurveDefaultParams.PointRadius = 0.014f;
 Editor->CurveDefaultParams.PolylineColor = PolylineColor;
 Editor->CurveDefaultParams.PolylineWidth = LineWidth;
 Editor->CurveDefaultParams.ConvexHullColor = PolylineColor;
 Editor->CurveDefaultParams.ConvexHullWidth = LineWidth;
 Editor->CurveDefaultParams.PointCountPerSegment = 50;
 Editor->CurveDefaultParams.Parametric.MaxT = 1.0f;
 
 Editor->Camera.P = V2(0.0f, 0.0f);
 Editor->Camera.Rotation = Rotation2DZero();
 Editor->Camera.Zoom = 1.0f;
 Editor->Camera.ZoomSensitivity = 0.05f;
 Editor->Camera.ReachingTargetSpeed = 10.0f;
 
 Editor->FrameStats.Calculation.MinFrameTime = +F32_INF;
 Editor->FrameStats.Calculation.MaxFrameTime = -F32_INF;
 
 Editor->MovingPointArena = AllocArena();
 
 for (u32 EntityIndex = 0;
      EntityIndex < MAX_ENTITY_COUNT;
      ++EntityIndex)
 {
  entity *Entity = Editor->Entities + EntityIndex;
  Entity->Arena = AllocArena();
  Entity->Curve.DegreeLowering.Arena = AllocArena();
  
  curve *Curve = &Entity->Curve;
  Curve->ParametricResources.Arena = AllocArena();
 }
 
 Editor->EntityListWindow = true;
 // TODO(hbr): Change to false by default
 Editor->DiagnosticsWindow = true;
 Editor->SelectedEntityWindow = true;
 
 Editor->LeftClick.OriginalVerticesArena = AllocArena();
 
 {
  entity *Entity = AllocEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), StrLit("special"), 0);
  curve_params Params = Editor->CurveDefaultParams;
  Params.Interpolation = Interpolation_Bezier;
  InitCurve(&EntityWitness, Params);
  
  AppendControlPoint(&EntityWitness, V2(-0.5f, -0.5f));
  AppendControlPoint(&EntityWitness, V2(+0.5f, -0.5f));
  AppendControlPoint(&EntityWitness, V2(+0.5f, +0.5f));
  AppendControlPoint(&EntityWitness, V2(-0.5f, +0.5f));
  
  Entity->Curve.PointTracking.Active = true;
  Entity->Curve.PointTracking.IsSplitting = false;
  SetTrackingPointFraction(&EntityWitness, 0.5f);
  
  EndEntityModify(EntityWitness);
 }
 
 editor_assets *Assets = &Editor->Assets;
 u32 TextureCount = Memory->MaxTextureCount;
 for (u32 Index = 0;
      Index < TextureCount;
      ++Index)
 {
  renderer_index *TextureIndex = PushStruct(Memory->PermamentArena, renderer_index);
  TextureIndex->Index = TextureCount-1 - Index;
  TextureIndex->Next = Assets->FirstFreeTextureIndex;
  Assets->FirstFreeTextureIndex = TextureIndex;
 }
 u32 BufferCount = Memory->MaxBufferCount;
 for (u32 Index = 0;
      Index < BufferCount;
      ++Index)
 {
  renderer_index *BufferIndex = PushStruct(Memory->PermamentArena, renderer_index);
  BufferIndex->Index = BufferCount-1 - Index;
  BufferIndex->Next = Assets->FirstFreeBufferIndex;
  Assets->FirstFreeBufferIndex = BufferIndex;
 }
 Assets->RendererQueue = Memory->RendererQueue;
 
 Editor->LowPriorityQueue = Memory->LowPriorityQueue;
 Editor->HighPriorityQueue = Memory->HighPriorityQueue;
 
 // NOTE(hbr): Initialize just a few, others will probably never be initialized
 // but if they do, then they will be initialized lazily.
 for (u32 TaskIndex = 0;
      TaskIndex < Min(MAX_TASK_COUNT, 4);
      ++TaskIndex)
 {
  task_with_memory *Task = Editor->Tasks + TaskIndex;
  Task->Arena = AllocArena();
 }
 for (u32 TaskIndex = 0;
      TaskIndex < Min(MAX_TASK_COUNT, 4);
      ++TaskIndex)
 {
  async_task *Task = Editor->AsyncTasks + TaskIndex;
  Task->Arena = AllocArena();
 }
 
 Editor->AnimatingCurves.Arena = AllocArena();
}

internal void
UpdateAndRenderAnimatingCurves(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 entity *Entity0 = Animation->Curves[0];
 entity *Entity1 = Animation->Curves[1];
 
 if ((Animation->Flags & AnimatingCurves_Active) && Entity0 && Entity1)
 {
  if (Animation->Flags & AnimatingCurves_Animating)
  {
   UpdateBouncingParam(&Animation->Bouncing, Input->dtForFrame);
   Input->RefreshRequested = true;
  }
  
  v2 Points[4] = {
   V2(0.0f, 0.0f),
   V2(1.0f, 0.0f),
   V2(0.0f, 1.0f),
   V2(1.0f, 1.0f),
  };
  f32 MappedT = BezierCurveEvaluate(Animation->Bouncing.T, Points, 4).Y;
  
  curve *Curve0 = SafeGetCurve(Entity0);
  curve *Curve1 = SafeGetCurve(Entity1);
  
  v2 *LinePoints0 = Curve0->LinePoints;
  v2 *LinePoints1 = Curve1->LinePoints;
  
  u32 LinePointCount0 = Curve0->LinePointCount;
  u32 LinePointCount1 = Curve1->LinePointCount;
  
  // TODO(hbr): Multithread this
  arena *Arena = Animation->Arena;
  ClearArena(Arena);
  
  u32 LinePointCount = Min(Curve0->LinePointCount, Curve1->LinePointCount);
  v2 *LinePoints = PushArrayNonZero(Arena, LinePointCount, v2);
  for (u32 LinePointIndex = 0;
       LinePointIndex < LinePointCount;
       ++LinePointIndex)
  {
   u32 LinePointIndex0 = ClampTop(LinePointCount0 * LinePointIndex / (LinePointCount - 1), LinePointCount0 - 1);
   u32 LinePointIndex1 = ClampTop(LinePointCount1 * LinePointIndex / (LinePointCount - 1), LinePointCount1 - 1);
   
   v2 PointLocal0 = Curve0->LinePoints[LinePointIndex0];
   v2 PointLocal1 = Curve1->LinePoints[LinePointIndex1];
   
   v2 Point0 = LocalEntityPositionToWorld(Entity0, PointLocal0);
   v2 Point1 = LocalEntityPositionToWorld(Entity1, PointLocal1);
   
   v2 Point  = Lerp(Point0, Point1, MappedT);
   LinePoints[LinePointIndex] = Point;
  }
  
  f32 LineWidth0 = Curve0->Params.LineWidth;
  f32 LineWidth1 = Curve1->Params.LineWidth;
  f32 LineWidth  = Lerp(LineWidth0, LineWidth1, MappedT);
  
  v4 LineColor0 = Curve0->Params.LineColor;
  v4 LineColor1 = Curve1->Params.LineColor;
  v4 LineColor  = Lerp(LineColor0, LineColor1, MappedT);
  
  f32 ZOffset0 = Cast(f32)Entity0->SortingLayer;
  f32 ZOffset1 = Cast(f32)Entity1->SortingLayer;
  f32 ZOffset  = Lerp(ZOffset0, ZOffset1, MappedT);
  
  vertex_array LineVertices = ComputeVerticesOfThickLine(Arena, LinePointCount, LinePoints, LineWidth, false);
  PushVertexArray(RenderGroup,
                  LineVertices.Vertices, LineVertices.VertexCount, LineVertices.Primitive,
                  LineColor,
                  ZOffset);
 }
}

internal void
UpdateAndRenderEntities(editor *Editor, render_group *RenderGroup)
{
 for (u32 EntryIndex = 0;
      EntryIndex < MAX_ENTITY_COUNT;
      ++EntryIndex)
 {
  entity *Entity = Editor->Entities + EntryIndex;
  if ((Entity->Flags & EntityFlag_Active) && IsEntityVisible(Entity))
  {
   mat3 Model = ModelTransform(Entity->P, Entity->Rotation, Entity->Scale);
   SetTransform(RenderGroup, Model, Cast(f32)Entity->SortingLayer);
   
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     curve_params *CurveParams = &Curve->Params;
     
     if (!CurveParams->LineDisabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->LineVertices.Vertices,
                      Curve->LineVertices.VertexCount,
                      Curve->LineVertices.Primitive,
                      Curve->Params.LineColor,
                      GetCurvePartZOffset(CurvePart_CurveLine));
     }
     
     if (CurveParams->PolylineEnabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->PolylineVertices.Vertices,
                      Curve->PolylineVertices.VertexCount,
                      Curve->PolylineVertices.Primitive,
                      Curve->Params.PolylineColor,
                      GetCurvePartZOffset(CurvePart_CurvePolyline));
     }
     
     if (CurveParams->ConvexHullEnabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->ConvexHullVertices.Vertices,
                      Curve->ConvexHullVertices.VertexCount,
                      Curve->ConvexHullVertices.Primitive,
                      Curve->Params.ConvexHullColor,
                      GetCurvePartZOffset(CurvePart_CurveConvexHull));
     }
     
     if (AreLinePointsVisible(Curve))
     {
      visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
      for (u32 Index = 0;
           Index < VisibleBeziers.Count;
           ++Index)
      {
       cubic_bezier_point_index BezierIndex = VisibleBeziers.Indices[Index];
       
       v2 BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
       v2 CenterPoint = GetCenterPointFromCubicBezierPointIndex(Curve, BezierIndex);
       
       f32 BezierPointRadius = GetCurveCubicBezierPointRadius(Curve);
       f32 HelperLineWidth = 0.2f * CurveParams->LineWidth;
       
       // TODO(hbr): It is really fucked up I have to add Entity->P when I do [SetTransform] above.
       // clean up this mess
       PushLine(RenderGroup,
                Entity->P + BezierPoint,
                Entity->P + CenterPoint,
                HelperLineWidth,
                CurveParams->LineColor,
                GetCurvePartZOffset(CurvePart_CubicBezierHelperLines));
       PushCircle(RenderGroup,
                  Entity->P + BezierPoint, Entity->Rotation, Entity->Scale,
                  BezierPointRadius, CurveParams->PointColor,
                  GetCurvePartZOffset(CurvePart_CubicBezierHelperPoints));
      }
      
      u32 ControlPointCount = Curve->ControlPointCount;
      v2 *ControlPoints = Curve->ControlPoints;
      for (u32 PointIndex = 0;
           PointIndex < ControlPointCount;
           ++PointIndex)
      {
       point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
       PushCircle(RenderGroup,
                  Entity->P + ControlPoints[PointIndex],
                  Entity->Rotation, Entity->Scale,
                  PointInfo.Radius,
                  PointInfo.Color,
                  GetCurvePartZOffset(CurvePart_CurveControlPoint),
                  PointInfo.OutlineThickness,
                  PointInfo.OutlineColor);
      }
     }
     
     UpdateAndRenderDegreeLowering(RenderGroup, Entity);
     
     // TODO(hbr): Update this
     curve_combining_state *Combining = &Editor->CurveCombining;
     if (Combining->SourceEntity == Entity)
     {
      UpdateAndRenderCurveCombining(RenderGroup, Editor);
     }
     
     UpdateAndRenderPointTracking(RenderGroup, Editor, Entity);
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     PushImage(RenderGroup, Image->Dim, Image->TextureIndex->Index);
    }break;
    
    case Entity_Count: InvalidPath; break;
   }
   
   ResetTransform(RenderGroup);
  }
 }
}

internal void
UpdateRenderSelectedEntityUI(editor *Editor)
{
 entity *Entity = Editor->SelectedEntity;
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 
 if (Editor->SelectedEntityWindow && Entity)
 {
  UI_PushLabelF("SelectedEntity");
  if (UI_BeginWindowF(&Editor->SelectedEntityWindow, WindowFlag_AutoResize, "Selected Entity"))
  {
   curve *Curve = 0;
   image *Image = 0;
   switch (Entity->Type)
   {
    case Entity_Curve: {Curve = &Entity->Curve;}break;
    case Entity_Image: {Image = &Entity->Image;}break;
    case Entity_Count: InvalidPath; break;
   }
   
   UI_SeparatorTextF("General");
   UI_LabelF("General")
   {
    Entity->Name = UI_InputTextF(Entity->NameBuffer,
                                 ArrayCount(Entity->NameBuffer),
                                 "Name");
    
    UI_DragFloat2F(Entity->P.E, 0.0f, 0.0f, 0, "Position");
    if (ResetCtxMenu(StrLit("PositionReset")))
    {
     Entity->P = V2(0.0f, 0.0f);
    }
    
    UI_AngleSliderF(&Entity->Rotation, "Rotation");
    if (ResetCtxMenu(StrLit("RotationReset")))
    {
     Entity->Rotation = Rotation2DZero();
    }
    
    UI_DragFloat2F(Entity->Scale.E, 0.0f, 0.0f, 0, "Scale");
    if (ResetCtxMenu(StrLit("ScaleReset")))
    {
     Entity->Scale = V2(1.0f, 1.0f);
    }
    
    {
     UI_PushLabelF("DragMe");
     f32 UniformScale = 0.0f;
     UI_DragFloatF(&UniformScale, 0.0f, 0.0f, "Drag Me!", "Uniform Scale");
     f32 WidthOverHeight = Entity->Scale.X / Entity->Scale.Y;
     Entity->Scale = Entity->Scale + V2(WidthOverHeight * UniformScale, UniformScale);
     if (ResetCtxMenu(StrLit("DragMeReset")))
     {
      Entity->Scale = V2(1.0f, 1.0f);
     }
     UI_PopLabel();
    }
    
    UI_SliderIntegerF(&Entity->SortingLayer, -100, 100, "Sorting Layer");
    if (ResetCtxMenu(StrLit("SortingLayerReset")))
    {
     Entity->SortingLayer = 0;
    }
   }
   
   if (Curve)
   {
    curve_params CurveParams = Curve->Params;
    curve_params *DefaultParams = &Editor->CurveDefaultParams;
    
    UI_SeparatorTextF("Curve");
    UI_LabelF("Curve")
    {       
     UI_ComboF(SafeCastToPtr(CurveParams.Interpolation, u32), Interpolation_Count, InterpolationNames, "Interpolation");
     if (ResetCtxMenu(StrLit("InterpolationReset")))
     {
      CurveParams.Interpolation = DefaultParams->Interpolation;
     }
     
     switch (CurveParams.Interpolation)
     {
      case Interpolation_Polynomial: {
       polynomial_interpolation_params *Polynomial = &CurveParams.Polynomial;
       
       UI_ComboF(SafeCastToPtr(Polynomial->Type, u32), PolynomialInterpolation_Count, PolynomialInterpolationNames, "Variant");
       if (ResetCtxMenu(StrLit("PolynomialReset")))
       {
        Polynomial->Type = DefaultParams->Polynomial.Type;
       }
       
       UI_ComboF(SafeCastToPtr(Polynomial->PointSpacing, u32), PointSpacing_Count, PointSpacingNames, "Point Spacing");
       if (ResetCtxMenu(StrLit("PointSpacingReset")))
       {
        Polynomial->PointSpacing = DefaultParams->Polynomial.PointSpacing;
       }
      }break;
      
      case Interpolation_CubicSpline: {
       UI_ComboF(SafeCastToPtr(CurveParams.CubicSpline, u32), CubicSpline_Count, CubicSplineNames, "Variant");
       if (ResetCtxMenu(StrLit("SplineReset")))
       {
        CurveParams.CubicSpline = DefaultParams->CubicSpline;
       }
      }break;
      
      case Interpolation_Bezier: {
       UI_ComboF(SafeCastToPtr(CurveParams.Bezier, u32), Bezier_Count, BezierNames, "Variant");
       if (ResetCtxMenu(StrLit("BezierReset")))
       {
        CurveParams.Bezier = DefaultParams->Bezier;
       }
      }break;
      
      case Interpolation_Parametric: {
       parametric_curve_params *Parametric = &CurveParams.Parametric;
       parametric_curve_resources *Resources = &Curve->ParametricResources;
       
       string MinT_Equation = UI_InputTextF(Resources->MinT_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, "##MinT");
       UI_SameRow();
       UI_TextF(" <= t <= ");
       UI_SameRow();
       string MaxT_Equation = UI_InputTextF(Resources->MaxT_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, "##MaxT");
       
       UI_TextF("x(t) := ");
       UI_SameRow();
       string X_Equation = UI_InputTextF(Resources->X_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, "##x(t)");
       
       UI_TextF("y(t) := ");
       UI_SameRow();
       string Y_Equation = UI_InputTextF(Resources->Y_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, "##y(t)");
       
       arena *Arena = Resources->Arena;
       temp_arena Temp = TempArena(Arena);
       
       parametric_equation_eval_result MinT_Eval = ParametricEquationEval(Temp.Arena, MinT_Equation);
       
       string MinT_ErrorMsg = {};
       if (!MinT_Eval.ErrorMsg)
       {
        f32 MinT = MinT_Eval.Value;
        if (MinT <= Parametric->MaxT)
        {
         Parametric->MinT = MinT;
        }
        else
        {
         MinT_ErrorMsg = StrF(Temp.Arena,
                              "min_t (%f) exceeds max_t (%f)",
                              MinT, Parametric->MaxT);
        }
       }
       else
       {
        MinT_ErrorMsg = StrFromCStr(MinT_Eval.ErrorMsg);
       }
       if (MinT_ErrorMsg.Data)
       {
        UI_ColoredText(RedColor)
        {
         UI_TextF("t_min: %S", MinT_ErrorMsg);
        }
       }
       
       parametric_equation_eval_result MaxT_Eval = ParametricEquationEval(Temp.Arena, MaxT_Equation);
       
       string MaxT_ErrorMsg = {};
       if (!MaxT_Eval.ErrorMsg)
       {
        f32 MaxT = MaxT_Eval.Value;
        if (MaxT >= Parametric->MinT)
        {
         Parametric->MaxT = MaxT;
        }
        else
        {
         MaxT_ErrorMsg = StrF(Temp.Arena,
                              "min_t (%f) exceeds max_t (%f)",
                              Parametric->MinT, MaxT);
        }
       }
       else
       {
        MaxT_ErrorMsg = StrFromCStr(MaxT_Eval.ErrorMsg);
       }
       if (MaxT_ErrorMsg.Data)
       {
        UI_ColoredText(RedColor)
        {
         UI_TextF("t_max: %S", MaxT_ErrorMsg);
        }
       }
       
       // TODO(hbr): There is a memory leak here - if parametric equations suceeed to parse, then we will not free
       // the previous ones
       {       
        temp_arena Checkpoint = BeginTemp(Arena);
        parametric_equation_parse_result X_Parse = ParametricEquationParse(Checkpoint.Arena, X_Equation);
        if (!X_Parse.ErrorMsg)
        {
         Parametric->X_Equation = *X_Parse.ParsedExpr;
        }
        else
        {
         UI_TextF(X_Parse.ErrorMsg);
         EndTemp(Checkpoint);
        }
       }
       
       {       
        temp_arena Checkpoint = BeginTemp(Arena);
        parametric_equation_parse_result Y_Parse = ParametricEquationParse(Checkpoint.Arena, Y_Equation);
        if (!Y_Parse.ErrorMsg)
        {
         Parametric->Y_Equation = *Y_Parse.ParsedExpr;
        }
        else
        {
         UI_TextF(Y_Parse.ErrorMsg);
         EndTemp(Checkpoint);
        }
       }
       
       EndTemp(Temp);
       
#if 0
       // TODO(hbr): Move this out into a library
       // TODO(hbr): Also grow this buffer if necessary
       // TODO(hbr): And also, make it possible to move it into a separate window to have more space
       ImGui::InputTextMultiline("Parametric Equation",
                                 Parametric->SourceCodeBuffer,
                                 MAX_PARAMETRIC_CURVE_SOURCE_CODE_SIZE,
                                 ImVec2(0, 0),
                                 ImGuiInputTextFlags_AllowTabInput);
       Parametric->SourceCode = StrFromCStr(Parametric->SourceCodeBuffer);
#endif
       
      }break;
      
      case Interpolation_Count: InvalidPath;
     }
    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    CurveParams.LineDisabled = !UI_BeginTreeF("Line");
    if (!CurveParams.LineDisabled)
    {
     UI_ColorPickerF(&CurveParams.LineColor, "Color");
     if (ResetCtxMenu(StrLit("ColorReset")))
     {
      CurveParams.LineColor = DefaultParams->LineColor;
     }
     
     UI_DragFloatF(&CurveParams.LineWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("WidthReset")))
     {
      CurveParams.LineWidth = DefaultParams->LineWidth;
     }
     
     UI_SliderIntegerF(SafeCastToPtr(CurveParams.PointCountPerSegment, i32), 1, 2000, "Detail");
     if (ResetCtxMenu(StrLit("DetailReset")))
     {
      CurveParams.PointCountPerSegment = DefaultParams->PointCountPerSegment;
     }
     
     UI_EndTree();
    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    CurveParams.PointsDisabled = !UI_BeginTreeF("Control Points");
    if (!CurveParams.PointsDisabled)
    {
     UI_ColorPickerF(&CurveParams.PointColor, "Color");
     if (ResetCtxMenu(StrLit("PointColorReset")))
     {
      CurveParams.PointColor = DefaultParams->PointColor;
     }
     
     UI_DragFloatF(&CurveParams.PointRadius, 0.0f, FLT_MAX, 0, "Radius");
     if (ResetCtxMenu(StrLit("PointRadiusReset")))
     {
      CurveParams.PointRadius = DefaultParams->PointRadius;
     }
     
     if (CurveParams.Interpolation == Interpolation_Bezier &&
         CurveParams.Bezier == Bezier_Regular)
     {
      b32 WeightChanged = false;
      u32 PointCount = Curve->ControlPointCount;
      f32 *Weights = Curve->ControlPointWeights;
      
      u32 Selected = 0;
      if (IsControlPointSelected(Curve))
      {
       Selected = Curve->SelectedIndex.Index;
       WeightChanged |= UI_DragFloatF(&Weights[Selected], 0.0f, FLT_MAX, 0, "Weight (%u)", Selected);
       if (ResetCtxMenu(StrLit("WeightReset")))
       {
        Weights[Selected] = 1.0f;
        WeightChanged = true;
       }
       ImGui::Spacing();
      }
      
      if (UI_BeginTreeF("Weights"))
      {
       // NOTE(hbr): Limit the number of points displayed in the case
       // curve has A LOT of them
       u32 ShowCount = 100;
       u32 ToIndex = ClampTop(Selected + ShowCount/2, PointCount);
       u32 FromIndex = Selected - Min(Selected, ShowCount/2);
       u32 LeftCount = ShowCount - (ToIndex - FromIndex);
       ToIndex = ClampTop(ToIndex + LeftCount, PointCount);
       FromIndex = FromIndex - Min(FromIndex, LeftCount);
       
       for (u32 PointIndex = FromIndex;
            PointIndex < ToIndex;
            ++PointIndex)
       {
        UI_Id(PointIndex)
        {                              
         WeightChanged |= UI_DragFloatF(&Weights[PointIndex], 0.0f, FLT_MAX, 0, "Point (%u)", PointIndex);
         if (ResetCtxMenu(StrLit("WeightReset")))
         {
          Weights[PointIndex] = 1.0f;
          WeightChanged = true;
         }
        }
       }
       
       UI_EndTree();
      }
      
      if (WeightChanged)
      {
       MarkEntityModified(&EntityWitness);
      }
     }
     
     UI_EndTree();
    }
    
    CurveParams.PolylineEnabled = UI_BeginTreeF("Polyline");
    if (CurveParams.PolylineEnabled)
    {
     UI_ColorPickerF(&CurveParams.PolylineColor, "Color");
     if (ResetCtxMenu(StrLit("PolylineColorReset")))
     {
      CurveParams.PolylineColor = DefaultParams->PolylineColor;
     }
     
     UI_DragFloatF(&CurveParams.PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("PolylineWidthReset")))
     {
      CurveParams.PolylineWidth = DefaultParams->PolylineWidth;
     }
     UI_EndTree();
    }
    
    CurveParams.ConvexHullEnabled = UI_BeginTreeF("Convex Hull");
    if (CurveParams.ConvexHullEnabled)
    {
     UI_ColorPickerF(&CurveParams.ConvexHullColor, "Color");
     if (ResetCtxMenu(StrLit("ConvexHullColorReset")))
     {
      CurveParams.ConvexHullColor = DefaultParams->ConvexHullColor;
     }
     
     UI_DragFloatF(&CurveParams.ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("ConvexHullWidthReset")))
     {
      CurveParams.ConvexHullWidth = DefaultParams->ConvexHullWidth;
     }
     
     UI_EndTree();
    }
    
    // TODO(hbr): I don't know if misc is a good name here
    UI_LabelF("Misc")
    {
     UI_SeparatorTextF("Misc");
     if (UI_ButtonF("Swap Append Side"))
     {
      Entity->Flags ^= EntityFlag_CurveAppendFront;
     }
     
     if (IsCurveEligibleForPointTracking(Curve))
     {
      UI_LabelF("PointTracking")
      {
       curve_point_tracking_state *Tracking = &Curve->PointTracking;
       f32 Fraction = Tracking->Fraction;
       b32 BezierTrackingActive = (Tracking->IsSplitting ? false : Tracking->Active);
       b32 SplittingTrackingActive = (Tracking->IsSplitting ? Tracking->Active : false);
       
       if (UI_CheckboxF(&BezierTrackingActive, "##DeCasteljauEnabled"))
       {
        Tracking->Active = BezierTrackingActive;
        if (Tracking->Active)
        {
         Tracking->IsSplitting = false;
        }
       }
       UI_SameRow();
       UI_SeparatorTextF("De Casteljau's Algorithm");
       if (BezierTrackingActive)
       {
        UI_SliderFloatF(&Fraction, 0.0f, 1.0f, "t");
       }
       
       if (UI_CheckboxF(&SplittingTrackingActive, "##SplittingEnabled"))
       {
        Tracking->Active = SplittingTrackingActive;
        if (Tracking->Active)
        {
         Tracking->IsSplitting = true;
        }
       }
       UI_SameRow();
       UI_SeparatorTextF("Split Bezier Curve");
       if (SplittingTrackingActive)
       {
        UI_SliderFloatF(&Fraction, 0.0f, 1.0f, "t");
        UI_SameRow();
        if (UI_ButtonF("Split!"))
        {
         PerformBezierCurveSplit(Editor, Entity);
        }
       }
       
       if (Fraction != Tracking->Fraction)
       {
        SetTrackingPointFraction(&EntityWitness, Fraction);
       }
      }
     }
    }
    
    // TODO(hbr): This is wrong now with SourceCode stored as string
    if (!StructsEqual(Curve->Params, CurveParams))
    {
     Curve->Params = CurveParams;
     MarkEntityModified(&EntityWitness);
    }
   }
  }
  UI_EndWindow();
  UI_PopLabel();
 }
 
 EndEntityModify(EntityWitness);
}

internal void
UpdateRenderMenuBarUI(editor *Editor,
                      b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                      b32 *SaveProjectAs, b32 *QuitProject, b32 *AnimateCurves)
{
 if (UI_BeginMainMenuBar())
 {
  if (UI_BeginMenuF("File"))
  {
   *NewProject     = UI_MenuItemF(0, "Ctrl+N",       "New");
   *OpenFileDialog = UI_MenuItemF(0, "Ctrl+O",       "Open");
   *SaveProject    = UI_MenuItemF(0, "Ctrl+S",       "Save");
   *SaveProjectAs  = UI_MenuItemF(0, "Ctrl+Shift+S", "Save As");
   *QuitProject    = UI_MenuItemF(0, "Escape",     "Quit");
   UI_EndMenu();
  }
  
  if (UI_BeginMenuF("Actions"))
  {
   *AnimateCurves = UI_MenuItemF(0, 0, "Animate Curves");
   UI_EndMenu();
  }
  
  if (UI_BeginMenuF("View"))
  {
   UI_MenuItemF(&Editor->EntityListWindow, 0, "Entity List");
   UI_MenuItemF(&Editor->SelectedEntityWindow, 0, "Selected Entity");
   UI_EndMenu();
  }
  
  if (UI_BeginMenuF("Settings"))
  {
   if (UI_BeginMenuF("Camera"))
   {
    camera *Camera = &Editor->Camera;
    
    UI_DragFloat2F(Camera->P.E, 0.0f, 0.0f, 0, "Position");
    if (ResetCtxMenu(StrLit("PositionReset")))
    {
     TranslateCamera(Camera, -Camera->P);
    }
    
    UI_AngleSliderF(&Camera->Rotation, "Rotation");
    if (ResetCtxMenu(StrLit("RotationReset")))
    {
     RotateCameraAround(Camera, Rotation2DInverse(Camera->Rotation), Camera->P);
    }
    
    UI_DragFloatF(&Camera->Zoom, 0.0f, 0.0f, 0, "Zoom");
    if (ResetCtxMenu(StrLit("ZoomReset")))
    {
     SetCameraZoom(Camera, 1.0f);
    }
    
    UI_EndMenu();
   }
   UI_ColorPickerF(&Editor->BackgroundColor, "Background Color");
   if (ResetCtxMenu(StrLit("BackgroundColorReset")))
   {
    Editor->BackgroundColor = Editor->DefaultBackgroundColor;
   }
   UI_EndMenu();
  }
  
  // TODO(hbr): Complete help menu
  if (UI_BeginMenuF("Help"))
  {
   UI_EndMenu();
  }
  
  UI_EndMainMenuBar();
 }
}

internal void
UpdateRenderEntityListUI(editor *Editor)
{
 if (Editor->EntityListWindow)
 {
  if (UI_BeginWindowF(&Editor->EntityListWindow, 0, "Entities"))
  {
   entity *Entities = Editor->Entities;
   for (u32 EntityTypeIndex = 0;
        EntityTypeIndex < Entity_Count;
        ++EntityTypeIndex)
   {
    entity_type EntityType = Cast(entity_type)EntityTypeIndex;
    string EntityTypeLabel = {};
    switch (EntityType)
    {
     case Entity_Curve: {EntityTypeLabel = StrLit("Curve"); }break;
     case Entity_Image: {EntityTypeLabel = StrLit("Image"); }break;
     case Entity_Count: InvalidPath; break;
    }
    
    if (UI_CollapsingHeader(EntityTypeLabel))
    {
     for (u32 EntityIndex = 0;
          EntityIndex < MAX_ENTITY_COUNT;
          ++EntityIndex)
     {
      entity *Entity = Entities + EntityIndex;
      if ((Entity->Flags & EntityFlag_Active) && Entity->Type == EntityType)
      {
       UI_PushId(EntityIndex);
       b32 Selected = IsEntitySelected(Entity);
       
       if (UI_SelectableItem(Selected, Entity->Name))
       {
        SelectEntity(Editor, Entity);
       }
       
       string CtxMenu = StrLit("EntityContextMenu");
       if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
       {
        UI_OpenPopup(CtxMenu);
       }
       if (UI_BeginPopup(CtxMenu))
       {
        if (UI_MenuItemF(0, 0, "Delete"))
        {
         DeallocEntity(Editor, Entity);
        }
        if (UI_MenuItemF(0, 0, "Copy"))
        {
         DuplicateEntity(Entity, Editor);
        }
        if (UI_MenuItemF(0, 0, (Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide"))
        {
         Entity->Flags ^= EntityFlag_Hidden;
        }
        if (UI_MenuItemF(0, 0, (Selected ? "Deselect" : "Select")))
        {
         SelectEntity(Editor, (Selected ? 0 : Entity));
        }
        if (UI_MenuItemF(0, 0, "Focus"))
        {
         FocusCameraOnEntity(Editor, Entity);
        }
        
        UI_EndPopup();
       }
       
       UI_PopId();
      }
     }
    }
   }
  }
  UI_EndWindow();
 }
}

internal void
UpdateRenderDiagnosticsUI(editor *Editor, platform_input *Input)
{
 //- render diagnostics UI
 if (Editor->DiagnosticsWindow)
 {
  if (UI_BeginWindowF(&Editor->DiagnosticsWindow, WindowFlag_AutoResize, "Diagnostics"))
  {
   frame_stats *Stats = &Editor->FrameStats;
   UI_TextF("%-20s %.2f ms", "Frame time", 1000.0f * Input->dtForFrame);
   UI_TextF("%-20s %.0f", "FPS", Stats->FPS);
   UI_TextF("%-20s %.2f ms", "Min frame time", 1000.0f * Stats->MinFrameTime);
   UI_TextF("%-20s %.2f ms", "Max frame time", 1000.0f * Stats->MaxFrameTime);
   UI_TextF("%-20s %.2f ms", "Average frame time", 1000.0f * Stats->AvgFrameTime);
  }
  UI_EndWindow();
 }
}

internal void
UpdateAndRenderNotifications(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
 UI_LabelF("Notifications")
 {
  v2u WindowDim = RenderGroup->Frame->WindowDim;
  f32 Padding = 0.01f * WindowDim.X;
  f32 TargetPosY = WindowDim.Y - Padding;
  f32 WindowWidth = 0.1f * WindowDim.X;
  
  for (u32 NotificationIndex = 0;
       NotificationIndex < MAX_NOTIFICATION_COUNT;
       ++NotificationIndex)
  {
   notification *Notification = Editor->Notifications + NotificationIndex;
   if (Notification->Type != Notification_None)
   {
    b32 Remove = false;
    
    Notification->LifeTime += Input->dtForFrame;
    
    f32 MoveSpeed = 20.0f;
    f32 NextPosY = Lerp(Notification->ScreenPosY,
                        TargetPosY,
                        1.0f - PowF32(2.0f, -MoveSpeed * Input->dtForFrame));
    Notification->ScreenPosY = NextPosY;
    
    ImVec2 WindowPosition = ImVec2(WindowDim.X - Padding, NextPosY);
    ImVec2 WindowMinSize = ImVec2(WindowWidth, 0.0f);
    ImVec2 WindowMaxSize = ImVec2(WindowWidth, FLT_MAX);
    ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSizeConstraints(WindowMinSize, WindowMaxSize);
    
    enum notification_phase
    {
     NotificationPhase_FadeIn,
     NotificationPhase_ProperLife,
     NotificationPhase_FadeOut,
     NotificationPhase_Invisible,
     NotificationPhase_Count,
    };
    local f32 Phases[NotificationPhase_Count] = {
     0.15f, // fade in
     10.0f, // proper lifetime
     0.15f, // fade out
     0.1f,  // invisible
    };
    f32 PhaseFraction = Notification->LifeTime;
    u32 PhaseIndex = 0;
    for (; PhaseIndex < NotificationPhase_Count; ++PhaseIndex)
    {
     if (PhaseFraction <= Phases[PhaseIndex])
     {
      break;
     }
     PhaseFraction -= Phases[PhaseIndex];
    }
    f32 Fade = 0.0f;
    switch (Cast(notification_phase)PhaseIndex)
    {
     case NotificationPhase_FadeIn:     { Fade = Map01(PhaseFraction, 0.0f, Phases[NotificationPhase_FadeIn]); }break;
     case NotificationPhase_ProperLife: { Fade = 1.0f; }break;
     case NotificationPhase_FadeOut:    { Fade = 1.0f - Map01(PhaseFraction, 0.0f, Phases[NotificationPhase_FadeOut]); } break;
     case NotificationPhase_Invisible:  { Fade = 0.0f; } break;
     case NotificationPhase_Count:      { Remove = true; } break;
    }
    // NOTE(hbr): Quadratic interpolation instead of linear.
    Fade = 1.0f - Square(1.0f - Fade);
    
    UI_Alpha(Fade)
    {
     DeferBlock(UI_BeginWindowF(0, WindowFlag_AutoResize | WindowFlag_NoTitleBar | WindowFlag_NoFocusOnAppearing,
                                "Notification%lu", NotificationIndex),
                UI_EndWindow())
     {
      ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
      if (ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(0) ||
                                       ImGui::IsMouseClicked(1)))
      {
       Remove = true;
      }
      
      string Title = {};
      v4 TitleColor = {};
      switch (Notification->Type)
      {
       case Notification_Success: { Title = StrLit("Success"); TitleColor = GreenColor; } break;
       case Notification_Error:   { Title = StrLit("Error");   TitleColor = RedColor; } break;
       case Notification_Warning: { Title = StrLit("Warning"); TitleColor = YellowColor; } break;
       case Notification_None: InvalidPath; break;
      }
      
      UI_ColoredText(TitleColor)
      {
       UI_Text(Title);
      }
      
      ImGui::Separator();
      ImGui::PushTextWrapPos(0.0f);
      UI_Text(Notification->Content);
      ImGui::PopTextWrapPos();
      
      f32 WindowHeight = ImGui::GetWindowHeight();
      TargetPosY -= WindowHeight + Padding;
     }
    }
    
    if (Remove)
    {
     Notification->Type = Notification_None;
    }
   }
  }
 }
}

internal void
UpdateAndRenderAnimatingCurvesUI(editor *Editor)
{
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 if (Animation->Flags & AnimatingCurves_Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindowF(&WindowOpen, 0, "Curve Animation");
  
  if (WindowOpen)
  {
   // TODO(hbr): I think we need to introduce generational entity pointers
   entity *Curve0 = Animation->Curves[0];
   entity *Curve1 = Animation->Curves[1];
   
   b32 ChoosingCurve = (Animation->Flags & AnimatingCurves_ChoosingCurve);
   
   b32 Disable0 = false;
   string Button0 = {};
   if (ChoosingCurve && Animation->ChoosingCurveIndex == 0)
   {
    Disable0 = true;
    Button0 = StrLit("Click on Curve to choose");
   }
   else
   {
    if (Curve0)
    {
     Button0 = Curve0->Name;
    }
    else
    {
     Button0 = StrLit("...");
    }
   }
   
   UI_Id(0)
   {
    UI_TextF("Curve 0: ");
    UI_SameRow();
    if (UI_Button(Button0))
    {
     if (Disable0)
     {
      Animation->Flags &= ~AnimatingCurves_ChoosingCurve;
     }
     else
     {
      Animation->Flags |= AnimatingCurves_ChoosingCurve;
      Animation->ChoosingCurveIndex = 0;
     }
    }
   }
   
   string Button1 = {};
   b32 Disable1 = false;
   if (ChoosingCurve && Animation->ChoosingCurveIndex == 1)
   {
    Disable1 = true;
    Button1 = StrLit("Click on Curve to choose");
   }
   else
   {
    if (Curve1)
    {
     Button1 = Curve1->Name;
    }
    else
    {
     Button1 = StrLit("...");
    }
   }
   
   UI_Id(1)
   {
    UI_TextF("Curve 1: ");
    UI_SameRow();
    if (UI_Button(Button1))
    {
     if (Disable1)
     {
      Animation->Flags &= ~AnimatingCurves_ChoosingCurve;
     }
     else
     {
      Animation->Flags |= AnimatingCurves_ChoosingCurve;
      Animation->ChoosingCurveIndex = 1;
     }
    }
   }
   
   UI_SeparatorTextF("");
   
   if (UI_SliderFloatF(&Animation->Bouncing.T, 0.0f, 1.0f, "t"))
   {
    Animation->Flags &= ~AnimatingCurves_Animating;
   }
   UI_SameRow();
   if (Animation->Flags & AnimatingCurves_Animating)
   {
    if (UI_ButtonF("Stop"))
    {
     Animation->Flags &= ~AnimatingCurves_Animating;
    }
   }
   else
   {
    UI_Disabled((Curve0 == 0) || (Curve1 == 0))
    {
     if (UI_ButtonF("Start"))
     {
      Animation->Flags |= AnimatingCurves_Animating;
     }
    }
   }
   
   if (UI_BeginTreeF("More"))
   {
    UI_SliderFloatF(&Animation->Bouncing.Speed, 0.0f, 4.0f, "Speed");
    UI_EndTree();
   }
  }
  else
  {
   EndAnimatingCurves(Animation);
  }
  
  UI_EndWindow();
 }
}

internal void
ProcessAsyncEvents(editor *Editor)
{
 for (u32 TaskIndex = 0;
      TaskIndex < MAX_TASK_COUNT;
      ++TaskIndex)
 {
  async_task *Task = Editor->AsyncTasks + TaskIndex;
  if (Task->Active)
  {
   if (Task->State == Image_Loading)
   {
    // NOTE(hbr): nothing to do
   }
   else
   {
    if (Task->State == Image_Loaded)
    {
     entity *Entity = AllocEntity(Editor);
     if (Entity)
     {
      string FileName = PathLastPart(Task->ImageFilePath);
      string FileNameNoExt = StrChopLastDot(FileName);
      
      InitEntity(Entity, Task->AtP, V2(1, 1), Rotation2DZero(), FileNameNoExt, 0);
      Entity->Type = Entity_Image;
      
      image *Image = &Entity->Image;
      Image->Dim.X = Cast(f32)Task->ImageWidth / Task->ImageHeight;
      Image->Dim.Y = 1.0f;
      Image->TextureIndex = Task->TextureIndex;
      
      SelectEntity(Editor, Entity);
     }
    }
    else
    {
     Assert(Task->State == Image_Failed);
     AddNotificationF(Editor, Notification_Error, "Image loading failed");
    }
    
    DeallocateAsyncTask(Editor, Task);
   }
  }
 }
}

internal void
UpdateFrameStats(editor *Editor, platform_input *Input)
{
 frame_stats *Stats = &Editor->FrameStats;
 f32 dt = Input->dtForFrame;
 
 Stats->Calculation.FrameCount += 1;
 Stats->Calculation.SumFrameTime += dt;
 Stats->Calculation.MinFrameTime = Min(Stats->Calculation.MinFrameTime, dt);
 Stats->Calculation.MaxFrameTime = Max(Stats->Calculation.MaxFrameTime, dt);
 
 if (Stats->Calculation.SumFrameTime >= 1.0f)
 {
  Stats->FPS = Stats->Calculation.FrameCount / Stats->Calculation.SumFrameTime;
  Stats->MinFrameTime = Stats->Calculation.MinFrameTime;
  Stats->MaxFrameTime = Stats->Calculation.MaxFrameTime;
  Stats->AvgFrameTime = Stats->Calculation.SumFrameTime / Stats->Calculation.FrameCount;
  
  Stats->Calculation.FrameCount = 0;
  Stats->Calculation.MinFrameTime = F32_INF;
  Stats->Calculation.MaxFrameTime = -F32_INF;
  Stats->Calculation.SumFrameTime = 0.0f;
 }
}

internal void
UpdateCamera(editor *Editor, platform_input *Input)
{
 camera *Camera = &Editor->Camera;
 if (Camera->ReachingTarget)
 {
  f32 t = 1.0f - PowF32(2.0f, -Camera->ReachingTargetSpeed * Input->dtForFrame);
  Camera->P = Lerp(Camera->P, Camera->TargetP, t);
  Camera->Zoom = Lerp(Camera->Zoom, Camera->TargetZoom, t);
  
  if (ApproxEq32(Camera->Zoom, Camera->TargetZoom) &&
      ApproxEq32(Camera->P.X, Camera->TargetP.X) &&
      ApproxEq32(Camera->P.Y, Camera->TargetP.Y))
  {
   Camera->P = Camera->TargetP;
   Camera->Zoom = Camera->TargetZoom;
   Camera->ReachingTarget = false;
  }
 }
}

internal void
ProcessInputEvents(editor *Editor, platform_input *Input, render_group *RenderGroup,
                   b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                   b32 *SaveProjectAs, b32 *QuitProject, b32 *AnimateCurves)
{
 editor_left_click_state *LeftClick = &Editor->LeftClick;
 editor_right_click_state *RightClick = &Editor->RightClick;
 editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 for (u32 EventIndex = 0;
      EventIndex < Input->EventCount;
      ++EventIndex)
 {
  platform_event *Event = Input->Events + EventIndex;
  b32 Eat = false;
  v2 MouseP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
  
  //- left click events processing
  b32 AnimationWantsInput = ((Animation->Flags & AnimatingCurves_Active) &&
                             (Animation->Flags & AnimatingCurves_ChoosingCurve));
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_LeftMouseButton &&
      (!LeftClick->Active || AnimationWantsInput))
  {
   collision Collision = {};
   if (Event->Flags & PlatformEventFlag_Alt)
   {
    // NOTE(hbr): Force no collision, so that user can add control point wherever they want
   }
   else
   {
    Collision = CheckCollisionWith(MAX_ENTITY_COUNT,
                                   Editor->Entities,
                                   MouseP,
                                   RenderGroup->CollisionTolerance);
   }
   
   if (AnimationWantsInput && Collision.Entity && Collision.Entity->Type == Entity_Curve)
   {
    Eat = true;
    
    Animation->Curves[Animation->ChoosingCurveIndex] = Collision.Entity;
    if (Animation->Curves[0] == 0)
    {
     Animation->ChoosingCurveIndex = 0;
    }
    else if (Animation->Curves[1] == 0)
    {
     Animation->ChoosingCurveIndex = 1;
    }
    else
    {
     Animation->Flags &= ~AnimatingCurves_ChoosingCurve;
     Animation->Flags |= AnimatingCurves_Animating;
    }
   }
   else
   {     
    Eat = true;
    
    LeftClick->Active = true;
    LeftClick->OriginalVerticesCaptured = false;
    LeftClick->LastMouseP = MouseP;
    
    entity_with_modify_witness CollisionEntityWitness = BeginEntityModify(Collision.Entity);
    curve *CollisionCurve = &Collision.Entity->Curve;
    curve_point_tracking_state *CollisionTracking = &CollisionCurve->PointTracking;
    
    if (Collision.Entity && (Event->Flags & PlatformEventFlag_Ctrl))
    {
     // NOTE(hbr): just move entity if ctrl is pressed
     LeftClick->Mode = EditorLeftClick_MovingEntity;
     LeftClick->TargetEntity = Collision.Entity;
    }
    else if ((Collision.Flags & Collision_CurveLine) && CollisionTracking->Active)
    {
     LeftClick->Mode = EditorLeftClick_MovingTrackingPoint;
     LeftClick->TargetEntity = Collision.Entity;
     
     f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveLinePointIndex, (CollisionCurve->LinePointCount- 1));
     SetTrackingPointFraction(&CollisionEntityWitness, Fraction);
    }
    else if (Collision.Flags & Collision_CurvePoint)
    {
     LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
     LeftClick->TargetEntity = Collision.Entity;
     LeftClick->CurvePointIndex = Collision.CurvePointIndex;
    }
    else if (Collision.Flags & Collision_CurveLine)
    {
     control_point_index Index = CurveLinePointIndexToControlPointIndex(CollisionCurve, Collision.CurveLinePointIndex);
     u32 InsertAt = Index.Index + 1;
     InsertControlPoint(&CollisionEntityWitness, MouseP, InsertAt);
     
     LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
     LeftClick->TargetEntity = Collision.Entity;
     LeftClick->CurvePointIndex = CurvePointIndexFromControlPointIndex(MakeControlPointIndex(InsertAt));
    }
    else if (Collision.Flags & Collision_TrackedPoint)
    {
     // NOTE(hbr): This shouldn't really happen, Collision_CurveLine should be set as well.
     // But if it does, just backout.
     LeftClick->Active = false;
    }
    else
    {
     if (Editor->SelectedEntity && Editor->SelectedEntity->Type == Entity_Curve)
     {
      LeftClick->TargetEntity = Editor->SelectedEntity;
     }
     else
     {
      temp_arena Temp = TempArena(0);
      
      entity *Entity = AllocEntity(Editor);
      entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
      
      string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EverIncreasingEntityCounter++);
      InitEntity(Entity, V2(0.0f, 0.0f), V2(1.0f, 1.0f), Rotation2DZero(), Name, 0);
      InitCurve(&EntityWitness, Editor->CurveDefaultParams);
      
      LeftClick->TargetEntity = Entity;
      
      EndEntityModify(EntityWitness);
      
      EndTemp(Temp);
     }
     Assert(LeftClick->TargetEntity);
     
     entity_with_modify_witness TargetEntityWitness = BeginEntityModify(LeftClick->TargetEntity);
     
     LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
     control_point_index Appended = AppendControlPoint(&TargetEntityWitness, MouseP);
     LeftClick->CurvePointIndex = CurvePointIndexFromControlPointIndex(Appended);
     
     EndEntityModify(TargetEntityWitness);
    }
    
    SelectEntity(Editor, LeftClick->TargetEntity);
    if (LeftClick->Mode == EditorLeftClick_MovingCurvePoint)
    {
     SelectControlPointFromCurvePointIndex(SafeGetCurve(LeftClick->TargetEntity),
                                           LeftClick->CurvePointIndex);
    }
    
    EndEntityModify(CollisionEntityWitness);
   }
  }
  
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_LeftMouseButton && LeftClick->Active)
  {
   Eat = true;
   LeftClick->Active = false;
  }
  
  if (!Eat && Event->Type == PlatformEvent_MouseMove && LeftClick->Active)
  {
   // NOTE(hbr): don't eat mouse move event
   
   entity *Entity = LeftClick->TargetEntity;
   entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
   
   v2 Translate = MouseP - LeftClick->LastMouseP;
   v2 TranslateLocal = (WorldToLocalEntityPosition(Entity, MouseP) -
                        WorldToLocalEntityPosition(Entity, LeftClick->LastMouseP));
   
   switch (LeftClick->Mode)
   {
    case EditorLeftClick_MovingTrackingPoint: {
     curve *Curve = SafeGetCurve(Entity);
     curve_point_tracking_state *Tracking = &Curve->PointTracking;
     f32 Fraction = Tracking->Fraction;
     
     MovePointAlongCurve(Curve, &TranslateLocal, &Fraction, true);
     MovePointAlongCurve(Curve, &TranslateLocal, &Fraction, false);
     
     SetTrackingPointFraction(&EntityWitness, Fraction);
    }break;
    
    case EditorLeftClick_MovingCurvePoint: {
     if (!LeftClick->OriginalVerticesCaptured)
     {
      curve *Curve = SafeGetCurve(Entity);
      arena *Arena = LeftClick->OriginalVerticesArena;
      ClearArena(Arena);
      LeftClick->OriginalLineVertices = CopyLineVertices(Arena, Curve->LineVertices);
      LeftClick->OriginalVerticesCaptured = true;
     }
     
     translate_curve_point_flags Flags = (TranslateCurvePoint_MatchBezierTwinDirection |
                                          TranslateCurvePoint_MatchBezierTwinLength);
     if (Event->Flags & PlatformEventFlag_Shift)
     {
      Flags &= ~TranslateCurvePoint_MatchBezierTwinDirection;
     }
     if (Event->Flags & PlatformEventFlag_Ctrl)
     {
      Flags &= ~TranslateCurvePoint_MatchBezierTwinLength;
     }
     SetCurvePoint(&EntityWitness, LeftClick->CurvePointIndex, MouseP, Flags);
    }break;
    
    case EditorLeftClick_MovingEntity: {
     Entity->P += Translate;
    }break;
   }
   
   LeftClick->LastMouseP = MouseP;
   
   EndEntityModify(EntityWitness);
  }
  
  //- right click events processing
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_RightMouseButton && !RightClick->Active)
  {
   Eat = true;
   
   collision Collision = CheckCollisionWith(MAX_ENTITY_COUNT,
                                            Editor->Entities,
                                            MouseP,
                                            RenderGroup->CollisionTolerance);
   
   RightClick->Active = true;
   RightClick->ClickP = MouseP;
   RightClick->CollisionAtP = Collision;
   
   entity *Entity = Collision.Entity;
   if (Collision.Flags & Collision_CurvePoint)
   {
    curve *Curve = SafeGetCurve(Entity);
    SelectControlPointFromCurvePointIndex(Curve, Collision.CurvePointIndex);
   }
   
   if (Entity)
   {
    SelectEntity(Editor, Entity);
   }
  }
  
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_RightMouseButton && RightClick->Active)
  {
   Eat = true;
   RightClick->Active = false;
   
   // NOTE(hbr): perform click action only if button was released roughly in the same place
   b32 ReleasedClose = (NormSquared(RightClick->ClickP - MouseP) <= Square(RenderGroup->CollisionTolerance));
   if (ReleasedClose)
   {
    collision *Collision = &RightClick->CollisionAtP;
    entity *Entity = Collision->Entity;
    curve *Curve = &Entity->Curve;
    
    if (Collision->Flags & Collision_TrackedPoint)
    {
     if (Curve->PointTracking.IsSplitting)
     {
      PerformBezierCurveSplit(Editor, Entity);
     }
    }
    else if (Collision->Flags & Collision_CurvePoint)
    {
     if (Collision->CurvePointIndex.Type == CurvePoint_ControlPoint)
     {
      entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
      RemoveControlPoint(&EntityWitness, Collision->CurvePointIndex.ControlPoint);
      EndEntityModify(EntityWitness);
     }
     else
     {
      SelectControlPointFromCurvePointIndex(Curve, Collision->CurvePointIndex);
     }
     SelectEntity(Editor, Entity);
    }
    else if (Entity)
    {
     DeallocEntity(Editor, Entity);
    }
    else
    {
     SelectEntity(Editor, 0);
    }
   }
  }
  
  //- camera control events processing
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_MiddleMouseButton)
  {
   Eat = true;
   MiddleClick->Active = true;
   MiddleClick->Rotate = (Event->Flags & PlatformEventFlag_Ctrl);
   MiddleClick->ClipSpaceLastMouseP = Event->ClipSpaceMouseP;
  }
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_MiddleMouseButton)
  {
   Eat = true;
   MiddleClick->Active = false;
  }
  if (!Eat && Event->Type == PlatformEvent_MouseMove && MiddleClick->Active)
  {
   // NOTE(hbr): don't eat mouse move event
   
   camera *Camera = &Editor->Camera;
   v2 FromP = Unproject(RenderGroup, MiddleClick->ClipSpaceLastMouseP);
   v2 ToP = MouseP;
   if (MiddleClick->Rotate)
   {
    v2 CenterP = Camera->P;
    if (NormSquared(FromP - CenterP) >= Square(RenderGroup->RotationRadius) &&
        NormSquared(ToP   - CenterP) >= Square(RenderGroup->RotationRadius))
    {
     v2 Rotation = Rotation2DFromMovementAroundPoint(FromP, ToP, CenterP);
     RotateCameraAround(Camera, Rotation2DInverse(Rotation), CenterP);
    }
   }
   else
   {
    v2 Translate = ToP - FromP;
    TranslateCamera(&Editor->Camera, -Translate);
   }
   MiddleClick->ClipSpaceLastMouseP = Event->ClipSpaceMouseP;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Scroll)
  {
   Eat = true;
   ZoomCamera(&Editor->Camera, Event->ScrollDelta);
  }
  
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_Tab)
  {
   Eat = true;
   Editor->HideUI = !Editor->HideUI;
  }
  
  //- shortcuts
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_N && (Event->Flags & PlatformEventFlag_Ctrl))
  {
   Eat = true;
   *NewProject = true;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_O && (Event->Flags & PlatformEventFlag_Ctrl))
  {
   Eat = true;
   *OpenFileDialog = true;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_S && (Event->Flags & PlatformEventFlag_Ctrl))
  {
   Eat = true;
   *SaveProject = true;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_S && (Event->Flags & PlatformEventFlag_Ctrl) && (Event->Flags & PlatformEventFlag_Shift))
  {
   Eat = true;
   *SaveProjectAs = true;
  }
  
  if (!Eat && ((Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_Escape) || (Event->Type == PlatformEvent_WindowClose)))
  {
   Eat = true;
   *QuitProject = true;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_Delete)
  {
   entity *Entity = Editor->SelectedEntity;
   entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
   
   if (Entity)
   {
    Eat = true;
    switch (Entity->Type)
    {
     case Entity_Curve: {
      curve *Curve = SafeGetCurve(Entity);
      if (IsControlPointSelected(Curve))
      {
       RemoveControlPoint(&EntityWitness, Curve->SelectedIndex);
      }
      else
      {
       DeallocEntity(Editor, Entity);
      }
     }break;
     
     case Entity_Image: {
      DeallocEntity(Editor, Entity);
     }break;
     
     case Entity_Count: InvalidPath;
    }
   }
   
   EndEntityModify(EntityWitness);
  }
  
  //- misc
  if (!Eat && Event->Type == PlatformEvent_FilesDrop)
  {
   Eat = true;
   v2 AtP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
   TryLoadImages(Editor, Event->FileCount, Event->FilePaths, AtP);
  }
  
  // TODO(hbr): remove this
#if BUILD_DEBUG
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_E)
  {
   Eat = true;
   AddNotificationF(Editor, Notification_Success, "hello dasdasdsad as dsa d sad sa ds dsa d  d sa dsa sd a dsa d a dsa d sa dad sasda");
  }
#endif
  
  //-
  if (Eat)
  {
   Event->Flags |= PlatformEventFlag_Eaten;
  }
 }
 
 // NOTE(hbr): these "sanity checks" are only necessary because ImGui captured any input that ends
 // with some possible ImGui interaction - e.g. we start by clicking in our editor but end releasing
 // on some ImGui window
 if (!Input->Pressed[PlatformKey_LeftMouseButton])
 {
  LeftClick->Active = false;
 }
 if (!Input->Pressed[PlatformKey_RightMouseButton])
 {
  RightClick->Active = false;
 }
 if (!Input->Pressed[PlatformKey_MiddleMouseButton])
 {
  MiddleClick->Active = false;
 }
 
 //- render line shadow when moving
 {  
  entity *Entity = LeftClick->TargetEntity;
  if (LeftClick->Active && LeftClick->OriginalVerticesCaptured && !Entity->Curve.Params.LineDisabled)
  {
   v4 ShadowColor = Entity->Curve.Params.LineColor;
   ShadowColor.A *= 0.15f;
   
   // TODO(hbr): This is a little janky we call this function everytime
   // Either solve it somehow or remove this function and do it more manually
   mat3 Model = ModelTransform(Entity->P, Entity->Rotation, Entity->Scale);
   SetTransform(RenderGroup, Model, Cast(f32)Entity->SortingLayer);
   
   PushVertexArray(RenderGroup,
                   LeftClick->OriginalLineVertices.Vertices,
                   LeftClick->OriginalLineVertices.VertexCount,
                   LeftClick->OriginalLineVertices.Primitive,
                   ShadowColor, GetCurvePartZOffset(CurvePart_LineShadow));
  }
 }
 
 //- render "remove" indicator
 if (RightClick->Active)
 {
  v4 Color = V4(0.5f, 0.5f, 0.5f, 0.3f);
  // TODO(hbr): Again, zoffset of this thing is wrong
  PushCircle(RenderGroup, RightClick->ClickP, Rotation2DZero(), V2(1, 1), RenderGroup->CollisionTolerance, Color, 0.0f);
 }
 
 //- render rotation indicator
 if (MiddleClick->Active && MiddleClick->Rotate)
 {
  camera *Camera = &Editor->Camera;
  f32 Radius = RenderGroup->RotationRadius;
  v4 Color = RGBA_Color(30, 56, 87, 80);
  f32 OutlineThickness = 0.1f * Radius;
  v4 OutlineColor = RGBA_Color(255, 255, 255, 24);
  // TODO(hbr): ZOffset here is possibly wrong, it should be something on top of everything
  // instead
  PushCircle(RenderGroup,
             Camera->P,
             Rotation2DZero(),
             V2(1, 1),
             Radius - OutlineThickness,
             Color, 0.0f,
             OutlineThickness, OutlineColor);
 }
}

internal void
RenderParametricExpressionUI(parametric_equation_expr *Expr)
{
 UI_Id(Cast(u32)(Cast(u64)Expr))
 {
  if (Expr)
  {
   temp_arena Temp = TempArena(0);
   
   string TypeStr = {};
   switch (Expr->Type)
   {
    case ParametricEquationExpr_Negation: {
     TypeStr = StrLit("Negation");
    }break;
    
    case ParametricEquationExpr_BinaryOp: {
     char const *OpStr = 0;
     switch (Expr->Binary.Type)
     {
      case ParametricEquationOp_Pow: {OpStr = "^";}break;
      case ParametricEquationOp_Plus: {OpStr = "+";}break;
      case ParametricEquationOp_Minus: {OpStr = "-";}break;
      case ParametricEquationOp_Mult: {OpStr = "*";}break;
      case ParametricEquationOp_Div: {OpStr = "/";}break;
     }
     TypeStr = StrF(Temp.Arena, "BinaryOp(%s)", OpStr);
    }break;
    
    case ParametricEquationExpr_Number: {
     TypeStr = StrF(Temp.Arena, "Number(%f)", Expr->Number.Number);
    }break;
    
    case ParametricEquationExpr_Application: {
     string Identifier = {};
     switch (Expr->Application.Identifier.Type)
     {
      case ParametricEquationIdentifier_Sin: {Identifier=StrLit("sin");}break;
      case ParametricEquationIdentifier_Cos: {Identifier=StrLit("Cos");}break;
      case ParametricEquationIdentifier_Tan: {Identifier=StrLit("Tan");}break;
      case ParametricEquationIdentifier_Sqrt: {Identifier=StrLit("Sqrt");}break;
      case ParametricEquationIdentifier_Log: {Identifier=StrLit("Log");}break;
      case ParametricEquationIdentifier_Log10: {Identifier=StrLit("Log10");}break;
      case ParametricEquationIdentifier_Floor: {Identifier=StrLit("Floor");}break;
      case ParametricEquationIdentifier_Ceil: {Identifier=StrLit("Ceil");}break;
      case ParametricEquationIdentifier_Round: {Identifier=StrLit("Round");}break;
      case ParametricEquationIdentifier_Pow: {Identifier=StrLit("Pow");}break;
      case ParametricEquationIdentifier_Tanh: {Identifier=StrLit("Tanh");}break;
      case ParametricEquationIdentifier_Exp: {Identifier=StrLit("Exp");}break;
      case ParametricEquationIdentifier_Pi: {Identifier=StrLit("Pi");}break;
      case ParametricEquationIdentifier_Tau: {Identifier=StrLit("Tau");}break;
      case ParametricEquationIdentifier_Euler: {Identifier=StrLit("Euler");}break;
      case ParametricEquationIdentifier_Custom: {Identifier=Expr->Application.Identifier.Custom;}break;
     }
     TypeStr = StrF(Temp.Arena, "Application(%S)", Identifier);
    }break;
   }
   UI_Text(TypeStr);
   
   switch (Expr->Type)
   {
    case ParametricEquationExpr_Negation: {
     parametric_equation_expr_negation Negation = Expr->Negation;
     ImGui::SetNextItemOpen(true);
     if (UI_BeginTreeF("Sub"))
     {
      RenderParametricExpressionUI(Negation.SubExpr);
      UI_EndTree();
     }
    }break;
    
    case ParametricEquationExpr_BinaryOp: {
     parametric_equation_expr_binary_op Binary = Expr->Binary;
     ImGui::SetNextItemOpen(true);
     if (UI_BeginTreeF("Left"))
     {
      RenderParametricExpressionUI(Binary.Left);
      UI_EndTree();
     }
     ImGui::SetNextItemOpen(true);
     if (UI_BeginTreeF("Right"))
     {
      RenderParametricExpressionUI(Binary.Right);
      UI_EndTree();
     }
    }break;
    
    case ParametricEquationExpr_Number: {
    }break;
    
    case ParametricEquationExpr_Application: {
     parametric_equation_expr_application Application = Expr->Application;
     for (u32 ArgIndex = 0;
          ArgIndex < Application.ArgCount;
          ++ArgIndex)
     {
      ImGui::SetNextItemOpen(true);
      if (UI_BeginTreeF("%u", ArgIndex))
      {
       RenderParametricExpressionUI(Application.ArgExprs + ArgIndex);
       UI_EndTree();
      }
     }
    }break;
   }
   
   EndTemp(Temp);
  }
 }
}

internal void
RenderParametricEquationUI(void)
{
 if (UI_BeginWindowF(0, 0, "Parametric Equation"))
 {
  local char SourceCodeBuffer[1024];
  UI_InputTextF(SourceCodeBuffer, ArrayCount(SourceCodeBuffer), "Parametric Equation");
  string Equation = StrFromCStr(SourceCodeBuffer);
  
  temp_arena Temp = TempArena(0);
  
  parametric_equation_parse_result Parse = ParametricEquationParse(Temp.Arena, Equation);
  RenderParametricExpressionUI(Parse.ParsedExpr);
  
  EndTemp(Temp);
  
  UI_EndWindow();
 }
}

internal void
EditorUpdateAndRender_(editor_memory *Memory, platform_input *Input, struct render_frame *Frame)
{
 Platform = Memory->PlatformAPI;
 
 editor *Editor = Memory->Editor;
 if (!Editor)
 {
  Editor = Memory->Editor = PushStruct(Memory->PermamentArena, editor);
  InitEditor(Editor, Memory);
 }
 
 render_group RenderGroup_;
 render_group *RenderGroup = &RenderGroup_;
 {
  camera *Camera = &Editor->Camera;
  RenderGroup_ = BeginRenderGroup(Frame,
                                  Camera->P, Camera->Rotation, Camera->Zoom,
                                  Editor->BackgroundColor,
                                  Editor->CollisionToleranceClip,
                                  Editor->RotationRadiusClip);
  Editor->RenderGroup = RenderGroup;
 }
 
 ProcessAsyncEvents(Editor);
 
 //- process events and update click events
 b32 NewProject = false;
 b32 OpenFileDialog = false;
 b32 SaveProject = false;
 b32 SaveProjectAs = false;
 b32 QuitProject = false;
 b32 AnimateCurves = false;
 
 ProcessInputEvents(Editor, Input, RenderGroup,
                    &NewProject, &OpenFileDialog, &SaveProject,
                    &SaveProjectAs, &QuitProject, &AnimateCurves);
 
 UpdateCamera(Editor, Input);
 UpdateFrameStats(Editor, Input);
 
 if (!Editor->HideUI)
 {
  UpdateRenderSelectedEntityUI(Editor);
  UpdateRenderMenuBarUI(Editor, &NewProject, &OpenFileDialog, &SaveProject, &SaveProjectAs, &QuitProject, &AnimateCurves);
  UpdateRenderEntityListUI(Editor);
  UpdateRenderDiagnosticsUI(Editor, Input);
  UpdateAndRenderNotifications(Editor, Input, RenderGroup);
  UpdateAndRenderAnimatingCurvesUI(Editor);
  
  // TODO(hbr): temp
  RenderParametricEquationUI();
  
#if BUILD_DEBUG
  ImGui::ShowDemoWindow();
#endif
 }
 
 if (OpenFileDialog)
 {
  temp_arena Temp = TempArena(0);
  camera *Camera = &Editor->Camera;
  platform_file_dialog_result OpenDialog = Platform.OpenFileDialog(Temp.Arena);
  TryLoadImages(Editor, OpenDialog.FileCount, OpenDialog.FilePaths, Camera->P);
  EndTemp(Temp);
 }
 
 if (QuitProject)
 {
  Input->QuitRequested = true;
 }
 
 if (AnimateCurves)
 {
  BeginAnimatingCurves(&Editor->AnimatingCurves);
 }
 
 UpdateAndRenderEntities(Editor, RenderGroup);
 UpdateAndRenderAnimatingCurves(Editor, Input, RenderGroup);
 
#if BUILD_DEBUG
 Input->RefreshRequested = true;
#endif
}

DLL_EXPORT
EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender)
{
 EditorUpdateAndRender_(Memory, Input, Frame);
}

IMGUI_INIT_FUNC();
IMGUI_NEW_FRAME_FUNC();
IMGUI_RENDER_FUNC();
IMGUI_MAYBE_CAPTURE_INPUT_FUNC();

DLL_EXPORT
EDITOR_GET_IMGUI_BINDINGS(EditorGetImGuiBindings)
{
 imgui_bindings Bindings = {};
 Bindings.Init = ImGuiInit;
 Bindings.NewFrame = ImGuiNewFrame;
 Bindings.Render = ImGuiRender;
 Bindings.MaybeCaptureInput = ImGuiMaybeCaptureInput;
 
 return Bindings;
}
