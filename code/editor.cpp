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
#include "editor_debug.cpp"
#include "editor_camera.cpp"

platform_api Platform;

// NOTE(hbr): this is code that is

internal void
MovePointAlongCurve(curve *Curve, v2 *TranslateInOut, f32 *PointFractionInOut, b32 Forward)
{
 v2 Translate = *TranslateInOut;
 f32 Fraction = *PointFractionInOut;
 
 u32 SampleCount = Curve->CurveSampleCount;
 v2 *Samples = Curve->CurveSamples;
 f32 DeltaFraction = 1.0f / (SampleCount - 1);
 
 f32 PointIndexFloat = Fraction * (SampleCount - 1);
 u32 PointIndex = Cast(u32)(Forward ? FloorF32(PointIndexFloat) : CeilF32(PointIndexFloat));
 PointIndex = ClampTop(PointIndex, SampleCount - 1);
 
 i32 DirSign = (Forward ? 1 : -1);
 
 b32 Moving = true;
 while (Moving)
 {
  u32 PrevPointIndex = PointIndex;
  PointIndex += DirSign;
  if (PointIndex >= SampleCount)
  {
   Moving = false;
  }
  
  if (Moving)
  {
   v2 AlongCurve = Samples[PointIndex] - Samples[PrevPointIndex];
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
CheckCollisionWithMultiLine(v2 LocalAtP, v2 *CurveSamples, u32 PointCount, f32 Width, f32 Tolerance)
{
 multiline_collision Result = {};
 
 f32 CheckWidth = Width + Tolerance;
 u32 MinDistancePointIndex = 0;
 f32 MinDistance = 1.0f;
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
        v2 *CurveSamples = Points + PointIndex;
        u32 PointCount = IterationCount - Iteration;
        multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP, CurveSamples, PointCount,
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
                                                             Curve->CurveSamples, Curve->CurveSampleCount,
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
   u32 Generation = Entity->Generation;
   
   StructZero(Entity);
   
   Entity->Arena = EntityArena;
   Entity->Curve.DegreeLowering.Arena = DegreeLoweringArena;
   Entity->Curve.ParametricResources.Arena = ParametricArena;
   Entity->Generation = Generation;
   
   Entity->Flags |= EntityFlag_Active;
   ClearArena(Entity->Arena);
   
   break;
  }
 }
 
 return Entity;
}

internal void
DeallocEntity(editor *Editor, entity *Entity)
{
 if (Entity->Type == Entity_Image)
 {
  image *Image = &Entity->Image;
  DeallocateTextureIndex(&Editor->Assets, Image->TextureIndex);
 }
 
 Entity->Flags &= ~EntityFlag_Active;
 ++Entity->Generation;
}

internal void
SelectEntity(editor *Editor, entity *Entity)
{
 entity *SelectedEntity = EntityFromId(Editor->SelectedEntityId);
 if (SelectedEntity)
 {
  SelectedEntity->Flags &= ~EntityFlag_Selected;
 }
 if (Entity)
 {
  Entity->Flags |= EntityFlag_Selected;
 }
 Editor->SelectedEntityId = MakeEntityId(Entity);
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
PerformBezierCurveSplit(entity_with_modify_witness *EntityWitness, editor *Editor)
{
 temp_arena Temp = TempArena(0);
 
 entity *Entity = EntityWitness->Entity;
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
// TODO(hbr): This function is kind of broken - for example, we can modify the curve while we are in the "lowering degree failed" mode. While
// this on its own can be useful, because one can fit the curve more tighly to the original one, when we add some control points due to modyfing this,
// and then go back to tweaking the Middle_T value to tweak the middle point when fixing "failed lowering", the point that we tweak might no longer be
// the point that was originally there. In other words, saved control point index got invalidated.
internal void
LowerBezierCurveDegree(entity_with_modify_witness *EntityWitness)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 
 u32 PointCount = Curve->ControlPointCount;
 if (PointCount > 0)
 {
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
   
   Lowering->SavedLineVertices = CopyLineVertices(Lowering->Arena, Curve->CurveVertices);
   
   f32 T = 0.5f;
   Lowering->LowerDegree = LowerDegree;
   Lowering->MixParameter = T;
   LowerPoints[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.P_I, LowerDegree.P_II, T);
   LowerWeights[LowerDegree.MiddlePointIndex] = Lerp(LowerDegree.W_I, LowerDegree.W_II, T);
  }
  
  // TODO(hbr): refactor this, it only has to be here because we still modify control points above
  BezierCubicCalculateAllControlPoints(PointCount - 1, LowerPoints, LowerBeziers);
  SetCurveControlPoints(EntityWitness, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(entity_with_modify_witness *EntityWitness)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
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
 
 SetCurveControlPoints(EntityWitness,
                       ControlPointCount + 1,
                       ElevatedControlPoints,
                       ElevatedControlPointWeights,
                       ElevatedCubicBezierPoints);
 
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
SplitCurveOnControlPoint(entity_with_modify_witness *EntityWitness, editor *Editor)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  u32 HeadPointCount = Curve->SelectedIndex.Index + 1;
  u32 TailPointCount = Curve->ControlPointCount - Curve->SelectedIndex.Index;
  
  entity *HeadEntity = Entity;
  entity *TailEntity = AllocEntity(Editor);
  
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

struct rendering_entity_handle
{
 entity *Entity;
 render_group *RenderGroup;
};
// TODO(hbr): I would ideally get rid of these calls
internal rendering_entity_handle
BeginRenderingEntity(entity *Entity, render_group *RenderGroup)
{
 rendering_entity_handle Handle = {};
 Handle.Entity = Entity;
 Handle.RenderGroup = RenderGroup;
 
 mat3 Model = ModelTransform(Entity->P, Entity->Rotation, Entity->Scale);
 SetTransform(RenderGroup, Model, Cast(f32)Entity->SortingLayer);
 
 return Handle;
}
internal void
EndRenderingEntity(rendering_entity_handle Handle)
{
 ResetTransform(Handle.RenderGroup);
}

internal void
UpdateAndRenderPointTracking(rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
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
   
   PushCircle(RenderGroup,
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
    
    PushVertexArray(RenderGroup,
                    Tracking->LineVerticesPerIteration[Iteration].Vertices,
                    Tracking->LineVerticesPerIteration[Iteration].VertexCount,
                    Tracking->LineVerticesPerIteration[Iteration].Primitive,
                    IterationColor, GetCurvePartZOffset(CurvePart_DeCasteljauAlgorithmLines));
    
    for (u32 I = 0; I < IterationCount - Iteration; ++I)
    {
     v2 Point = Tracking->Intermediate.P[PointIndex];
     PushCircle(RenderGroup,
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
UpdateAndRenderDegreeLowering(rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
 if (Entity->Type == Entity_Curve)
 {
  curve *Curve = SafeGetCurve(Entity);
  curve_params *CurveParams = &Curve->Params;
  curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  if (Lowering->Active)
  {
   Assert(CurveParams->Type == Curve_Bezier);
   
   b32 IsDegreeLoweringWindowOpen = true;
   b32 MixChanged = false;
   b32 Ok = false;
   b32 Revert = false;
   
   if (UI_BeginWindowF(&IsDegreeLoweringWindowOpen, 0, "Degree Lowering"))
   {          
    // TODO(hbr): Add that to ui library
    ImGui::TextWrapped("Degree lowering failed. Tweak the middle point to fit the curve manually.");
    UI_SeparatorText(NilStr);
    MixChanged = UI_SliderFloatF(&Lowering->MixParameter, 0.0f, 1.0f, "Middle Point Mix");
    
    Ok = UI_ButtonF("OK");
    UI_SameRow();
    Revert = UI_ButtonF("Revert");
   }
   UI_EndWindow();
   
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
   v4 Color = Curve->Params.LineColor;
   Color.A *= 0.5f;
   
   PushVertexArray(RenderGroup,
                   Lowering->SavedLineVertices.Vertices,
                   Lowering->SavedLineVertices.VertexCount,
                   Lowering->SavedLineVertices.Primitive,
                   Color,
                   GetCurvePartZOffset(CurvePart_LineShadow));
  }
  
  EndEntityModify(EntityWitness);
 }
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
 
 Choosing->Curves[Choosing->ChoosingCurveIndex] = MakeEntityId(Curve);
 if (EntityFromId(Choosing->Curves[0]) == 0)
 {
  Choosing->ChoosingCurveIndex = 0;
 }
 else if (EntityFromId(Choosing->Curves[1]) == 0)
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
  entity *Entity = AllocEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  InitEntityFromEntity(&EntityWitness, &Merging->MergeEntity);
  
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
InitEditor(editor *Editor, editor_memory *Memory)
{
 Editor->BackgroundColor = Editor->DefaultBackgroundColor = RGBA_Color(21, 21, 21);
 Editor->CollisionToleranceClip = 0.04f;
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
 Editor->CurveDefaultParams.SamplesPerControlPoint = 50;
 Editor->CurveDefaultParams.TotalSamples = 1000;
 Editor->CurveDefaultParams.Parametric.MaxT = 1.0f;
 Editor->CurveDefaultParams.Parametric.X_Equation = &NilExpr;
 Editor->CurveDefaultParams.Parametric.Y_Equation = &NilExpr;
 Editor->CurveDefaultParams.B_Spline.KnotPointRadius = 0.010f;
 Editor->CurveDefaultParams.B_Spline.KnotPointColor = RGBA_Color(138, 0, 0, 148);
 
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
  AllocEntityResources(Entity);
 }
 
 Editor->EntityListWindow = true;
 // TODO(hbr): Change to false by default
 Editor->DiagnosticsWindow = true;
 Editor->SelectedEntityWindow = true;
 
 Editor->LeftClick.OriginalVerticesArena = AllocArena();
 
 AllocEntityResources(&Editor->MergingCurves.MergeEntity);
 
 {
  entity *Entity = AllocEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), StrLit("special"), 0);
  curve_params Params = Editor->CurveDefaultParams;
  Params.Type = Curve_Bezier;
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
UpdateAndRenderAnimatingCurves(animating_curves_state *Animation, platform_input *Input, render_group *RenderGroup)
{
 entity *Entity0 = EntityFromId(Animation->Choose2Curves.Curves[0]);
 entity *Entity1 = EntityFromId(Animation->Choose2Curves.Curves[1]);
 
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
  
  v2 *CurveSamples0 = Curve0->CurveSamples;
  v2 *CurveSamples1 = Curve1->CurveSamples;
  
  u32 CurveSampleCount0 = Curve0->CurveSampleCount;
  u32 CurveSampleCount1 = Curve1->CurveSampleCount;
  
  // TODO(hbr): Multithread this
  arena *Arena = Animation->Arena;
  ClearArena(Arena);
  
  // TODO(hbr): Use max instead
  u32 CurveSampleCount = Min(Curve0->CurveSampleCount, Curve1->CurveSampleCount);
  v2 *CurveSamples = PushArrayNonZero(Arena, CurveSampleCount, v2);
  for (u32 LinePointIndex = 0;
       LinePointIndex < CurveSampleCount;
       ++LinePointIndex)
  {
   u32 LinePointIndex0 = ClampTop(CurveSampleCount0 * LinePointIndex / (CurveSampleCount - 1), CurveSampleCount0 - 1);
   u32 LinePointIndex1 = ClampTop(CurveSampleCount1 * LinePointIndex / (CurveSampleCount - 1), CurveSampleCount1 - 1);
   
   if (IsCurveReversed(Entity0)) LinePointIndex0 = (CurveSampleCount0-1 - LinePointIndex0);
   if (IsCurveReversed(Entity1)) LinePointIndex1 = (CurveSampleCount1-1 - LinePointIndex1);
   
   v2 PointLocal0 = Curve0->CurveSamples[LinePointIndex0];
   v2 PointLocal1 = Curve1->CurveSamples[LinePointIndex1];
   
   v2 Point0 = LocalEntityPositionToWorld(Entity0, PointLocal0);
   v2 Point1 = LocalEntityPositionToWorld(Entity1, PointLocal1);
   
   v2 Point  = Lerp(Point0, Point1, MappedT);
   CurveSamples[LinePointIndex] = Point;
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
  
  vertex_array LineVertices = ComputeVerticesOfThickLine(Arena, CurveSampleCount, CurveSamples, LineWidth, false);
  PushVertexArray(RenderGroup,
                  LineVertices.Vertices, LineVertices.VertexCount, LineVertices.Primitive,
                  LineColor,
                  ZOffset);
 }
}

internal void
RenderEntity(rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   curve_params *CurveParams = &Curve->Params;
   
   if (!CurveParams->LineDisabled)
   {
    PushVertexArray(RenderGroup,
                    Curve->CurveVertices.Vertices,
                    Curve->CurveVertices.VertexCount,
                    Curve->CurveVertices.Primitive,
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
   
   if (AreCurvePointsVisible(Curve))
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
   
   if (Are_B_SplineKnotsVisible(Curve))
   {
    b_spline_knots Knots = Curve->B_SplineKnots;
    v2 *PartitionKnotPoints = Curve->B_SplinePartitionKnotPoints;
    b_spline_params B_Spline = CurveParams->B_Spline;
    point_info KnotPointInfo = Get_B_SplineKnotPointInfo(Entity);
    
    for (u32 KnotIndex = 0;
         KnotIndex < Knots.PartitionSize;
         ++KnotIndex)
    {
     v2 Knot = PartitionKnotPoints[KnotIndex];
     PushCircle(RenderGroup,
                Entity->P + Knot,
                Entity->Rotation,
                Entity->Scale,
                KnotPointInfo.Radius,
                KnotPointInfo.Color,
                GetCurvePartZOffset(CurvePart_B_SplineKnot),
                KnotPointInfo.OutlineThickness,
                KnotPointInfo.OutlineColor);
    }
   }
   
  } break;
  
  case Entity_Image: {
   image *Image = &Entity->Image;
   PushImage(RenderGroup, Image->Dim, Image->TextureIndex->Index);
  }break;
  
  case Entity_Count: InvalidPath; break;
 }
}

internal void
UpdateAndRenderEntities(u32 EntityCount, entity *Entities, render_group *RenderGroup)
{
 for (u32 EntityIndex = 0;
      EntityIndex < EntityCount;
      ++EntityIndex)
 {
  entity *Entity = Entities + EntityIndex;
  if ((Entity->Flags & EntityFlag_Active))
  {
   if (IsEntityVisible(Entity))
   {
    rendering_entity_handle Handle = BeginRenderingEntity(Entity, RenderGroup);
    
    RenderEntity(Handle);
    UpdateAndRenderDegreeLowering(Handle);
    UpdateAndRenderPointTracking(Handle);
    
    EndRenderingEntity(Handle);
   }
  }
 }
}

struct update_parametric_curve_var
{
 b32 Changed;
 b32 Remove;
 f32 Value;
};
enum update_parametric_curve_var_mode
{
 UpdateParametricCurveVar_Static,
 UpdateParametricCurveVar_Dynamic,
};
internal update_parametric_curve_var
UpdateAndRenderParametricCurveVar(arena *Arena,
                                  parametric_curve_var *Var,
                                  b32 ForceRecomputeEquation,
                                  string *VarNames,
                                  f32 *VarValues,
                                  u32 VarCount,
                                  update_parametric_curve_var_mode Mode)
{
 b32 VarChanged = false;
 
 b32 VarEquationChanged = false;
 if (Var->EquationMode)
 {
  VarEquationChanged = UI_InputTextF(Var->VarEquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, "##Equation").Changed;
 }
 else
 {
  VarChanged |= UI_DragFloat(&Var->DragValue, 0, 0, 0, StrLit("##Drag"));
 }
 
 if (ForceRecomputeEquation || VarEquationChanged)
 {
  VarChanged = true;
  
  string VarEquationInput = StrFromCStr(Var->VarEquationBuffer);
  
  parametric_equation_eval_result VarEval = ParametricEquationEval(Arena, VarEquationInput, VarCount, VarNames, VarValues);
  Var->EquationFail = VarEval.Fail;
  Var->EquationErrorMessage = VarEval.ErrorMessage;
  Var->EquationValue = VarEval.Value;
 }
 
 if (Var->EquationMode)
 {
  Var->DragValue = Var->EquationValue;
 }
 
 b32 Remove = false;
 if (Mode == UpdateParametricCurveVar_Dynamic)
 {
  UI_SameRow();
  Remove = UI_ButtonF("-");
 }
 
 UI_SameRow();
 b32 SwapMode = UI_ButtonF(Var->EquationMode ? "D" : "C");
 
 if (Var->EquationMode && Var->EquationFail)
 {
  UI_ColoredText(RedColor)
  {
   UI_SameRow();
   UI_Text(Var->EquationErrorMessage);
  }
 }
 
 if (SwapMode)
 {
  VarChanged = true;
  Var->EquationMode = !Var->EquationMode;
 }
 
 if (Remove)
 {
  VarChanged = true;
 }
 
 update_parametric_curve_var Result = {};
 Result.Changed = VarChanged;
 Result.Remove = Remove;
 Result.Value = (Var->EquationMode ? Var->EquationValue : Var->DragValue);
 
 return Result;
}

internal void
UpdateAndRenderSelectedEntityUI(editor *Editor)
{
 entity *Entity = EntityFromId(Editor->SelectedEntityId);
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 b32 DeleteEntity = false;
 b32 CrucialEntityParamChanged = false;
 
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
                                 0, "Name").Input;
    
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
    
    b32 Hidden = !IsEntityVisible(Entity);
    UI_Checkbox(&Hidden, StrLit("Hidden"));
    SetEntityVisibility(Entity, Hidden);
    
    DeleteEntity = UI_Button(StrLit("Delete"));
    
    if (UI_Button(StrLit("Focus")))
    {
     FocusCameraOnEntity(Editor, Entity);
    }
    
    if (UI_Button(StrLit("Copy")))
    {
     DuplicateEntity(Entity, Editor);
    }
    
    if (Curve)
    {
     // TODO(hbr): Most of those buttons shouldn't be here in case of Entity_Image.
     // But they will not be buttons in the first place as well.
     UI_Disabled(!IsControlPointSelected(Curve))
     {
      if (UI_Button(StrLit("Split on Control Point")))
      {
       SplitCurveOnControlPoint(&EntityWitness, Editor);
      }
     }
     
     UI_Disabled(!IsRegularBezierCurve(Curve))
     {
      if (UI_Button(StrLit("Elevate Degree")))
      {
       // TODO(hbr): Work on this function
       ElevateBezierCurveDegree(&EntityWitness);
      }
      
      if (UI_Button(StrLit("Lower Degree")))
      {
       // TODO(hbr): Work on this function
       LowerBezierCurveDegree(&EntityWitness);
      }
     }
    }
   }
   
   if (Curve)
   {
    curve_params *CurveParams = &Curve->Params;
    curve_params *DefaultParams = &Editor->CurveDefaultParams;
    
    UI_SeparatorTextF("Curve");
    UI_LabelF("Curve")
    {
     CrucialEntityParamChanged |= UI_ComboF(SafeCastToPtr(CurveParams->Type, u32), Curve_Count, CurveTypeNames, "Interpolation");
     if (ResetCtxMenu(StrLit("InterpolationReset")))
     {
      CurveParams->Type = DefaultParams->Type;
      CrucialEntityParamChanged = true;
     }
     
     switch (CurveParams->Type)
     {
      case Curve_Polynomial: {
       polynomial_interpolation_params *Polynomial = &CurveParams->Polynomial;
       
       CrucialEntityParamChanged |= UI_ComboF(SafeCastToPtr(Polynomial->Type, u32), PolynomialInterpolation_Count, PolynomialInterpolationTypeNames, "Variant");
       if (ResetCtxMenu(StrLit("PolynomialReset")))
       {
        Polynomial->Type = DefaultParams->Polynomial.Type;
        CrucialEntityParamChanged = true;
       }
       
       CrucialEntityParamChanged |= UI_ComboF(SafeCastToPtr(Polynomial->PointSpacing, u32), PointSpacing_Count, PointSpacingNames, "Point Spacing");
       if (ResetCtxMenu(StrLit("PointSpacingReset")))
       {
        Polynomial->PointSpacing = DefaultParams->Polynomial.PointSpacing;
        CrucialEntityParamChanged = true;
       }
      }break;
      
      case Curve_CubicSpline: {
       CrucialEntityParamChanged |= UI_ComboF(SafeCastToPtr(CurveParams->CubicSpline, u32), CubicSpline_Count, CubicSplineNames, "Variant");
       if (ResetCtxMenu(StrLit("SplineReset")))
       {
        CurveParams->CubicSpline = DefaultParams->CubicSpline;
        CrucialEntityParamChanged = true;
       }
      }break;
      
      case Curve_Bezier: {
       CrucialEntityParamChanged |= UI_ComboF(SafeCastToPtr(CurveParams->Bezier, u32), Bezier_Count, BezierNames, "Variant");
       if (ResetCtxMenu(StrLit("BezierReset")))
       {
        CurveParams->Bezier = DefaultParams->Bezier;
        CrucialEntityParamChanged = true;
       }
      }break;
      
      case Curve_Parametric: {
       ImGui::SetNextItemOpen(true, ImGuiCond_Once);
       if (UI_BeginTreeF("Equation"))
       {
        parametric_curve_params *Parametric = &CurveParams->Parametric;
        parametric_curve_resources *Resources = &Curve->ParametricResources;
        arena *EquationArena = Resources->Arena;
        temp_arena Temp = TempArena(EquationArena);
        b32 EquationChanged = false;
        
        b32 ArenaCleared = false;
        if (Resources->ShouldClearArena)
        {
         ArenaCleared = true;
         Resources->ShouldClearArena = false;
         ClearArena(EquationArena);
        }
        
        //- additional vars
        UI_TextF("Additional Vars");
        UI_SameRow();
        UI_Disabled(!HasFreeAdditionalVar(Resources))
        {
         if (UI_ButtonF("+"))
         {
          ActivateNewAdditionalVar(Resources);
         }
        }
        
        string *VarNames = PushArrayNonZero(Temp.Arena, Resources->AdditionalVarCount, string);
        f32 *VarValues = PushArrayNonZero(Temp.Arena, Resources->AdditionalVarCount, f32);
        u32 VarCount = 0;
        b32 VarChanged = false;
        
        for (u32 VarIndex = 0;
             VarIndex < Resources->AdditionalVarCount;
             ++VarIndex)
        {
         parametric_curve_var *Var = Resources->AdditionalVars + VarIndex;
         
         UI_PushId(Var->Id);
         
         ui_input_result VarName = UI_InputTextF(Var->VarNameBuffer, MAX_VAR_NAME_BUFFER_LENGTH, 3, "##Var");
         if (VarName.Changed)
         {
          VarChanged = true;
         }
         UI_SameRow();
         UI_TextF(" := ");
         UI_SameRow();
         
         update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena, Var,
                                                                                ArenaCleared || VarChanged,
                                                                                VarNames, VarValues, VarCount,
                                                                                UpdateParametricCurveVar_Dynamic);
         VarChanged |= Update.Changed;
         if (Update.Remove)
         {
          DeactiveAdditionalVar(Resources, VarIndex);
         }
         else
         {
          VarValues[VarCount] = Update.Value;
          VarNames[VarCount] = VarName.Input;
          ++VarCount;
         }
         
         UI_PopId();
        }
        
        //- min/max bounds
        UI_Label(StrLit("t_min"))
        {
         UI_TextF("t_min := ");
         UI_SameRow();
         update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena,
                                                                                &Resources->MinT_Var,
                                                                                ArenaCleared || VarChanged,
                                                                                VarNames, VarValues, VarCount,
                                                                                UpdateParametricCurveVar_Static);
         EquationChanged |= Update.Changed;
         Parametric->MinT = Update.Value;
        }
        
        UI_Label(StrLit("t_max"))
        {
         UI_TextF("t_max := ");
         UI_SameRow();
         update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena,
                                                                                &Resources->MaxT_Var,
                                                                                ArenaCleared || VarChanged,
                                                                                VarNames, VarValues, VarCount,
                                                                                UpdateParametricCurveVar_Static);
         EquationChanged |= Update.Changed;
         Parametric->MaxT = Update.Value;
        }
        
        //- (x,y) equations
        UI_TextF("x(t)  := ");
        UI_SameRow();
        ui_input_result X_Equation = UI_InputTextF(Resources->X_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, "##x(t)");
        if (ArenaCleared || VarChanged || X_Equation.Changed)
        {
         EquationChanged = true;
         
         parametric_equation_parse_result X_Parse = ParametricEquationParse(EquationArena, X_Equation.Input, VarCount, VarNames, VarValues);
         Parametric->X_Equation = X_Parse.ParsedExpr;
         Resources->X_Fail = X_Parse.Fail;
         Resources->X_ErrorMessage = X_Parse.ErrorMessage;
        }
        if (Resources->X_Fail)
        {
         UI_SameRow();
         UI_ColoredText(RedColor)
         {
          UI_Text(Resources->X_ErrorMessage);
         }
        }
        
        UI_TextF("y(t)  := ");
        UI_SameRow();
        ui_input_result Y_Equation = UI_InputTextF(Resources->Y_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, "##y(t)");
        if (ArenaCleared || VarChanged || Y_Equation.Changed)
        {
         EquationChanged = true;
         
         parametric_equation_parse_result Y_Parse = ParametricEquationParse(EquationArena, Y_Equation.Input, VarCount, VarNames, VarValues);
         Parametric->Y_Equation = Y_Parse.ParsedExpr;
         Resources->Y_Fail = Y_Parse.Fail;
         Resources->Y_ErrorMessage = Y_Parse.ErrorMessage;
        }
        if (Resources->Y_Fail)
        {
         UI_SameRow();
         UI_ColoredText(RedColor)
         {
          UI_Text(Resources->Y_ErrorMessage);
         }
        }
        
        if (EquationChanged)
        {
         CrucialEntityParamChanged = true;
        }
        
        if (EquationChanged && !ArenaCleared)
        {
         Resources->ShouldClearArena = true;
        }
        
        EndTemp(Temp);
        
        UI_EndTree();
       }
      }break;
      
      case Curve_B_Spline: {
       b_spline_params *B_Spline = &CurveParams->B_Spline;
       b_spline_degree_bounds Bounds = B_SplineDegreeBounds(Curve->ControlPointCount);
       
       CrucialEntityParamChanged |= UI_SliderIntegerF(SafeCastToPtr(B_Spline->Degree, i32), Bounds.MinDegree, Bounds.MaxDegree, "Degree");
       if (ResetCtxMenu(StrLit("Degree")))
       {
        B_Spline->Degree = DefaultParams->B_Spline.Degree;
        CrucialEntityParamChanged = true;
       }
       
       CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(B_Spline->Partition, u32), B_SplinePartition_Count, B_SplinePartitionNames, StrLit("Partition"));
       if (ResetCtxMenu(StrLit("Partition")))
       {
        B_Spline->Partition = DefaultParams->B_Spline.Partition;
        CrucialEntityParamChanged = true;
       }
       
       if (UI_BeginTreeF("Knots"))
       {
        UI_Checkbox(&B_Spline->ShowPartitionKnotPoints, StrLit("Show Partition Knots"));
        UI_DragFloat(&B_Spline->KnotPointRadius, 0.0f, FLT_MAX, 0, StrLit("Radius"));
        UI_ColorPicker(&B_Spline->KnotPointColor, StrLit("Color"));
        
        // TODO(hbr): Add way to customize knots directly
        UI_EndTree();
       }
      }break;
      
      case Curve_Count: InvalidPath;
     }
    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    CurveParams->LineDisabled = !UI_BeginTreeF("Line");
    if (!CurveParams->LineDisabled)
    {
     UI_ColorPickerF(&CurveParams->LineColor, "Color");
     if (ResetCtxMenu(StrLit("ColorReset")))
     {
      CurveParams->LineColor = DefaultParams->LineColor;
     }
     
     CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->LineWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("WidthReset")))
     {
      CurveParams->LineWidth = DefaultParams->LineWidth;
      CrucialEntityParamChanged = true;
     }
     
     if (IsCurveTotalSamplesMode(Curve))
     {
      CrucialEntityParamChanged |= UI_SliderIntegerF(SafeCastToPtr(CurveParams->TotalSamples, i32), 1, 5000, "Total Samples");
      if (ResetCtxMenu(StrLit("Samples")))
      {
       CurveParams->TotalSamples = DefaultParams->TotalSamples;
       CrucialEntityParamChanged = true;
      }
     }
     else
     {
      CrucialEntityParamChanged |= UI_SliderIntegerF(SafeCastToPtr(CurveParams->SamplesPerControlPoint, i32), 1, 500, "Samples per Control Point");
      if (ResetCtxMenu(StrLit("Samples")))
      {
       CurveParams->SamplesPerControlPoint = DefaultParams->SamplesPerControlPoint;
       CrucialEntityParamChanged = true;
      }
     }
     
     UI_EndTree();
    }
    
    if (UsesControlPoints(Curve))
    {
     ImGui::SetNextItemOpen(true, ImGuiCond_Once);
     CurveParams->PointsDisabled = !UI_BeginTreeF("Control Points");
     if (!CurveParams->PointsDisabled)
     {
      UI_ColorPickerF(&CurveParams->PointColor, "Color");
      if (ResetCtxMenu(StrLit("PointColorReset")))
      {
       CurveParams->PointColor = DefaultParams->PointColor;
      }
      
      UI_DragFloatF(&CurveParams->PointRadius, 0.0f, FLT_MAX, 0, "Radius");
      if (ResetCtxMenu(StrLit("PointRadiusReset")))
      {
       CurveParams->PointRadius = DefaultParams->PointRadius;
      }
      
      if (CurveHasWeights(Curve))
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
        CrucialEntityParamChanged = true;
       }
      }
      
      UI_EndTree();
     }
    }
    
    CurveParams->PolylineEnabled = UI_BeginTreeF("Polyline");
    if (CurveParams->PolylineEnabled)
    {
     UI_ColorPickerF(&CurveParams->PolylineColor, "Color");
     if (ResetCtxMenu(StrLit("PolylineColorReset")))
     {
      CurveParams->PolylineColor = DefaultParams->PolylineColor;
     }
     
     // TODO(hbr): This isn't really crucial in a sense that we have to recompute everything.
     // Consider distinguishing between that.
     CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("PolylineWidthReset")))
     {
      CurveParams->PolylineWidth = DefaultParams->PolylineWidth;
      CrucialEntityParamChanged = true;
     }
     UI_EndTree();
    }
    
    CurveParams->ConvexHullEnabled = UI_BeginTreeF("Convex Hull");
    if (CurveParams->ConvexHullEnabled)
    {
     UI_ColorPickerF(&CurveParams->ConvexHullColor, "Color");
     if (ResetCtxMenu(StrLit("ConvexHullColorReset")))
     {
      CurveParams->ConvexHullColor = DefaultParams->ConvexHullColor;
     }
     
     CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
     if (ResetCtxMenu(StrLit("ConvexHullWidthReset")))
     {
      CurveParams->ConvexHullWidth = DefaultParams->ConvexHullWidth;
      CrucialEntityParamChanged = true;
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
         PerformBezierCurveSplit(&EntityWitness, Editor);
        }
       }
       
       if (Fraction != Tracking->Fraction)
       {
        SetTrackingPointFraction(&EntityWitness, Fraction);
       }
      }
     }
    }
    
    if (CrucialEntityParamChanged)
    {
     MarkEntityModified(&EntityWitness);
    }
   }
  }
  UI_EndWindow();
  UI_PopLabel();
 }
 
 EndEntityModify(EntityWitness);
 
 if (DeleteEntity)
 {
  DeallocEntity(Editor, Entity);
 }
}

