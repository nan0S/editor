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

internal v2
ClipToWorld(v2 Clip, render_group *RenderGroup)
{
 v2 World = Unproject(&RenderGroup->ProjXForm, Clip);
 return World;
}

internal void
SetEntityModelTransform(render_group *Group, entity *Entity)
{
 SetModelTransform(Group, GetEntityModelTransform(Entity), Entity->SortingLayer);
}

internal void
MovePointAlongCurve(curve *Curve, v2 *TranslateInOut, f32 *PointFractionInOut, b32 Forward)
{
 v2 Translate = *TranslateInOut;
 f32 Fraction = *PointFractionInOut;
 
 u64 LinePointCount = Curve->LinePointCount;
 v2 *LinePoints = Curve->LinePoints;
 f32 DeltaFraction = 1.0f / (LinePointCount - 1);
 
 f32 PointIndexFloat = Fraction * (LinePointCount - 1);
 u64 PointIndex = Cast(u64)(Forward ? FloorF32(PointIndexFloat) : CeilF32(PointIndexFloat));
 PointIndex = ClampTop(PointIndex, LinePointCount - 1);
 
 s64 DirSign = (Forward ? 1 : -1);
 
 b32 Moving = true;
 while (Moving)
 {
  u64 PrevPointIndex = PointIndex;
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

internal texture_index *
AllocateTextureIndex(editor_assets *Assets)
{
 texture_index *Result = Assets->FirstFreeTextureIndex;
 if (Result)
 {
  Assets->FirstFreeTextureIndex = Result->Next;
 }
 return Result;
}

internal void
DeallocateTextureIndex(editor_assets *Assets, texture_index *Index)
{
 if (Index)
 {
  Index->Next = Assets->FirstFreeTextureIndex;
  Assets->FirstFreeTextureIndex = Index;
 }
}
//////////////////////

internal v2
CameraToWorldSpace(v2 P, render_group *Group)
{
 v2 Result = Unproject(&Group->WorldToCamera, P);
 return Result;
}

internal v2
WorldToCameraSpace(v2 P, render_group *Group)
{
 v2 Result = Project(&Group->WorldToCamera, P);
 return Result;
}

internal v2
ScreenToCameraSpace(v2s P, render_group *Group)
{
 v2 Clip = Unproject(&Group->ClipToScreen, V2(P.X, P.Y));
 v2 Result = Unproject(&Group->CameraToClip, Clip);
 return Result;
}

internal v2
ScreenToWorldSpace(v2s P, render_group *Group)
{
 v2 CameraP = ScreenToCameraSpace(P, Group);
 v2 WorldP = CameraToWorldSpace(CameraP, Group);
 
 return WorldP;
}

// NOTE(hbr): Distance in space [-AspectRatio, AspectRatio] x [-1, 1]
internal f32
ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, render_group *Group)
{
 f32 Result = UnprojectLength(&Group->WorldToCamera, V2(ClipSpaceDistance, 0)).X;
 return Result;
}

struct line_collision
{
 b32 Collided;
 u64 PointIndex;
};
internal line_collision
CheckCollisionWithMultiLine(v2 LocalAtP, v2 *LinePoints, u64 PointCount, f32 Width, f32 Tolerance)
{
 line_collision Result = {};
 
 f32 CheckWidth = Width + 2 * Tolerance;
 for (u64 PointIndex = 0;
      PointIndex + 1 < PointCount;
      ++PointIndex)
 {
  v2 P1 = LinePoints[PointIndex + 0];
  v2 P2 = LinePoints[PointIndex + 1];
  if (SegmentCollision(LocalAtP, P1, P2, CheckWidth))
  {
   Result.Collided = true;
   Result.PointIndex = PointIndex;
   break;
  }
 }
 
 return Result;
}

internal collision
CheckCollisionWith(u64 EntityCount, entity *Entities, v2 AtP, f32 Tolerance)
{
 collision Result = {};
 temp_arena Temp = TempArena(0);
 
 entity_array EntityArray = {};
 EntityArray.Count = EntityCount;
 EntityArray.Entities = Entities;
 sorted_entries Sorted = SortEntities(Temp.Arena, EntityArray);
 
 for (u64 Index = 0;
      Index < Sorted.Count;
      ++Index)
 {
  entity *Entity = EntityArray.Entities + Sorted.Entries[Index].Index;
  if (IsEntityVisible(Entity))
  {
   v2 LocalAtP = WorldToLocalEntityPosition(Entity, AtP);
   // TODO(hbr): Note that Tolerance becomes wrong when we want to compute everything in
   // local space, fix that
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     if (AreLinePointsVisible(Curve))
     {
      u64 ControlPointCount = Curve->ControlPointCount;
      v2 *ControlPoints = Curve->ControlPoints;
      for (u64 PointIndex = 0;
           PointIndex < ControlPointCount;
           ++PointIndex)
      {
       // TODO(hbr): Maybe move this function call outside of this loop
       // due to performance reasons.
       point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
       f32 CollisionRadius = PointInfo.Radius + PointInfo.OutlineThickness + Tolerance;
       if (PointCollision(LocalAtP, ControlPoints[PointIndex], CollisionRadius))
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePointIndex = CurvePointIndexFromControlPointIndex({PointIndex});
        break;
       }
      }
      
      visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
      for (u64 Index = 0;
           Index < VisibleBeziers.Count;
           ++Index)
      {
       f32 PointRadius = GetCurveCubicBezierPointRadius(Curve);
       cubic_bezier_point_index BezierIndex = VisibleBeziers.Indices[Index];
       v2 BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
       if (PointCollision(LocalAtP, BezierPoint, PointRadius + Tolerance))
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePointIndex = CurvePointIndexFromBezierPointIndex(BezierIndex);
        break;
       }
      }
     }
     
     curve_point_tracking_state *Tracking = &Curve->PointTracking;
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
       u64 IterationCount = Tracking->Intermediate.IterationCount;
       v2 *Points = Tracking->Intermediate.P;
       
       u64 PointIndex = 0;
       for (u64 Iteration = 0;
            Iteration < IterationCount;
            ++Iteration)
       {
        v2 *LinePoints = Points + PointIndex;
        u64 PointCount = IterationCount - Iteration;
        line_collision Line = CheckCollisionWithMultiLine(LocalAtP, LinePoints, PointCount,
                                                          Curve->Params.LineWidth, Tolerance);
        if (Line.Collided)
        {
         Result.Entity = Entity;
         goto collided_label;
        }
        
        for (u64 I = 0; I < PointCount; ++I)
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
       int x = 1;
      }
     }
     
     if (!Curve->Params.LineDisabled)
     {
      line_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                        Curve->LinePoints, Curve->LinePointCount,
                                                        Curve->Params.LineWidth, Tolerance);
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
      line_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                        Curve->ConvexHullPoints, Curve->ConvexHullCount,
                                                        Curve->Params.ConvexHullWidth, Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (Curve->Params.PolylineEnabled && !Result.Entity)
     {
      line_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                        Curve->ControlPoints, Curve->ControlPointCount,
                                                        Curve->Params.PolylineWidth, Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     v2 Extents = 0.5f * Image->Dim;
     if (-Extents.X <= LocalAtP.X && LocalAtP.X <= Extents.X &&
         -Extents.Y <= LocalAtP.Y && LocalAtP.Y <= Extents.Y)
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

internal f32
CalculateAnimation(animation_type Animation, f32 T)
{
 Assert(0.0f <= T && T <= 1.0f);
 f32 Result = 0.0f;
 
 switch (Animation)
 {
  case Animation_Smooth: {
   // TODO(hbr): Consider just hardcoding.
   v2 P[] = {
    V2(0.0f, 0.0f),
    V2(1.0f, 0.0f),
    V2(0.0f, 1.0f),
    V2(1.0f, 1.0f),
   };
   
   Result = BezierCurveEvaluate(T, P, ArrayCount(P)).Y;
  } break;
  
  case Animation_Linear: { Result = T; } break;
  case Animation_Count: InvalidPath;
 }
 
 return Result;
}

internal entity *
AllocEntity(editor *Editor)
{
 entity *Entity = 0;
 
 entity *Entities = Editor->Entities;
 for (u64 EntityIndex = 0;
      EntityIndex < MAX_ENTITY_COUNT;
      ++EntityIndex)
 {
  entity *Current = Entities + EntityIndex;
  if (!(Current->Flags & EntityFlag_Active))
  {
   Entity = Current;
   
   arena *EntityArena = Entity->Arena;
   arena *PointTrackingArena = Entity->Curve.PointTracking.Arena;
   arena *DegreeLoweringArena = Entity->Curve.DegreeLowering.Arena;
   StructZero(Entity);
   Entity->Arena = EntityArena;
   Entity->Curve.PointTracking.Arena = PointTrackingArena;
   Entity->Curve.DegreeLowering.Arena = DegreeLoweringArena;
   
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
 if (Editor->CurveAnimation.FromCurveEntity == Entity ||
     Editor->CurveAnimation.ToCurveEntity == Entity)
 {
  Editor->CurveAnimation.Stage = AnimateCurveAnimation_None;
 }
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
 Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, vertex);
 ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
 
 return Result;
}

// TODO(hbr): Maybe move into editor_entity.h
internal void
InitEntityFromEntity(entity *Dest, entity *Src)
{
 InitEntity(Dest, Src->P, Src->Scale, Src->Rotation, Src->Name, Src->SortingLayer);
 switch (Src->Type)
 {
  case Entity_Curve: {
   curve *Curve = GetCurve(Src);
   InitCurve(Dest, Curve->Params);
   SetCurveControlPoints(Dest, Curve->ControlPointCount, Curve->ControlPoints, Curve->ControlPointWeights, Curve->CubicBezierPoints);
   SelectControlPoint(GetCurve(Dest), Curve->SelectedIndex);
  } break;
  
  case Entity_Image: {
   image *Image = GetImage(Src);
   InitImage(Dest);
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
}

internal void
PerformBezierCurveSplit(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve = GetCurve(Entity);
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 
 u64 ControlPointCount = Curve->ControlPointCount;
 
 entity *LeftEntity = Entity;
 entity *RightEntity = AllocEntity(Editor);
 string LeftName = StrF(Temp.Arena, "%S(left)", Entity->Name);
 string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
 
 SetEntityName(LeftEntity, LeftName);
 InitEntityFromEntity(RightEntity, LeftEntity);
 SetEntityName(RightEntity, RightName);
 
 BeginLinePoints(&LeftEntity->Curve, ControlPointCount);
 BeginLinePoints(&RightEntity->Curve, ControlPointCount);
 
 BezierCurveSplit(Tracking->Fraction,
                  ControlPointCount, Curve->ControlPoints, Curve->ControlPointWeights,
                  LeftEntity->Curve.ControlPoints,
                  LeftEntity->Curve.ControlPointWeights,
                  RightEntity->Curve.ControlPoints,
                  RightEntity->Curve.ControlPointWeights);
 BezierCubicCalculateAllControlPoints(ControlPointCount,
                                      LeftEntity->Curve.ControlPoints,
                                      LeftEntity->Curve.CubicBezierPoints);
 BezierCubicCalculateAllControlPoints(ControlPointCount,
                                      RightEntity->Curve.ControlPoints,
                                      RightEntity->Curve.CubicBezierPoints);
 
 EndLinePoints(&RightEntity->Curve);
 EndLinePoints(&LeftEntity->Curve);
 
 EndTemp(Temp);
}

internal void
DuplicateEntity(entity *Entity, editor *Editor)
{
 temp_arena Temp = TempArena(0);
 
 entity *Copy = AllocEntity(Editor);
 string CopyName = StrF(Temp.Arena, "%S(copy)", Entity->Name);
 InitEntityFromEntity(Copy, Entity);
 SetEntityName(Copy, CopyName);
 
 SelectEntity(Editor, Copy);
 f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, Editor->RenderGroup);
 v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
 Copy->P += SlightTranslation;
 
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

internal void
BeginAnimatingCurve(curve_animation_state *Animation, entity *CurveEntity)
{
 Animation->Stage = AnimateCurveAnimation_PickingTarget;
 Animation->FromCurveEntity = CurveEntity;
 Animation->ToCurveEntity = 0;
}

// TODO(hbr): Instead of copying all the verices and control points and weights manually, maybe just
// wrap control points, weights and cubicbezier poiints in a structure and just assign data here.
// In other words - refactor this function
// TODO(hbr): Refactor this function big time!!!
internal void
LowerBezierCurveDegree(entity *Entity)
{
 curve *Curve = GetCurve(Entity);
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 u64 PointCount = Curve->ControlPointCount;
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
  
  // TODO(hbr): refactor this, it only has to be here because we still modify control points above
  BezierCubicCalculateAllControlPoints(PointCount - 1, LowerPoints, LowerBeziers);
  SetCurveControlPoints(Entity, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(entity *Entity)
{
 curve *Curve = GetCurve(Entity);
 Assert(Curve->Params.Interpolation == Interpolation_Bezier);
 temp_arena Temp = TempArena(0);
 
 u64 ControlPointCount = Curve->ControlPointCount;
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
 
 SetCurveControlPoints(Entity,
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
   
   u64 PointCount = Curve->ControlPointCount;
   v2 *Points = Curve->ControlPoints;
   for (u64 PointIndex = 0;
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
 curve *Curve = GetCurve(Entity);
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  u64 LeftPointCount = Curve->SelectedIndex.Index + 1;
  u64 RightPointCount = Curve->ControlPointCount - Curve->SelectedIndex.Index;
  
  entity *LeftEntity = Entity;
  entity *RightEntity = AllocEntity(Editor);
  InitEntityFromEntity(RightEntity, LeftEntity);
  
  curve *LeftCurve = GetCurve(LeftEntity);
  curve *RightCurve = GetCurve(RightEntity);
  
  string LeftName  = StrF(Temp.Arena, "%S(left)", Entity->Name);
  string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
  SetEntityName(LeftEntity, LeftName);
  SetEntityName(RightEntity, RightName);
  
  b32 LeftOK = BeginLinePoints(LeftCurve, LeftPointCount);
  b32 RightOK = BeginLinePoints(RightCurve, RightPointCount);
  Assert(LeftOK);
  Assert(RightOK);
  
  ArrayCopy(RightCurve->ControlPoints,
            Curve->ControlPoints + Curve->SelectedIndex.Index,
            RightPointCount);
  ArrayCopy(RightCurve->ControlPointWeights,
            Curve->ControlPointWeights + Curve->SelectedIndex.Index,
            RightPointCount);
  ArrayCopy(RightCurve->CubicBezierPoints,
            Curve->CubicBezierPoints + Curve->SelectedIndex.Index,
            RightPointCount);
  
  EndLinePoints(&LeftEntity->Curve);
  EndLinePoints(&RightEntity->Curve);
  
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
      RecomputeCurve(Entity);
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
 
 auto FileDialog = ImGuiFileDialog::Instance();
 
#if 0
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
 TimeFunction;
 
 curve *Curve = &Entity->Curve;
 curve_params *CurveParams = &Curve->Params;
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 temp_arena Temp = TempArena(Tracking->Arena);
 
 if (Entity->Type == Entity_Curve && Tracking->Active)
 {
  b32 IsBezierRegular = (CurveParams->Interpolation == Interpolation_Bezier &&
                         CurveParams->Bezier == Bezier_Regular);
  
  if (IsBezierRegular)
  {
   if (Tracking->IsSplitting)
   {
    b32 PerformSplit = false;
    b32 Cancel = false;
    b32 IsWindowOpen = true;
    DeferBlock(UI_BeginWindowF(&IsWindowOpen, 0, "Splitting '%S'", Entity->Name),
               UI_EndWindow())
    {
     Tracking->NeedsRecomputationThisFrame |= UI_SliderFloatF(&Tracking->Fraction, 0.0f, 1.0f, "Split Point");
     PerformSplit = UI_ButtonF("Split");
     UI_SameRow();
     Cancel = UI_ButtonF("Cancel");
    }
    
    if (PerformSplit)
    {
     PerformBezierCurveSplit(Editor, Entity);
    }
    
    if (PerformSplit || Cancel || !IsWindowOpen)
    {
     Tracking->Active = false;
    }
    else
    {
     if (Tracking->NeedsRecomputationThisFrame || Curve->RecomputeRequested)
     {
      Tracking->LocalSpaceTrackedPoint = BezierCurveEvaluateWeighted(Tracking->Fraction,
                                                                     Curve->ControlPoints,
                                                                     Curve->ControlPointWeights,
                                                                     Curve->ControlPointCount);
     }
     
     f32 Radius = GetCurveTrackedPointRadius(Curve);
     v4 Color = V4(0.0f, 1.0f, 0.0f, 0.5f);
     f32 OutlineThickness = 0.3f * Radius;
     v4 OutlineColor = DarkenColor(Color, 0.5f);
     PushCircle(Group,
                Tracking->LocalSpaceTrackedPoint, Radius, Color,
                GetCurvePartZOffset(CurvePart_Special),
                OutlineThickness, OutlineColor);
    }
   }
   else
   {
    b32 IsWindowOpen = true;
    DeferBlock(UI_BeginWindowF(&IsWindowOpen, WindowFlag_AutoResize,
                               "De Casteljau Algorithm '%S'", Entity->Name),
               UI_EndWindow())
    {
     Tracking->NeedsRecomputationThisFrame |= UI_SliderFloatF(&Tracking->Fraction, 0.0f, 1.0f, "t");
    }
    
    if (!IsWindowOpen)
    {
     Tracking->Active = false;
    }
    else
    {
     if (Tracking->NeedsRecomputationThisFrame || Curve->RecomputeRequested)
     {
      ClearArena(Tracking->Arena);
      
      all_de_casteljau_intermediate_results Intermediate =
       DeCasteljauAlgorithm(Tracking->Arena,
                            Tracking->Fraction,
                            Curve->ControlPoints,
                            Curve->ControlPointWeights,
                            Curve->ControlPointCount);
      vertex_array *LineVerticesPerIteration = PushArray(Tracking->Arena,
                                                         Intermediate.IterationCount,
                                                         vertex_array);
      f32 LineWidth = Curve->Params.LineWidth;
      
      u64 IterationPointsOffset = 0;
      for (u64 Iteration = 0;
           Iteration < Intermediate.IterationCount;
           ++Iteration)
      {
       u64 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
       LineVerticesPerIteration[Iteration] =
        ComputeVerticesOfThickLine(Tracking->Arena,
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
     
     u64 IterationCount = Tracking->Intermediate.IterationCount;
     
     f32 P = 0.0f;
     f32 Delta_P = 1.0f / (IterationCount - 1);
     
     v4 GradientA = RGBA_Color(255, 0, 144);
     v4 GradientB = RGBA_Color(155, 200, 0);
     
     f32 PointSize = CurveParams->PointRadius;
     
     transform ModelXForm = GetEntityModelTransform(Entity);
     
     u64 PointIndex = 0;
     for (u64 Iteration = 0;
          Iteration < IterationCount;
          ++Iteration)
     {
      v4 IterationColor = Lerp(GradientA, GradientB, P);
      
      PushVertexArray(Group,
                      Tracking->LineVerticesPerIteration[Iteration].Vertices,
                      Tracking->LineVerticesPerIteration[Iteration].VertexCount,
                      Tracking->LineVerticesPerIteration[Iteration].Primitive,
                      IterationColor, GetCurvePartZOffset(CurvePart_Special));
      
      for (u64 I = 0; I < IterationCount - Iteration; ++I)
      {
       v2 Point = Tracking->Intermediate.P[PointIndex];
       PushCircle(Group, Point, PointSize, IterationColor, GetCurvePartZOffset(CurvePart_Special));
       ++PointIndex;
      }
      
      P += Delta_P;
     }
    }
   }
  }
  else
  {
   Tracking->Active = false;
  }
 }
 
 Tracking->NeedsRecomputationThisFrame = false;
 
 EndTemp(Temp);
}

internal void
UpdateAndRenderDegreeLowering(render_group *RenderGroup, entity *Entity)
{
 TimeFunction;
 
 curve *Curve = GetCurve(Entity);
 curve_params *CurveParams = &Curve->Params;
 curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
 
 if (Lowering->Active)
 {
  if (Curve->RecomputeRequested)
  { 
   Lowering->Active = false;
  }
 }
 
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
   
   SetCurveControlPoint(Entity,
                        Lowering->LowerDegree.MiddlePointIndex,
                        NewControlPoint,
                        NewControlPointWeight);
  }
  
  if (Revert)
  {
   SetCurveControlPoints(Entity, Curve->ControlPointCount + 1,
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
  SetEntityModelTransform(RenderGroup, Entity);
  
  v4 Color = Curve->Params.LineColor;
  Color.A *= 0.5f;
  
  PushVertexArray(RenderGroup,
                  Lowering->SavedLineVertices.Vertices,
                  Lowering->SavedLineVertices.VertexCount,
                  Lowering->SavedLineVertices.Primitive,
                  Color,
                  GetCurvePartZOffset(CurvePart_LineShadow));
  
  ResetModelTransform(RenderGroup);
 }
}

#define CURVE_NAME_HIGHLIGHT_COLOR ImVec4(1.0f, 0.5, 0.0f, 1.0f)

#if 0
internal void
UpdateAnimateCurveAnimation(editor *Editor, editor_input *Input, render_group *Group)
{
 TimeFunction;
 
 // TODO(hbr): Rename to [State] or don't use at all.
 curve_animation_state *Animation = &Editor->CurveAnimation;
 
 local char const *AnimationWindowTitle = "Curve Animation";
 switch (Animation->Stage)
 {
  case AnimateCurveAnimation_None: {} break;
  
  case AnimateCurveAnimation_PickingTarget: {
   bool IsWindowOpen = true;
   bool Animate = false;
   bool Cancel = false;
   
   DeferBlock(ImGui::Begin(AnimationWindowTitle, &IsWindowOpen,
                           ImGuiWindowFlags_AlwaysAutoResize),
              ImGui::End())
   {                    
    UI_TextF("Animate");
    UI_SameRow();
    // TODO(hbr): Add v4 UI_TextFColored(CURVE_NAME_HIGHLIGHT_COLOR, Animation->FromCurveEntity->Name.Data);
    UI_TextF(Animation->FromCurveEntity->Name.Data);
    UI_SameRow();
    UI_TextF("with");
    UI_SameRow();
    char const *ComboPreview = (Animation->ToCurveEntity ? Animation->ToCurveEntity->Name.Data : "");
    if (ImGui::BeginCombo("##AnimationTarget", ComboPreview))
    {
     ListIter(Entity, Editor->EntitiesHead, entity)
     {
      if (Entity->Type == Entity_Curve)
      {
       if (Entity != Animation->FromCurveEntity)
       {
        // TODO(hbr): Do a pass over this whole internal, [Selected] should not be initialized like this
        b32 Selected = (Animation->ToCurveEntity == Entity);
        if (ImGui::Selectable(Entity->Name.Data, Selected))
        {
         Animation->ToCurveEntity = Entity;
        }
       }
      }
     }
     
     ImGui::EndCombo();
    }
    UI_SameRow();
    UI_TextF("(choose from list or click on curve).");
    
    if (Animation->ToCurveEntity)
    {
     Animate = UI_ButtonF("Animate");
     UI_SameRow();
    }
    Cancel = UI_ButtonF("Cancel");
   }
   
   if (!IsWindowOpen || Cancel)
   {
    Animation->Stage = AnimateCurveAnimation_None;
   }
   
   if (Animate)
   {
    Assert(Animation->ToCurveEntity);
    
    Animation->Stage = AnimateCurveAnimation_Animating;
    Animation->SavedToCurveVersion = U64_MAX;
    Animation->Blend = 0.0f;
    Animation->IsAnimationPlaying = true;
    Animation->AnimationMultiplier = 1.0f;
   }
  } break;
  
  case AnimateCurveAnimation_Animating: {
   bool BlendChanged = false;
   bool IsWindowOpen = true;
   
   DeferBlock(ImGui::Begin(AnimationWindowTitle, &IsWindowOpen,
                           ImGuiWindowFlags_AlwaysAutoResize),
              ImGui::End())
   {                    
    
    int AnimationAsInt = Cast(int)Animation->Animation;
    for (int AnimationType = 0;
         AnimationType < Animation_Count;
         ++AnimationType)
    {
     if (AnimationType > 0)
     {
      UI_SameRow();
     }
     
     ImGui::RadioButton(AnimationToString(Cast(animation_type)AnimationType),
                        &AnimationAsInt,
                        AnimationType);
    }
    Animation->Animation = Cast(animation_type)AnimationAsInt;
    UI_SameRow();
    if (UI_ButtonF(Animation->IsAnimationPlaying ? "Stop" : "Start"))
    {
     Animation->IsAnimationPlaying = !Animation->IsAnimationPlaying;
    }
    BlendChanged = ImGui::SliderFloat("Blend", &Animation->Blend, 0.0f, 1.0f);
    UI_DragFloatF(&Animation->AnimationSpeed, 0.0f, FLT_MAX, 0, "Speed");
   }
   
   if (BlendChanged)
   {
    Animation->IsAnimationPlaying = false;
   }
   
   if (Animation->IsAnimationPlaying)
   {
    Animation->Blend += DeltaTime * Animation->AnimationMultiplier * Animation->AnimationSpeed;
    if (Animation->Blend > 1.0f)
    {
     Animation->Blend = 1.0f;
     Animation->AnimationMultiplier = -Animation->AnimationMultiplier;
    }
    if (Animation->Blend < 0.0f)
    {
     Animation->Blend = 0.0f;
     Animation->AnimationMultiplier = -Animation->AnimationMultiplier;
    }
    BlendChanged = true;
   }
   
   if (!IsWindowOpen)
   {
    Animation->Stage = AnimateCurveAnimation_None;
   }
   else
   {
    temp_arena Temp = TempArena(Animation->Arena);
    
    entity *FromCurveEntity = Animation->FromCurveEntity;
    entity *ToCurveEntity = Animation->ToCurveEntity;
    curve *FromCurve = &FromCurveEntity->Curve;
    curve *ToCurve = &ToCurveEntity->Curve;
    u64 LinePointCount = FromCurve->LinePointCount;
    
    b32 ToLinePointsRecalculated = false;
    if (LinePointCount != Animation->LinePointCount ||
        Animation->SavedToCurveVersion != ToCurve->CurveVersion)
    {
     ClearArena(Animation->Arena);
     
     Animation->LinePointCount = LinePointCount;
     Animation->ToLinePoints = PushArrayNonZero(Animation->Arena, LinePointCount, v2);
     EvaluateCurve(ToCurve, LinePointCount, Animation->ToLinePoints);
     Animation->SavedToCurveVersion = ToCurve->CurveVersion;
     
     ToLinePointsRecalculated = true;
    }
    
    if (ToLinePointsRecalculated || BlendChanged)
    {               
     v2 *ToLinePoints = Animation->ToLinePoints;
     v2 *FromLinePoints = FromCurve->LinePoints;
     v2 *AnimatedLinePoints = PushArrayNonZero(Temp.Arena, LinePointCount, v2);
     
     f32 Blend = CalculateAnimation(Animation->Animation, Animation->Blend);
     f32 T = 0.0f;
     f32 Delta_T = 1.0f / (LinePointCount - 1);
     for (u64 VertexIndex = 0;
          VertexIndex < LinePointCount;
          ++VertexIndex)
     {
      v2 From = LocalEntityPositionToWorld(FromCurveEntity,
                                           FromLinePoints[VertexIndex]);
      v2 To = LocalEntityPositionToWorld(ToCurveEntity,
                                         ToLinePoints[VertexIndex]);
      
      AnimatedLinePoints[VertexIndex] = Lerp(From, To, Blend);
      
      T += Delta_T;
     }
     
     f32 AnimatedLineWidth = Lerp(FromCurve->Params.LineWidth,
                                  ToCurve->Params.LineWidth,
                                  Blend);
     v4 AnimatedLineColor = LerpColor(FromCurve->Params.LineColor,
                                      ToCurve->Params.LineColor,
                                      Blend);
     
     // NOTE(hbr): If there is no recalculation we can reuse previous buffer without
     // any calculation, because there is already enough space.
     vertex_array_allocation Allocation = (ToLinePointsRecalculated ?
                                           LineVerticesAllocationArena(Animation->Arena) :
                                           LineVerticesAllocationNone(Animation->AnimatedLineVertices.Vertices));
     Animation->AnimatedLineVertices =
      ComputeVerticesOfThickLine(LinePointCount, AnimatedLinePoints,
                                 AnimatedLineWidth, AnimatedLineColor,
                                 false, Allocation);
    }
    
    EndTemp(Temp);
   }
  } break;
 }
}
#endif

// TODO(hbr): Refactor this function
internal void
RenderEntityCombo(u64 EntityCount, entity *Entities, entity **InOutEntity, string Label)
{
 entity *Entity = *InOutEntity;
 string Preview = (Entity ? Entity->Name : StrLit(""));
 if (UI_BeginCombo(Preview, Label))
 {
  for (u64 EntityIndex = 0;
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
    UI_ComboF(&State->CombinationType, CurveCombination_Count, CurveCombinationNames, "Method");
   }
   
   b32 CanCombine = (State->CombinationType != CurveCombination_None);
   if (State->WithEntity)
   {
    curve_params *SourceParams = &GetCurve(State->SourceEntity)->Params;
    curve_params *WithParams = &GetCurve(State->WithEntity)->Params;
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
   curve  *From       = GetCurve(FromEntity);
   curve  *To         = GetCurve(ToEntity);
   u64     FromCount  = From->ControlPointCount;
   u64     ToCount    = To->ControlPointCount;
   
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
   
   u64 CombinedPointCount = ToCount;
   u64 StartIndex = 0;
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
   for (u64 I = StartIndex; I < FromCount; ++I)
   {
    v2 FromPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
    v2 ToPoint   = WorldToLocalEntityPosition(ToEntity, FromPoint + Translation);
    CombinedPoints[ToCount - StartIndex + I] = ToPoint;
   }
   for (u64 I = StartIndex; I < FromCount; ++I)
   {
    for (u64 J = 0; J < 3; ++J)
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
   
   SetCurveControlPoints(ToEntity, CombinedPointCount, CombinedPoints,
                         CombinedWeights, CombinedBeziers);
   
   DeallocEntity(Editor, FromEntity);
   SelectEntity(Editor, ToEntity);
  }
  
  if (Combine || !IsWindowOpen || Cancel)
  {
   EndCurveCombining(State);
  }
 }
 
 curve *SourceCurve = GetCurve(State->SourceEntity);
 curve *WithCurve = GetCurve(State->WithEntity);
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
  PushLine(Group, SourcePoint, BaseVertex, LineWidth, Color, GetCurvePartZOffset(CurvePart_Special));
  
  v2 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
  v2 LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
  v2 RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
  PushTriangle(Group, LeftVertex, RightVertex, WithPoint, Color, GetCurvePartZOffset(CurvePart_Special));
 }
 
 EndTemp(Temp);
}

struct loaded_image
{
 b32 Success;
 u64 Width;
 u64 Height;
 char *Pixels;
};
internal loaded_image
LoadImageFromMemory(arena *Arena, char *ImageData, u64 Count)
{
 loaded_image Result = {};
 
 stbi_set_flip_vertically_on_load(1);
 int Width, Height;
 int Components;
 int RequestComponents = 4;
 char *Data = Cast(char *)stbi_load_from_memory(Cast(stbi_uc const *)ImageData, Count, &Width, &Height, &Components, RequestComponents);
 if (Data)
 {
  u64 TotalSize = Cast(u64)Width * Height * RequestComponents;
  char *Pixels = PushArrayNonZero(Arena, TotalSize, char);
  MemoryCopy(Pixels, Data, TotalSize);
  
  Result.Success = true;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pixels = Pixels;
  
  stbi_image_free(Data);
 }
 
 return Result;
}

EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender)
{
 TimeFunction;
 
 editor *Editor = Memory->Editor;
 if (!Editor)
 {
  Editor = Memory->Editor = PushStruct(Memory->PermamentArena, editor);
  
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
  
  Editor->Camera = {
   .P = V2(0.0f, 0.0f),
   .Rotation = Rotation2DZero(),
   .Zoom = 1.0f,
   .ZoomSensitivity = 0.05f,
   .ReachingTargetSpeed = 10.0f,
  };
  
  Editor->FrameStats.Calculation.MinFrameTime = +INF_F32;
  Editor->FrameStats.Calculation.MaxFrameTime = -INF_F32;
  
  Editor->MovingPointArena = AllocArena();
  Editor->MovingPointArena = AllocArena();
  Editor->CurveAnimation.Arena = AllocArena();
  Editor->CurveAnimation.AnimationSpeed = 1.0f;
  
  for (u64 EntityIndex = 0;
       EntityIndex < MAX_ENTITY_COUNT;
       ++EntityIndex)
  {
   entity *Entity = Editor->Entities + EntityIndex;
   Entity->Arena = AllocArena();
   Entity->Curve.PointTracking.Arena = AllocArena();
   Entity->Curve.DegreeLowering.Arena = AllocArena();
  }
  
  Editor->EntityListWindow = true;
  // TODO(hbr): Change to false by default
  Editor->DiagnosticsWindow = true;
  Editor->SelectedEntityWindow = true;
  
  Editor->LeftClick.OriginalVerticesArena = AllocArena();
  
  {
   entity *Entity = AllocEntity(Editor);
   InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), StrLit("special"), 0);
   curve_params Params = Editor->CurveDefaultParams;
   Params.Interpolation = Interpolation_Bezier;
   InitCurve(Entity, Params);
   
   AppendControlPoint(Entity, V2(-0.5f, -0.5f));
   AppendControlPoint(Entity, V2(+0.5f, -0.5f));
   AppendControlPoint(Entity, V2(+0.5f, +0.5f));
   AppendControlPoint(Entity, V2(-0.5f, +0.5f));
   
   BeginCurvePointTracking(&Entity->Curve, false);
   SetTrackingPointFraction(&Entity->Curve.PointTracking, 0.5f);
  }
  
  editor_assets *Assets = &Editor->Assets;
  u64 TextureCount = Memory->MaxTextureCount;
  for (u64 Index = 0;
       Index < TextureCount;
       ++Index)
  {
   texture_index *TextureIndex = PushStruct(Memory->PermamentArena, texture_index);
   TextureIndex->Index = TextureCount - 1 - Index;
   TextureIndex->Next = Assets->FirstFreeTextureIndex;
   Assets->FirstFreeTextureIndex = TextureIndex;
  }
  Assets->TextureQueue = Memory->TextureQueue;
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
 
 //- process events and update click events
 b32 NewProject = false;
 b32 OpenFileDialog = false;
 b32 SaveProject = false;
 b32 SaveProjectAs = false;
 b32 QuitProject = false;
 {
  editor_left_click_state *LeftClick = &Editor->LeftClick;
  editor_right_click_state *RightClick = &Editor->RightClick;
  editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
  for (u64 EventIndex = 0;
       EventIndex < Input->EventCount;
       ++EventIndex)
  {
   platform_event *Event = Input->Events + EventIndex;
   b32 Eat = false;
   v2 MouseP = ClipToWorld(Event->ClipSpaceMouseP, RenderGroup);
   
   //- left click events processing
   if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_LeftMouseButton && !LeftClick->Active)
   {
    Eat = true;
    
    LeftClick->Active = true;
    LeftClick->OriginalVerticesCaptured = false;
    LeftClick->LastMouseP = MouseP;
    
    collision Collision = {};
    if (Input->Pressed[PlatformKey_LeftAlt])
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
    
    curve *Curve = &Collision.Entity->Curve;
    curve_point_tracking_state *Tracking = &Curve->PointTracking;
    if (Collision.Entity && Input->Pressed[PlatformKey_LeftCtrl])
    {
     // NOTE(hbr): just move entity if ctrl is pressed
     LeftClick->Mode = EditorLeftClick_MovingEntity;
     LeftClick->TargetEntity = Collision.Entity;
    }
    else if ((Collision.Flags & Collision_CurveLine) && Tracking->Active)
    {
     LeftClick->Mode = EditorLeftClick_MovingTrackingPoint;
     LeftClick->TargetEntity = Collision.Entity;
     
     f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveLinePointIndex, (Curve->LinePointCount- 1));
     SetTrackingPointFraction(Tracking, Fraction);
    }
    else if (Collision.Flags & Collision_CurvePoint)
    {
     LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
     LeftClick->TargetEntity = Collision.Entity;
     LeftClick->CurvePointIndex = Collision.CurvePointIndex;
    }
    else if (Collision.Flags & Collision_CurveLine)
    {
     control_point_index Index = CurveLinePointIndexToControlPointIndex(Curve, Collision.CurveLinePointIndex);
     u64 InsertAt = Index.Index + 1;
     InsertControlPoint(Collision.Entity, MouseP, InsertAt);
     
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
      string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EverIncreasingEntityCounter++);
      InitEntity(Entity, V2(0.0f, 0.0f), V2(1.0f, 1.0f), Rotation2DZero(), Name, 0);
      InitCurve(Entity, Editor->CurveDefaultParams);
      EndTemp(Temp);
      
      LeftClick->TargetEntity = Entity;
     }
     Assert(LeftClick->TargetEntity);
     
     LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
     LeftClick->CurvePointIndex = CurvePointIndexFromControlPointIndex(AppendControlPoint(LeftClick->TargetEntity, MouseP));
    }
    
    SelectEntity(Editor, LeftClick->TargetEntity);
    if (LeftClick->Mode == EditorLeftClick_MovingCurvePoint)
    {
     SelectControlPointFromCurvePointIndex(SafeGetCurve(LeftClick->TargetEntity),
                                           LeftClick->CurvePointIndex);
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
      
      SetTrackingPointFraction(Tracking, Fraction);
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
      
      translate_curve_point_flags Flags = 0;
      if (Input->Pressed[PlatformKey_LeftShift])
      {
       Flags |= TranslateCurvePoint_MatchBezierTwinDirection;
      }
      if (Input->Pressed[PlatformKey_LeftCtrl])
      {
       Flags |= TranslateCurvePoint_MatchBezierTwinLength;
      }
      SetCurvePoint(Entity, LeftClick->CurvePointIndex, MouseP, Flags);
     }break;
     
     case EditorLeftClick_MovingEntity: {
      Entity->P += Translate;
     }break;
    }
    
    LeftClick->LastMouseP = MouseP;
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
       RemoveControlPoint(Entity, Collision->CurvePointIndex.ControlPoint);
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
    MiddleClick->Rotate = (Input->Pressed[PlatformKey_LeftCtrl]);
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
    v2 FromP = ClipToWorld(MiddleClick->ClipSpaceLastMouseP, RenderGroup);
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
    Editor->HideUI = true;
   }
   if (!Eat && Event->Type == PlatformEvent_Release && Event->Key == PlatformKey_Tab)
   {
    Eat = true;
    Editor->HideUI = false;
   }
   
   //- shortcuts
   if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_N && (Event->Flags & PlatformEventFlag_Ctrl))
   {
    Eat = true;
    NewProject = true;
   }
   
   if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_O && (Event->Flags & PlatformEventFlag_Ctrl))
   {
    Eat = true;
    OpenFileDialog = true;
   }
   
   if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_S && (Event->Flags & PlatformEventFlag_Ctrl))
   {
    Eat = true;
    SaveProject = true;
   }
   
   if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_S && (Event->Flags & PlatformEventFlag_Ctrl) && (Event->Flags & PlatformEventFlag_Shift))
   {
    Eat = true;
    SaveProjectAs = true;
   }
   
   if (!Eat && ((Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_Escape) || (Event->Type == PlatformEvent_WindowClose)))
   {
    Eat = true;
    QuitProject = true;
   }
   
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
    SetEntityModelTransform(RenderGroup, Entity);
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
   PushCircle(RenderGroup, RightClick->ClickP, RenderGroup->CollisionTolerance, Color, 0.0f);
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
              Radius - OutlineThickness,
              Color, 0.0f,
              OutlineThickness, OutlineColor);
  }
 }
 
 //- update camera
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
 
 //- update frame stats
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
   Stats->Calculation.MinFrameTime = INF_F32;
   Stats->Calculation.MaxFrameTime = -INF_F32;
   Stats->Calculation.SumFrameTime = 0.0f;
  }
 }
 
 if (!Editor->HideUI)
 {
  //- selected entity UI
  {
   entity *Entity = Editor->SelectedEntity;
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
      curve_params *CurveParams = &Curve->Params;
      curve_params *DefaultParams = &Editor->CurveDefaultParams;
      b32 CurveChanged = false;
      
      UI_SeparatorTextF("Curve");
      UI_LabelF("Curve")
      {       
       CurveChanged |= UI_ComboF(&CurveParams->Interpolation, Interpolation_Count, InterpolationNames, "Interpolation");
       if (ResetCtxMenu(StrLit("InterpolationReset")))
       {
        CurveParams->Interpolation = DefaultParams->Interpolation;
        CurveChanged = true;
       }
       
       switch (CurveParams->Interpolation)
       {
        case Interpolation_Polynomial: {
         polynomial_interpolation_params *Polynomial = &CurveParams->Polynomial;
         
         CurveChanged |= UI_ComboF(&Polynomial->Type, PolynomialInterpolation_Count, PolynomialInterpolationNames, "Type");
         if (ResetCtxMenu(StrLit("PolynomialReset")))
         {
          Polynomial->Type = DefaultParams->Polynomial.Type;
          CurveChanged = true;
         }
         
         CurveChanged |= UI_ComboF(&Polynomial->PointSpacing, PointSpacing_Count, PointSpacingNames, "Point Spacing");
         if (ResetCtxMenu(StrLit("PointSpacingReset")))
         {
          Polynomial->PointSpacing = DefaultParams->Polynomial.PointSpacing;
          CurveChanged = true;
         }
        }break;
        
        case Interpolation_CubicSpline: {
         CurveChanged |= UI_ComboF(&CurveParams->CubicSpline, CubicSpline_Count, CubicSplineNames, "Type");
         if (ResetCtxMenu(StrLit("SplineReset")))
         {
          CurveParams->CubicSpline = DefaultParams->CubicSpline;
          CurveChanged = true;
         }
        }break;
        
        case Interpolation_Bezier: {
         CurveChanged |= UI_ComboF(&CurveParams->Bezier, Bezier_Count, BezierNames, "Type");
         if (ResetCtxMenu(StrLit("BezierReset")))
         {
          CurveParams->Bezier = DefaultParams->Bezier;
          CurveChanged = true;
         }
        }break;
        
        case Interpolation_Count: InvalidPath;
       }
      }
      
      UI_LabelF("Line")
      {
       b32 LineEnabled = !CurveParams->LineDisabled;
       UI_CheckboxF(&LineEnabled, "##Disabled");
       CurveParams->LineDisabled = !LineEnabled;
       UI_SameRow();
       UI_SeparatorTextF("Line");
       if (!CurveParams->LineDisabled)
       {
        CurveChanged |= UI_ColorPickerF(&CurveParams->LineColor, "Color");
        if (ResetCtxMenu(StrLit("ColorReset")))
        {
         CurveParams->LineColor = DefaultParams->LineColor;
         CurveChanged = true;
        }
        
        CurveChanged |= UI_DragFloatF(&CurveParams->LineWidth, 0.0f, FLT_MAX, 0, "Width");
        if (ResetCtxMenu(StrLit("WidthReset")))
        {
         CurveParams->LineWidth = DefaultParams->LineWidth;
         CurveChanged = true;
        }
        
        CurveChanged |= UI_SliderIntegerF(&CurveParams->PointCountPerSegment, 1, 2000, "Detail");
        if (ResetCtxMenu(StrLit("DetailReset")))
        {
         CurveParams->PointCountPerSegment = DefaultParams->PointCountPerSegment;
         CurveChanged = true;
        }
       }
      }
      
      UI_LabelF("Control Points")
      {      
       b32 PointsEnabled = !CurveParams->PointsDisabled;
       UI_CheckboxF(&PointsEnabled, "##Disabled");
       CurveParams->PointsDisabled = !PointsEnabled;
       UI_SameRow();
       UI_SeparatorTextF("Control Points");
       if (!CurveParams->PointsDisabled)
       {
        CurveChanged |= UI_ColorPickerF(&CurveParams->PointColor, "Color");
        if (ResetCtxMenu(StrLit("PointColorReset")))
        {
         CurveParams->PointColor = DefaultParams->PointColor;
         CurveChanged = true;
        }
        
        CurveChanged |= UI_DragFloatF(&CurveParams->PointRadius, 0.0f, FLT_MAX, 0, "Radius");
        if (ResetCtxMenu(StrLit("PointRadiusReset")))
        {
         CurveParams->PointRadius = DefaultParams->PointRadius;
         CurveChanged = true;
        }
        
        if (CurveParams->Interpolation == Interpolation_Bezier &&
            CurveParams->Bezier == Bezier_Regular)
        {
         u64 PointCount = Curve->ControlPointCount;
         f32 *Weights = Curve->ControlPointWeights;
         
         u64 Selected = 0;
         if (IsControlPointSelected(Curve))
         {
          Selected = Curve->SelectedIndex.Index;
          CurveChanged |= UI_DragFloatF(&Weights[Selected], 0.0f, FLT_MAX, 0, "Weight (%llu)", Selected);
          if (ResetCtxMenu(StrLit("WeightReset")))
          {
           Weights[Selected] = 1.0f;
          }
          
          ImGui::Spacing();
         }
         
         
         if (UI_BeginTreeF("Weights"))
         {
          // NOTE(hbr): Limit the number of points displayed in the case
          // curve has A LOT of them
          u64 ShowCount = 100;
          u64 ToIndex = ClampTop(Selected + ShowCount/2, PointCount);
          u64 FromIndex = Selected - Min(Selected, ShowCount/2);
          u64 LeftCount = ShowCount - (ToIndex - FromIndex);
          ToIndex = ClampTop(ToIndex + LeftCount, PointCount);
          FromIndex = FromIndex - Min(FromIndex, LeftCount);
          
          for (u64 PointIndex = FromIndex;
               PointIndex < ToIndex;
               ++PointIndex)
          {
           UI_Id(PointIndex)
           {                              
            CurveChanged |= UI_DragFloatF(&Weights[PointIndex], 0.0f, FLT_MAX, 0, "Point (%llu)", PointIndex);
            if (ResetCtxMenu(StrLit("WeightReset")))
            {
             Weights[PointIndex] = 1.0f;
            }
           }
          }
          
          UI_EndTree();
         }
        }
       }
      }
      
      UI_LabelF("Polyline")
      {      
       UI_CheckboxF(&CurveParams->PolylineEnabled, "##Enabled");
       UI_SameRow();
       UI_SeparatorTextF("Polyline");
       if (CurveParams->PolylineEnabled)
       {
        CurveChanged |= UI_ColorPickerF(&CurveParams->PolylineColor, "Color");
        if (ResetCtxMenu(StrLit("PolylineColorReset")))
        {
         CurveParams->PolylineColor = DefaultParams->PolylineColor;
         CurveChanged = true;
        }
        
        CurveChanged |= UI_DragFloatF(&CurveParams->PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
        if (ResetCtxMenu(StrLit("PolylineWidthReset")))
        {
         CurveParams->PolylineWidth = DefaultParams->PolylineWidth;
         CurveChanged = true;
        }
       }
      }
      
      UI_LabelF("Convex Hull")
      {   
       UI_CheckboxF(&CurveParams->ConvexHullEnabled, "##Enabled");
       UI_SameRow();
       UI_SeparatorTextF("Convex Hull");
       if (CurveParams->ConvexHullEnabled)
       {
        CurveChanged |= UI_ColorPickerF(&CurveParams->ConvexHullColor, "Color");
        if (ResetCtxMenu(StrLit("ConvexHullColorReset")))
        {
         CurveParams->ConvexHullColor = DefaultParams->ConvexHullColor;
         CurveChanged = true;
        }
        
        CurveChanged |= UI_DragFloatF(&CurveParams->ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
        if (ResetCtxMenu(StrLit("ConvexHullWidthReset")))
        {
         CurveParams->ConvexHullWidth = DefaultParams->ConvexHullWidth;
         CurveChanged = true;
        }
       }
      }
     }
    }
    UI_EndWindow();
    UI_PopLabel();
   }
  }
  
  //- render menu bar UI
  if (UI_BeginMainMenuBar())
  {
   if (UI_BeginMenuF("File"))
   {
    NewProject     = UI_MenuItemF(0, "Ctrl+N",       "New");
    OpenFileDialog = UI_MenuItemF(0, "Ctrl+O",       "Open");
    SaveProject    = UI_MenuItemF(0, "Ctrl+S",       "Save");
    SaveProjectAs  = UI_MenuItemF(0, "Ctrl+Shift+S", "Save As");
    QuitProject    = UI_MenuItemF(0, "Escape",     "Quit");
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
  
  //- render entity list UI
  if (Editor->EntityListWindow)
  {
   if (UI_BeginWindowF(&Editor->EntityListWindow, 0, "Entities"))
   {
    entity *Entities = Editor->Entities;
    for (u64 EntityTypeIndex = 0;
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
      for (u64 EntityIndex = 0;
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
  
  //- render notifications
  UI_LabelF("Notifications")
  {
   v2u WindowDim = RenderGroup->Frame->WindowDim;
   f32 Padding = 0.01f * WindowDim.X;
   f32 TargetPosY = WindowDim.Y - Padding;
   f32 WindowWidth = 0.1f * WindowDim.X;
   
   for (u64 NotificationIndex = 0;
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
     u64 PhaseIndex = 0;
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
  
#if BUILD_DEBUG
  ImGui::ShowDemoWindow();
#endif
 }
 
 if (OpenFileDialog)
 {
  temp_arena Temp = TempArena(0);
  
  b32 Success = false;
  platform_file_dialog_result OpenDialog = Platform.OpenFileDialog(Temp.Arena);
  if (OpenDialog.Success)
  {
   string FilePath = OpenDialog.FilePath;
   string FileData = OS_ReadEntireFile(Temp.Arena, FilePath);
   loaded_image LoadedImage = LoadImageFromMemory(Temp.Arena, FileData.Data, FileData.Count);
   if (LoadedImage.Success)
   {
    u64 RequestSize = LoadedImage.Width * LoadedImage.Height * 4;
    editor_assets *Assets = &Editor->Assets;
    texture_transfer_op *TextureOp = PushTextureTransfer(Assets->TextureQueue, RequestSize);
    if (TextureOp)
    {
     texture_index *TextureIndex = AllocateTextureIndex(Assets);
     if (TextureIndex)
     {
      MemoryCopy(TextureOp->Pixels, LoadedImage.Pixels, RequestSize);
      TextureOp->Width = LoadedImage.Width;
      TextureOp->Height = LoadedImage.Height;
      TextureOp->TextureIndex = TextureIndex->Index;
      
      string FileName = PathLastPart(FilePath);
      string FileNameNoExt = StrChopLastDot(FileName);
      
      entity *Entity = AllocEntity(Editor);
      InitEntity(Entity, V2(0, 0), V2(1, 1), Rotation2DZero(), FileNameNoExt, 0);
      Entity->Type = Entity_Image;
      
      image *Image = &Entity->Image;
      Image->Dim.X = 2.0f * Cast(f32)LoadedImage.Width / LoadedImage.Height;
      Image->Dim.Y = 2.0f;
      Image->TextureIndex = TextureIndex;
      
      Success = true;
     }
    }
    
    if (!Success && TextureOp)
    {
     PopTextureTransfer(Assets->TextureQueue, TextureOp);
    }
   }
  }
  
  if (!Success)
  {
   AddNotificationF(Editor, Notification_Error, "failed to open %S", OpenDialog.FilePath);
  }
  
  EndTemp(Temp);
 }
 
 if (QuitProject)
 {
  Input->QuitRequested = true;
 }
 
 //- update and render entities
 for (u64 EntryIndex = 0;
      EntryIndex < MAX_ENTITY_COUNT;
      ++EntryIndex)
 {
  entity *Entity = Editor->Entities + EntryIndex;
  if ((Entity->Flags & EntityFlag_Active) && IsEntityVisible(Entity))
  {
   SetEntityModelTransform(RenderGroup, Entity);
   f32 BaseZOffset = Cast(f32)Entity->SortingLayer;
   
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     curve_params *CurveParams = &Curve->Params;
     
     if (Curve->RecomputeRequested)
     {
      ActuallyRecomputeCurve(Entity);
     }
     
     if (!CurveParams->LineDisabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->LineVertices.Vertices,
                      Curve->LineVertices.VertexCount,
                      Curve->LineVertices.Primitive,
                      Curve->Params.LineColor,
                      GetCurvePartZOffset(CurvePart_Line));
     }
     
     if (CurveParams->PolylineEnabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->PolylineVertices.Vertices,
                      Curve->PolylineVertices.VertexCount,
                      Curve->PolylineVertices.Primitive,
                      Curve->Params.PolylineColor,
                      GetCurvePartZOffset(CurvePart_Special));
     }
     
     if (CurveParams->ConvexHullEnabled)
     {
      PushVertexArray(RenderGroup,
                      Curve->ConvexHullVertices.Vertices,
                      Curve->ConvexHullVertices.VertexCount,
                      Curve->ConvexHullVertices.Primitive,
                      Curve->Params.ConvexHullColor,
                      GetCurvePartZOffset(CurvePart_Special));
     }
     
     if (AreLinePointsVisible(Curve))
     {
      visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
      for (u64 Index = 0;
           Index < VisibleBeziers.Count;
           ++Index)
      {
       cubic_bezier_point_index BezierIndex = VisibleBeziers.Indices[Index];
       
       v2 BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
       v2 CenterPoint = GetCenterPointFromCubicBezierPointIndex(Curve, BezierIndex);
       
       f32 BezierPointRadius = GetCurveCubicBezierPointRadius(Curve);
       f32 HelperLineWidth = 0.2f * CurveParams->LineWidth;
       
       PushLine(RenderGroup, BezierPoint, CenterPoint, HelperLineWidth, CurveParams->LineColor,
                GetCurvePartZOffset(CurvePart_Line));
       PushCircle(RenderGroup, BezierPoint, BezierPointRadius, CurveParams->PointColor,
                  GetCurvePartZOffset(CurvePart_ControlPoint));
      }
      
      u64 ControlPointCount = Curve->ControlPointCount;
      v2 *ControlPoints = Curve->ControlPoints;
      for (u64 PointIndex = 0;
           PointIndex < ControlPointCount;
           ++PointIndex)
      {
       point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
       PushCircle(RenderGroup,
                  ControlPoints[PointIndex],
                  PointInfo.Radius,
                  PointInfo.Color,
                  GetCurvePartZOffset(CurvePart_ControlPoint),
                  PointInfo.OutlineThickness,
                  PointInfo.OutlineColor);
      }
     }
     
     UpdateAndRenderDegreeLowering(RenderGroup, Entity);
     
     curve_animation_state *Animation = &Editor->CurveAnimation;
     if (Animation->Stage == AnimateCurveAnimation_Animating && Animation->FromCurveEntity == Entity)
     {
      // TODO(hbr): Update this color calculation. This should be interpolation between two curves' colors.
      // TODO(hbr): Not only colors but also z offset
      v4 Color = Curve->Params.LineColor;
      PushVertexArray(RenderGroup,
                      Animation->AnimatedLineVertices.Vertices,
                      Animation->AnimatedLineVertices.VertexCount,
                      Animation->AnimatedLineVertices.Primitive,
                      Color,
                      GetCurvePartZOffset(CurvePart_Line));
     }
     
     // TODO(hbr): Update this
     curve_combining_state *Combining = &Editor->CurveCombining;
     if (Combining->SourceEntity == Entity)
     {
      UpdateAndRenderCurveCombining(RenderGroup, Editor);
     }
     
     UpdateAndRenderPointTracking(RenderGroup, Editor, Entity);
     
     Curve->RecomputeRequested = false;
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     PushImage(RenderGroup, Image->Dim, Image->TextureIndex->Index);
    }break;
    
    case Entity_Count: InvalidPath; break;
   }
   
   ResetModelTransform(RenderGroup);
  }
 }
}

// NOTE(hbr): Specifically after every file is included. Works in Unity build only.
StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors), ProfileAnchorsFitIntoArray);