#include "editor.h"

#include "editor_base.cpp"
#include "editor_os.cpp"
#include "editor_profiler.cpp"
#include "editor_math.cpp"
#include "editor_ui.cpp"

#include "editor_input.cpp"
#include "editor_entity.cpp"
#include "editor_debug.cpp"
#include "editor_draw.cpp"

#include "editor_entity2.cpp"

global editor_params GlobalDefaultEditorParams = {
   .RotationIndicator = {
      .RadiusClipSpace = 0.06f,
      .OutlineThicknessFraction = 0.1f,
      .FillColor = MakeColor(30, 56, 87, 80),
      .OutlineColor = MakeColor(255, 255, 255, 24),
   },
   .BezierSplitPoint = {
      .RadiusClipSpace = 0.025f,
      .OutlineThicknessFraction = 0.1f,
      .FillColor = MakeColor(0, 255, 0, 100),
      .OutlineColor = MakeColor(0, 200, 0, 200),
   },
   .BackgroundColor = MakeColor(21, 21, 21),
   .CollisionToleranceClipSpace = 0.02f,
   .CubicBezierHelperLineWidthClipSpace = 0.003f,
   .CurveDefaultParams = DefaultCurveParams(),
};

internal void
SetCameraZoom(camera *Camera, f32 Zoom)
{
   Camera->Zoom = Zoom;
   Camera->ReachingTarget = false;
}

internal void
UpdateCamera(camera *Camera, f32 MouseWheelDelta, f32 DeltaTime)
{
   if (MouseWheelDelta != 0.0f)
   {
      f32 ZoomDelta  = Camera->Zoom * Camera->ZoomSensitivity * MouseWheelDelta;
      SetCameraZoom(Camera, Camera->Zoom + ZoomDelta);
   }
   
   if (Camera->ReachingTarget)
   {
      f32 T = 1.0f - PowF32(2.0f, -Camera->ReachingTargetSpeed * DeltaTime);
      Camera->Position = Lerp(Camera->Position, Camera->PositionTarget, T);
      Camera->Zoom = Lerp(Camera->Zoom, Camera->ZoomTarget, T);
      
      if (ApproxEq32(Camera->Zoom, Camera->ZoomTarget) &&
          ApproxEq32(Camera->Position.X, Camera->PositionTarget.X) &&
          ApproxEq32(Camera->Position.Y, Camera->PositionTarget.Y))
      {
         Camera->ReachingTarget = false;
      }
   }
}

internal void
MoveCamera(camera *Camera, v2 Translation)
{
   Camera->Position += Translation;
   Camera->ReachingTarget = false;
}

internal void
RotateCamera(camera *Camera, rotation_2d Rotation)
{
   Camera->Rotation = CombineRotations2D(Camera->Rotation, Rotation);
   Camera->ReachingTarget = false;
}

internal world_position
CameraToWorldSpace(camera_position Position, render_data *Data)
{
   v2 Rotated = RotateAround(Position, V2(0.0f, 0.0f), Data->Camera.Rotation);
   v2 Scaled = Rotated / Data->Camera.Zoom;
   v2 Translated = Scaled + Data->Camera.Position;
   
   world_position WorldPosition = Translated;
   return WorldPosition;
}

internal camera_position
WorldToCameraSpace(world_position Position, render_data *Data)
{
   v2 Translated = Position - Data->Camera.Position;
   v2 Scaled = Data->Camera.Zoom * Translated;
   v2 Rotated = RotateAround(Scaled,
                             V2(0.0f, 0.0f),
                             Rotation2DInverse(Data->Camera.Rotation));
   
   camera_position CameraPosition = Rotated;
   return CameraPosition;
}

internal camera_position
ScreenToCameraSpace(screen_position Position, render_data *Data)
{
   v2 ClipPosition = V2FromVec(Data->Window->mapPixelToCoords(V2S32ToVector2i(Position)));
   
   f32 FrustumExtent = 0.5f * Data->FrustumSize;
   f32 CameraPositionX = ClipPosition.X * Data->AspectRatio * FrustumExtent;
   f32 CameraPositionY = ClipPosition.Y * FrustumExtent;
   
   camera_position CameraPosition = V2(CameraPositionX, CameraPositionY);
   return CameraPosition;
}

internal world_position
ScreenToWorldSpace(screen_position Position, render_data *Data)
{
   camera_position CameraPosition = ScreenToCameraSpace(Position, Data);
   world_position WorldPosition = CameraToWorldSpace(CameraPosition, Data);
   
   return WorldPosition;
}

// NOTE(hbr): Distance in space [-AspectRatio, AspectRatio] x [-1, 1]
internal f32
ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, render_data *Data)
{
   f32 CameraSpaceDistance = ClipSpaceDistance * 0.5f * Data->FrustumSize;
   f32 WorldSpaceDistance = CameraSpaceDistance / Data->Camera.Zoom;
   
   return WorldSpaceDistance;
}