internal void
UpdateAndRenderMenuBarUI(editor *Editor,
                         b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                         b32 *SaveProjectAs, b32 *QuitProject, b32 *AnimateCurves,
                         b32 *MergeCurves)
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
   *MergeCurves = UI_MenuItemF(0, 0, "Merge Curves");
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
  
  UI_MenuItemF(&Editor->HelpWindow, 0, "Help");
  
  UI_EndMainMenuBar();
 }
}

internal void
UpdateAndRenderEntityListUI(editor *Editor)
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
UpdateAndRenderDiagnosticsUI(editor *Editor, platform_input *Input)
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
UpdateAndRenderHelpWindowUI(editor *Editor)
{
 if (Editor->HelpWindow)
 {
  if (UI_BeginWindow(&Editor->HelpWindow, 0, StrLit("Help")))
  {
   ImGui::TextWrapped("(very unstructured) Controls overview:\n\n"
                      " - controls are made to be as intuitive as possible; read this, no need to understand everything, note which keys and mouse buttons might do something interesting and go experiment as soon as possible instead\n\n"
                      " - use left mouse button to add/move/select control points\n\n"
                      " - hold Left Ctrl key and use left mouse button to move/select entities themselves instead\n\n"
                      " - clicking on curve's shape will insert control point in the middle of the curve's shape, hold Left Alt to append control point as the last point instead, regardless whether curve's shape was clicked\n\n"
                      " - use right mouse button to delete/deselect entities; releasing button outside of the small circle that will appear will cancel that action\n\n"
                      " - use middle mouse button to move the camera\n\n");
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
UpdateAndRenderChoose2CurvesUI(choose_2_curves_state *Choosing, editor *Editor)
{
 entity *Curve0 = EntityFromId(Choosing->Curves[0]);
 entity *Curve1 = EntityFromId(Choosing->Curves[1]);
 
 b32 ChoosingCurve = Choosing->WaitingForChoice;
 
 b32 Disable0 = false;
 string Button0 = {};
 if (ChoosingCurve && Choosing->ChoosingCurveIndex == 0)
 {
  Disable0 = true;
  Button0 = StrLit("Click on curve!");
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
    Choosing->WaitingForChoice = false;
   }
   else
   {
    Choosing->WaitingForChoice = true;
    Choosing->ChoosingCurveIndex = 0;
   }
  }
  
  UI_SameRow();
  
  UI_Disabled(Curve0 == 0)
  {
   if (UI_Button(StrLit("Select")))
   {
    Assert(Curve0);
    SelectEntity(Editor, Curve0);
   }
  }
 }
 
 string Button1 = {};
 b32 Disable1 = false;
 if (ChoosingCurve && Choosing->ChoosingCurveIndex == 1)
 {
  Disable1 = true;
  Button1 = StrLit("Click on curve");
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
    Choosing->WaitingForChoice = false;
   }
   else
   {
    Choosing->WaitingForChoice = true;
    Choosing->ChoosingCurveIndex = 1;
   }
  }
  
  UI_SameRow();
  
  UI_Disabled(Curve1 == 0)
  {
   if (UI_Button(StrLit("Select")))
   {
    Assert(Curve1);
    SelectEntity(Editor, Curve1);
   }
  }
 }
 
 UI_Disabled((Curve0 == 0) || (Curve1 == 0))
 {
  b32 Hidden = false;
  if (Curve0 && Curve1)
  {
   Hidden = (!IsEntityVisible(Curve0) && !IsEntityVisible(Curve1));
  }
  
  if (UI_Checkbox(&Hidden, StrLit("Hide Curves")))
  {
   Assert(Curve0);
   Assert(Curve1);
   SetEntityVisibility(Curve0, Hidden);
   SetEntityVisibility(Curve1, Hidden);
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
   UpdateAndRenderChoose2CurvesUI(&Animation->Choose2Curves, Editor);
   
   entity *Curve0 = EntityFromId(Animation->Choose2Curves.Curves[0]);
   entity *Curve1 = EntityFromId(Animation->Choose2Curves.Curves[1]);
   
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
     AddNotificationF(Editor, Notification_Error, "failed to load image from %S", Task->ImageFilePath);
    }
    
    DeallocateAsyncTask(Editor, Task);
   }
  }
 }
}

internal void
UpdateFrameStats(frame_stats *Stats, platform_input *Input)
{
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
BeginMovingEntity(editor_left_click_state *Left, entity_id Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingEntity;
 Left->TargetEntity = Target;
}

internal void
BeginMovingCurvePoint(editor_left_click_state *Left, entity_id Target, curve_point_index CurvePoint)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingCurvePoint;
 Left->TargetEntity = Target;
 Left->CurvePointIndex = CurvePoint;
}

internal void
BeginMovingTrackingPoint(editor_left_click_state *Left, entity_id Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingTrackingPoint;
 Left->TargetEntity = Target;
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
ProcessInputEvents(editor *Editor, platform_input *Input, render_group *RenderGroup,
                   b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                   b32 *SaveProjectAs, b32 *QuitProject)
{
 editor_left_click_state *LeftClick = &Editor->LeftClick;
 editor_right_click_state *RightClick = &Editor->RightClick;
 editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 merging_curves_state *Merging = &Editor->MergingCurves;
 for (u32 EventIndex = 0;
      EventIndex < Input->EventCount;
      ++EventIndex)
 {
  platform_event *Event = Input->Events + EventIndex;
  b32 Eat = false;
  v2 MouseP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
  
  //- left click events processing
  b32 DoesAnimationWantInput = AnimationWantsInput(Animation);
  b32 DoesMergingWantInput = MergingWantsInput(Merging);
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_LeftMouseButton &&
      (!LeftClick->Active || DoesAnimationWantInput || DoesMergingWantInput))
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
   
   if ((DoesAnimationWantInput || DoesMergingWantInput) &&
       Collision.Entity && Collision.Entity->Type == Entity_Curve)
   {
    Eat = true;
    
    if (DoesAnimationWantInput)
    {
     if (SupplyCurve(&Animation->Choose2Curves, Collision.Entity))
     {
      Animation->Flags |= AnimatingCurves_Animating;
     }
    }
    else
    {
     Assert(DoesMergingWantInput);
     SupplyCurve(&Merging->Choose2Curves, Collision.Entity);
    }
   }
   else
   {     
    Eat = true;
    
    LeftClick->OriginalVerticesCaptured = false;
    LeftClick->LastMouseP = MouseP;
    
    entity_with_modify_witness CollisionEntityWitness = BeginEntityModify(Collision.Entity);
    curve *CollisionCurve = &Collision.Entity->Curve;
    curve_point_tracking_state *CollisionTracking = &CollisionCurve->PointTracking;
    
    entity *SelectedEntity = EntityFromId(Editor->SelectedEntityId);
    curve *SelectedCurve = &SelectedEntity->Curve;
    
    if ((Collision.Entity || SelectedEntity) && (Event->Flags & PlatformEventFlag_Ctrl))
    {
     entity *Entity = (Collision.Entity ? Collision.Entity : SelectedEntity);
     // NOTE(hbr): just move entity if ctrl is pressed
     BeginMovingEntity(LeftClick, MakeEntityId(Entity));
    }
    else if ((Collision.Flags & Collision_CurveLine) && CollisionTracking->Active)
    {
     BeginMovingTrackingPoint(LeftClick, MakeEntityId(Collision.Entity));
     
     f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveLinePointIndex, (CollisionCurve->CurveSampleCount- 1));
     SetTrackingPointFraction(&CollisionEntityWitness, Fraction);
    }
    else if (Collision.Flags & Collision_CurvePoint)
    {
     BeginMovingCurvePoint(LeftClick, MakeEntityId(Collision.Entity), Collision.CurvePointIndex);
    }
    else if (Collision.Flags & Collision_CurveLine)
    {
     if (UsesControlPoints(CollisionCurve))
     {
      control_point_index Index = CurveLinePointIndexToControlPointIndex(CollisionCurve, Collision.CurveLinePointIndex);
      u32 InsertAt = Index.Index + 1;
      InsertControlPoint(&CollisionEntityWitness, MouseP, InsertAt);
      BeginMovingCurvePoint(LeftClick, MakeEntityId(Collision.Entity), CurvePointIndexFromControlPointIndex(MakeControlPointIndex(InsertAt)));
     }
     else
     {
      SelectEntity(Editor, Collision.Entity);
     }
    }
    else if (Collision.Flags & Collision_TrackedPoint)
    {
     // NOTE(hbr): This shouldn't really happen, Collision_CurveLine should be set as well.
     // But if it does, just don't do anything.
    }
    else
    {
     entity *TargetEntity = 0;
     if (SelectedEntity && SelectedEntity->Type == Entity_Curve && UsesControlPoints(SelectedCurve))
     {
      TargetEntity = SelectedEntity;
     }
     else
     {
      temp_arena Temp = TempArena(0);
      
      entity *Entity = AllocEntity(Editor);
      entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
      
      string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EverIncreasingEntityCounter++);
      InitEntity(Entity, V2(0.0f, 0.0f), V2(1.0f, 1.0f), Rotation2DZero(), Name, 0);
      InitCurve(&EntityWitness, Editor->CurveDefaultParams);
      
      TargetEntity = Entity;
      
      EndEntityModify(EntityWitness);
      
      EndTemp(Temp);
     }
     Assert(TargetEntity);
     
     entity_with_modify_witness TargetEntityWitness = BeginEntityModify(TargetEntity);
     control_point_index Appended = AppendControlPoint(&TargetEntityWitness, MouseP);
     BeginMovingCurvePoint(LeftClick, MakeEntityId(TargetEntity), CurvePointIndexFromControlPointIndex(Appended));
     EndEntityModify(TargetEntityWitness);
    }
    
    entity *TargetEntity = EntityFromId(LeftClick->TargetEntity);
    if (TargetEntity)
    {
     SelectEntity(Editor, TargetEntity);
    }
    
    if (LeftClick->Mode == EditorLeftClick_MovingCurvePoint)
    {
     curve *TargetCurve = SafeGetCurve(TargetEntity);
     SelectControlPointFromCurvePointIndex(TargetCurve, LeftClick->CurvePointIndex);
    }
    
    EndEntityModify(CollisionEntityWitness);
   }
  }
  
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_LeftMouseButton && LeftClick->Active)
  {
   Eat = true;
   EndLeftClick(LeftClick);
  }
  
  if (!Eat && Event->Type == PlatformEvent_MouseMove && LeftClick->Active)
  {
   // NOTE(hbr): don't eat mouse move event
   
   entity *Entity = EntityFromId(LeftClick->TargetEntity);
   if (Entity)
   {
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
       LeftClick->OriginalCurveVertices = CopyLineVertices(Arena, Curve->CurveVertices);
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
  }
  
  //- right click events processing
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_RightMouseButton && !RightClick->Active)
  {
   Eat = true;
   
   collision Collision = CheckCollisionWith(MAX_ENTITY_COUNT,
                                            Editor->Entities,
                                            MouseP,
                                            RenderGroup->CollisionTolerance);
   
   BeginRightClick(RightClick, MouseP, Collision);
   
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
   
   // NOTE(hbr): perform click action only if button was released roughly in the same place
   b32 ReleasedClose = (NormSquared(RightClick->ClickP - MouseP) <= Square(RenderGroup->CollisionTolerance));
   if (ReleasedClose)
   {
    collision *Collision = &RightClick->CollisionAtP;
    entity *Entity = Collision->Entity;
    entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
    curve *Curve = &Entity->Curve;
    
    if (Collision->Flags & Collision_TrackedPoint)
    {
     if (Curve->PointTracking.IsSplitting)
     {
      PerformBezierCurveSplit(&EntityWitness, Editor);
     }
    }
    else if (Collision->Flags & Collision_CurvePoint)
    {
     if (Collision->CurvePointIndex.Type == CurvePoint_ControlPoint)
     {
      RemoveControlPoint(&EntityWitness, Collision->CurvePointIndex.ControlPoint);
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
    
    EndEntityModify(EntityWitness);
   }
   
   EndRightClick(RightClick);
  }
  
  //- camera control events processing
  if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_MiddleMouseButton)
  {
   Eat = true;
   BeginMiddleClick(MiddleClick, Event->Flags & PlatformEventFlag_Ctrl, Event->ClipSpaceMouseP);
  }
  if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_MiddleMouseButton)
  {
   Eat = true;
   EndMiddleClick(MiddleClick);
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
   entity *Entity = EntityFromId(Editor->SelectedEntityId);
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
  EndLeftClick(LeftClick);
 }
 if (!Input->Pressed[PlatformKey_RightMouseButton])
 {
  EndRightClick(RightClick);
 }
 if (!Input->Pressed[PlatformKey_MiddleMouseButton])
 {
  EndMiddleClick(MiddleClick);
 }
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
 InitCurve(MergeWitness, Curve0->Params);
 
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