internal collision
CheckCollisionWith(entities *Entities,
                   world_position CheckPosition,
                   f32 CollisionTolerance,
                   editor_params *EditorParams,
                   collision_flags CollisionWithFlags)
{
   collision Result = {};
   
   temp_arena Temp = TempArena(0);
   
   entity_array EntityArray = {};
   EntityArray.Count = MAX_ENTITY_COUNT;
   EntityArray.Entities = Entities->Entities;
   sorted_entries Sorted = SortEntities(Temp.Arena, EntityArray);
   
   for (u64 Index = 0;
        Index < Sorted.Count;
        ++Index)
   {
      entity *Entity = EntityArray.Entities + Sorted.Entries[Index].Index;
      if (IsEntityVisible(Entity))
      {
         local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               if (AreCurvePointsVisible(Curve) && (CollisionWithFlags & Collision_CurvePoint))
               {
                  u64 ControlPointCount = Curve->ControlPointCount;
                  local_position *ControlPoints = Curve->ControlPoints;
                  for (u64 PointIndex = 0;
                       PointIndex < ControlPointCount;
                       ++PointIndex)
                  {
                     // TODO(hbr): Maybe move this function call outside of this loop
                     // due to performance reasons.
                     point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
                     if (PointCollision(CheckPositionLocal,
                                        ControlPoints[PointIndex],
                                        PointInfo.Radius + PointInfo.OutlineThickness + CollisionTolerance))
                     {
                        Result.Entity = Entity;
                        Result.Flags |= Collision_CurvePoint;
                        Result.CurvePointIndex = MakeCurvePointIndexFromControlPointIndex(PointIndex);
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
                     local_position BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
                     if (PointCollision(CheckPositionLocal,
                                        BezierPoint,
                                        PointRadius + CollisionTolerance))
                     {
                        Result.Entity = Entity;
                        Result.Flags |= Collision_CurvePoint;
                        Result.CurvePointIndex = MakeCurvePointIndexFromBezierPointIndex(BezierIndex);
                        break;
                     }
                  }
               }
               
               curve_point_tracking_state *Tracking = &Curve->PointTracking;
               if (Tracking->Active && (CollisionWithFlags & Collision_TrackedPoint))
               {
                  f32 PointSize = GetCurveTrackedPointSize(Curve);
                  if (PointCollision(CheckPositionLocal,
                                     Tracking->TrackedPoint,
                                     PointSize + CollisionTolerance))
                  {
                     Result.Entity = Entity;
                     Result.Flags |= Collision_TrackedPoint;
                  }
               }
               
               if (CollisionWithFlags & Collision_CurveLine)
               {
                  local_position *CurvePoints = Curve->CurvePoints;
                  u64 CurvePointCount = Curve->CurvePointCount;
                  f32 CurveWidth = 2.0f * CollisionTolerance + Curve->CurveParams.CurveWidth;
                  local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
                  
                  for (u64 CurvePointIndex = 0;
                       CurvePointIndex + 1 < CurvePointCount;
                       ++CurvePointIndex)
                  {
                     v2 P1 = CurvePoints[CurvePointIndex];
                     v2 P2 = CurvePoints[CurvePointIndex + 1];
                     if (SegmentCollision(CheckPositionLocal, P1, P2, CurveWidth))
                     {
                        Result.Entity = Entity;
                        Result.Flags |= Collision_CurveLine;
                        Result.CurveLinePointIndex = CurvePointIndex;
                        break;
                     }
                  }
               }
            } break;
            
            case Entity_Image: {
               image *Image = &Entity->Image;
               if (CollisionWithFlags & Collision_Image)
               {
                  sf::Vector2u TextureSize = Image->Texture.getSize();
                  f32 SizeX = Abs(GlobalImageScaleFactor * Entity->Scale.X * TextureSize.x);
                  f32 SizeY = Abs(GlobalImageScaleFactor * Entity->Scale.Y * TextureSize.y);
                  v2 Extents = 0.5f * V2(SizeX + CollisionTolerance,
                                         SizeY + CollisionTolerance);
                  
                  if (-Extents.X <= CheckPositionLocal.X && CheckPositionLocal.X <= Extents.X &&
                      -Extents.Y <= CheckPositionLocal.Y && CheckPositionLocal.Y <= Extents.Y)
                  {
                     Result.Entity = Entity;
                     Result.Flags |= Collision_Image;
                  }
               }
            } break;
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

internal editor_mode
EditorModeNormal(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Normal;
   
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
   entities *Entities = &Editor->Entities;
   for (u64 EntityIndex = 0;
        EntityIndex < ArrayCount(Entities->Entities);
        ++EntityIndex)
   {
      entity *Current = Entities->Entities + EntityIndex;
      if (!(Current->Flags & EntityFlag_Active))
      {
         Entity = Current;
         Entity->Flags |= EntityFlag_Active;
         ClearArena(Entity->Arena);
         Entity->Curve.PointTracking.Active = false;
         Entity->Curve.DegreeLowering.Active = false;
         ++Editor->EntityCount;
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
   
   Entity->Flags &= ~EntityFlag_Active;
   --Editor->EntityCount;
}

internal void
DeselectCurrentEntity(editor *Editor)
{
   entity *Entity = Editor->SelectedEntity;
   if (Entity)
   {
      Entity->Flags &= ~EntityFlag_Selected;
   }
   Editor->SelectedEntity = 0;
}

internal void
SelectEntity(editor *Editor, entity *Entity)
{
   if (Editor->SelectedEntity)
   {
      DeselectCurrentEntity(Editor);
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
   
   if (Editor->NotificationCount == ArrayCount(Editor->Notifications))
   {
      MemoryMove(Editor->Notifications, Editor->Notifications + 1,
                 SizeOf(notification) * (ArrayCount(Editor->Notifications) - 1));
      --Editor->NotificationCount;
   }
   Assert(Editor->NotificationCount < ArrayCount(Editor->Notifications));
   
   notification *Notification = Editor->Notifications + Editor->NotificationCount++;
   StructZero(Notification);
   Notification->Type = Type;
   u64 ContentCount = FmtV(Notification->ContentBuffer, ArrayCount(Notification->ContentBuffer), Format, Args);
   Notification->Content = MakeStr(Notification->ContentBuffer, ContentCount);
   
   va_end(Args);
}

internal void
EditorSetProjectPath(editor *Editor, b32 Empty, string ProjectPath)
{
   Editor->Empty = Empty;
   ClearArena(Editor->ProjectPathArena);
   Editor->ProjectPath = StrCopy(Editor->ProjectPathArena, ProjectPath);
   
   temp_arena Temp = TempArena(0);
   string WindowTitle = StrLit("Untitled");
   if (!Empty)
   {
      string FileName = StrChopLastSlash(ProjectPath);
      string WithoutExt = StrChopLastDot(FileName);
      WindowTitle = CStrFromStr(Temp.Arena, WithoutExt);
   }
   Editor->RenderData.Window->setTitle(WindowTitle.Data);
   EndTemp(Temp);
}

internal b32
ScreenPointsAreClose(screen_position A, screen_position B,
                     render_data * CoordinateSystemData)
{
   camera_position C = ScreenToCameraSpace(A, CoordinateSystemData);
   camera_position D = ScreenToCameraSpace(B, CoordinateSystemData);
   
   local f32 CameraSpaceEpsilon = 0.01f;
   b32 Result = NormSquared(C - D) <= Square(CameraSpaceEpsilon);
   
   return Result;
}

// TODO(hbr): Move those input functions into editor_input
internal b32
JustReleasedButton(button_state *ButtonState)
{
   b32 Result = (ButtonState->WasPressed && !ButtonState->Pressed);
   return Result;
}

internal b32
ClickedWithButton(button_state *ButtonState,
                  render_data * CoordinateSystemData)
{
   b32 Result = ButtonState->WasPressed && !ButtonState->Pressed &&
      ScreenPointsAreClose(ButtonState->PressPosition,
                           ButtonState->ReleasePosition,
                           CoordinateSystemData);
   
   return Result;
}

internal b32
DraggedWithButton(button_state *ButtonState,
                  v2s MousePosition,
                  render_data * CoordinateSystemData)
{
   b32 Result = ButtonState->Pressed &&
      !ScreenPointsAreClose(ButtonState->PressPosition,
                            MousePosition,
                            CoordinateSystemData);
   
   return Result;
}

internal editor_mode
MakeMovingEntityMode(entity *Entity)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_Entity;
   Result.Moving.Entity = Entity;
   
   return Result;
}

internal editor_mode
MakeMovingCameraMode(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_Camera;
   
   return Result;
}

internal editor_mode
MakeMovingCurvePointMode(entity *CurveEntity,
                         curve_point_index Index,
                         arena *Arena)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_CurvePoint;
   Result.Moving.Entity = CurveEntity;
   Result.Moving.MovingPointIndex = Index;
   
   ClearArena(Arena);
   
   // TODO(hbr): Maybe have some more civilized way of saving the vertices than this
   curve *Curve = &CurveEntity->Curve;
   line_vertices CurveVertices = Curve->CurveVertices;
   sf::Vertex *SavedCurveVertices = PushArrayNonZero(Arena,
                                                     CurveVertices.NumVertices,
                                                     sf::Vertex);
   MemoryCopy(SavedCurveVertices,
              CurveVertices.Vertices,
              CurveVertices.NumVertices * SizeOf(SavedCurveVertices[0]));
   for (u64 CurveVertexIndex = 0;
        CurveVertexIndex < CurveVertices.NumVertices;
        ++CurveVertexIndex)
   {
      sf::Vertex *Vertex = SavedCurveVertices + CurveVertexIndex;
      Vertex->color.a = Cast(u8)(0.2f * Vertex->color.a);
   }
   Result.Moving.SavedCurveVertices = SavedCurveVertices;
   Result.Moving.SavedNumCurveVertices = CurveVertices.NumVertices;
   Result.Moving.SavedPrimitiveType = CurveVertices.PrimitiveType;
   
   return Result;
}

internal editor_mode
MakeRotatingMode(entity *Entity, screen_position Center, button Button)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Rotating;
   Result.Rotating.Entity = Entity;
   Result.Rotating.RotationCenter = Center;
   Result.Rotating.Button = Button;
   
   return Result;
}

internal editor_mode
MakeMovingTrackedPointMode(entity *Entity)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_TrackedPoint;
   Result.Moving.Entity = Entity;
   
   return Result;
}

internal void SetCurveControlPoints(entity *Entity, u64 ControlPointCount, local_position *ControlPoints,
                                    f32 *ControlPointWeights, local_position *CubicBezierPoints);
internal void SelectControlPoint(curve *Curve, control_point_index Index);

// TODO(hbr): Maybe move into editor_entity.h
internal void
InitEntityFromEntity(entity *Dest, entity *Src)
{
   InitEntity(Dest, Src->Position, Src->Scale, Src->Rotation, Src->Name, Src->SortingLayer);
   switch (Src->Type)
   {
      case Entity_Curve: {
         curve *Curve = GetCurve(Src);
         InitCurve(Dest, Curve->CurveParams);
         SetCurveControlPoints(Dest, Curve->ControlPointCount, Curve->ControlPoints, Curve->ControlPointWeights, Curve->CubicBezierPoints);
         SelectControlPoint(GetCurve(Dest), Curve->SelectedIndex);
      } break;
      
      case Entity_Image: {
         image *Image = GetImage(Src);
         InitImage(Dest, Image->ImagePath, &Image->Texture);
      } break;
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
   
   BeginCurvePoints(&LeftEntity->Curve, ControlPointCount);
   BeginCurvePoints(&RightEntity->Curve, ControlPointCount);
   
   BezierCurveSplit(Tracking->T,
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
   
   EndCurvePoints(&RightEntity->Curve);
   EndCurvePoints(&LeftEntity->Curve);
   
   EndTemp(Temp);
}

internal void
ExecuteUserActionNormalMode(editor *Editor, user_action Action)
{
   temp_arena Temp = TempArena(0);
   switch (Action.Type)
   {
      case UserAction_ButtonClicked: {
         world_position ClickPosition = ScreenToWorldSpace(Action.Click.ClickPosition, &Editor->RenderData);
         switch (Action.Click.Button)
         {
            case Button_Left: {
               collision Collision = {};
               if (Action.Input->Keys[Key_LeftShift].Pressed)
               {
                  // NOTE(hbr): Force no collision when shift if pressed, so that user can
                  // add control point wherever he wants
               }
               else
               {
                  collision_flags CollisionWithFlags = (Collision_CurvePoint | Collision_CurveLine);
                  f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                                                       &Editor->RenderData);
                  Collision = CheckCollisionWith(&Editor->Entities, ClickPosition, CollisionTolerance,
                                                 &Editor->Params, CollisionWithFlags);
               }
               
               entity *FocusEntity = 0;
               b32 FocusCurvePoint = false;
               curve_point_index FocusCurvePointIndex = {};
               if (Collision.Entity)
               {
                  FocusEntity = Collision.Entity;
                  
                  curve *Curve = GetCurve(Collision.Entity);
                  
                  curve_point_tracking_state *Tracking = &Curve->PointTracking;
                  if (Tracking->Active && (Collision.Flags & Collision_CurveLine))
                  {
                     f32 T = SafeDiv0(Cast(f32)Collision.CurveLinePointIndex, (Curve->CurvePointCount- 1));
                     T = Clamp(T, 0.0f, 1.0f); // NOTE(hbr): Be safe
                     SetTrackingPointT(Tracking, T);
                  }
                  else if (Collision.Flags & Collision_CurvePoint)
                  {
                     FocusCurvePoint = true;
                     FocusCurvePointIndex = Collision.CurvePointIndex;
                  }
                  else if (Collision.Flags & Collision_CurveLine)
                  {
                     control_point_index Index = CurvePointIndexToControlPointIndex(Curve, Collision.CurveLinePointIndex);
                     u64 InsertAt = Index.Index + 1;
                     InsertControlPoint(Collision.Entity, ClickPosition, InsertAt);
                     FocusCurvePoint = true;
                     FocusCurvePointIndex = MakeCurvePointIndexFromControlPointIndex(InsertAt);
                  }
               }
               else
               {
                  if (Editor->SelectedEntity &&
                      Editor->SelectedEntity->Type == Entity_Curve)
                  {
                     FocusEntity = Editor->SelectedEntity;
                  }
                  else
                  {
                     FocusEntity = AllocEntity(Editor);
                     string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EntityCounter++);
                     InitEntity(FocusEntity, V2(0.0f, 0.0f), V2(1.0f, 1.0f), Rotation2DZero(), Name, 0);
                     InitCurve(FocusEntity, Editor->Params.CurveDefaultParams);
                  }
                  Assert(FocusEntity);
                  
                  FocusCurvePoint = true;
                  control_point_index Appended = AppendControlPoint(FocusEntity, ClickPosition);
                  FocusCurvePointIndex = MakeCurvePointIndexFromControlPointIndex(Appended);
               }
               
               SelectEntity(Editor, FocusEntity);
               if (FocusCurvePoint)
               {
                  curve *FocusCurve = GetCurve(FocusEntity);
                  SelectControlPointFromCurvePointIndex(FocusCurve, FocusCurvePointIndex);
               }
#if 0
               if (Collision.Entity)
               {
                  curve *Curve = GetCurve(Collision.Entity);
                  b32 ClickActionDone = false; 
                  
                  if (Editor->CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget &&
                      Collision.Entity != Editor->CurveAnimation.FromCurveEntity)
                  {
                     Editor->CurveAnimation.ToCurveEntity = Collision.Entity;
                     ClickActionDone = true;
                  }
                  
                  curve_combining_state *Combining = &Editor->CurveCombining;
                  if (Combining->SourceEntity)
                  {
                     if (Collision.Entity != Combining->SourceEntity)
                     {
                        Combining->WithEntity = Collision.Entity;
                     }
                     
                     // NOTE(hbr): Logic to recognize whether first/last point of
                     // combine/target curve was clicked. If so then change which
                     // point should be combined with which one (first with last,
                     // first with first, ...). On top of that if user clicked
                     // on already selected point but in different direction
                     // (target->combine rather than combine->target) then swap
                     // target and combine curves to combine in the other direction.
                     
                     // TODO(hbr): Probably simplify this logic
                     if (Collision.Type == CurveCollision_ControlPoint)
                     {
                        if (Collision.Entity == Combining->SourceEntity)
                        {
                           curve *SourceCurve = GetCurve(Combining->SourceEntity);
                           if (Collision.PointIndex == 0)
                           {
                              if (!Combining->SourceCurveLastControlPoint)
                              {
                                 Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
                                 Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
                                 Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
                                 Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
                              }
                              else
                              {
                                 Combining->SourceCurveLastControlPoint = false;
                              }
                           }
                           else if (Collision.PointIndex == SourceCurve->ControlPointCount - 1)
                           {
                              if (Combining->SourceCurveLastControlPoint)
                              {
                                 Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
                                 Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
                                 Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
                                 Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
                              }
                              else
                              {
                                 Combining->SourceCurveLastControlPoint = true;
                              }
                           }
                        }
                        else if (Collision.Entity == Combining->WithEntity)
                        {
                           curve *WithCurve = GetCurve(Combining->WithEntity);
                           if (Collision.PointIndex == 0)
                           {
                              Combining->WithCurveFirstControlPoint = true;
                           }
                           else if (Collision.PointIndex == WithCurve->ControlPointCount - 1)
                           {
                              Combining->WithCurveFirstControlPoint = false;
                           }
                        }
                     }
                     
                     ClickActionDone = true;
                  }
               }
               else
               {
                  
               }
#endif
            } break;
            
            case Button_Right: {
               collision_flags CollisionWithFlags = (Collision_CurvePoint | Collision_TrackedPoint | Collision_Image);
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                                                    &Editor->RenderData);
               collision Collision = CheckCollisionWith(&Editor->Entities, ClickPosition, CollisionTolerance,
                                                        &Editor->Params, CollisionWithFlags);
               
               if (Collision.Entity)
               {
                  switch (Collision.Entity->Type)
                  {
                     case Entity_Curve: {
                        curve *Curve = GetCurve(Collision.Entity);
                        
                        if ((Collision.Flags & Collision_TrackedPoint) &&
                            Curve->PointTracking.IsSplitting)
                        {
                           PerformBezierCurveSplit(Editor, Collision.Entity);
                        }
                        else if (Collision.Flags & Collision_CurvePoint)
                        {
                           if (Collision.CurvePointIndex.Type == CurvePoint_ControlPoint)
                           {
                              RemoveControlPoint(Collision.Entity, Collision.CurvePointIndex.ControlPoint);
                           }
                           SelectEntity(Editor, Collision.Entity);
                        }
                     } break;
                     
                     case Entity_Image: {
                        DeallocEntity(Editor, Collision.Entity);
                     } break;
                  }
               }
               else
               {
                  DeselectCurrentEntity(Editor);
               }
            } break;
            
            case Button_Middle: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonDrag: {
         button DragButton = Action.Drag.Button;
         screen_position DragFromScreenPosition = Action.Drag.DragFromPosition;
         world_position DragFromWorldPosition = ScreenToWorldSpace(DragFromScreenPosition,
                                                                   &Editor->RenderData);
         
         switch (DragButton)
         {
            case Button_Left: {
               collision_flags CollisionWithFlags =
                  Collision_CurvePoint |
                  Collision_TrackedPoint |
                  Collision_CurveLine |
                  Collision_Image;
               f32 CollisionTolerance =
                  ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                              &Editor->RenderData);
               collision Collision = CheckCollisionWith(&Editor->Entities,
                                                        DragFromWorldPosition,
                                                        CollisionTolerance,
                                                        &Editor->Params,
                                                        CollisionWithFlags);
               
               if (Collision.Entity)
               {
                  // TODO(hbr): Try to simplify this logic - we effectively just translate CurveCollision_* to MovingMode_*
                  switch (Collision.Entity->Type)
                  {
                     case Entity_Curve: {
                        if (Collision.Flags & Collision_TrackedPoint)
                        {
                           Editor->Mode = MakeMovingTrackedPointMode(Collision.Entity);
                        }
                        else if (Collision.Flags & Collision_CurvePoint)
                        {
                           curve *Curve = GetCurve(Collision.Entity);
                           SelectControlPointFromCurvePointIndex(Curve, Collision.CurvePointIndex);
                           Editor->Mode = MakeMovingCurvePointMode(Collision.Entity, Collision.CurvePointIndex,
                                                                   Editor->MovingPointArena);
                        }
                        else if (Collision.Flags & Collision_CurveLine)
                        {
                           Editor->Mode = MakeMovingEntityMode(Collision.Entity);
                        }
                     } break;
                     
                     case Entity_Image: {
                        Editor->Mode = MakeMovingEntityMode(Collision.Entity);
                     } break;
                  }
                  
                  SelectEntity(Editor, Collision.Entity);
               }
               else
               {
                  Editor->Mode = MakeMovingCameraMode(); 
               }
            } break;
            
            case Button_Right: {
               Editor->Mode = MakeRotatingMode(Editor->SelectedEntity, DragFromScreenPosition, DragButton);
            } break;
            
            case Button_Middle: { 
               Editor->Mode = MakeRotatingMode(0, DragFromScreenPosition, DragButton);
            } break;
            
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonReleased: {} break;
      case UserAction_MouseMove: {} break;
      case UserAction_None: {} break;
   }
   
   EndTemp(Temp);
}

internal void
ExecuteUserActionMoveMode(editor *Editor, user_action Action)
{
   auto *Moving = &Editor->Mode.Moving;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         world_position From = ScreenToWorldSpace(Action.MouseMove.FromPosition, &Editor->RenderData);
         world_position To = ScreenToWorldSpace(Action.MouseMove.ToPosition, &Editor->RenderData);
         v2 Translation = To - From;
         
         switch (Moving->Type)
         {
            case MovingMode_Entity: {
               Moving->Entity->Position += Translation;
            } break;
            
            case MovingMode_Camera: {
               MoveCamera(&Editor->RenderData.Camera, -Translation);
            } break;
            
            case MovingMode_CurvePoint: {
               translate_curve_point_flags Flags = 0;
               if (!Action.Input->Keys[Key_LeftCtrl].Pressed)  Flags |= TranslateCurvePoint_MatchBezierTwinDirection;
               if (!Action.Input->Keys[Key_LeftShift].Pressed) Flags |= TranslateCurvePoint_MatchBezierTwinLength;
               TranslateCurvePoint(Moving->Entity, Moving->MovingPointIndex, Translation, Flags);
            } break;
            
            case MovingMode_TrackedPoint: {
               entity *Entity = Moving->Entity;
               curve *Curve = GetCurve(Entity);
               
               // NOTE(hbr): Pretty involved logic to move point along the curve and have pleasant experience
               u64 CurvePointCount = Curve->CurvePointCount;
               local_position *CurvePoints = Curve->CurvePoints;
               curve_point_tracking_state *Tracking = &Curve->PointTracking;
               
               f32 T = Curve->PointTracking.T;
               f32 DeltaT = 1.0f / (CurvePointCount - 1);
               {
                  u64 SplitCurvePointIndex = ClampTop(Cast(u64)FloorF32(T * (CurvePointCount - 1)), CurvePointCount - 1);
                  while (SplitCurvePointIndex + 1 < CurvePointCount)
                  {
                     f32 NextT = Cast(f32)(SplitCurvePointIndex + 1) / (CurvePointCount - 1);
                     f32 PrevT = Cast(f32)SplitCurvePointIndex / (CurvePointCount - 1);
                     T = Clamp(T, PrevT, NextT);
                     
                     f32 SegmentFraction = (NextT - T) / DeltaT;
                     SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
                     
                     v2 CurveSegment =
                        LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex + 1])
                        - LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex]);
                     
                     f32 CurveSegmentLength = Norm(CurveSegment);
                     f32 InvCurveSegmentLength = 1.0f / CurveSegmentLength;
                     
                     f32 ProjectionSegmentFraction = ClampBot(Dot(CurveSegment, Translation) *
                                                              InvCurveSegmentLength * InvCurveSegmentLength,
                                                              0.0f);
                     
                     if (SegmentFraction <= ProjectionSegmentFraction)
                     {
                        T = NextT;
                        Translation -= SegmentFraction * CurveSegment;
                        SplitCurvePointIndex += 1;
                     }
                     else
                     {
                        T += ProjectionSegmentFraction * DeltaT;
                        Translation -= ProjectionSegmentFraction * CurveSegment;
                        break;
                     }
                  }
               }
               {
                  u64 SplitCurvePointIndex = ClampTop(Cast(u64)CeilF32(T * (CurvePointCount - 1)), CurvePointCount - 1);
                  while (SplitCurvePointIndex >= 1)
                  {
                     f32 NextT = Cast(f32)(SplitCurvePointIndex - 1) / (CurvePointCount - 1);
                     f32 PrevT = Cast(f32)SplitCurvePointIndex / (CurvePointCount - 1);
                     T = Clamp(T, NextT, PrevT);
                     
                     f32 SegmentFraction = (T - NextT) / DeltaT;
                     SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
                     
                     v2 CurveSegment =
                        LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex - 1])
                        - LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex]);
                     
                     f32 CurveSegmentLength = Norm(CurveSegment);
                     f32 InvCurveSegmentLength = 1.0f / CurveSegmentLength;
                     
                     f32 ProjectionSegmentFraction = ClampBot(Dot(CurveSegment, Translation) *
                                                              InvCurveSegmentLength * InvCurveSegmentLength,
                                                              0.0f);
                     
                     if (SegmentFraction <= ProjectionSegmentFraction)
                     {
                        T = NextT;
                        Translation -= SegmentFraction * CurveSegment;
                        SplitCurvePointIndex -= 1;
                     }
                     else
                     {
                        T -= ProjectionSegmentFraction * DeltaT;
                        Translation -= ProjectionSegmentFraction * CurveSegment;
                        break;
                     }
                  }
               }
               SetTrackingPointT(Tracking, T);
            } break;
         }
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Release.Button)
         {
            case Button_Left: { Editor->Mode = EditorModeNormal(); } break;
            case Button_Middle:
            case Button_Right: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag:
      case UserAction_None: {} break;
   }
}