internal void
UpdateAndRenderMergingCurvesUI(editor *Editor)
{
 merging_curves_state *Merging = &Editor->MergingCurves;
 if (Merging->Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindow(&WindowOpen, 0, StrLit("Merging Curves"));
  
  if (WindowOpen)
  {
   UpdateAndRenderChoose2CurvesUI(&Merging->Choose2Curves, Editor);
   
   b32 MergeMethodChanged = UI_Combo(SafeCastToPtr(Merging->Method, u32), CurveMerge_Count, CurveMergeNames, StrLit("Merge Method"));
   
   entity *Entity0 = EntityFromId(Merging->Choose2Curves.Curves[0]);
   entity *Entity1 = EntityFromId(Merging->Choose2Curves.Curves[1]);
   
   curve *Curve0 = ((Entity0 && Entity0->Type == Entity_Curve) ? &Entity0->Curve : 0);
   curve *Curve1 = ((Entity1 && Entity1->Type == Entity_Curve) ? &Entity1->Curve : 0);
   
   curve_merge_compatibility Compatibility = {};
   if (Curve0 && Curve1)
   {
    Compatibility = AreCurvesCompatibleForMerging(Curve0, Curve1, Merging->Method);
   }
   else
   {
    Compatibility.WhyIncompatible = StrLit("not all curves selected");
   }
   
   b32 Changed0 = EntityModified(Merging->EntityVersioned[0], Entity0);
   b32 Changed1 = EntityModified(Merging->EntityVersioned[1], Entity1);
   
   if (Changed0 || Changed1 || MergeMethodChanged)
   {
    entity_with_modify_witness MergeWitness = BeginEntityModify(&Merging->MergeEntity);
    if (Compatibility.Compatible)
    {
     Merge2Curves(&MergeWitness, Entity0, Entity1, Merging->Method);
    }
    else
    {
     SetCurveControlPoints(&MergeWitness, 0, 0, 0, 0);
    }
    EndEntityModify(MergeWitness);
    
    Merging->EntityVersioned[0] = MakeEntitySnapshotForMerging(Entity0);
    Merging->EntityVersioned[1] = MakeEntitySnapshotForMerging(Entity1);
   }
   
   UI_Disabled(!Compatibility.Compatible)
   {
    if (UI_ButtonF("Merge"))
    {
     EndMergingCurves(Editor, true);
    }
    if (!Compatibility.Compatible)
    {
     UI_Tooltip(Compatibility.WhyIncompatible);
    }
   }
  }
  else
  {
   EndMergingCurves(Editor, false);
  }
  
  UI_EndWindow();
 }
}