struct stable_rotation
{
   b32 IsStable;
   rotation_2d Rotation;
   world_position RotationCenter;
};
internal stable_rotation
CalculateRotationIfStable(screen_position From,
                          screen_position To,
                          camera_position RotationCenterCamera,
                          render_data * CoordinateSystemData,
                          editor_params Params)
{
   stable_rotation Result = {};
   
   world_position FromWorld = ScreenToWorldSpace(From, CoordinateSystemData);
   world_position ToWorld = ScreenToWorldSpace(To, CoordinateSystemData);
   world_position CenterWorld = CameraToWorldSpace(RotationCenterCamera, CoordinateSystemData);
   f32 DeadDistance = ClipSpaceLengthToWorldSpace(Params.RotationIndicator.RadiusClipSpace,
                                                  CoordinateSystemData);
   
   if (NormSquared(FromWorld - CenterWorld) >= Square(DeadDistance) &&
       NormSquared(ToWorld   - CenterWorld) >= Square(DeadDistance))
   {
      Result.IsStable = true;
      Result.RotationCenter = CenterWorld;
      Result.Rotation = Rotation2DFromMovementAroundPoint(FromWorld, ToWorld, CenterWorld);
   }
   
   return Result;
}

internal void
ExecuteUserActionRotateMode(editor *Editor, user_action Action)
{
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         entity *Entity = Editor->Mode.Rotating.Entity;
         
         // TODO(hbr): Merge cases below
         camera_position RotationCenter = {};
         if (Entity)
         {
            switch (Entity->Type)
            {
               case Entity_Curve: {
                  RotationCenter = ScreenToCameraSpace(Editor->Mode.Rotating.RotationCenter,
                                                       &Editor->RenderData);
               } break;
               case Entity_Image: {
                  RotationCenter = WorldToCameraSpace(Entity->Position, &Editor->RenderData);
               } break;
            }
         }
         else
         {
            RotationCenter = WorldToCameraSpace(Editor->RenderData.Camera.Position, &Editor->RenderData);
         }
         
         stable_rotation StableRotation =
            CalculateRotationIfStable(Action.MouseMove.FromPosition,
                                      Action.MouseMove.ToPosition,
                                      RotationCenter,
                                      &Editor->RenderData,
                                      Editor->Params);
         
         if (StableRotation.IsStable)
         {
            if (Entity)
            {
               switch (Entity->Type)
               {
                  // TODO(hbr): Merging case [Entity_Curve] with [Entity_Image] is possible (and necessary xd) for sure.
                  case Entity_Curve: {
                     CurveRotateAround(Entity,
                                       StableRotation.RotationCenter,
                                       StableRotation.Rotation);
                  } break;
                  
                  case Entity_Image: {
                     rotation_2d NewRotation = CombineRotations2D(Entity->Rotation, StableRotation.Rotation);
                     Entity->Rotation = NewRotation;
                  } break;
               }
            }
            else
            {
               rotation_2d InverseRotation = Rotation2DInverse(StableRotation.Rotation);
               RotateCamera(&Editor->RenderData.Camera, InverseRotation);
            }
         }
      } break;
      
      case UserAction_ButtonReleased: {
         if (Action.Release.Button == Editor->Mode.Rotating.Button)
         {
            Editor->Mode = EditorModeNormal();
         }
      } break;
      
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag:
      case UserAction_None: {} break;
   }
}

internal void
ExecuteUserAction(editor *Editor, user_action Action)
{
   switch (Editor->Mode.Type)
   {
      case EditorMode_Normal:   { ExecuteUserActionNormalMode(Editor, Action); } break;
      case EditorMode_Moving:   { ExecuteUserActionMoveMode(Editor, Action); } break;
      case EditorMode_Rotating: { ExecuteUserActionRotateMode(Editor, Action); } break;
   }
}

internal void
RenderPoint(v2 Position,
            render_point_data PointData,
            render_data *RenderData,
            sf::Transform Transform)
{
   f32 TotalRadius = ClipSpaceLengthToWorldSpace(PointData.RadiusClipSpace, RenderData);
   f32 OutlineThickness = PointData.OutlineThicknessFraction * TotalRadius;
   f32 InsideRadius = TotalRadius - OutlineThickness;
   
   DrawCircle(Position, InsideRadius, PointData.FillColor,
              Transform, RenderData->Window, OutlineThickness,
              PointData.OutlineColor);
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
   f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, &Editor->RenderData);
   v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
   Copy->Position += SlightTranslation;
   
   EndTemp(Temp);
}