internal void
RenderMergingCurves(merging_curves_state *Merging, render_group *RenderGroup)
{
 if (Merging->Active)
 {
  entity *Entity = &Merging->MergeEntity;
  rendering_entity_handle RenderingHandle = BeginRenderingEntity(Entity, RenderGroup);
  
  entity_colors Colors = ExtractEntityColors(Entity);
  entity_colors Fade = Colors;
  for (u32 ColorIndex = 0;
       ColorIndex < ArrayCount(Fade.AllColors);
       ++ColorIndex)
  {
   Fade.AllColors[ColorIndex] = FadeColor(Fade.AllColors[ColorIndex], 0.15f);
  }
  ApplyColorsToEntity(Entity, Fade);
  RenderEntity(RenderingHandle);
  ApplyColorsToEntity(Entity, Colors);
  
  EndRenderingEntity(RenderingHandle);
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
 
 render_group RenderGroup_ = {};
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
 b32 MergeCurves = false;
 
 ProcessInputEvents(Editor, Input, RenderGroup,
                    &NewProject, &OpenFileDialog, &SaveProject,
                    &SaveProjectAs, &QuitProject);
 
 //- render line shadow when moving
 {
  // TODO(hbr): This looks like a mess, probably use functions that already exist
  editor_left_click_state *LeftClick = &Editor->LeftClick;
  entity *Entity = EntityFromId(LeftClick->TargetEntity);
  if (LeftClick->Active && Entity && LeftClick->OriginalVerticesCaptured && !Entity->Curve.Params.LineDisabled)
  {
   v4 ShadowColor = FadeColor(Entity->Curve.Params.LineColor, 0.15f);
   rendering_entity_handle RenderingHandle = BeginRenderingEntity(Entity, RenderGroup);
   
   PushVertexArray(RenderGroup,
                   LeftClick->OriginalCurveVertices.Vertices,
                   LeftClick->OriginalCurveVertices.VertexCount,
                   LeftClick->OriginalCurveVertices.Primitive,
                   ShadowColor,
                   GetCurvePartZOffset(CurvePart_LineShadow));
   
   EndRenderingEntity(RenderingHandle);
  }
 }
 
 //- render "remove" indicator
 {
  editor_right_click_state *RightClick = &Editor->RightClick;
  if (RightClick->Active)
  {
   v4 Color = V4(0.5f, 0.5f, 0.5f, 0.3f);
   // TODO(hbr): Again, zoffset of this thing is wrong
   PushCircle(RenderGroup, RightClick->ClickP, Rotation2DZero(), V2(1, 1), RenderGroup->CollisionTolerance, Color, 0.0f);
  }
 }
 
 //- render rotation indicator
 {
  editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
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
 
 UpdateCamera(&Editor->Camera, Input);
 UpdateFrameStats(&Editor->FrameStats, Input);
 
 if (!Editor->HideUI)
 {
  UpdateAndRenderSelectedEntityUI(Editor);
  UpdateAndRenderMenuBarUI(Editor, &NewProject, &OpenFileDialog, &SaveProject,
                           &SaveProjectAs, &QuitProject, &AnimateCurves, &MergeCurves);
  UpdateAndRenderEntityListUI(Editor);
  UpdateAndRenderDiagnosticsUI(Editor, Input);
  UpdateAndRenderHelpWindowUI(Editor);
  UpdateAndRenderNotifications(Editor, Input, RenderGroup);
  UpdateAndRenderAnimatingCurvesUI(Editor);
  UpdateAndRenderMergingCurvesUI(Editor);
  
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
 
 if (MergeCurves)
 {
  BeginMergingCurves(&Editor->MergingCurves);
 }
 
 UpdateAndRenderEntities(MAX_ENTITY_COUNT, Editor->Entities, RenderGroup);
 UpdateAndRenderAnimatingCurves(&Editor->AnimatingCurves, Input, RenderGroup);
 RenderMergingCurves(&Editor->MergingCurves, RenderGroup);
 
 // TODO(hbr): Only in debug???
 //#if BUILD_DEBUG
 Input->RefreshRequested = true;
 //#endif
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