// NOTE(hbr): This should really be some highly local internal, don't expect to reuse.
// It's highly specific.
internal b32
ResetCtxMenu_(string Label)
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
#define ResetCtxMenu(LabelLit) ResetCtxMenu_(StrLit(LabelLit))

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
      Assert(Curve->CurveParams.InterpolationType == Interpolation_Bezier);
      temp_arena Temp = TempArena(Lowering->Arena);
      
      local_position *LowerPoints = PushArrayNonZero(Temp.Arena, PointCount, local_position);
      f32 *LowerWeights = PushArrayNonZero(Temp.Arena, PointCount, f32);
      cubic_bezier_point *LowerBeziers = PushArrayNonZero(Temp.Arena, PointCount - 1, cubic_bezier_point);
      ArrayCopy(LowerPoints, Curve->ControlPoints, PointCount);
      ArrayCopy(LowerWeights, Curve->ControlPointWeights, PointCount);
      
      bezier_lower_degree LowerDegree = BezierCurveLowerDegree(LowerPoints, LowerWeights, PointCount);
      if (LowerDegree.Failure)
      {
         Lowering->Active = true;
         
         ClearArena(Lowering->Arena);
         
         Lowering->SavedControlPoints = PushArrayNonZero(Lowering->Arena, PointCount, local_position);
         Lowering->SavedControlPointWeights = PushArrayNonZero(Lowering->Arena, PointCount, f32);
         Lowering->SavedCubicBezierPoints = PushArrayNonZero(Lowering->Arena, PointCount, cubic_bezier_point);
         ArrayCopy(Lowering->SavedControlPoints, Curve->ControlPoints, PointCount);
         ArrayCopy(Lowering->SavedControlPointWeights, Curve->ControlPointWeights, PointCount);
         ArrayCopy(Lowering->SavedCubicBezierPoints, Curve->CubicBezierPoints, PointCount);
         
         line_vertices CurveVertices = Curve->CurveVertices;
         Lowering->NumSavedCurveVertices = CurveVertices.NumVertices;
         Lowering->SavedCurveVertices = PushArrayNonZero(Lowering->Arena, CurveVertices.NumVertices, sf::Vertex);
         Lowering->SavedPrimitiveType = CurveVertices.PrimitiveType;
         MemoryCopy(Lowering->SavedCurveVertices, CurveVertices.Vertices, CurveVertices.NumVertices * SizeOf(Lowering->SavedCurveVertices[0]));
         for (u64 CurveVertexIndex = 0;
              CurveVertexIndex < CurveVertices.NumVertices;
              ++CurveVertexIndex)
         {
            sf::Vertex *Vertex = Lowering->SavedCurveVertices + CurveVertexIndex;
            Vertex->color.a = Cast(u8)(0.5f * Vertex->color.a);
         }
         
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
ElevateBezierCurveDegree(entity *Entity, editor_params *EditorParams)
{
   curve *Curve = GetCurve(Entity);
   Assert(Curve->CurveParams.InterpolationType == Interpolation_Bezier);
   temp_arena Temp = TempArena(0);
   
   u64 ControlPointCount = Curve->ControlPointCount;
   local_position *ElevatedControlPoints = PushArrayNonZero(Temp.Arena,
                                                            ControlPointCount + 1,
                                                            local_position);
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

internal void
FocusCameraOn(editor *Editor, world_position Position, v2 Extents)
{
   Editor->RenderData.Camera.PositionTarget = Position;
   local f32 Margin = 0.7f;
   f32 ZoomX = 0.5f * (1.0f - Margin) * Editor->RenderData.FrustumSize * Editor->RenderData.AspectRatio / Extents.X;
   f32 ZoomY = 0.5f * (1.0f - Margin) * Editor->RenderData.FrustumSize / Extents.Y;
   Editor->RenderData.Camera.ZoomTarget = Min(ZoomX, ZoomY);
   Editor->RenderData.Camera.ReachingTarget = true;
}

// TODO(hbr): Optimize this function
internal void
FocusCameraOnEntity(editor *Editor, entity *Entity)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         if (Curve->ControlPointCount > 0)
         {
            f32 Left = INF_F32, Right = -INF_F32;
            f32 Down = INF_F32, Top = -INF_F32;
            for (u64 ControlPointIndex = 0;
                 ControlPointIndex < Curve->ControlPointCount;
                 ++ControlPointIndex)
            {
               world_position ControlPointWorld = LocalEntityPositionToWorld(Entity,
                                                                             Curve->ControlPoints[ControlPointIndex]);
               v2 ControlPointRotated = RotateAround(ControlPointWorld,
                                                     V2(0.0f, 0.0f),
                                                     Rotation2DInverse(Editor->RenderData.Camera.Rotation));
               
               if (ControlPointRotated.X < Left)  Left  = ControlPointRotated.X;
               if (ControlPointRotated.X > Right) Right = ControlPointRotated.X;
               if (ControlPointRotated.Y < Down)  Down  = ControlPointRotated.Y;
               if (ControlPointRotated.Y > Top)   Top   = ControlPointRotated.Y;
            }
            
            world_position FocusPosition =  RotateAround(V2(0.5f * (Left + Right), 0.5f * (Down + Top)),
                                                         V2(0.0f, 0.0f), Editor->RenderData.Camera.Rotation);
            v2 Extents = V2(0.5f * (Right - Left) + 2.0f * Curve->CurveParams.PointRadius,
                            0.5f * (Top - Down) + 2.0f * Curve->CurveParams.PointRadius);
            FocusCameraOn(Editor, FocusPosition, Extents);
         }
      } break;
      
      case Entity_Image: {
         f32 ScaleX = Abs(Entity->Scale.X);
         f32 ScaleY = Abs(Entity->Scale.Y);
         if (ScaleX != 0.0f && ScaleY != 0.0f)
         {
            sf::Vector2u TextureSize = Entity->Image.Texture.getSize();
            v2 Extents = V2(0.5f * GlobalImageScaleFactor * ScaleX * TextureSize.x,
                            0.5f * GlobalImageScaleFactor * ScaleY * TextureSize.y);
            FocusCameraOn(Editor, Entity->Position, Extents);
         }
      } break;
   }
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
      
      b32 LeftOK = BeginCurvePoints(LeftCurve, LeftPointCount);
      b32 RightOK = BeginCurvePoints(RightCurve, RightPointCount);
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
      
      EndCurvePoints(&LeftEntity->Curve);
      EndCurvePoints(&RightEntity->Curve);
      
      EndTemp(Temp);
   }
}

// TODO(hbr): Do a pass oveer this internal to simplify the logic maybe (and simplify how the UI looks like in real life)
// TODO(hbr): Maybe also shorten some labels used to pass to ImGUI
internal void
RenderSelectedEntityUI(editor *Editor)
{
   entity *Entity = Editor->SelectedEntity;
   ui_config *Config = &Editor->UI_Config;
   if (Config->ViewSelectedEntityWindow && Entity)
   {
      UI_LabelF("SelectedEntity")
      {
         if (UI_BeginWindowF(&Config->ViewSelectedEntityWindow, "Entity Parameters"))
         {
            curve *Curve = 0;
            image *Image = 0;
            switch (Entity->Type)
            {
               case Entity_Curve: { Curve = &Entity->Curve; } break;
               case Entity_Image: { Image = &Entity->Image; } break;
            }
            
            Entity->Name = UI_InputTextF(Entity->NameBuffer, ArrayCount(Entity->NameBuffer), "Name");
            
            if (Image)
            {
               UI_NewRow();
               UI_SeparatorTextF("Image");
            }
            
            if (Curve)
            {
               UI_DragFloat2F(Entity->Position.Es, 0.0f, 0.0f, 0, "Position");
               if (ResetCtxMenu("PositionReset"))
               {
                  Entity->Position = V2(0.0f, 0.0f);
               }
            }
            
            UI_AngleSliderF(&Entity->Rotation, "Rotation");
            if (ResetCtxMenu("RotationReset"))
            {
               Entity->Rotation = Rotation2DZero();
            }
            
            if (Image)
            {
               sf::Vector2u ImageTextureSize = Image->Texture.getSize();
               v2 SelectedImageScale = Entity->Scale;
               f32 ImageWidth = SelectedImageScale.X * ImageTextureSize.x;
               f32 ImageHeight = SelectedImageScale.Y * ImageTextureSize.y;
               f32 ImageScale = 1.0f;
               
               UI_DragFloatF(&ImageWidth, 0.0f, 0.0f, 0, "Width");
               if (ResetCtxMenu("WidthReset"))
               {
                  ImageWidth = Cast(f32)ImageTextureSize.x;
               }
               
               UI_DragFloatF(&ImageHeight, 0.0f, 0.0f, 0, "Height");
               if (ResetCtxMenu("HeightReset"))
               {
                  ImageHeight = Cast(f32)ImageTextureSize.y;
               }
               
               UI_DragFloatF(&ImageScale, 0.0f, FLT_MAX, "Drag Me!", "Scale");
               if (ResetCtxMenu("ScaleReset"))
               {
                  ImageWidth = Cast(f32)ImageTextureSize.x;
                  ImageHeight = Cast(f32)ImageTextureSize.y;
               }
               
               f32 ImageNewScaleX = ImageScale * ImageWidth / ImageTextureSize.x;
               f32 ImageNewScaleY = ImageScale * ImageHeight / ImageTextureSize.y;
               Entity->Scale = V2(ImageNewScaleX, ImageNewScaleY);
            }
            
            UI_SliderIntegerF(&Entity->SortingLayer, -100, 100, "Sorting Layer");
            if (ResetCtxMenu("SortingLayerReset"))
            {
               Entity->SortingLayer = 0;
            }
            
            b32 SomeCurveParamChanged = false;
            if (Curve)
            {
               curve_params DefaultParams = Editor->Params.CurveDefaultParams;
               curve_params *CurveParams = &Curve->CurveParams;
               
               UI_NewRow();
               UI_SeparatorTextF("Curve");
               UI_LabelF("Curve")
               {
                  SomeCurveParamChanged |= UI_ComboF(&CurveParams->InterpolationType, Interpolation_Count, InterpolationNames, "Interpolation Type");
                  if (ResetCtxMenu("InterpolationTypeReset"))
                  {
                     CurveParams->InterpolationType = DefaultParams.InterpolationType;
                     SomeCurveParamChanged = true;
                  }
                  
                  switch (CurveParams->InterpolationType)
                  {
                     case Interpolation_Polynomial: {
                        polynomial_interpolation_params *PolynomialParams = &CurveParams->PolynomialInterpolationParams;
                        
                        SomeCurveParamChanged |= UI_ComboF(&PolynomialParams->Type, PolynomialInterpolation_Count, PolynomialInterpolationNames, "Polynomial Type");
                        if (ResetCtxMenu("PolynomialTypeReset"))
                        {
                           PolynomialParams->Type = DefaultParams.PolynomialInterpolationParams.Type;
                           SomeCurveParamChanged = true;
                        }
                        
                        SomeCurveParamChanged |= UI_ComboF(&PolynomialParams->PointsArrangement, PointsArrangement_Count, PointsArrangementNames, "Points Arrangement");
                        if (ResetCtxMenu("PointsArrangementReset"))
                        {
                           PolynomialParams->PointsArrangement =
                              DefaultParams.PolynomialInterpolationParams.PointsArrangement;
                           SomeCurveParamChanged = true;
                        }
                     } break;
                     
                     case Interpolation_CubicSpline: {
                        SomeCurveParamChanged |= UI_ComboF(&CurveParams->CubicSplineType, CubicSpline_Count, CubicSplineNames, "Spline Types");
                        if (ResetCtxMenu("SplineTypeReset"))
                        {
                           CurveParams->CubicSplineType = DefaultParams.CubicSplineType;
                           SomeCurveParamChanged = true;
                        }
                     } break;
                     
                     case Interpolation_Bezier: {
                        SomeCurveParamChanged |= UI_ComboF(&CurveParams->BezierType, Bezier_Count, BezierNames, "Bezier Types");
                        if (ResetCtxMenu("BezierTypeReset"))
                        {
                           CurveParams->BezierType = DefaultParams.BezierType;
                           SomeCurveParamChanged = true;
                        }
                     } break;
                     
                     case Interpolation_Count: InvalidPath;
                  }
               }
               
               UI_NewRow();
               UI_SeparatorTextF("Line");
               UI_LabelF("Line")
               {
                  SomeCurveParamChanged |= UI_ColorPickerF(&CurveParams->CurveColor, "Color");
                  if (ResetCtxMenu("CurveColorReset"))
                  {
                     CurveParams->CurveColor = DefaultParams.CurveColor;
                     SomeCurveParamChanged = true;
                  }
                  
                  SomeCurveParamChanged |= UI_DragFloatF(&CurveParams->CurveWidth, 0.0f, FLT_MAX, 0, "Width");
                  if (ResetCtxMenu("CurveWidthReset"))
                  {
                     CurveParams->CurveWidth = DefaultParams.CurveWidth;
                     SomeCurveParamChanged = true;
                  }
                  
                  SomeCurveParamChanged |= UI_SliderIntegerF(&CurveParams->CurvePointCountPerSegment, 1, 2000, "Detail");
                  if (ResetCtxMenu("DetailReset"))
                  {
                     CurveParams->CurvePointCountPerSegment = DefaultParams.CurvePointCountPerSegment;
                     SomeCurveParamChanged = true;
                  }
               }
               
               UI_NewRow();
               UI_SeparatorTextF("Control Points");
               UI_LabelF("ControlPoints")
               {
                  UI_CheckboxF(&CurveParams->PointsDisabled, "Points Disabled");
                  if (ResetCtxMenu("PointsDisabledReset"))
                  {
                     CurveParams->PointsDisabled= DefaultParams.PointsDisabled;
                  }
                  
                  if (!CurveParams->PointsDisabled)
                  {                     
                     SomeCurveParamChanged |= UI_ColorPickerF(&CurveParams->PointColor, "Color");
                     if (ResetCtxMenu("PointColorReset"))
                     {
                        CurveParams->PointColor = DefaultParams.PointColor;
                        SomeCurveParamChanged = true;
                     }
                     
                     SomeCurveParamChanged |= UI_DragFloatF(&CurveParams->PointRadius, 0.0f, FLT_MAX, 0, "Radius");
                     if (ResetCtxMenu("PointRadiusReset"))
                     {
                        CurveParams->PointRadius = DefaultParams.PointRadius;
                        SomeCurveParamChanged = true;
                     }
                     
                     {
                        // TODO(hbr): Maybe let ControlPoint weight to be 0 as well
                        local f32 ControlPointMinWeight = 0.0f;
                        local f32 ControlPointMaxWeight = FLT_MAX;
                        if (CurveParams->InterpolationType == Interpolation_Bezier &&
                            CurveParams->BezierType == Bezier_Regular)
                        {
                           UI_SeparatorTextF("Weights");
                           
                           curve *SelectedCurve = Curve;
                           u64 ControlPointCount = SelectedCurve->ControlPointCount;
                           f32 *ControlPointWeights = SelectedCurve->ControlPointWeights;
                           
                           if (IsControlPointSelected(Curve))
                           {
                              u64 SelectedIndex = Curve->SelectedIndex.Index;
                              SomeCurveParamChanged |= UI_DragFloatF(&ControlPointWeights[SelectedIndex],
                                                                     ControlPointMinWeight, ControlPointMaxWeight,
                                                                     0, "Selected Point (%llu)", SelectedIndex);
                              if (ResetCtxMenu("SelectedPointWeightReset"))
                              {
                                 ControlPointWeights[SelectedIndex] = 1.0f;
                              }
                              
                              ImGui::Spacing();
                           }
                           
                           if (ImGui::TreeNode("All Points"))
                           {
                              for (u64 ControlPointIndex = 0;
                                   ControlPointIndex < ControlPointCount;
                                   ++ControlPointIndex)
                              {
                                 DeferBlock(UI_PushId(ControlPointIndex), UI_PopId())
                                 {                              
                                    SomeCurveParamChanged |= UI_DragFloatF(&ControlPointWeights[ControlPointIndex],
                                                                           ControlPointMinWeight, ControlPointMaxWeight,
                                                                           0, "Point (%llu)", ControlPointIndex);
                                    if (ResetCtxMenu("PointWeightReset"))
                                    {
                                       ControlPointWeights[ControlPointIndex] = 1.0f;
                                    }
                                 }
                              }
                              
                              ImGui::TreePop();
                           }
                        }
                     }
                  }
                  
               }
               
               UI_NewRow();
               UI_SeparatorTextF("Polyline");
               DeferBlock(UI_PushLabelF("Polyline"), UI_PopId())
               {
                  UI_CheckboxF(&CurveParams->PolylineEnabled, "Enabled");
                  if (ResetCtxMenu("PolylineEnabled"))
                  {
                     CurveParams->PolylineEnabled = DefaultParams.PolylineEnabled;
                  }
                  
                  if (CurveParams->PolylineEnabled)
                  {
                     SomeCurveParamChanged |= UI_ColorPickerF(&CurveParams->PolylineColor, "Color");
                     if (ResetCtxMenu("PolylineColorReset"))
                     {
                        CurveParams->PolylineColor = DefaultParams.PolylineColor;
                        SomeCurveParamChanged = true;
                     }
                     
                     SomeCurveParamChanged |= UI_DragFloatF(&CurveParams->PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
                     if (ResetCtxMenu("PolylineWidthReset"))
                     {
                        CurveParams->PolylineWidth = DefaultParams.PolylineWidth;
                        SomeCurveParamChanged = true;
                     }
                  }
               }
               
               UI_NewRow();
               UI_SeparatorTextF("Convex Hull");
               DeferBlock(UI_PushLabelF("ConvexHull"), UI_PopId())
               {
                  UI_CheckboxF(&CurveParams->ConvexHullEnabled, "Enabled");
                  if (ResetCtxMenu("ConvexHullEnabledReset"))
                  {
                     CurveParams->ConvexHullEnabled = DefaultParams.ConvexHullEnabled;
                  }
                  
                  if (CurveParams->ConvexHullEnabled)
                  {
                     SomeCurveParamChanged |= UI_ColorPickerF(&CurveParams->ConvexHullColor, "Color");
                     if (ResetCtxMenu("ConvexHullColorReset"))
                     {
                        CurveParams->ConvexHullColor = DefaultParams.ConvexHullColor;
                        SomeCurveParamChanged = true;
                     }
                     
                     SomeCurveParamChanged |= UI_DragFloatF(&CurveParams->ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
                     if (ResetCtxMenu("ConvexHullWidthReset"))
                     {
                        CurveParams->ConvexHullWidth = DefaultParams.ConvexHullWidth;
                        SomeCurveParamChanged = true;
                     }
                  }
               }
            }
            
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
                  curve_params *CurveParams = &Curve->CurveParams;
                  
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
                  
                  b32 IsBezierRegular = (CurveParams->InterpolationType == Interpolation_Bezier &&
                                         CurveParams->BezierType == Bezier_Regular);
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
               ElevateBezierCurveDegree(Entity, &Editor->Params);
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

internal void
RenderUIForRenderPointData(char const *Label, render_point_data *PointData, render_point_data DefaultData)
{
   DeferBlock(UI_PushLabelF(Label), UI_PopId())
   {      
      UI_DragFloatF(&PointData->RadiusClipSpace, 0.0f, FLT_MAX, 0, "Radius");
      if (ResetCtxMenu("RadiusReset"))
      {
         PointData->RadiusClipSpace = DefaultData.RadiusClipSpace;
      }
      
      UI_DragFloatF(&PointData->OutlineThicknessFraction, 0.0f, 1.0f, 0, "Outline Thickness");
      if (ResetCtxMenu("OutlineThicknessReset"))
      {
         PointData->OutlineThicknessFraction = DefaultData.OutlineThicknessFraction;
      }
      
      UI_ColorPickerF(&PointData->FillColor, "Fill Color");
      if (ResetCtxMenu("FillColorReset"))
      {
         PointData->FillColor = DefaultData.FillColor;
      }
      
      UI_ColorPickerF(&PointData->OutlineColor, "Outline Color");
      if (ResetCtxMenu("OutlineColorReset"))
      {
         PointData->OutlineColor = DefaultData.OutlineColor;
      }
   }
}

internal void
UpdateAndRenderNotifications(editor *Editor, f32 DeltaTime)
{
   temp_arena Temp = TempArena(0);
   
   UI_Label(StrLit("Notifications"))
   {
      sf::Vector2u WindowSize = Editor->RenderData.Window->getSize();
      f32 Padding = 0.01f * WindowSize.x;
      f32 TargetPosY = WindowSize.y - Padding;
      
      f32 WindowWidth = 0.1f * WindowSize.x;
      ImVec2 WindowMinSize = ImVec2(WindowWidth, 0.0f);
      ImVec2 WindowMaxSize = ImVec2(WindowWidth, FLT_MAX);
      
      notification *FreeNotification = Editor->Notifications;
      u64 RemoveCount = 0;
      for (u64 NotificationIndex = 0;
           NotificationIndex < Editor->NotificationCount;
           ++NotificationIndex)
      {
         notification *Notification = Editor->Notifications + NotificationIndex;
         Notification->LifeTime += DeltaTime;
         
         enum notification_life_time {
            NotificationLifeTime_In,
            NotificationLifeTime_Proper,
            NotificationLifeTime_Out,
            NotificationLifeTime_Invisible,
            NotificationLifeTime_Count,
         };
         local f32 LifeTimes[NotificationLifeTime_Count] = {
            0.15f, 10.0f, 0.15f, 0.1f,
         };
         
         f32 RelLifeTime = Notification->LifeTime;
         u64 LifeTimeIndex = 0;
         for (; LifeTimeIndex < NotificationLifeTime_Count;
              ++LifeTimeIndex)
         {
            if (RelLifeTime <= LifeTimes[LifeTimeIndex])
            {
               break;
            }
            RelLifeTime -= LifeTimes[LifeTimeIndex];
         }
         
         b32 ShouldBeRemoved = false;
         f32 Fade = 0.0f;
         switch (Cast(notification_life_time)LifeTimeIndex)
         {
            case NotificationLifeTime_In:        { Fade = Map01(RelLifeTime, 0.0f, LifeTimes[NotificationLifeTime_In]); } break;
            case NotificationLifeTime_Proper:    { Fade = 1.0f; } break;
            case NotificationLifeTime_Out:       { Fade = 1.0f - Map01(RelLifeTime, 0.0f, LifeTimes[NotificationLifeTime_Out]); } break;
            case NotificationLifeTime_Invisible: { Fade = 0.0f; } break;
            case NotificationLifeTime_Count:     { ShouldBeRemoved = true; } break;
         }
         // NOTE(hbr): Quadratic interpolation instead of linear.
         Fade = 1.0f - Square(1.0f - Fade);
         Assert(-EPS_F32 <= Fade && Fade <= 1.0f + EPS_F32);
         
         f32 CurrentPosY = Notification->ScreenPosY;
         local f32 MoveSpeed = 20.0f;
         f32 NextPosY = Lerp(CurrentPosY, TargetPosY, 1.0f - PowF32(2.0f, -MoveSpeed * DeltaTime));
         if (ApproxEq32(TargetPosY, NextPosY))
         {
            NextPosY = TargetPosY;
         }
         Notification->ScreenPosY = NextPosY;
         
         ImVec2 WindowPosition = ImVec2(WindowSize.x - Padding, NextPosY);
         ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
         ImGui::SetNextWindowSizeConstraints(WindowMinSize, WindowMaxSize);
         
         UI_Alpha(Fade)
         {
            string Label = StrF(Temp.Arena, "Notification#%llu", NotificationIndex);
            DeferBlock(ImGui::Begin(Label.Data, 0,
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoFocusOnAppearing),
                       ImGui::End())
            {
               ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
               
               if (ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(0) ||
                                                ImGui::IsMouseClicked(1)))
               {
                  ShouldBeRemoved = true;
               }
               
               char const *Title = 0;
               color TitleColor = {};
               switch (Notification->Type)
               {
                  case Notification_Success: { Title = "Success"; TitleColor = GreenColor; } break;
                  case Notification_Error:   { Title = "Error";   TitleColor = RedColor; } break;
                  case Notification_Warning: { Title = "Warning"; TitleColor = YellowColor; } break;
               }
               
               UI_ColoredText(TitleColor) UI_TextF(Title);
               
               ImGui::Separator();
               DeferBlock(ImGui::PushTextWrapPos(0.0f), ImGui::PopTextWrapPos())
               {
                  string Content = CStrFromStr(Temp.Arena, Notification->Content);
                  UI_TextF(Content.Data);
               }
               
               f32 WindowHeight = ImGui::GetWindowHeight();
               Notification->BoxHeight = WindowHeight;
               TargetPosY -= WindowHeight + Padding;
            }
         }
         
         if (ShouldBeRemoved)
         {
            ++RemoveCount;
         }
         else
         {
            *FreeNotification++ = *Notification;
         }
      }
      Editor->NotificationCount -= RemoveCount;
   }
   
   EndTemp(Temp);
}

internal void
RenderListOfEntitiesForEntityType(editor *Editor, entity_type Type, string Label)
{
   if (UI_CollapsingHeader(Label))
   {
      entities *Entities = &Editor->Entities;
      for (u64 EntityIndex = 0;
           EntityIndex < ArrayCount(Entities->Entities);
           ++EntityIndex)
      {
         entity *Entity = Entities->Entities + EntityIndex;
         if ((Entity->Flags & EntityFlag_Active) && Entity->Type == Type)
         {
            UI_PushId(EntityIndex);
            
            if (Entity->RenamingFrame)
            {
               // TODO(hbr): Remove this Cast
               if (Entity->RenamingFrame == Cast(u64)ImGui::GetFrameCount())
               {
                  ImGui::SetKeyboardFocusHere(0);
               }
               
               // NOTE(hbr): Make sure text starts rendering at the same position
               DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0}),
                          ImGui::PopStyleVar())
               {
                  Entity->Name = UI_InputTextF(Entity->NameBuffer, ArrayCount(Entity->NameBuffer), "");
               }
               
               b32 ClickedOutside = (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0));
               b32 PressedEnter = ImGui::IsKeyPressed(ImGuiKey_Enter);
               if (ClickedOutside || PressedEnter)
               {
                  Entity->RenamingFrame = 0;
               }
            }
            else
            {
               b32 Selected = (Entity->Flags & EntityFlag_Selected);
               
               if (UI_SelectableItem(Selected, Entity->Name))
               {
                  SelectEntity(Editor, Entity);
               }
               
               b32 DoubleClickedOrSelectedAndClicked = ((Selected && ImGui::IsMouseClicked(0)) ||
                                                        ImGui::IsMouseDoubleClicked(0));
               if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
               {
                  Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
               }
               
               string CtxMenu = StrLit("EntityContextMenu");
               if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
               {
                  UI_OpenPopup(CtxMenu);
               }
               if (UI_BeginPopup(CtxMenu))
               {
                  if (UI_MenuItemF(0, 0, "Rename"))
                  {
                     Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
                  }
                  
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
                  
                  UI_Disabled(!Selected)
                  {
                     if (UI_MenuItemF(0, 0, "Deselect"))
                     {
                        DeselectCurrentEntity(Editor);
                     }
                  }
                  
                  if (UI_MenuItemF(0, 0, "Focus"))
                  {
                     FocusCameraOnEntity(Editor, Entity);
                  }
                  
                  UI_EndPopup();
               }
            }
            
            UI_PopId();
         }
      }
   }
}

internal void
RenderListOfEntitiesWindow(editor *Editor)
{
   ui_config *Config = &Editor->UI_Config;
   if (UI_BeginWindowF(&Config->ViewListOfEntitiesWindow, "Entities") &&
       Config->ViewListOfEntitiesWindow)
   {
      RenderListOfEntitiesForEntityType(Editor, Entity_Curve, StrLit("Curves"));
      RenderListOfEntitiesForEntityType(Editor, Entity_Image, StrLit("Images"));
   }
   UI_EndWindow();
}

struct load_texture_result
{
   b32 Success;
   sf::Texture Texture;
};
// TODO(hbr): I'm not sure if this internal belongs here, in this file
internal load_texture_result
LoadTextureFromFile(string FilePath)
{
   load_texture_result Result = {};
   temp_arena Temp = TempArena(0);
   string Texture = OS_ReadEntireFile(Temp.Arena, FilePath);
   if (Result.Texture.loadFromMemory(Texture.Data, Texture.Count))
   {
      Result.Success = true;
   }
   EndTemp(Temp);
   return Result;
}

internal void
UpdateAndRenderMenuBar(editor *Editor, user_input *Input)
{
   b32 NewProject    = false;
   b32 OpenProject   = false;
   b32 Quit          = false;
   b32 SaveProject   = false;
   b32 SaveProjectAs = false;
   b32 LoadImage     = false;
   
   local char const *SaveAsLabel = "SaveAsWindow";
   local char const *SaveAsTitle = "Save As";
   local char const *OpenNewProjectLabel = "OpenNewProject";
   local char const *OpenNewProjectTitle = "Open";
   local char const *LoadImageLabel = "LoadImage";
   local char const *LoadImageTitle = "Load Image";
   
   local ImGuiWindowFlags FileDialogWindowFlags = ImGuiWindowFlags_NoCollapse;
   
   sf::Vector2u WindowSize = Editor->RenderData.Window->getSize();
   ImVec2 HalfWindowSize = ImVec2(0.5f * WindowSize.x, 0.5f * WindowSize.y);
   ImVec2 FileDialogMinSize = HalfWindowSize;
   ImVec2 FileDialogMaxSize = ImVec2(Cast(f32)WindowSize.x, Cast(f32)WindowSize.y);
   
   auto FileDialog = ImGuiFileDialog::Instance();
   
   if (UI_BeginMainMenuBar())
   {
      if (UI_BeginMenuF("Project"))
      {
         NewProject    = UI_MenuItemF(0, 0, "Ctrl+N",       "New");
         OpenProject   = UI_MenuItemF(0, 0, "Ctrl+O",       "Open");
         SaveProject   = UI_MenuItemF(0, 0, "Ctrl+S",       "Save");
         SaveProjectAs = UI_MenuItemF(0, 0, "Shift+Ctrl+S", "Save As");
         Quit          = UI_MenuItemF(0, 0, "Q/Escape",     "Quit");
         UI_EndMenu();
      }
      LoadImage = UI_MenuItemF(0, 0, "Load Image");
      if (UI_BeginMenuF("View"))
      {
         ui_config *Config = &Editor->UI_Config;
         UI_MenuItemF(&Config->ViewListOfEntitiesWindow, 0, "List of Entities");
         UI_MenuItemF(&Config->ViewSelectedEntityWindow, 0, "Selected Entity");
         UI_MenuItemF(&Config->ViewParametersWindow,     0, "Parameters");
         UI_MenuItemF(&Config->ViewProfilerWindow,       0, "Profiler");
         UI_MenuItemF(&Config->ViewDebugWindow,          0, "Debug");
         if (UI_BeginMenuF("Camera"))
         {
            camera *Camera = &Editor->RenderData.Camera;
            if (UI_MenuItemF(0, 0, "Reset Position"))
            {
               MoveCamera(Camera, -Camera->Position);
            }
            if (UI_MenuItemF(0, 0, "Reset Rotation"))
            {
               RotateCamera(Camera, Rotation2DInverse(Camera->Rotation));
            }
            if (UI_MenuItemF(0, 0, "Reset Zoom"))
            {
               SetCameraZoom(Camera, 1.0f);
            }
            UI_EndMenu();
         }
         UI_MenuItemF(&Config->ViewDiagnosticsWindow, 0, "Diagnostics");
         UI_EndMenu();
      }
      // TODO(hbr): Complete help menu
      if (UI_BeginMenuF("Help"))
      {
         UI_EndMenu();
      }
      UI_EndMainMenuBar();
   }
   
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
#if 0
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
#endif
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
      ImGui::SetNextWindowPos(HalfWindowSize, ImGuiCond_Always, ImVec2(0.5f,0.5f));
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
#if 0
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
#endif
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
   
   // TODO(hbr): Implement
#if 0   
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
#endif
   
   switch (ActionToDo)
   {
      case ActionToDo_Nothing: {} break;
      
      case ActionToDo_NewProject: {
         
         // TODO(hbr): Load new project
#if 0
         editor_state NewEditor =
            CreateDefaultEditor(Editor->EntityPool,
                                Editor->DeCasteljauVisual.Arena,
                                Editor->DegreeLowering.Arena,
                                Editor->MovingPointArena,
                                Editor->CurveAnimation.Arena);
         
         DeallocEditor(&Editor);
         Editor = NewEditor;
         
         EditorSetSaveProjectPath(Editor, SaveProjectFormat_None, {});
#endif
      } break;
      
      case ActionToDo_OpenProject: {
         Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
         FileDialog->OpenModal(OpenNewProjectLabel, OpenNewProjectTitle,
                               SAVED_PROJECT_FILE_EXTENSION, ".");
      } break;
      
      case ActionToDo_Quit: {
         Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
         Editor->RenderData.Window->close();
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
#if 0
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
         
#endif
      }
      
      
      FileDialog->Close();
   }
   
   // TODO(hbr): Implement
#if 0
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
RenderSettingsWindow(editor *Editor)
{
   ui_config *Config = &Editor->UI_Config;
   editor_params *Params = &Editor->Params;
   if (Config->ViewParametersWindow)
   {
      if (UI_BeginWindowF(&Config->ViewParametersWindow, "Editor Settings"))
      {
         if (UI_CollapsingHeaderF("Rotation Indicator"))
         {
            RenderUIForRenderPointData("RotationIndicator",
                                       &Params->RotationIndicator,
                                       GlobalDefaultEditorParams.RotationIndicator);
         }
         
         if (UI_CollapsingHeaderF("Bezier Split Point"))
         {
            RenderUIForRenderPointData("BezierSplitPoint",
                                       &Params->BezierSplitPoint,
                                       GlobalDefaultEditorParams.BezierSplitPoint);
         }
         
         if (UI_CollapsingHeaderF("Other Settings"))
         {
            DeferBlock(UI_PushLabelF("OtherSettings"), UI_PopId())
            {
               UI_ColorPickerF(&Params->BackgroundColor, "Background Color");
               if (ResetCtxMenu("BackgroundColorReset"))
               {
                  Params->BackgroundColor = GlobalDefaultEditorParams.BackgroundColor;
               }
               
               UI_DragFloatF(&Params->CollisionToleranceClipSpace, 0.0f, 1.0f, 0, "Collision Tolerance");
               if (ResetCtxMenu("CollisionToleranceReset"))
               {
                  Params->CollisionToleranceClipSpace = GlobalDefaultEditorParams.CollisionToleranceClipSpace;
               }
               
               UI_DragFloatF(&Params->LastControlPointSizeMultiplier, 0.0f, FLT_MAX, 0, "Last Control Point Size");
               if (ResetCtxMenu("LastControlPointSizeReset"))
               {
                  Params->LastControlPointSizeMultiplier = GlobalDefaultEditorParams.LastControlPointSizeMultiplier;
               }
               
               UI_DragFloatF(&Params->CubicBezierHelperLineWidthClipSpace, 0.0f, FLT_MAX, 0, "Cubic Bezier Helper Line Width");
               if (ResetCtxMenu("CubicBezierHelperLineWidthReset"))
               {
                  Params->CubicBezierHelperLineWidthClipSpace = GlobalDefaultEditorParams.CubicBezierHelperLineWidthClipSpace;
               }
            }
         }
      }
      UI_EndWindow();
   }
}

internal void
RenderDiagnosticsWindow(editor *Editor, f32 DeltaTime)
{
   ui_config *Config = &Editor->UI_Config;
   if (Config->ViewDiagnosticsWindow)
   {
      if (UI_BeginWindowF(&Config->ViewDiagnosticsWindow, "Diagnostics"))
      {
         frame_stats *Stats = &Editor->FrameStats;
         UI_TextF("%20s: %.2f ms", "Frame time", 1000.0f * DeltaTime);
         UI_TextF("%20s: %.0f", "FPS", Stats->FPS);
         UI_TextF("%20s: %.2f ms", "Min frame time", 1000.0f * Stats->MinFrameTime);
         UI_TextF("%20s: %.2f ms", "Max frame time", 1000.0f * Stats->MaxFrameTime);
         UI_TextF("%20s: %.2f ms", "Average frame time", 1000.0f * Stats->AvgFrameTime);
      }
      UI_EndWindow();
   }
}

internal void
UpdateAndRenderPointTracking(editor *Editor, entity *Entity, sf::Transform Transform)
{
   TimeFunction;
   
   curve *Curve = &Entity->Curve;
   curve_params *CurveParams = &Curve->CurveParams;
   curve_point_tracking_state *Tracking = &Curve->PointTracking;
   temp_arena Temp = TempArena(Tracking->Arena);
   editor_params *EditorParams = &Editor->Params;
   sf::RenderWindow *Window = Editor->RenderData.Window;
   
   if (Entity->Type == Entity_Curve && Tracking->Active)
   {
      b32 IsBezierRegular = (CurveParams->InterpolationType == Interpolation_Bezier &&
                             CurveParams->BezierType == Bezier_Regular);
      
      if (IsBezierRegular)
      {
         if (Tracking->IsSplitting)
         {
            b32 PerformSplit = false;
            b32 Cancel = false;
            b32 IsWindowOpen = true;
            DeferBlock(UI_BeginWindowF(&IsWindowOpen, "Splitting '%S'", Entity->Name),
                       UI_EndWindow())
            {
               Tracking->NeedsRecomputationThisFrame |= UI_SliderFloatF(&Tracking->T, 0.0f, 1.0f, "Split Point");
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
                  Tracking->TrackedPoint = BezierCurveEvaluateWeighted(Tracking->T,
                                                                       Curve->ControlPoints,
                                                                       Curve->ControlPointWeights,
                                                                       Curve->ControlPointCount);
               }
               
               RenderPoint(Tracking->TrackedPoint,
                           EditorParams->BezierSplitPoint,
                           &Editor->RenderData,
                           Transform);
            }
         }
         else
         {
            b32 IsWindowOpen = true;
            DeferBlock(UI_BeginWindowF(&IsWindowOpen, "De Casteljau Algorithm '%S'", Entity->Name),
                       UI_EndWindow())
            {
               Tracking->NeedsRecomputationThisFrame |= UI_SliderFloatF(&Tracking->T, 0.0f, 1.0f, "T");
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
                                          Tracking->T,
                                          Curve->ControlPoints,
                                          Curve->ControlPointWeights,
                                          Curve->ControlPointCount);
                  color *IterationColors = PushArrayNonZero(Tracking->Arena, Intermediate.IterationCount, color);
                  line_vertices *LineVerticesPerIteration = PushArray(Tracking->Arena,
                                                                      Intermediate.IterationCount,
                                                                      line_vertices);
                  
                  f32 LineWidth = Curve->CurveParams.CurveWidth;
                  color GradientA = MakeColor(255, 0, 144);
                  color GradientB = MakeColor(155, 200, 0);
                  
                  f32 P = 0.0f;
                  f32 Delta_P = 1.0f / (Intermediate.IterationCount - 1);
                  u64 IterationPointsOffset = 0;
                  for (u64 Iteration = 0;
                       Iteration < Intermediate.IterationCount;
                       ++Iteration)
                  {
                     color IterationColor = LerpColor(GradientA, GradientB, P);
                     IterationColors[Iteration] = IterationColor;
                     
                     u64 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
                     LineVerticesPerIteration[Iteration] =
                        CalculateLineVertices(CurrentIterationPointCount,
                                              Intermediate.P + IterationPointsOffset,
                                              LineWidth, IterationColor,
                                              false,
                                              LineVerticesAllocationArena(Tracking->Arena));
                     
                     P += Delta_P;
                     IterationPointsOffset += CurrentIterationPointCount;
                  }
                  
                  Tracking->Intermediate = Intermediate;
                  Tracking->IterationColors = IterationColors;
                  Tracking->LineVerticesPerIteration = LineVerticesPerIteration;
                  Tracking->TrackedPoint = Intermediate.P[Intermediate.TotalPointCount - 1];
               }
               
               u64 IterationCount = Tracking->Intermediate.IterationCount;
               for (u64 Iteration = 0;
                    Iteration < IterationCount;
                    ++Iteration)
               {
                  Window->draw(Tracking->LineVerticesPerIteration[Iteration].Vertices,
                               Tracking->LineVerticesPerIteration[Iteration].NumVertices,
                               Tracking->LineVerticesPerIteration[Iteration].PrimitiveType,
                               Transform);
               }
               
               f32 PointSize = CurveParams->PointRadius;
               u64 PointIndex = 0;
               for (u64 Iteration = 0;
                    Iteration < IterationCount;
                    ++Iteration)
               {
                  color PointColor = Tracking->IterationColors[Iteration];
                  for (u64 I = 0; I < IterationCount - Iteration; ++I)
                  {
                     local_position Point = Tracking->Intermediate.P[PointIndex];
                     DrawCircle(Point, PointSize, PointColor, Transform, Window);
                     ++PointIndex;
                  }
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
UpdateAndRenderDegreeLowering(entity *Entity,
                              sf::Transform Transform,
                              sf::RenderWindow *Window)
{
   TimeFunction;
   
   curve *Curve = GetCurve(Entity);
   curve_params *CurveParams = &Curve->CurveParams;
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
      Assert(CurveParams->InterpolationType == Interpolation_Bezier);
      
      b32 IsDegreeLoweringWindowOpen = true;
      b32 MixChanged = false;
      if (UI_BeginWindowF(&IsDegreeLoweringWindowOpen, "Degree Lowering"))
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
         local_position NewControlPoint = Lerp(Lowering->LowerDegree.P_I, Lowering->LowerDegree.P_II, Lowering->MixParameter);
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
      sf::Transform Model = CurveGetAnimate(Entity);
      sf::Transform MVP = Transform * Model;
      
      Window->draw(Lowering->SavedCurveVertices,
                   Lowering->NumSavedCurveVertices,
                   Lowering->SavedPrimitiveType,
                   MVP);
   }
}

#define CURVE_NAME_HIGHLIGHT_COLOR ImVec4(1.0f, 0.5, 0.0f, 1.0f)

internal void
UpdateAnimateCurveAnimation(editor *Editor, f32 DeltaTime, sf::Transform Transform)
{
#if 0
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
            // TODO(hbr): Add color UI_TextFColored(CURVE_NAME_HIGHLIGHT_COLOR, Animation->FromCurveEntity->Name.Data);
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
            u64 CurvePointCount = FromCurve->CurvePointCount;
            
            b32 ToCurvePointsRecalculated = false;
            if (CurvePointCount != Animation->CurvePointCount ||
                Animation->SavedToCurveVersion != ToCurve->CurveVersion)
            {
               ClearArena(Animation->Arena);
               
               Animation->CurvePointCount = CurvePointCount;
               Animation->ToCurvePoints = PushArrayNonZero(Animation->Arena, CurvePointCount, v2);
               EvaluateCurve(ToCurve, CurvePointCount, Animation->ToCurvePoints);
               Animation->SavedToCurveVersion = ToCurve->CurveVersion;
               
               ToCurvePointsRecalculated = true;
            }
            
            if (ToCurvePointsRecalculated || BlendChanged)
            {               
               v2 *ToCurvePoints = Animation->ToCurvePoints;
               v2 *FromCurvePoints = FromCurve->CurvePoints;
               v2 *AnimatedCurvePoints = PushArrayNonZero(Temp.Arena, CurvePointCount, v2);
               
               f32 Blend = CalculateAnimation(Animation->Animation, Animation->Blend);
               f32 T = 0.0f;
               f32 Delta_T = 1.0f / (CurvePointCount - 1);
               for (u64 VertexIndex = 0;
                    VertexIndex < CurvePointCount;
                    ++VertexIndex)
               {
                  v2 From = LocalEntityPositionToWorld(FromCurveEntity,
                                                       FromCurvePoints[VertexIndex]);
                  v2 To = LocalEntityPositionToWorld(ToCurveEntity,
                                                     ToCurvePoints[VertexIndex]);
                  
                  AnimatedCurvePoints[VertexIndex] = Lerp(From, To, Blend);
                  
                  T += Delta_T;
               }
               
               f32 AnimatedCurveWidth = Lerp(FromCurve->CurveParams.CurveWidth,
                                             ToCurve->CurveParams.CurveWidth,
                                             Blend);
               color AnimatedCurveColor = LerpColor(FromCurve->CurveParams.CurveColor,
                                                    ToCurve->CurveParams.CurveColor,
                                                    Blend);
               
               // NOTE(hbr): If there is no recalculation we can reuse previous buffer without
               // any calculation, because there is already enough space.
               line_vertices_allocation Allocation = (ToCurvePointsRecalculated ?
                                                      LineVerticesAllocationArena(Animation->Arena) :
                                                      LineVerticesAllocationNone(Animation->AnimatedCurveVertices.Vertices));
               Animation->AnimatedCurveVertices =
                  CalculateLineVertices(CurvePointCount, AnimatedCurvePoints,
                                        AnimatedCurveWidth, AnimatedCurveColor,
                                        false, Allocation);
            }
            
            EndTemp(Temp);
         }
      } break;
   }
#endif
}

// TODO(hbr): Refactor this function
internal void
RenderEntityCombo(entities *Entities, entity **InOutEntity, string Label)
{
   entity *Entity = *InOutEntity;
   string Preview = (Entity ? Entity->Name : StrLit(""));
   if (UI_BeginCombo(Preview, Label))
   {
      for (u64 EntityIndex = 0;
           EntityIndex < MAX_ENTITY_COUNT;
           ++EntityIndex)
      {
         entity *Current= Entities->Entities + EntityIndex;
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
UpdateAndRenderCurveCombining(editor *Editor, sf::Transform Transform)
{
   temp_arena Temp = TempArena(0);
   
   curve_combining_state *State = &Editor->CurveCombining;
   if (State->SourceEntity)
   {
      b32 Combine      = false;
      b32 Cancel       = false;
      b32 IsWindowOpen = true;
      if (UI_BeginWindowF(&IsWindowOpen, "Combine Curves"))
      {                 
         if (IsWindowOpen)
         {
            RenderEntityCombo(&Editor->Entities, &State->SourceEntity, StrLit("Curve 1"));
            RenderEntityCombo(&Editor->Entities, &State->WithEntity,   StrLit("Curve 2"));
            UI_ComboF(&State->CombinationType, CurveCombination_Count, CurveCombinationNames, "Method");
         }
         
         b32 CanCombine = (State->CombinationType != CurveCombination_None);
         if (State->WithEntity)
         {
            curve_params *SourceParams = &GetCurve(State->SourceEntity)->CurveParams;
            curve_params *WithParams = &GetCurve(State->WithEntity)->CurveParams;
            if (SourceParams != WithParams &&
                SourceParams->InterpolationType == WithParams->InterpolationType)
            {
               switch (SourceParams->InterpolationType)
               {
                  case Interpolation_Polynomial:  {
                     CanCombine = (State->CombinationType == CurveCombination_Merge);
                  } break;
                  case Interpolation_CubicSpline: {
                     CanCombine = ((SourceParams->CubicSplineType == WithParams->CubicSplineType) &&
                                   (State->CombinationType == CurveCombination_Merge));
                  } break;
                  case Interpolation_Bezier: {
                     CanCombine = ((SourceParams->BezierType == WithParams->BezierType) &&
                                   (SourceParams->BezierType != Bezier_Cubic));
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
            ArrayReverse(From->ControlPoints,       FromCount, local_position);
            ArrayReverse(From->ControlPointWeights, FromCount, f32);
            ArrayReverse(From->CubicBezierPoints,   FromCount, cubic_bezier_point);
         }
         
         if (State->WithCurveFirstControlPoint)
         {
            ArrayReverse(To->ControlPoints,       ToCount, local_position);
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
         local_position *CombinedPoints  = PushArrayNonZero(Temp.Arena, CombinedPointCount, local_position);
         f32 *CombinedWeights = PushArrayNonZero(Temp.Arena, CombinedPointCount, f32);
         cubic_bezier_point *CombinedBeziers = PushArrayNonZero(Temp.Arena, CombinedPointCount, cubic_bezier_point);
         ArrayCopy(CombinedPoints, To->ControlPoints, ToCount);
         ArrayCopy(CombinedWeights, To->ControlPointWeights, ToCount);
         ArrayCopy(CombinedBeziers, To->CubicBezierPoints, ToCount);
         
         // TODO(hbr): SIMD?
         for (u64 I = StartIndex; I < FromCount; ++I)
         {
            world_position FromPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
            local_position ToPoint   = WorldToLocalEntityPosition(ToEntity, FromPoint + Translation);
            CombinedPoints[ToCount - StartIndex + I] = ToPoint;
         }
         for (u64 I = StartIndex; I < FromCount; ++I)
         {
            for (u64 J = 0; J < 3; ++J)
            {
               world_position FromBezier = LocalEntityPositionToWorld(FromEntity, From->CubicBezierPoints[I].Ps[J]);
               local_position ToBezier   = WorldToLocalEntityPosition(ToEntity, FromBezier + Translation); 
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
      local_position SourcePointLocal = (State->SourceCurveLastControlPoint ?
                                         SourceCurve->ControlPoints[SourceCurve->ControlPointCount - 1] :
                                         SourceCurve->ControlPoints[0]);
      local_position WithPointLocal = (State->WithCurveFirstControlPoint ?
                                       WithCurve->ControlPoints[0] :
                                       WithCurve->ControlPoints[WithCurve->ControlPointCount - 1]);
      
      world_position SourcePoint = LocalEntityPositionToWorld(State->SourceEntity, SourcePointLocal);
      world_position WithPoint = LocalEntityPositionToWorld(State->WithEntity, WithPointLocal);
      
      f32 LineWidth = 0.5f * SourceCurve->CurveParams.CurveWidth;
      color Color = SourceCurve->CurveParams.CurveColor;
      Color.A = Cast(u8)(0.5f * Color.A);
      
      v2 LineDirection = WithPoint - SourcePoint;
      Normalize(&LineDirection);
      
      f32 TriangleSide = 10.0f * LineWidth;
      f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
      world_position BaseVertex = WithPoint - TriangleHeight * LineDirection;
      DrawLine(SourcePoint, BaseVertex, LineWidth, Color, Transform, Editor->RenderData.Window);
      
      v2 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
      world_position LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
      world_position RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
      DrawTriangle(LeftVertex, RightVertex, WithPoint, Color, Transform, Editor->RenderData.Window);
   }
   
   EndTemp(Temp);
}

internal void
UpdateAndRenderEntities(editor *Editor, f32 DeltaTime, sf::Transform VP)
{
   TimeFunction;
   
   temp_arena Temp = TempArena(0);
   sf::RenderWindow *Window = Editor->RenderData.Window;
   
   entity_array EntityArray = {};
   EntityArray.Entities = Editor->Entities.Entities;
   EntityArray.Count = MAX_ENTITY_COUNT;
   sorted_entries Sorted = SortEntities(Temp.Arena, EntityArray);
   
   UpdateAnimateCurveAnimation(Editor, DeltaTime, VP);
   
   for (u64 EntryIndex = 0;
        EntryIndex < Sorted.Count;
        ++EntryIndex)
   {
      entity *Entity = EntityArray.Entities + Sorted.Entries[EntryIndex].Index;
      if (IsEntityVisible(Entity))
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               curve_params *CurveParams = &Curve->CurveParams;
               
               if (Curve->RecomputeRequested)
               {
                  ActuallyRecomputeCurve(Entity);
               }
               
               sf::Transform Model = CurveGetAnimate(Entity);
               sf::Transform MVP = VP * Model;
               
               if (Editor->Mode.Type == EditorMode_Moving &&
                   Editor->Mode.Moving.Type == MovingMode_CurvePoint &&
                   Editor->Mode.Moving.Entity == Entity)
               {
                  Window->draw(Editor->Mode.Moving.SavedCurveVertices,
                               Editor->Mode.Moving.SavedNumCurveVertices,
                               Editor->Mode.Moving.SavedPrimitiveType,
                               MVP);
               }
               
               Window->draw(Curve->CurveVertices.Vertices,
                            Curve->CurveVertices.NumVertices,
                            Curve->CurveVertices.PrimitiveType,
                            MVP);
               
               if (CurveParams->PolylineEnabled)
               {
                  Window->draw(Curve->PolylineVertices.Vertices,
                               Curve->PolylineVertices.NumVertices,
                               Curve->PolylineVertices.PrimitiveType,
                               MVP);
               }
               
               if (CurveParams->ConvexHullEnabled)
               {
                  Window->draw(Curve->ConvexHullVertices.Vertices,
                               Curve->ConvexHullVertices.NumVertices,
                               Curve->ConvexHullVertices.PrimitiveType,
                               MVP);
               }
               
               if (AreCurvePointsVisible(Curve))
               {
                  visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
                  for (u64 Index = 0;
                       Index < VisibleBeziers.Count;
                       ++Index)
                  {
                     cubic_bezier_point_index BezierIndex = VisibleBeziers.Indices[Index];
                     
                     local_position BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
                     local_position CenterPoint = GetCenterPointFromCubicBezierPointIndex(Curve, BezierIndex);
                     
                     f32 BezierPointRadius = GetCurveCubicBezierPointRadius(Curve);
                     f32 HelperLineWidth = 0.2f * CurveParams->CurveWidth;
                     
                     DrawLine(BezierPoint, CenterPoint, HelperLineWidth, CurveParams->CurveColor, MVP, Window);
                     DrawCircle(BezierPoint, BezierPointRadius, CurveParams->PointColor, MVP, Window);
                  }
                  
                  u64 ControlPointCount = Curve->ControlPointCount;
                  local_position *ControlPoints = Curve->ControlPoints;
                  for (u64 PointIndex = 0;
                       PointIndex < ControlPointCount;
                       ++PointIndex)
                  {
                     point_info PointInfo = GetCurveControlPointInfo(Entity, PointIndex);
                     DrawCircle(ControlPoints[PointIndex],
                                PointInfo.Radius,
                                PointInfo.Color,
                                MVP, Window,
                                PointInfo.OutlineThickness,
                                PointInfo.OutlineColor);
                  }
               }
               
               UpdateAndRenderDegreeLowering(Entity, VP, Editor->RenderData.Window);
               
               curve_animation_state *Animation = &Editor->CurveAnimation;
               if (Animation->Stage == AnimateCurveAnimation_Animating && Animation->FromCurveEntity == Entity)
               {
                  Window->draw(Animation->AnimatedCurveVertices.Vertices,
                               Animation->AnimatedCurveVertices.NumVertices,
                               Animation->AnimatedCurveVertices.PrimitiveType,
                               VP);
               }
               
               // TODO(hbr): Update this
               curve_combining_state *Combining = &Editor->CurveCombining;
               if (Combining->SourceEntity == Entity)
               {
                  UpdateAndRenderCurveCombining(Editor, VP);
               }
               
               UpdateAndRenderPointTracking(Editor, Entity, MVP);
               
               Curve->RecomputeRequested = false;
            } break;
            
            case Entity_Image: {
               image *Image = &Entity->Image;
               
               sf::Sprite Sprite;
               Sprite.setTexture(Image->Texture);
               
               sf::Vector2u TextureSize = Image->Texture.getSize();
               Sprite.setOrigin(0.5f*TextureSize.x, 0.5f*TextureSize.y);
               
               v2 Scale = GlobalImageScaleFactor * Entity->Scale;
               // NOTE(hbr): Flip vertically because SFML images are flipped.
               Sprite.setScale(Scale.X, -Scale.Y);
               
               v2 Position = Entity->Position;
               Sprite.setPosition(Position.X, Position.Y);
               
               f32 AngleRotation = Rotation2DToDegrees(Entity->Rotation);
               Sprite.setRotation(AngleRotation);
               
               Window->draw(Sprite, VP);
            }break;
         }
      }
   }
   
   EndTemp(Temp);
}

internal void
UpdateAndRender(f32 DeltaTime, user_input *Input, editor *Editor)
{
   TimeFunction;
   
   FrameStatsUpdate(&Editor->FrameStats, DeltaTime);
   UpdateCamera(&Editor->RenderData.Camera, Input->MouseWheelDelta, DeltaTime);
   UpdateAndRenderNotifications(Editor, DeltaTime);
   
   {
      TimeBlock("editor state update");
      
      user_action Action = {};
      Action.Input = Input;
      for (u64 ButtonIndex = 0;
           ButtonIndex < Button_Count;
           ++ButtonIndex)
      {
         button Button = Cast(button)ButtonIndex;
         button_state *ButtonState = &Input->Buttons[ButtonIndex];
         
         if (ClickedWithButton(ButtonState, &Editor->RenderData))
         {
            Action.Type = UserAction_ButtonClicked;
            Action.Click.Button = Button;
            // TODO(hbr): I know, I know. But should it be ReleasePosition???
            Action.Click.ClickPosition = ButtonState->ReleasePosition;
         }
         else if (DraggedWithButton(ButtonState, Input->MousePosition, &Editor->RenderData))
         {
            Action.Type = UserAction_ButtonDrag;
            Action.Drag.Button = Button;
            Action.Drag.DragFromPosition = Input->MouseLastPosition;
         }
         else if (JustReleasedButton(ButtonState))
         {
            Action.Type = UserAction_ButtonReleased;
            Action.Release.Button = Button;
            Action.Release.ReleasePosition = ButtonState->ReleasePosition;
         }
         if (Action.Type != UserAction_None)
         {
            ExecuteUserAction(Editor, Action);
         }
      }
      
      if (Input->MouseLastPosition != Input->MousePosition)
      {
         Action.Type = UserAction_MouseMove;
         Action.MouseMove.FromPosition = Input->MouseLastPosition;
         Action.MouseMove.ToPosition = Input->MousePosition;
         ExecuteUserAction(Editor, Action);
      }
   }
   
   // TODO(hbr): Remove this
   if (KeyPressed(Input, Key_E, 0))
   {
      AddNotificationF(Editor, Notification_Error, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
   }
   
   if (KeyPressed(Input, Key_Tab, 0))
   {
      Editor->HideUI = !Editor->HideUI;
   }
   
   RenderSelectedEntityUI(Editor);
   RenderListOfEntitiesWindow(Editor);
   RenderSettingsWindow(Editor);
   RenderDiagnosticsWindow(Editor, DeltaTime);
   DebugUpdateAndRender(Editor);
   
   // NOTE(hbr): World Space -> Clip Space
   sf::Transform VP = sf::Transform();
   {
      render_data *Data = &Editor->RenderData;
      f32 CameraRotationInDegrees = Rotation2DToDegrees(Data->Camera.Rotation);
      // NOTE(hbr): World Space -> Camera/View Space
      sf::Transform ViewAnimate =
         sf::Transform()
         .rotate(-CameraRotationInDegrees)
         .scale(Data->Camera.Zoom, Data->Camera.Zoom)
         .translate(-V2ToVector2f(Data->Camera.Position));
      // NOTE(hbr): Data->Camera Space -> Clip Space
      sf::Transform ProjectionAnimate =
         sf::Transform()
         .scale(1.0f / (0.5f * Data->FrustumSize * Data->AspectRatio),
                1.0f / (0.5f * Data->FrustumSize));
      VP = ProjectionAnimate * ViewAnimate;
   }
   
   UpdateAndRenderEntities(Editor, DeltaTime, VP);
   
   // NOTE(hbr): Render rotation indicator if rotating
   if (Editor->Mode.Type == EditorMode_Rotating)
   {
      // TODO(hbr): Merge cases here.
      entity *Entity = Editor->Mode.Rotating.Entity;
      world_position RotationIndicatorPosition = {};
      if (Entity)
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               RotationIndicatorPosition = ScreenToWorldSpace(Editor->Mode.Rotating.RotationCenter,
                                                              &Editor->RenderData);
            } break;
            
            case Entity_Image: {
               RotationIndicatorPosition = Entity->Position;
            } break;
         }
      }
      else
      {
         RotationIndicatorPosition = Editor->RenderData.Camera.Position;
      }
      
      RenderPoint(RotationIndicatorPosition,
                  Editor->Params.RotationIndicator,
                  &Editor->RenderData, VP);
   }
   
   // NOTE(hbr): Update menu bar here, because world has to already be rendered
   // and UI not, because we might save our project into image file and we
   // don't want to include UI render into our image.
   UpdateAndRenderMenuBar(Editor, Input);
}

internal void
SetNormalizedDeviceCoordinatesView(sf::RenderWindow *Window)
{
   sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
   sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
   sf::View View = sf::View(Center, Size);
   
   Window->setView(View);
}

internal f32
CalculateAspectRatio(u64 Width, u64 Height)
{
   f32 AspectRatio = Cast(f32)Width / Height;
   return AspectRatio;
}

int
main()
{
   InitThreadCtx();
   InitOS();
   InitProfiler();
   
   arena *PermamentArena = AllocArena();
   
   sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
   sf::ContextSettings ContextSettings = sf::ContextSettings();
   ContextSettings.antialiasingLevel = 4;
   sf::RenderWindow Window(VideoMode, WINDOW_TITLE, sf::Style::Default, ContextSettings);
   
   SetNormalizedDeviceCoordinatesView(&Window);
   
#if 0
   //#if not(BUILD_DEBUG)
   Window.setFramerateLimit(60);
   //#endif
#endif
   
   if (Window.isOpen())
   {
      bool ImGuiInitSuccess = ImGui::SFML::Init(Window, true);
      if (ImGuiInitSuccess)
      {
         editor *Editor = PushStruct(PermamentArena, editor);
         Editor->RenderData = {
            .Window = &Window,
            .Camera = {
               .Position = V2(0.0f, 0.0f),
               .Rotation = Rotation2DZero(),
               .Zoom = 1.0f,
               .ZoomSensitivity = 0.05f,
               .ReachingTargetSpeed = 10.0f,
            },
            .FrustumSize = 2.0f,
         };
         Editor->FrameStats = MakeFrameStats();
         Editor->MovingPointArena = AllocArena();
         Editor->MovingPointArena = AllocArena();
         Editor->CurveAnimation.Arena = AllocArena();
         Editor->CurveAnimation.AnimationSpeed = 1.0f;
         
         entities *Entities = &Editor->Entities;
         for (u64 EntityIndex = 0;
              EntityIndex < ArrayCount(Editor->Entities.Entities);
              ++EntityIndex)
         {
            entity *Entity = Entities->Entities + EntityIndex;
            Entity->Arena = AllocArena();
            Entity->Curve.PointTracking.Arena = AllocArena();
            Entity->Curve.DegreeLowering.Arena = AllocArena();
            // NOTE(hbr): Ugly C++ thing we have to do because of constructors/destructors.
            new (&Entity->Image.Texture) sf::Texture();
         }
         
         Editor->UI_Config = {
            .ViewSelectedEntityWindow = true,
            .ViewListOfEntitiesWindow = true,
            .ViewParametersWindow = false,
            .ViewDiagnosticsWindow = false,
            .ViewProfilerWindow = true,
            .ViewDebugWindow = true,
            .DefaultCurveHeaderCollapsed = false,
            .RotationIndicatorHeaderCollapsed = false,
            .BezierSplitPointHeaderCollapsed = false,
            .OtherSettingsHeaderCollapsed = false,
         };
         Editor->Params = GlobalDefaultEditorParams;
         
         sf::Vector2i MousePosition = sf::Mouse::getPosition(Window);
         
         user_input Input = {};
         Input.MousePosition = V2S32FromVec(MousePosition);
         Input.MouseLastPosition = V2S32FromVec(MousePosition);
         Input.WindowWidth = VideoMode.width;
         Input.WindowHeight = VideoMode.height;
         
         while (Window.isOpen())
         {
            auto SFMLDeltaTime = Editor->DeltaClock.restart();
            f32 DeltaTime = SFMLDeltaTime.asSeconds();
            {
               TimeBlock("ImGui::SFML::Update");
               ImGui::SFML::Update(Window, SFMLDeltaTime);
            }
            // NOTE(hbr): Beginning profiling frame is inside the loop, because we have only
            // space for one frame and rendering has to be done between ImGui::SFML::Update
            // and ImGui::SFML::Render calls
#if EDITOR_PROFILER
            FrameProfilePoint(&Editor->UI_Config.ViewProfilerWindow);
#endif
            HandleEvents(&Window, &Input);
            Editor->RenderData.AspectRatio = CalculateAspectRatio(Input.WindowWidth, Input.WindowHeight);
            Window.clear(ColorToSFMLColor(Editor->Params.BackgroundColor));
            UpdateAndRender(DeltaTime, &Input, Editor);
            {
               TimeBlock("ImGui::SFML::Render");
               ImGui::SFML::Render(Window);
            }
            {
               TimeBlock("Window.display()");
               Window.display();
            }
         }
      }
      else
      {
         OutputError("failed to initialize ImGui");
      }
   }
   else
   {
      OutputError("failed to open window");
   }
   
   return 0;
}

// NOTE(hbr): Specifically after every file is included. Works in Unity build only.
StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors), ProfileAnchorsFitsIntoArray);