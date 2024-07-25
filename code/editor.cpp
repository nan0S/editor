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

global editor_params GlobalDefaultEditorParams = {
   .RotationIndicator = {
      .RadiusClipSpace = 0.06f,
      .OutlineThicknessFraction = 0.1f,
      .FillColor = ColorMake(30, 56, 87, 80),
      .OutlineColor = ColorMake(255, 255, 255, 24),
   },
   .BezierSplitPoint = {
      .RadiusClipSpace = 0.025f,
      .OutlineThicknessFraction = 0.1f,
      .FillColor = ColorMake(0, 255, 0, 100),
      .OutlineColor = ColorMake(0, 200, 0, 200),
   },
   .BackgroundColor = ColorMake(21, 21, 21),
   .CollisionToleranceClipSpace = 0.02f,
   .LastControlPointSizeMultiplier = 1.5f,
   .SelectedCurveControlPointOutlineThicknessScale = 0.55f,
   .SelectedCurveControlPointOutlineColor = ColorMake(1, 52, 49, 209),
   .SelectedControlPointOutlineColor = ColorMake(58, 183, 183, 177),
   .CubicBezierHelperLineWidthClipSpace = 0.003f,
   .CurveDefaultParams = DefaultCurveParams(),
};

internal void
CameraSetZoom(camera *Camera, f32 Zoom)
{
   Camera->Zoom = Zoom;
   Camera->ReachingTarget = false;
}

internal void
CameraUpdate(camera *Camera, f32 MouseWheelDelta, f32 DeltaTime)
{
   if (MouseWheelDelta != 0.0f)
   {
      f32 ZoomDelta  = Camera->Zoom * Camera->ZoomSensitivity * MouseWheelDelta;
      CameraSetZoom(Camera, Camera->Zoom + ZoomDelta);
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
CameraMove(camera *Camera, v2f32 Translation)
{
   Camera->Position += Translation;
   Camera->ReachingTarget = false;
}

internal void
CameraRotate(camera *Camera, rotation_2d Rotation)
{
   Camera->Rotation = CombineRotations2D(Camera->Rotation, Rotation);
   Camera->ReachingTarget = false;
}

internal world_position
CameraToWorldSpace(camera_position Position, render_data *Data)
{
   v2f32 Rotated = RotateAround(Position, V2F32(0.0f, 0.0f), Data->Camera.Rotation);
   v2f32 Scaled = Rotated / Data->Camera.Zoom;
   v2f32 Translated = Scaled + Data->Camera.Position;
   
   world_position WorldPosition = Translated;
   return WorldPosition;
}

internal camera_position
WorldToCameraSpace(world_position Position, render_data *Data)
{
   v2f32 Translated = Position - Data->Camera.Position;
   v2f32 Scaled = Data->Camera.Zoom * Translated;
   v2f32 Rotated = RotateAround(Scaled,
                                V2F32(0.0f, 0.0f),
                                Rotation2DInverse(Data->Camera.Rotation));
   
   camera_position CameraPosition = Rotated;
   return CameraPosition;
}

internal camera_position
ScreenToCameraSpace(screen_position Position, render_data *Data)
{
   v2f32 ClipPosition = V2F32FromVec(Data->Window->mapPixelToCoords(V2S32ToVector2i(Position)));
   
   f32 FrustumExtent = 0.5f * Data->FrustumSize;
   f32 CameraPositionX = ClipPosition.X * Data->AspectRatio * FrustumExtent;
   f32 CameraPositionY = ClipPosition.Y * FrustumExtent;
   
   camera_position CameraPosition = V2F32(CameraPositionX, CameraPositionY);
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
                   editor_params EditorParams,
                   check_collision_with_flags CheckCollisionWithFlags)
{
   collision Result = {};
   
   temp_arena Temp = TempArena(0);
   sorted_entity_array SortedEntities = SortEntitiesByLayer(Temp.Arena, Entities);
   
   // NOTE(hbr): Check collisions in reversed drawing order.
   for (u64 EntityIndex = SortedEntities.EntityCount;
        EntityIndex > 0;
        --EntityIndex)
   {
      entity *Entity = SortedEntities.Entries[EntityIndex - 1].Entity;
      if (!(Entity->Flags & EntityFlag_Hidden))
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               
               if (CheckCollisionWithFlags & CheckCollisionWith_ControlPoints &&
                   !Curve->CurveParams.PointsDisabled)
               {
                  local_position *ControlPoints = Curve->ControlPoints;
                  u64 ControlPointCount = Curve->ControlPointCount;
                  f32 NormalPointSize = Curve->CurveParams.PointSize;
                  local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
                  f32 OutlineThicknessScale = 0.0f;
                  if (Entity->Flags & EntityFlag_Selected)
                  {
                     OutlineThicknessScale = EditorParams.SelectedCurveControlPointOutlineThicknessScale;
                  }
                  
                  for (u64 ControlPointIndex = 0;
                       ControlPointIndex < ControlPointCount;
                       ++ControlPointIndex)
                  {
                     f32 PointSize = NormalPointSize;
                     if (ControlPointIndex == ControlPointCount - 1)
                     {
                        PointSize *= EditorParams.LastControlPointSizeMultiplier;
                     }
                     
                     f32 PointSizeWithOutline = (1.0f + OutlineThicknessScale) * PointSize;
                     
                     if (PointCollision(CheckPositionLocal, ControlPoints[ControlPointIndex],
                                        PointSizeWithOutline + CollisionTolerance))
                     {
                        Result = {
                           .Entity = Entity,
                           .Type = CurveCollision_ControlPoint,
                           .PointIndex = ControlPointIndex
                        };
                        goto collision_found_label;
                     }
                  }
                  
                  if (!Curve->CurveParams.CubicBezierHelpersDisabled &&
                      Curve->SelectedControlPointIndex != U64_MAX &&
                      Curve->CurveParams.InterpolationType == Interpolation_Bezier &&
                      Curve->CurveParams.BezierType == Bezier_Cubic)
                  {
                     // TODO(hbr): Maybe don't calculate how many Bezier points there are in all the places, maybe
                     local_position *CubicBezierPoints = Curve->CubicBezierPoints;
                     f32 CubicBezierPointSize = NormalPointSize + CollisionTolerance;
                     u64 NumCubicBezierPoints = 3 * ControlPointCount;
                     u64 CenterPoint = 3 * Curve->SelectedControlPointIndex + 1;
                     
                     // NOTE(hbr): Slightly complex logic due to unsignedness of u64, maybe refactor
                     for (u64 Index = 0; Index <= 4; ++Index)
                     {
                        if (CenterPoint + Index >= 2)
                        {
                           u64 CheckIndex = CenterPoint + Index - 2;
                           if (CheckIndex < NumCubicBezierPoints && CheckIndex != CenterPoint)
                           {
                              local_position CubicBezierPoint = CubicBezierPoints[CheckIndex];
                              if (PointCollision(CheckPositionLocal, CubicBezierPoint, CubicBezierPointSize))
                              {
                                 Result = {
                                    .Entity = Entity,
                                    .Type = CurveCollision_CubicBezierPoint,
                                    .PointIndex = CheckIndex
                                 };
                                 goto collision_found_label;
                              }
                           }
                        }
                     }
                  }
               }
               
               if (CheckCollisionWithFlags & CheckCollisionWith_CurvePoints)
               {
                  local_position *CurvePoints = Curve->CurvePoints;
                  u64 CurvePointCount = Curve->CurvePointCount;
                  f32 CurveWidth = 2.0f * CollisionTolerance + Curve->CurveParams.CurveWidth;
                  local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
                  
                  for (u64 CurvePointIndex = 0;
                       CurvePointIndex + 1 < CurvePointCount;
                       ++CurvePointIndex)
                  {
                     v2f32 P1 = CurvePoints[CurvePointIndex];
                     v2f32 P2 = CurvePoints[CurvePointIndex + 1];
                     if (SegmentCollision(CheckPositionLocal, P1, P2, CurveWidth))
                     {
                        Result = {
                           .Entity = Entity,
                           .Type = CurveCollision_CurvePoint,
                           .PointIndex = CurvePointIndex
                        };
                        goto collision_found_label;
                     }
                  }
               }
            } break;
            
            case Entity_Image: {
               image *Image = &Entity->Image;
               
               if (CheckCollisionWithFlags & CheckCollisionWith_Images)
               {
                  v2f32 Position = Entity->Position;
                  
                  sf::Vector2u TextureSize = Image->Texture.getSize();
                  f32 SizeX = Abs(GlobalImageScaleFactor * Entity->Scale.X * TextureSize.x);
                  f32 SizeY = Abs(GlobalImageScaleFactor * Entity->Scale.Y * TextureSize.y);
                  v2f32 Extents = 0.5f * V2F32(SizeX + CollisionTolerance,
                                               SizeY + CollisionTolerance);
                  
                  rotation_2d InverseRotation = Rotation2DInverse(Entity->Rotation);
                  
                  v2f32 CheckPositionInImageSpace = CheckPosition - Position;
                  CheckPositionInImageSpace = RotateAround(CheckPositionInImageSpace,
                                                           V2F32(0.0f, 0.0f),
                                                           InverseRotation);
                  
                  if (-Extents.X <= CheckPositionInImageSpace.X && CheckPositionInImageSpace.X <= Extents.X &&
                      -Extents.Y <= CheckPositionInImageSpace.Y && CheckPositionInImageSpace.Y <= Extents.Y)
                  {
                     Result = { .Entity = Entity, .Type = Cast(curve_collision_type)0, .PointIndex = 0 };
                     goto collision_found_label;
                  }
               }
            } break;
         }
      }
   }
   
   collision_found_label:
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
         v2f32 P[] = {
            V2F32(0.0f, 0.0f),
            V2F32(1.0f, 0.0f),
            V2F32(0.0f, 1.0f),
            V2F32(1.0f, 1.0f),
         };
         
         Result = BezierCurveEvaluateFast(T, P, ArrayCount(P)).Y;
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
         ++Entities->EntityCount;
         break;
      }
   }
   
   return Entity;
}

internal void
BeginCurveCombining(curve_combining_state *State, entity *CurveEntity)
{
   ZeroStruct(State);
   State->SourceEntity = CurveEntity;
}

internal void
EndCurveCombining(curve_combining_state *State)
{
   ZeroStruct(State);
}

internal void
DeallocEntity(editor *Editor, entity *Entity)
{
   if (Editor->CurveSplitting.IsSplitting &&
       Editor->CurveSplitting.SplitCurveEntity == Entity)
   {
      Editor->CurveSplitting.IsSplitting = false;
      Editor->CurveSplitting.SplitCurveEntity = 0;
   }
   if (Editor->DeCasteljauVisual.IsVisualizing &&
       Editor->DeCasteljauVisual.CurveEntity == Entity)
   {
      Editor->DeCasteljauVisual.IsVisualizing = false;
      Editor->DeCasteljauVisual.CurveEntity = 0;
   }
   if (Editor->DegreeLowering.Entity == Entity)
   {
      Editor->DegreeLowering.Entity = 0;
   }
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
   ClearArena(Entity->Arena);
   --Editor->Entities.EntityCount;
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
   ZeroStruct(Notification);
   Notification->Type = Type;
   u64 ContentCount = FmtV(ArrayCount(Notification->ContentBuffer), Notification->ContentBuffer, Format, Args);
   Notification->Content = MakeStr(Notification->ContentBuffer, ContentCount);
   
   va_end(Args);
}

internal void
EditorSetSaveProjectPath(editor *Editor,
                         save_project_format SaveProjectFormat,
                         string SaveProjectFilePath)
{
   // TODO(hbr): Don't need to do that
   if (IsValid(Editor->ProjectSavePath))
   {
      FreeStr(&Editor->ProjectSavePath);
   }
   
   if (IsValid(SaveProjectFilePath))
   {
      Editor->SaveProjectFormat = SaveProjectFormat;
      Editor->ProjectSavePath = DuplicateStr(SaveProjectFilePath);
      
      temp_arena Temp = TempArena(0);
      
      string SaveProjectFileName = StringChopFileNameWithoutExtension(Temp.Arena,
                                                                      SaveProjectFilePath);
      Editor->RenderData.Window->setTitle(SaveProjectFileName.Data);
      
      EndTemp(Temp);
   }
   else
   {
      Editor->SaveProjectFormat = SaveProjectFormat_None;
      Editor->ProjectSavePath = {};
      Editor->RenderData.Window->setTitle(WINDOW_TITLE);
   }
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
                  v2s32 MousePosition,
                  render_data * CoordinateSystemData)
{
   b32 Result = ButtonState->Pressed &&
      !ScreenPointsAreClose(ButtonState->PressPosition,
                            MousePosition,
                            CoordinateSystemData);
   
   return Result;
}

internal b32
PressedWithKey(key_state Key, modifier_flags Flags)
{
   b32 JustPressed = (Key.Pressed && !Key.WasPressed);
   b32 WithModifiers = (Key.ModifierFlags == Flags);
   b32 Result = (JustPressed && WithModifiers);
   
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
MakeMovingCurvePointMode(entity *CurveEntity, u64 PointIndex,
                         b32 IsBezierPoint, arena *Arena)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_CurvePoint;
   Result.Moving.Entity = CurveEntity;
   Result.Moving.PointIndex = PointIndex;
   Result.Moving.IsBezierPoint = IsBezierPoint;
   
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
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                                                    &Editor->RenderData);
               
               collision Collision = {};
               if (Action.Input->Keys[Key_LeftShift].Pressed)
               {
                  // NOTE(hbr): Force no collision when shift if pressed, so that user can
                  // add control point wherever he wants
                  // TODO(hbr): Not really needed, just part of refactor to be equivalent, remove in the future
                  Collision.Entity = 0;
               }
               else
               {
                  check_collision_with_flags CheckCollisionWithFlags =
                     CheckCollisionWith_ControlPoints |
                     CheckCollisionWith_CurvePoints;
                  Collision = CheckCollisionWith(&Editor->Entities, ClickPosition, CollisionTolerance,
                                                 Editor->Params,  CheckCollisionWithFlags);
               }
               
               b32 SkipNormalModeHandling = false;
               if (Editor->CurveSplitting.IsSplitting)
               {
                  entity *SplitEntity = Editor->CurveSplitting.SplitCurveEntity;
                  curve *Curve = &SplitEntity->Curve;
                  
                  // TODO(hbr): Is this clicked necessary when checking for [Collision.Entity]? Check after implementing CubicBezierPoint
                  b32 ClickedOnSplitCurve = false;
                  f32 T = 0.0f;
                  if (Collision.Entity)
                  {
                     Assert(Collision.Entity->Type == Entity_Curve);
                     switch (Collision.Type)
                     {
                        case CurveCollision_ControlPoint:
                        case CurveCollision_CurvePoint: {
                           u64 NumPoints = ((Collision.Type == CurveCollision_ControlPoint) ? Curve->ControlPointCount : Curve->CurvePointCount);
                           T = Cast(f32)Collision.PointIndex / (NumPoints - 1);
                           ClickedOnSplitCurve = true;
                        } break;
                        
                        case CurveCollision_CubicBezierPoint: {
                           NotImplemented;
                        } break;
                     }
                  }
                  
                  if (ClickedOnSplitCurve)
                  {
                     Editor->CurveSplitting.T = Clamp(T, 0.0f, 1.0f); // NOTE(hbr): Be safe
                     SelectEntity(Editor, SplitEntity);
                     SkipNormalModeHandling = true;
                  }
               }
               
               if (Editor->CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget &&
                   Collision.Entity && Collision.Entity != Editor->CurveAnimation.FromCurveEntity)
               {
                  Editor->CurveAnimation.ToCurveEntity = Collision.Entity;
                  SkipNormalModeHandling = true;
               }
               
               curve_combining_state *Combining = &Editor->CurveCombining;
               if (Collision.Entity && Combining->SourceEntity)
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
                  
                  SkipNormalModeHandling = true;
               }
               
               if (!SkipNormalModeHandling)
               {
                  if (Collision.Entity)
                  {
                     switch (Collision.Type)
                     {
                        case CurveCollision_ControlPoint: {
                           SelectEntity(Editor, Collision.Entity);
                           // TODO(hbr): Select ControlPoint
                        } break;
                        
                        case CurveCollision_CubicBezierPoint: {
                           NotImplemented;
                        } break;
                        
                        case CurveCollision_CurvePoint: {
                           curve *Curve = GetCurve(Collision.Entity);
                           
                           f32 Segment = Cast(f32)Collision.PointIndex/ (Curve->CurvePointCount - 1);
                           u64 InsertAfterPointIndex =
                              Cast(u64)(Segment * (IsCurveLooped(Curve) ?
                                                   Curve->ControlPointCount :
                                                   Curve->ControlPointCount - 1));
                           InsertAfterPointIndex = Clamp(InsertAfterPointIndex, 0, Curve->ControlPointCount - 1);
                           
                           u64 InsertPointIndex = CurveInsertControlPoint(Collision.Entity, ClickPosition, InsertAfterPointIndex, 1.0f);
                           SelectControlPoint(Collision.Entity, InsertPointIndex);
                           SelectEntity(Editor, Collision.Entity);
                        } break;
                     }
                  }
                  else
                  {
                     entity *AddCurveEntity = 0;
                     if (Editor->SelectedEntity &&
                         Editor->SelectedEntity->Type == Entity_Curve)
                     {
                        AddCurveEntity = Editor->SelectedEntity;
                     }
                     else
                     {
                        entity *Entity = AllocEntity(Editor);
                        world_position Position = V2F32(0.0f, 0.0f);
                        v2f32 Scale = V2F32(1.0f, 1.0f);
                        rotation_2d Rotation = Rotation2DZero();
                        string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EntityCounter++);
                        InitCurveEntity(Entity,
                                        Position, Scale, Rotation,
                                        Name, Editor->Params.CurveDefaultParams);
                        
                        AddCurveEntity = Entity;
                     }
                     Assert(AddCurveEntity);
                     
                     u64 AddedPointIndex = AppendCurveControlPoint(AddCurveEntity, ClickPosition, 1.0f);
                     SelectControlPoint(AddCurveEntity, AddedPointIndex);
                     SelectEntity(Editor, AddCurveEntity);
                  }
               }
            } break;
            
            case Button_Right: {
               check_collision_with_flags CheckCollisionWithFlags =
                  CheckCollisionWith_ControlPoints | CheckCollisionWith_Images;
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                                                    &Editor->RenderData);
               collision Collision = CheckCollisionWith(&Editor->Entities, ClickPosition, CollisionTolerance,
                                                        Editor->Params, CheckCollisionWithFlags);
               
               if (Collision.Entity)
               {
                  switch (Collision.Entity->Type)
                  {
                     case Entity_Curve: {
                        switch (Collision.Type)
                        {
                           case CurveCollision_ControlPoint: {
                              RemoveCurveControlPoint(Collision.Entity, Collision.PointIndex);
                              SelectEntity(Editor, Collision.Entity);
                           } break;
                           
                           case CurveCollision_CubicBezierPoint: { NotImplemented; } break;
                           case CurveCollision_CurvePoint: InvalidPath;
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
               f32 CollisionTolerance =
                  ClipSpaceLengthToWorldSpace(Editor->Params.CollisionToleranceClipSpace,
                                              &Editor->RenderData);
               
               b32 SkipCheckingNormalModeCollisions = false;
               if (Editor->CurveSplitting.IsSplitting)
               {
                  if (Editor->CurveSplitting.DraggingSplitPoint)
                  {
                     SkipCheckingNormalModeCollisions = true;
                  }
                  else
                  {
                     entity *CurveEntity = Editor->CurveSplitting.SplitCurveEntity;
                     world_position SplitPoint = Editor->CurveSplitting.SplitPoint;
                     f32 SplitPointRadius =
                        ClipSpaceLengthToWorldSpace(Editor->Params.BezierSplitPoint.RadiusClipSpace,
                                                    &Editor->RenderData);
                     
                     if (PointCollision(SplitPoint, DragFromWorldPosition, SplitPointRadius + CollisionTolerance))
                     {
                        Editor->CurveSplitting.DraggingSplitPoint = true;
                        SelectEntity(Editor, CurveEntity);
                        SkipCheckingNormalModeCollisions = true;
                     }
                  }
               }
               
               if (!SkipCheckingNormalModeCollisions)
               {
                  check_collision_with_flags CheckCollisionWithFlags =
                     CheckCollisionWith_ControlPoints |
                     CheckCollisionWith_CurvePoints |
                     CheckCollisionWith_Images;
                  collision Collision = CheckCollisionWith(&Editor->Entities,
                                                           DragFromWorldPosition,
                                                           CollisionTolerance,
                                                           Editor->Params,
                                                           CheckCollisionWithFlags);
                  
                  if (Collision.Entity)
                  {
                     switch (Collision.Entity->Type)
                     {
                        case Entity_Curve: {
                           switch (Collision.Type)
                           {
                              case CurveCollision_ControlPoint:
                              case CurveCollision_CubicBezierPoint: {
                                 curve *Curve = GetCurve(Collision.Entity);
                                 
                                 b32 IsBezierPoint = (Collision.Type == CurveCollision_CubicBezierPoint);
                                 if (!IsBezierPoint)
                                 {
                                    SelectControlPoint(Collision.Entity, Collision.PointIndex);
                                 }
                                 
                                 Editor->Mode = MakeMovingCurvePointMode(Collision.Entity, Collision.PointIndex,
                                                                         IsBezierPoint, Editor->MovingPointArena);
                              } break;
                              
                              case CurveCollision_CurvePoint: {
                                 Editor->Mode = MakeMovingEntityMode(Collision.Entity);
                              } break;
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
      
      case UserAction_ButtonReleased: {
         switch (Action.Drag.Button)
         {
            case Button_Left: { Editor->CurveSplitting.DraggingSplitPoint = false; } break;
            case Button_Right:
            case Button_Middle: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_MouseMove: {
         curve_splitting_state *Split = &Editor->CurveSplitting;
         
         // NOTE(hbr): Pretty involved logic to move point along the curve and have pleasant experience
         if (Editor->CurveSplitting.DraggingSplitPoint)
         {
            world_position FromPosition = ScreenToWorldSpace(Action.MouseMove.FromPosition, &Editor->RenderData);
            world_position ToPosition = ScreenToWorldSpace(Action.MouseMove.ToPosition, &Editor->RenderData);
            
            v2f32 Translation = ToPosition - FromPosition;
            
            entity *CurveEntity = Split->SplitCurveEntity;
            curve *Curve = &CurveEntity->Curve;
            u64 CurvePointCount = Curve->CurvePointCount;
            local_position *CurvePoints = Curve->CurvePoints;
            
            f32 T = Clamp(Split->T, 0.0f, 1.0f);
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
                  
                  v2f32 CurveSegment =
                     LocalEntityPositionToWorld(CurveEntity, CurvePoints[SplitCurvePointIndex + 1])
                     - LocalEntityPositionToWorld(CurveEntity, CurvePoints[SplitCurvePointIndex]);
                  
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
                  
                  v2f32 CurveSegment =
                     LocalEntityPositionToWorld(CurveEntity, CurvePoints[SplitCurvePointIndex - 1])
                     - LocalEntityPositionToWorld(CurveEntity, CurvePoints[SplitCurvePointIndex]);
                  
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
            
            Editor->CurveSplitting.T = T;
         }
      } break;
      
      case UserAction_None: {} break;
   }
   EndTemp(Temp);
}

internal void
ExecuteUserActionMoveMode(editor *Editor, user_action Action)
{
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         world_position From = ScreenToWorldSpace(Action.MouseMove.FromPosition, &Editor->RenderData);
         world_position To = ScreenToWorldSpace(Action.MouseMove.ToPosition, &Editor->RenderData);
         v2f32 Translation = To - From;
         
         auto *Moving = &Editor->Mode.Moving;
         if (Moving->Type == MovingMode_CurvePoint)
         {
            translate_control_point_flags TranslateFlags = 0;
            if (Editor->Mode.Moving.IsBezierPoint)       TranslateFlags |= TranslateControlPoint_BezierPoint;
            if (!Action.Input->Keys[Key_LeftCtrl].Pressed)  TranslateFlags |= TranslateControlPoint_MatchBezierTwinDirection;
            if (!Action.Input->Keys[Key_LeftShift].Pressed) TranslateFlags |= TranslateControlPoint_MatchBezierTwinDirection;
            
            TranslateCurveControlPoint(Editor->Mode.Moving.Entity,
                                       Editor->Mode.Moving.PointIndex,
                                       TranslateFlags, Translation);
         }
         else
         {
            switch (Moving->Type)
            {
               case MovingMode_Entity: {
                  Moving->Entity->Position += Translation;
               } break;
               
               case MovingMode_Camera: {
                  // TODO(hbr): Maybe tackle this note.
                  // NOTE(hbr): This is a little janky, in a sense that we assume the camera is moving when [Entity == 0].
                  // Maybe get rid of this and store [camera] pointer directly in [Moving], but the camera right now is copied
                  // to [Result] by value so have to do it like this.
                  CameraMove(&Editor->RenderData.Camera, -Translation);
               } break;
               
               case MovingMode_CurvePoint: InvalidPath;
            }
         }
         
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Release.Button)
         {
            case Button_Left:
            case Button_Middle: { Editor->Mode = EditorModeNormal(); } break;
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
               CameraRotate(&Editor->RenderData.Camera, InverseRotation);
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
RenderPoint(v2f32 Position,
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
RenderRotationIndicator(editor *Editor, sf::Transform Transform)
{
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
                  &Editor->RenderData, Transform);
   }
}

internal void
DuplicateEntity(entity *Entity, editor *Editor)
{
   temp_arena Temp = TempArena(0);
   entity *Copy = AllocEntity(Editor);
   InitEntityFromEntity(Copy, Entity);
   string CopyName = StrF(Temp.Arena, "%S(copy)", Entity->Name);
   SetEntityName(Copy, CopyName);
   SelectEntity(Editor, Copy);
   f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, &Editor->RenderData);
   v2f32 SlightTranslation = V2F32(SlightTranslationX, 0.0f);
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

enum change_curve_params_ui_mode
{
   ChangeCurveParamsUI_DefaultCurve,
   ChangeCurveParamsUI_SelectedCurve,
};
struct change_curve_params_ui
{
   change_curve_params_ui_mode Mode;
   curve_params *DefaultCurveParams;
   curve *SelectedCurve;
};
internal change_curve_params_ui
ChangeCurveShapeUIModeDefaultCurve(curve_params *DefaultCurveParams)
{
   change_curve_params_ui Result = {};
   Result.Mode = ChangeCurveParamsUI_DefaultCurve;
   Result.DefaultCurveParams = DefaultCurveParams;
   
   return Result;
}
internal change_curve_params_ui
ChangeCurveShapeUIModeSelectedCurve(curve_params *DefaultCurveParams, curve *SelectedCurve)
{
   change_curve_params_ui Result = {};
   Result.Mode = ChangeCurveParamsUI_SelectedCurve;
   Result.DefaultCurveParams = DefaultCurveParams;
   Result.SelectedCurve = SelectedCurve;
   
   return Result;
}

internal b32
RenderChangeCurveParametersUI(char const *PushId, change_curve_params_ui UIMode)
{
   b32 SomeCurveParamChanged = false;
   
   curve_params DefaultParams = {};
   curve_params *ChangeParams = 0;
   switch (UIMode.Mode)
   {
      case ChangeCurveParamsUI_DefaultCurve: {
         DefaultParams = DefaultCurveParams();
         ChangeParams = UIMode.DefaultCurveParams;
      } break;
      
      case ChangeCurveParamsUI_SelectedCurve: {
         DefaultParams = *UIMode.DefaultCurveParams;
         ChangeParams = &UIMode.SelectedCurve->CurveParams;
      } break;
   }
   
   DeferBlock(UI_PushLabelF(PushId), UI_PopId())
   {   
      UI_SeparatorTextF("Curve");
      DeferBlock(UI_PushLabelF("Curve"), UI_PopId())
      {
         SomeCurveParamChanged |= UI_ComboF(&ChangeParams->InterpolationType, Interpolation_Count, InterpolationNames, "Interpolation Type");
         if (ResetCtxMenu("InterpolationTypeReset"))
         {
            ChangeParams->InterpolationType = DefaultParams.InterpolationType;
            SomeCurveParamChanged = true;
         }
         
         switch (ChangeParams->InterpolationType)
         {
            case Interpolation_Polynomial: {
               polynomial_interpolation_params *PolynomialParams = &ChangeParams->PolynomialInterpolationParams;
               
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
               SomeCurveParamChanged |= UI_ComboF(&ChangeParams->CubicSplineType, CubicSpline_Count, CubicSplineNames, "Spline Types");
               if (ResetCtxMenu("SplineTypeReset"))
               {
                  ChangeParams->CubicSplineType = DefaultParams.CubicSplineType;
                  SomeCurveParamChanged = true;
               }
            } break;
            
            case Interpolation_Bezier: {
               SomeCurveParamChanged |= UI_ComboF(&ChangeParams->BezierType, Bezier_Count, BezierNames, "Bezier Types");
               if (ResetCtxMenu("BezierTypeReset"))
               {
                  ChangeParams->BezierType = DefaultParams.BezierType;
                  SomeCurveParamChanged = true;
               }
            } break;
            
            case Interpolation_Count: InvalidPath;
         }
      }
      
      UI_NewRow();
      UI_SeparatorTextF("Line");
      DeferBlock(UI_PushLabelF("Line"), UI_PopId())
      {
         SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->CurveColor, "Color");
         if (ResetCtxMenu("CurveColorReset"))
         {
            ChangeParams->CurveColor = DefaultParams.CurveColor;
            SomeCurveParamChanged = true;
         }
         
         SomeCurveParamChanged |= UI_DragFloatF(&ChangeParams->CurveWidth, 0.0f, FLT_MAX, 0, "Width");
         if (ResetCtxMenu("CurveWidthReset"))
         {
            ChangeParams->CurveWidth = DefaultParams.CurveWidth;
            SomeCurveParamChanged = true;
         }
         
         SomeCurveParamChanged |= UI_SliderIntegerF(&ChangeParams->CurvePointCountPerSegment, 1, 2000, "Detail");
         if (ResetCtxMenu("DetailReset"))
         {
            ChangeParams->CurvePointCountPerSegment = DefaultParams.CurvePointCountPerSegment;
            SomeCurveParamChanged = true;
         }
      }
      
      
      UI_NewRow();
      UI_SeparatorTextF("Control Points");
      DeferBlock(UI_PushLabelF("ControlPoints"), UI_PopId())
      {
         UI_CheckboxF(&ChangeParams->PointsDisabled, "Points Disabled");
         if (ResetCtxMenu("PointsDisabledReset"))
         {
            ChangeParams->PointsDisabled= DefaultParams.PointsDisabled;
         }
         
         SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->PointColor, "Color");
         if (ResetCtxMenu("PointColorReset"))
         {
            ChangeParams->PointColor = DefaultParams.PointColor;
            SomeCurveParamChanged = true;
         }
         
         SomeCurveParamChanged |= UI_DragFloatF(&ChangeParams->PointSize, 0.0f, FLT_MAX, 0, "Size");
         if (ResetCtxMenu("PointSizeReset"))
         {
            ChangeParams->PointSize = DefaultParams.PointSize;
            SomeCurveParamChanged = true;
         }
         
         {
            // TODO(hbr): Maybe let ControlPoint weight to be 0 as well
            local f32 ControlPointMinWeight = EPS_F32;
            local f32 ControlPointMaxWeight = FLT_MAX;
            switch (UIMode.Mode)
            {
               case ChangeCurveParamsUI_DefaultCurve: {} break;
               
               case ChangeCurveParamsUI_SelectedCurve: {
                  if (ChangeParams->InterpolationType == Interpolation_Bezier &&
                      ChangeParams->BezierType == Bezier_Weighted)
                  {
                     UI_SeparatorTextF("Weights");
                     
                     curve *SelectedCurve = UIMode.SelectedCurve;
                     u64 ControlPointCount = SelectedCurve->ControlPointCount;
                     f32 *ControlPointWeights = SelectedCurve->ControlPointWeights;
                     u64 SelectedControlPointIndex = SelectedCurve->SelectedControlPointIndex;
                     
                     if (SelectedControlPointIndex < ControlPointCount)
                     {
                        // TODO(hbr): Replace to StrF
                        char ControlPointWeightLabel[128];
                        Fmt(ArrayCount(ControlPointWeightLabel), ControlPointWeightLabel,
                            "Selected Point (%llu)", SelectedControlPointIndex);
                        
                        SomeCurveParamChanged |= UI_DragFloatF(&ControlPointWeights[SelectedControlPointIndex],
                                                               ControlPointMinWeight, ControlPointMaxWeight,
                                                               0, ControlPointWeightLabel);
                        if (ResetCtxMenu("SelectedPointWeightReset"))
                        {
                           ControlPointWeights[SelectedControlPointIndex] = 1.0f;
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
                              char ControlPointWeightLabel[128];
                              Fmt(ArrayCount(ControlPointWeightLabel), ControlPointWeightLabel,
                                  "Point (%llu)", ControlPointIndex);
                              
                              SomeCurveParamChanged |= UI_DragFloatF(&ControlPointWeights[ControlPointIndex],
                                                                     ControlPointMinWeight, ControlPointMaxWeight,
                                                                     0, ControlPointWeightLabel);
                              if (ResetCtxMenu("PointWeightReset"))
                              {
                                 ControlPointWeights[ControlPointIndex] = 1.0f;
                              }
                           }
                        }
                        
                        ImGui::TreePop();
                     }
                  }
               } break;
            }
         }
         if (ChangeParams->InterpolationType == Interpolation_Bezier &&
             ChangeParams->BezierType == Bezier_Cubic)
         {
            UI_CheckboxF(&ChangeParams->CubicBezierHelpersDisabled, "Helpers Disabled");
            if (ResetCtxMenu("HelpersDisabledReset"))
            {
               ChangeParams->CubicBezierHelpersDisabled = DefaultParams.CubicBezierHelpersDisabled;
            }
         }
      }
      
      UI_NewRow();
      UI_SeparatorTextF("Polyline");
      DeferBlock(UI_PushLabelF("Polyline"), UI_PopId())
      {
         UI_CheckboxF(&ChangeParams->PolylineEnabled, "Enabled");
         if (ResetCtxMenu("PolylineEnabled"))
         {
            ChangeParams->PolylineEnabled = DefaultParams.PolylineEnabled;
         }
         
         SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->PolylineColor, "Color");
         if (ResetCtxMenu("PolylineColorReset"))
         {
            ChangeParams->PolylineColor = DefaultParams.PolylineColor;
            SomeCurveParamChanged = true;
         }
         
         SomeCurveParamChanged |= UI_DragFloatF(&ChangeParams->PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
         if (ResetCtxMenu("PolylineWidthReset"))
         {
            ChangeParams->PolylineWidth = DefaultParams.PolylineWidth;
            SomeCurveParamChanged = true;
         }
      }
      
      UI_NewRow();
      UI_SeparatorTextF("Convex Hull");
      DeferBlock(UI_PushLabelF("ConvexHull"), UI_PopId())
      {
         UI_CheckboxF(&ChangeParams->ConvexHullEnabled, "Enabled");
         if (ResetCtxMenu("ConvexHullEnabledReset"))
         {
            ChangeParams->ConvexHullEnabled = DefaultParams.ConvexHullEnabled;
         }
         
         SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->ConvexHullColor, "Color");
         if (ResetCtxMenu("ConvexHullColorReset"))
         {
            ChangeParams->ConvexHullColor = DefaultParams.ConvexHullColor;
            SomeCurveParamChanged = true;
         }
         
         SomeCurveParamChanged |= UI_DragFloatF(&ChangeParams->ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
         if (ResetCtxMenu("ConvexHullWidthReset"))
         {
            ChangeParams->ConvexHullWidth = DefaultParams.ConvexHullWidth;
            SomeCurveParamChanged = true;
         }
      }
      
      // NOTE(hbr): De Casteljau section logic (somewhat complex, maybe refactor).
      b32 ShowDeCasteljauGradientPicking = false;
      b32 ShowDeCasteljauVisualParamtersPicking = false;
      switch (UIMode.Mode)
      {
         case ChangeCurveParamsUI_DefaultCurve: {
            ShowDeCasteljauGradientPicking = true;
         } break;
         
         case ChangeCurveParamsUI_SelectedCurve: {
            if (ChangeParams->InterpolationType == Interpolation_Bezier)
            {
               ShowDeCasteljauGradientPicking = true;
               ShowDeCasteljauVisualParamtersPicking = true;
            }
         } break;
      }
      
      if (ChangeParams->InterpolationType == Interpolation_Bezier)
      {
         switch (ChangeParams->BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: {
               UI_NewRow();
               UI_SeparatorTextF("De Casteljau's Visualization");
               DeferBlock(UI_PushLabelF("DeCasteljauVisual"), UI_PopId())
               {
                  SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->DeCasteljau.GradientA, "Gradient A");
                  if (ResetCtxMenu("DeCasteljauVisualGradientAReset"))
                  {
                     ChangeParams->DeCasteljau.GradientA = DefaultParams.DeCasteljau.GradientA;
                     SomeCurveParamChanged = true;
                  }
                  
                  SomeCurveParamChanged |= UI_ColorPickerF(&ChangeParams->DeCasteljau.GradientB, "Gradient B");
                  if (ResetCtxMenu("DeCasteljauVisualGradientBReset"))
                  {
                     ChangeParams->DeCasteljau.GradientB = DefaultParams.DeCasteljau.GradientB;
                     SomeCurveParamChanged = true;
                  }
               }
            } break;
            
            case Bezier_Cubic: {} break;
            case Bezier_Count: InvalidPath;
         }
      }
   }
   
   return SomeCurveParamChanged;
}

internal void
BeginCurveSplitting(curve_splitting_state *Splitting, entity *CurveEntity)
{
   Splitting->IsSplitting = true;
   Splitting->SplitCurveEntity = CurveEntity;
   Splitting->DraggingSplitPoint = false;
   Splitting->T = 0.0f;
   Splitting->SavedCurveVersion = U64_MAX;
}

internal void
BeginVisualizingDeCasteljauAlgorithm(de_casteljau_visual_state *Visualization, entity *CurveEntity)
{
   Visualization->IsVisualizing = true;
   Visualization->T = 0.0f;
   Visualization->CurveEntity = CurveEntity;
   Visualization->SavedCurveVersion = U64_MAX;
   ClearArena(Visualization->Arena);
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
internal void
LowerBezierCurveDegree(curve_degree_lowering_state *Lowering, entity *Entity)
{
   curve *Curve = GetCurve(Entity);
   u64 PointCount = Curve->ControlPointCount;
   if (PointCount > 0)
   {
      Assert(Curve->CurveParams.InterpolationType == Interpolation_Bezier);
      temp_arena Temp = TempArena(Lowering->Arena);
      
      local_position *LowerPoints = PushArrayNonZero(Temp.Arena, PointCount, local_position);
      f32 *LowerWeights = PushArrayNonZero(Temp.Arena, PointCount, f32);
      local_position *LowerBeziers = PushArrayNonZero(Temp.Arena, 3 * (PointCount - 1), local_position);
      
      MemoryCopy(LowerPoints, Curve->ControlPoints, PointCount * SizeOf(LowerPoints[0]));
      MemoryCopy(LowerWeights, Curve->ControlPointWeights, PointCount * SizeOf(LowerWeights[0]));
      
      bezier_lower_degree LowerDegree = {};
      b32 IncludeWeights = false;
      // NOTE(hbr): We cannot merge those two cases, because weights might be already modified.
      switch (Curve->CurveParams.BezierType)
      {
         case Bezier_Normal: {
            LowerDegree = BezierCurveLowerDegree(LowerPoints, PointCount);
         } break;
         
         case Bezier_Weighted: {
            IncludeWeights = true;
            LowerDegree = BezierWeightedCurveLowerDegree(LowerPoints, LowerWeights, PointCount);
         } break;
         
         case Bezier_Cubic:
         case Bezier_Count: InvalidPath;
      }
      
      if (LowerDegree.Failure)
      {
         Lowering->Entity = Entity;
         
         ClearArena(Lowering->Arena);
         
         Lowering->SavedControlPoints = PushArrayNonZero(Lowering->Arena, PointCount, local_position);
         Lowering->SavedControlPointWeights = PushArrayNonZero(Lowering->Arena, PointCount, f32);
         Lowering->SavedCubicBezierPoints = PushArrayNonZero(Lowering->Arena, 3 * PointCount, local_position);
         MemoryCopy(Lowering->SavedControlPoints, Curve->ControlPoints, PointCount * SizeOf(Lowering->SavedControlPoints[0]));
         MemoryCopy(Lowering->SavedControlPointWeights, Curve->ControlPointWeights, PointCount * SizeOf(Lowering->SavedControlPointWeights[0]));
         MemoryCopy(Lowering->SavedCubicBezierPoints, Curve->CubicBezierPoints, 3 * PointCount * SizeOf(Lowering->SavedCubicBezierPoints[0]));
         
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
         
         Lowering->LowerDegree = LowerDegree;
         Lowering->P_Mix = 0.5f;
         Lowering->W_Mix = 0.5f;
         
         LowerPoints[LowerDegree.MiddlePointIndex] =
            Lowering->P_Mix * LowerDegree.P_I + (1 - Lowering->P_Mix) * LowerDegree.P_II;
         if (IncludeWeights)
         {
            LowerWeights[LowerDegree.MiddlePointIndex] =
               Lowering->W_Mix * LowerDegree.W_I + (1 - Lowering->W_Mix) * LowerDegree.W_II;
         }
      }
      
      // TODO(hbr): refactor this, it only has to be here because we still modify control points above
      BezierCubicCalculateAllControlPoints(LowerPoints, PointCount - 1, LowerBeziers + 1);
      SetCurveControlPoints(Entity, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
      
      Lowering->SavedCurveVersion = Curve->CurveVersion;
      
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
   
   switch (Curve->CurveParams.BezierType)
   {
      case Bezier_Normal: {
         BezierCurveElevateDegree(ElevatedControlPoints, ControlPointCount);
         ElevatedControlPointWeights[ControlPointCount] = 1.0f;
      } break;
      
      case Bezier_Weighted: {
         BezierWeightedCurveElevateDegree(ElevatedControlPoints,
                                          ElevatedControlPointWeights,
                                          ControlPointCount);
      } break;
      
      case Bezier_Cubic:
      case Bezier_Count: InvalidPath;
   }
   
   local_position *ElevatedCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                3 * (ControlPointCount + 1),
                                                                local_position);
   BezierCubicCalculateAllControlPoints(ElevatedControlPoints,
                                        ControlPointCount + 1,
                                        ElevatedCubicBezierPoints + 1);
   
   SetCurveControlPoints(Entity,
                         ControlPointCount + 1,
                         ElevatedControlPoints,
                         ElevatedControlPointWeights,
                         ElevatedCubicBezierPoints);
   
   EndTemp(Temp);
}

internal void
FocusCameraOn(editor *Editor, world_position Position, v2f32 Extents)
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
               v2f32 ControlPointRotated = RotateAround(ControlPointWorld,
                                                        V2F32(0.0f, 0.0f),
                                                        Rotation2DInverse(Editor->RenderData.Camera.Rotation));
               
               if (ControlPointRotated.X < Left)  Left  = ControlPointRotated.X;
               if (ControlPointRotated.X > Right) Right = ControlPointRotated.X;
               if (ControlPointRotated.Y < Down)  Down  = ControlPointRotated.Y;
               if (ControlPointRotated.Y > Top)   Top   = ControlPointRotated.Y;
            }
            
            world_position FocusPosition =  RotateAround(V2F32(0.5f * (Left + Right), 0.5f * (Down + Top)),
                                                         V2F32(0.0f, 0.0f), Editor->RenderData.Camera.Rotation);
            v2f32 Extents = V2F32(0.5f * (Right - Left) + 2.0f * Curve->CurveParams.PointSize,
                                  0.5f * (Top - Down) + 2.0f * Curve->CurveParams.PointSize);
            FocusCameraOn(Editor, FocusPosition, Extents);
         }
      } break;
      
      case Entity_Image: {
         f32 ScaleX = Abs(Entity->Scale.X);
         f32 ScaleY = Abs(Entity->Scale.Y);
         if (ScaleX != 0.0f && ScaleY != 0.0f)
         {
            sf::Vector2u TextureSize = Entity->Image.Texture.getSize();
            v2f32 Extents = V2F32(0.5f * GlobalImageScaleFactor * ScaleX * TextureSize.x,
                                  0.5f * GlobalImageScaleFactor * ScaleY * TextureSize.y);
            FocusCameraOn(Editor, Entity->Position, Extents);
         }
      } break;
   }
}

internal void
SplitCurveOnControlPoint(entity *Entity, editor *Editor)
{
   temp_arena Temp = TempArena(0);
   curve *Curve = GetCurve(Entity);
   Assert(Curve->SelectedControlPointIndex < Curve->ControlPointCount);
   
   u64 LeftPointCount = Curve->SelectedControlPointIndex + 1;
   u64 RightPointCount = Curve->ControlPointCount - Curve->SelectedControlPointIndex;
   
   local_position *LeftPoints = PushArrayNonZero(Temp.Arena, LeftPointCount, local_position);
   local_position *RightPoints = PushArrayNonZero(Temp.Arena, RightPointCount, local_position);
   f32 *LeftWeights = PushArrayNonZero(Temp.Arena, LeftPointCount, f32);
   f32 *RightWeights = PushArrayNonZero(Temp.Arena, RightPointCount, f32);
   local_position *LeftBeziers = PushArrayNonZero(Temp.Arena, 3 * LeftPointCount, local_position);
   local_position *RightBeziers = PushArrayNonZero(Temp.Arena, 3 * RightPointCount, local_position);
   
   // TODO(hbr): Optimize to not do so many copies and allocations
   // TODO(hbr): Maybe mint internals that create duplicate curve but without allocating and with allocating.
   // TODO(hbr): Isn't here a good place to optimize memory copies?
   MemoryCopy(LeftPoints, Curve->ControlPoints, LeftPointCount * SizeOf(LeftPoints[0]));
   MemoryCopy(LeftWeights, Curve->ControlPointWeights, LeftPointCount * SizeOf(LeftWeights[0]));
   MemoryCopy(LeftBeziers, Curve->CubicBezierPoints, 3 * LeftPointCount * SizeOf(LeftPoints[0]));
   MemoryCopy(RightPoints, Curve->ControlPoints + Curve->SelectedControlPointIndex, RightPointCount * SizeOf(RightPoints[0]));
   MemoryCopy(RightWeights, Curve->ControlPointWeights + Curve->SelectedControlPointIndex, RightPointCount * SizeOf(RightWeights[0]));
   MemoryCopy(RightBeziers, Curve->CubicBezierPoints + 3 * Curve->SelectedControlPointIndex, 3 * RightPointCount * SizeOf(RightPoints[0]));
   
   entity *LeftEntity = Entity;
   entity *RightEntity = AllocEntity(Editor);
   InitEntityFromEntity(RightEntity, LeftEntity);
   
   string LeftName  = StrF(Temp.Arena, "%S(left)", Entity->Name);
   string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
   SetEntityName(LeftEntity, LeftName);
   SetEntityName(RightEntity, RightName);
   
   SetCurveControlPoints(LeftEntity, LeftPointCount, LeftPoints, LeftWeights, LeftBeziers);
   SetCurveControlPoints(RightEntity, RightPointCount, RightPoints, RightWeights, RightBeziers);
   
   EndTemp(Temp);
}

// TODO(hbr): Do a pass oveer this internal to simplify the logic maybe (and simplify how the UI looks like in real life)
// TODO(hbr): Maybe also shorten some labels used to pass to ImGUI
internal void
RenderSelectedEntityUI(editor *Editor)
{
   entity *Entity = Editor->SelectedEntity;
   Assert(Entity);
   DeferBlock(UI_PushLabelF("SelectedEntity"), UI_PopId())
   {
      if (Editor->UI_Config.ViewSelectedEntityWindow)
      {
         if (UI_BeginWindowF(&Editor->UI_Config.ViewSelectedEntityWindow, "Entity Parameters"))
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
                  Entity->Position = V2F32(0.0f, 0.0f);
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
               v2f32 SelectedImageScale = Entity->Scale;
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
               Entity->Scale = V2F32(ImageNewScaleX, ImageNewScaleY);
            }
            
            UI_SliderIntegerF(&Entity->SortingLayer, -100, 100, "Sorting Layer");
            if (ResetCtxMenu("SortingLayerReset"))
            {
               Entity->SortingLayer = 0;
            }
            
            b32 SomeCurveParamChanged = false;
            if (Curve)
            {
               change_curve_params_ui UIMode =
                  ChangeCurveShapeUIModeSelectedCurve(&Editor->Params.CurveDefaultParams, Curve);
               UI_NewRow();
               SomeCurveParamChanged |= RenderChangeCurveParametersUI("SelectedCurveShape", UIMode);
            }
            
            b32 Delete                        = false;
            b32 Copy                          = false;
            b32 SwitchVisibility              = false;
            b32 Deselect                      = false;
            b32 FocusOn                       = false;
            b32 ElevateBezierCurve            = false;
            b32 LowerBezierCurve              = false;
            b32 VisualizeDeCasteljauAlgorithm = false;
            b32 SplitBezierCurve              = false;
            b32 AnimateCurve                  = false;
            b32 CombineCurve                  = false;
            b32 SplitOnControlPoint           = false;
            
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
                  // TODO(hbr): Maybe pick better name than transform
                  AnimateCurve = UI_ButtonF("Animate");
                  UI_SameRow();
                  // TODO(hbr): Maybe pick better name than "Combine"
                  CombineCurve = UI_ButtonF("Combine");
                  
                  UI_Disabled(Curve->SelectedControlPointIndex >= Curve->ControlPointCount)
                  {
                     UI_SameRow();
                     SplitOnControlPoint = UI_ButtonF("Split on Control Point");
                  }
                  
                  curve_params *CurveParams = &Curve->CurveParams;
                  b32 IsBezierNormalOrWeighted =
                  (CurveParams->InterpolationType == Interpolation_Bezier &&
                   (CurveParams->BezierType == Bezier_Normal || CurveParams->BezierType == Bezier_Weighted));
                  
                  UI_Disabled(!IsBezierNormalOrWeighted)
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
                     VisualizeDeCasteljauAlgorithm = UI_ButtonF("Visualize De Casteljau's Algorithm");
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
               LowerBezierCurveDegree(&Editor->DegreeLowering, Entity);
            }
            
            if (SplitBezierCurve)
            {
               BeginCurveSplitting(&Editor->CurveSplitting, Entity);
            }
            
            if (VisualizeDeCasteljauAlgorithm)
            {
               BeginVisualizingDeCasteljauAlgorithm(&Editor->DeCasteljauVisual, Entity);
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
         // TODO(hbr): Try to create this table at compile time
         local f32 LifeTimes[NotificationLifeTime_Count] = {};
         LifeTimes[NotificationLifeTime_In] = 0.15f;
         LifeTimes[NotificationLifeTime_Out] = 0.15f;
         LifeTimes[NotificationLifeTime_Proper] = 10.0f;
         LifeTimes[NotificationLifeTime_Invisible] = 0.1f;
         
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
         
         DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Fade),
                    ImGui::PopStyleVar())
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

// TODO(hbr): Get rid of this, this is a stub
internal string
SaveProjectInFormat(arena *Arena, 
                    save_project_format Format,
                    string Path,
                    editor *Editor)
{
   string Result = {};
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
         // TODO(hbr): Probably get rid of this #ifs
#if EDITOR_PROFILER
         UI_MenuItemF(&Config->ViewProfilerWindow,       0, "Profiler");
#endif
#if BUILD_DEBUG
         UI_MenuItemF(&Config->ViewDebugWindow,          0, "Debug");
#endif
         
         if (UI_BeginMenuF("Camera"))
         {
            camera *Camera = &Editor->RenderData.Camera;
            
            if (UI_MenuItemF(0, 0, "Reset Position"))
            {
               CameraMove(Camera, -Camera->Position);
            }
            
            if (UI_MenuItemF(0, 0, "Reset Rotation"))
            {
               CameraRotate(Camera, Rotation2DInverse(Camera->Rotation));
            }
            
            if (UI_MenuItemF(0, 0, "Reset Zoom"))
            {
               CameraSetZoom(Camera, 1.0f);
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
   if (NewProject || PressedWithKey(Input->Keys[Key_N], Modifier_Ctrl))
   {
      UI_OpenPopup(ConfirmCloseProject);
      Editor->ActionWaitingToBeDone = ActionToDo_NewProject;
   }
   if (OpenProject || PressedWithKey(Input->Keys[Key_O], Modifier_Ctrl))
   {
      UI_OpenPopup(ConfirmCloseProject);
      Editor->ActionWaitingToBeDone = ActionToDo_OpenProject;
   }
   if (Quit ||
       PressedWithKey(Input->Keys[Key_Q], 0) ||
       PressedWithKey(Input->Keys[Key_ESC], 0))
   {
      UI_OpenPopup(ConfirmCloseProject);
      Editor->ActionWaitingToBeDone = ActionToDo_Quit;
   }
   
   if (SaveProject || PressedWithKey(Input->Keys[Key_S], Modifier_Ctrl))
   {
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
   
   if (SaveProjectAs || PressedWithKey(Input->Keys[Key_S],
                                       Modifier_Ctrl | Modifier_Shift))
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
         
         std::string const &SelectedPath = FileDialog->GetFilePathName();
         string OpenProjectFilePath = Str(Temp.Arena,
                                          SelectedPath.c_str(),
                                          SelectedPath.size());
         
         // TODO(hbr): Implement this
#if 0
         
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
                            V2F32(0.0f, 0.0f),
                            V2F32(1.0f, 1.0f),
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
         if (UI_CollapsingHeaderF("Default Curve"))
         {
            change_curve_params_ui UIMode =
               ChangeCurveShapeUIModeDefaultCurve(&Params->CurveDefaultParams);
            RenderChangeCurveParametersUI("DefaultCurve", UIMode);
         }
         
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
               
               UI_DragFloatF(&Params->SelectedCurveControlPointOutlineThicknessScale,
                             0.0f, FLT_MAX, 0, "Selected Curve Control Point Outline Thickness");
               if (ResetCtxMenu("SelectedCurveControlPointOutlineThicknessReset"))
               {
                  Params->SelectedCurveControlPointOutlineThicknessScale = GlobalDefaultEditorParams.SelectedCurveControlPointOutlineThicknessScale;
               }
               
               UI_ColorPickerF(&Params->SelectedCurveControlPointOutlineColor, "Selected Curve Control Point Outline Color");
               if (ResetCtxMenu("SelectedCurveControlPointOutlineColorReset"))
               {
                  Params->SelectedCurveControlPointOutlineColor = GlobalDefaultEditorParams.SelectedCurveControlPointOutlineColor;
               }
               
               UI_ColorPickerF(&Params->SelectedControlPointOutlineColor, "Selected Control Point Outline Color");
               if (ResetCtxMenu("SelectedControlPointOutlineColorReset"))
               {
                  Params->SelectedControlPointOutlineColor = GlobalDefaultEditorParams.SelectedControlPointOutlineColor;
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
UpdateAndRenderCurveSplitting(editor *Editor, sf::Transform Transform)
{
   TimeFunction;
   
   // TODO(hbr): Remove those variables or rename them
   editor_params *EditorParams = &Editor->Params;
   sf::RenderWindow *Window = Editor->RenderData.Window;
   
   curve_splitting_state *Splitting = &Editor->CurveSplitting;
   entity *Entity = Splitting->SplitCurveEntity;
   curve *Curve = GetCurve(Entity);
   curve_params *CurveParams = &Curve->CurveParams;
   
   if (Splitting->IsSplitting)
   {
      b32 CurveQualifiesForSplitting = false;
      if (CurveParams->InterpolationType == Interpolation_Bezier)
      {
         switch (CurveParams->BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: { CurveQualifiesForSplitting = true; } break;
            case Bezier_Cubic: {} break;
            case Bezier_Count: InvalidPath;
         }
      }
      
      if (!CurveQualifiesForSplitting)
      {
         Splitting->IsSplitting = false;
      }
   }
   
   bool PerformSplit = false;
   bool T_ParamterChanged = false;
   if (Splitting->IsSplitting)
   {
      bool Cancel = false;
      bool IsSplitWindowOpen = true;
      
      DeferBlock(ImGui::Begin("Curve Splitting", &IsSplitWindowOpen,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {
         T_ParamterChanged = ImGui::SliderFloat("Split Point", &Splitting->T, 0.0f, 1.0f);
         
         PerformSplit = UI_ButtonF("Split");
         UI_SameRow();
         Cancel = UI_ButtonF("Cancel");
      }
      
      if (Cancel || !IsSplitWindowOpen)
      {
         Splitting->IsSplitting = false;
      }
   }
   
   if (PerformSplit)
   {
      temp_arena Temp = TempArena(0);
      
      u64 ControlPointCount = Curve->ControlPointCount;
      f32 *ControlPointWeights = Curve->ControlPointWeights;
      
      switch (CurveParams->BezierType)
      {
         case Bezier_Normal: {
            ControlPointWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
            for (u64 I = 0; I < ControlPointCount; ++I)
            {
               ControlPointWeights[I] = 1.0f;
            }
         } break;
         
         case Bezier_Weighted: {} break;
         case Bezier_Cubic:
         case Bezier_Count: InvalidPath;
      }
      
      local_position *LeftPoints = PushArrayNonZero(Temp.Arena, ControlPointCount, local_position);
      local_position *RightPoints = PushArrayNonZero(Temp.Arena, ControlPointCount, local_position);
      f32 *LeftWeights  = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
      f32 *RightWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
      local_position *LeftBeziers = PushArrayNonZero(Temp.Arena, 3 * ControlPointCount, local_position);
      local_position *RightBeziers = PushArrayNonZero(Temp.Arena, 3 * ControlPointCount, local_position);
      BezierCurveSplit(Splitting->T, Curve->ControlPoints, ControlPointWeights, ControlPointCount,
                       LeftPoints, LeftWeights, RightPoints, RightWeights);
      BezierCubicCalculateAllControlPoints(LeftPoints, ControlPointCount, LeftBeziers + 1);
      BezierCubicCalculateAllControlPoints(RightPoints, ControlPointCount, RightBeziers + 1);
      
      // TODO(hbr): Optimize here not to do so many copies, maybe merge with split on control point code
      // TODO(hbr): Isn't this similar to split on curve point
      // TODO(hbr): Maybe remove this duplication of initialzation, maybe not?
      // TODO(hbr): Refactor that code into common internal that splits curve into two curves
      entity *LeftEntity = Entity;
      entity *RightEntity = AllocEntity(Editor);
      InitEntityFromEntity(RightEntity, LeftEntity);
      
      string LeftName = StrF(Temp.Arena, "%S(left)", Entity->Name);
      string RightName = StrF(Temp.Arena, "%S(right)", Entity->Name);
      SetEntityName(LeftEntity, LeftName);
      SetEntityName(RightEntity, RightName);
      
      SetCurveControlPoints(LeftEntity, ControlPointCount, LeftPoints, LeftWeights, LeftBeziers);
      SetCurveControlPoints(RightEntity, ControlPointCount, RightPoints, RightWeights, RightBeziers);
      
      Splitting->IsSplitting = false;
      
      EndTemp(Temp);
   }
   
   if (Splitting->IsSplitting)
   {
      if (T_ParamterChanged || Splitting->SavedCurveVersion != Curve->CurveVersion)
      {
         local_position SplitPoint = {};
         switch (CurveParams->BezierType)
         {
            case Bezier_Normal: {
               SplitPoint = BezierCurveEvaluateFast(Splitting->T,
                                                    Curve->ControlPoints,
                                                    Curve->ControlPointCount);
            } break;
            
            case Bezier_Weighted: {
               SplitPoint = BezierWeightedCurveEvaluateFast(Splitting->T,
                                                            Curve->ControlPoints,
                                                            Curve->ControlPointWeights,
                                                            Curve->ControlPointCount);
            } break;
            
            case Bezier_Cubic:
            case Bezier_Count: InvalidPath;
         }
         Splitting->SplitPoint = LocalEntityPositionToWorld(Entity, SplitPoint);
      }
      
      if (!(Entity->Flags & EntityFlag_Hidden))
      {
         RenderPoint(Splitting->SplitPoint,
                     EditorParams->BezierSplitPoint,
                     &Editor->RenderData, Transform);
      }
   }
}

internal void
UpdateAndRenderDeCasteljauVisual(de_casteljau_visual_state *Visualization,
                                 sf::Transform Transform,
                                 sf::RenderWindow *Window)
{
   entity *Entity = Visualization->CurveEntity;
   curve *Curve = GetCurve(Entity);
   curve_params *CurveParams = &Curve->CurveParams;
   
   if (Visualization->IsVisualizing)
   {
      b32 CurveQualifiesForVisualizing = false;
      if (CurveParams->InterpolationType == Interpolation_Bezier)
      {
         switch (CurveParams->BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: { CurveQualifiesForVisualizing = true; } break;
            case Bezier_Cubic: {} break;
            case Bezier_Count: InvalidPath;
         }
      }
      
      if (!CurveQualifiesForVisualizing)
      {
         Visualization->IsVisualizing = false;
      }
   }
   
   bool T_ParamterChanged = false;
   if (Visualization->IsVisualizing)
   {
      bool IsSplitWindowOpen = true;
      
      DeferBlock(ImGui::Begin("De Casteljau's Visualization", &IsSplitWindowOpen,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {                 
         T_ParamterChanged = ImGui::SliderFloat("T", &Visualization->T, 0.0f, 1.0f);
      }
      
      if (!IsSplitWindowOpen)
      {
         Visualization->IsVisualizing = false;
      }
   }
   
   if (Visualization->IsVisualizing)
   {
      b32 VisualizationNeedsRecomputation = (T_ParamterChanged ||
                                             Visualization->SavedCurveVersion != Curve->CurveVersion);
      
      if (VisualizationNeedsRecomputation)
      {
         temp_arena Temp = TempArena(Visualization->Arena);
         
         u64 ControlPointCount = Curve->ControlPointCount;
         u64 NumIterations = ControlPointCount;
         color *IterationColors = PushArrayNonZero(Visualization->Arena, NumIterations, color);
         line_vertices *LineVerticesPerIteration = PushArray(Visualization->Arena,
                                                             NumIterations,
                                                             line_vertices);
         u64 NumAllIntermediatePoints = ControlPointCount * ControlPointCount;
         local_position *AllIntermediatePoints = PushArrayNonZero(Visualization->Arena,
                                                                  NumAllIntermediatePoints,
                                                                  local_position);
         
         // NOTE(hbr): Deal with cases at once.
         f32 *ControlPointWeights = Curve->ControlPointWeights;
         switch (CurveParams->BezierType)
         {
            case Bezier_Normal: {
               ControlPointWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
               for (u64 I = 0; I < ControlPointCount; ++I)
               {
                  ControlPointWeights[I] = 1.0f;
               }
            } break;
            
            case Bezier_Weighted: {} break;
            case Bezier_Cubic:
            case Bezier_Count: InvalidPath;
         }
         
         // TODO(hbr): Consider passing 0 as ThrowAwayWeights, but make sure algorithm
         // handles that case
         f32 *ThrowAwayWeights = PushArrayNonZero(Temp.Arena, NumAllIntermediatePoints, f32);
         DeCasteljauAlgorithm(Visualization->T,
                              Curve->ControlPoints,
                              ControlPointWeights,
                              ControlPointCount,
                              AllIntermediatePoints,
                              ThrowAwayWeights);
         
         f32 LineWidth = Curve->CurveParams.CurveWidth;
         color GradientA = Curve->CurveParams.DeCasteljau.GradientA;
         color GradientB = Curve->CurveParams.DeCasteljau.GradientB;
         
         f32 P = 0.0f;
         f32 Delta_P = 1.0f / (NumIterations - 1);
         for (u64 Iteration = 0;
              Iteration < NumIterations;
              ++Iteration)
         {
            color IterationColor = LerpColor(GradientA, GradientB, P);
            IterationColors[Iteration] = IterationColor;
            
            u64 NumPoints = ControlPointCount - Iteration;
            u64 PointsIndex = Idx(Iteration, 0, NumIterations);
            line_vertices *OldLineVertices = &LineVerticesPerIteration[Iteration];
            *OldLineVertices = CalculateLineVertices(NumPoints,
                                                     AllIntermediatePoints + PointsIndex,
                                                     LineWidth, IterationColor,
                                                     false,
                                                     LineVerticesAllocationArena(Visualization->Arena));
            
            P += Delta_P;
         }
         
         Visualization->SavedCurveVersion = Curve->CurveVersion;
         Visualization->NumIterations = NumIterations;
         Visualization->IterationColors = IterationColors;
         Visualization->LineVerticesPerIteration = LineVerticesPerIteration;
         Visualization->AllIntermediatePoints = AllIntermediatePoints;
         
         EndTemp(Temp);
      }
   }
   
   if (Visualization->IsVisualizing && !(Visualization->CurveEntity->Flags & EntityFlag_Hidden))
   {
      sf::Transform Model = CurveGetAnimate(Visualization->CurveEntity);
      sf::Transform MVP = Transform * Model;
      
      u64 NumIterations = Visualization->NumIterations;
      
      for (u64 Iteration = 0;
           Iteration < NumIterations;
           ++Iteration)
      {
         Window->draw(Visualization->LineVerticesPerIteration[Iteration].Vertices,
                      Visualization->LineVerticesPerIteration[Iteration].NumVertices,
                      Visualization->LineVerticesPerIteration[Iteration].PrimitiveType,
                      MVP);
      }
      
      f32 PointSize = Visualization->CurveEntity->Curve.CurveParams.PointSize;
      for (u64 Iteration = 0;
           Iteration < NumIterations;
           ++Iteration)
      {
         color PointColor = Visualization->IterationColors[Iteration];
         for (u64 I = 0; I < NumIterations - Iteration; ++I)
         {
            local_position IntermediatePoint =
               Visualization->AllIntermediatePoints[Idx(Iteration, I, NumIterations)];
            DrawCircle(IntermediatePoint, PointSize, PointColor, MVP, Window);
         }
      }
   }
}

internal void
UpdateAndRenderDegreeLowering(curve_degree_lowering_state *Lowering,
                              sf::Transform Transform,
                              sf::RenderWindow *Window)
{
   TimeFunction;
   
   if (Lowering->Entity)
   {
      if (Lowering->SavedCurveVersion != GetCurve(Lowering->Entity)->CurveVersion)
      { 
         Lowering->Entity = 0;
      }
   }
   
   if (Lowering->Entity)
   {
      entity *Entity = Lowering->Entity;
      curve *Curve = GetCurve(Lowering->Entity);
      curve_params *CurveParams = &Curve->CurveParams;
      
      Assert(CurveParams->InterpolationType == Interpolation_Bezier);
      b32 IncludeWeights = false;
      switch (CurveParams->BezierType)
      {
         case Bezier_Normal: {} break;
         case Bezier_Weighted: { IncludeWeights = true; } break;
         case Bezier_Cubic:
         case Bezier_Count: InvalidPath;
      }
      
      b32 IsDegreeLoweringWindowOpen = true;
      b32 P_MixChanged = false;
      b32 W_MixChanged = false;
      if (UI_BeginWindowF(&IsDegreeLoweringWindowOpen, "Degree Lowering"))
      {          
         // TODO(hbr): Add wrapping
         UI_TextF("Degree lowering failed (curve has higher degree "
                  "than the one you are trying to fit). Tweak parameters"
                  "in order to fit curve manually or revert.");
         P_MixChanged = UI_SliderFloatF(&Lowering->P_Mix, 0.0f, 1.0f, "Middle Point Mix");
         if (IncludeWeights)
         {
            W_MixChanged = UI_SliderFloatF(&Lowering->W_Mix, 0.0f, 1.0f, "Middle Point Weight Mix");
         }
      }
      UI_EndWindow();
      
      b32 Ok     = UI_ButtonF("OK"); UI_SameRow();
      b32 Revert = UI_ButtonF("Revert");
      
      //-
      Assert(Lowering->LowerDegree.MiddlePointIndex < Curve->ControlPointCount);
      
      if (P_MixChanged || W_MixChanged)
      {
         local_position NewControlPoint =
            Lowering->P_Mix * Lowering->LowerDegree.P_I + (1 - Lowering->P_Mix) * Lowering->LowerDegree.P_II;
         f32 NewControlPointWeight =
            Lowering->W_Mix * Lowering->LowerDegree.W_I + (1 - Lowering->W_Mix) * Lowering->LowerDegree.W_II;
         
         SetCurveControlPoint(Lowering->Entity,
                              Lowering->LowerDegree.MiddlePointIndex,
                              NewControlPoint,
                              NewControlPointWeight);
         
         Lowering->SavedCurveVersion = Curve->CurveVersion;
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
         Lowering->Entity = 0;
      }
   }
   
   if (Lowering->Entity)
   {
      sf::Transform Model = CurveGetAnimate(Lowering->Entity);
      sf::Transform MVP = Transform * Model;
      
      Window->draw(Lowering->SavedCurveVertices,
                   Lowering->NumSavedCurveVertices,
                   Lowering->SavedPrimitiveType,
                   MVP);
   }
}

internal void
RenderCubicBezierPointsWithHelperLines(curve *Curve, u64 ControlPointIndex,
                                       b32 RenderPrevHelperPoint, b32 RenderNextHelperPoint,
                                       f32 PointRadius, color PointColor, f32 LineWidth,
                                       color LineColor, sf::Transform Transform,
                                       sf::RenderWindow *Window)
{
   local_position *CubicBezierPoints = Curve->CubicBezierPoints;
   u64 CenterPointIndex = 3 * ControlPointIndex + 1;
   
   if (RenderPrevHelperPoint || RenderNextHelperPoint)
   {
      local_position CenterPoint = CubicBezierPoints[CenterPointIndex];
      local_position PrevHelperPoint = (RenderPrevHelperPoint ?
                                        CubicBezierPoints[CenterPointIndex - 1] :
                                        CenterPoint);
      local_position NextHelperPoint = (RenderNextHelperPoint ?
                                        CubicBezierPoints[CenterPointIndex + 1] :
                                        CenterPoint);
      
      DrawLine(PrevHelperPoint, CenterPoint, LineWidth, LineColor, Transform, Window);
      DrawLine(CenterPoint, NextHelperPoint, LineWidth, LineColor, Transform, Window);
      
      if (RenderPrevHelperPoint) DrawCircle(PrevHelperPoint, PointRadius, PointColor, Transform, Window);
      if (RenderNextHelperPoint) DrawCircle(NextHelperPoint, PointRadius, PointColor, Transform, Window);
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
               Animation->ToCurvePoints = PushArrayNonZero(Animation->Arena, CurvePointCount, v2f32);
               EvaluateCurve(ToCurve, CurvePointCount, Animation->ToCurvePoints);
               Animation->SavedToCurveVersion = ToCurve->CurveVersion;
               
               ToCurvePointsRecalculated = true;
            }
            
            if (ToCurvePointsRecalculated || BlendChanged)
            {               
               v2f32 *ToCurvePoints = Animation->ToCurvePoints;
               v2f32 *FromCurvePoints = FromCurve->CurvePoints;
               v2f32 *AnimatedCurvePoints = PushArrayNonZero(Temp.Arena, CurvePointCount, v2f32);
               
               f32 Blend = CalculateAnimation(Animation->Animation, Animation->Blend);
               f32 T = 0.0f;
               f32 Delta_T = 1.0f / (CurvePointCount - 1);
               for (u64 VertexIndex = 0;
                    VertexIndex < CurvePointCount;
                    ++VertexIndex)
               {
                  v2f32 From = LocalEntityPositionToWorld(FromCurveEntity,
                                                          FromCurvePoints[VertexIndex]);
                  v2f32 To = LocalEntityPositionToWorld(ToCurveEntity,
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

internal void
RenderEntityCombo(entities *Entities, entity **InOutEntity, string Label)
{
   entity *Entity = *InOutEntity;
   string Preview = (Entity ? Entity->Name : StrLit(""));
   if (UI_BeginCombo(Preview, Label))
   {
      for (u64 EntityIndex = 0;
           EntityIndex < Entities->EntityCount;
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
            ArrayReverse(From->ControlPoints, FromCount, local_position);
            ArrayReverse(From->ControlPointWeights, FromCount, f32);
            ArrayReverse(From->CubicBezierPoints, 3 * FromCount, local_position);
         }
         
         if (State->WithCurveFirstControlPoint)
         {
            ArrayReverse(To->ControlPoints, ToCount, local_position);
            ArrayReverse(To->ControlPointWeights, ToCount, f32);
            ArrayReverse(To->CubicBezierPoints, 3 * ToCount, local_position);
         }
         
         u64 CombinedPointCount = ToCount;
         u64 StartIndex = 0;
         v2f32 Translation = {};
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
         v2f32 *CombinedPoints  = PushArrayNonZero(Temp.Arena, CombinedPointCount, v2f32);
         f32   *CombinedWeights = PushArrayNonZero(Temp.Arena, CombinedPointCount, f32);
         v2f32 *CombinedBeziers = PushArrayNonZero(Temp.Arena, 3 * CombinedPointCount, v2f32);
         MemoryCopy(CombinedPoints, To->ControlPoints, ToCount * SizeOf(CombinedPoints[0]));
         MemoryCopy(CombinedWeights, To->ControlPointWeights, ToCount * SizeOf(CombinedWeights[0]));
         MemoryCopy(CombinedBeziers, To->CubicBezierPoints, 3 * ToCount * SizeOf(CombinedBeziers[0]));
         
         // TODO(hbr): SIMD?
         for (u64 I = StartIndex; I < FromCount; ++I)
         {
            world_position FromPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
            local_position ToPoint   = WorldToLocalEntityPosition(ToEntity, FromPoint + Translation);
            CombinedPoints[ToCount - StartIndex + I] = ToPoint;
         }
         for (u64 I = 3 * StartIndex; I < 3 * FromCount; ++I)
         {
            world_position FromBezier = LocalEntityPositionToWorld(FromEntity, From->CubicBezierPoints[I]);
            local_position ToBezier   = WorldToLocalEntityPosition(ToEntity, FromBezier + Translation); 
            CombinedBeziers[3 * (ToCount - StartIndex) + I] = ToBezier;
         }
         MemoryCopy(CombinedWeights + ToCount,
                    From->ControlPointWeights + StartIndex,
                    (FromCount - StartIndex) * SizeOf(CombinedWeights[0]));
         
         // NOTE(hbr): Combine control points properly on the border
         switch (State->CombinationType)
         {
            // NOTE(hbr): Nothing to do
            case CurveCombination_Merge:
            case CurveCombination_C0: {} break;
            
            case CurveCombination_C1: {
               if (FromCount >= 2 && ToCount >= 2)
               {
                  v2f32 P = CombinedPoints[ToCount - 1];
                  v2f32 Q = CombinedPoints[ToCount - 2];
                  
                  // NOTE(hbr): First derivative equal
                  v2f32 FixedControlPoint = Cast(f32)ToCount/FromCount * (P - Q) + P;
                  v2f32 Fix = FixedControlPoint - CombinedPoints[ToCount];
                  CombinedPoints[ToCount] = FixedControlPoint;
                  
                  CombinedBeziers[3 * ToCount + 0] += Fix;
                  CombinedBeziers[3 * ToCount + 1] += Fix;
                  CombinedBeziers[3 * ToCount + 2] += Fix;
               }
            } break;
            
            case CurveCombination_C2: {
               if (FromCount >= 3 && ToCount >= 3)
               {
                  // TODO(hbr): Merge C1 with C2, maybe G1.
                  v2f32 R = CombinedPoints[ToCount - 3];
                  v2f32 Q = CombinedPoints[ToCount - 2];
                  v2f32 P = CombinedPoints[ToCount - 1];
                  
                  // NOTE(hbr): First derivative equal
                  v2f32 Fixed_T = Cast(f32)ToCount/FromCount * (P - Q) + P;
                  // NOTE(hbr): Second derivative equal
                  v2f32 Fixed_U = Cast(f32)(FromCount * (FromCount-1))/(ToCount * (ToCount-1)) * (P - 2.0f * Q + R) + 2.0f * Fixed_T - P;
                  v2f32 Fix_T = Fixed_T - CombinedPoints[ToCount];
                  v2f32 Fix_U = Fixed_U - CombinedPoints[ToCount + 1];
                  CombinedPoints[ToCount] = Fixed_T;
                  CombinedPoints[ToCount + 1] = Fixed_U;
                  
                  CombinedBeziers[3 * ToCount + 0] += Fix_T;
                  CombinedBeziers[3 * ToCount + 1] += Fix_T;
                  CombinedBeziers[3 * ToCount + 2] += Fix_T;
                  
                  CombinedBeziers[3 * (ToCount + 1) + 0] += Fix_U;
                  CombinedBeziers[3 * (ToCount + 1) + 1] += Fix_U;
                  CombinedBeziers[3 * (ToCount + 1) + 2] += Fix_U;
               }
            } break;
            
            case CurveCombination_G1: {
               if (FromCount >= 2 && ToCount >= 2)
               {
                  f32 PreserveLength = Norm(From->ControlPoints[1] - From->ControlPoints[0]);
                  
                  v2f32 P = CombinedPoints[ToCount - 2];
                  v2f32 Q = CombinedPoints[ToCount - 1];
                  v2f32 Direction = P - Q;
                  Normalize(&Direction);
                  
                  v2f32 FixedControlPoint = Q - PreserveLength * Direction;
                  v2f32 Fix = FixedControlPoint - CombinedPoints[ToCount];
                  CombinedPoints[ToCount] = FixedControlPoint;
                  
                  CombinedBeziers[3 * ToCount + 0] += Fix;
                  CombinedBeziers[3 * ToCount + 1] += Fix;
                  CombinedBeziers[3 * ToCount + 2] += Fix;
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
      
      v2f32 LineDirection = WithPoint - SourcePoint;
      Normalize(&LineDirection);
      
      f32 TriangleSide = 10.0f * LineWidth;
      f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
      world_position BaseVertex = WithPoint - TriangleHeight * LineDirection;
      DrawLine(SourcePoint, BaseVertex, LineWidth, Color, Transform, Editor->RenderData.Window);
      
      v2f32 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
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
   sorted_entity_array SortedEntities = SortEntitiesByLayer(Temp.Arena, &Editor->Entities);
   
   UpdateAnimateCurveAnimation(Editor, DeltaTime, VP);
   
   for (u64 EntityIndex = 0;
        EntityIndex < SortedEntities.EntityCount;
        ++EntityIndex)
   {
      entity *Entity = SortedEntities.Entries[EntityIndex].Entity;
      if (!(Entity->Flags & EntityFlag_Hidden))
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               curve_params *CurveParams = &Curve->CurveParams;
               
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
               
               if (!CurveParams->PointsDisabled)
               {
                  b32 IsSelected = (Entity->Flags & EntityFlag_Selected);
                  u64 SelectedControlPointIndex = Curve->SelectedControlPointIndex;
                  
                  if (!CurveParams->CubicBezierHelpersDisabled &&
                      IsSelected && SelectedControlPointIndex != U64_MAX &&
                      CurveParams->InterpolationType == Interpolation_Bezier &&
                      CurveParams->BezierType == Bezier_Cubic)
                  {
                     f32 HelperLineWidth =
                        ClipSpaceLengthToWorldSpace(Editor->Params.CubicBezierHelperLineWidthClipSpace,
                                                    &Editor->RenderData);
                     
                     RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex,
                                                            true, true, CurveParams->PointSize,
                                                            CurveParams->PointColor, HelperLineWidth,
                                                            CurveParams->CurveColor, MVP, Window);
                     if (SelectedControlPointIndex >= 1)
                     {
                        RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex - 1,
                                                               false, true, CurveParams->PointSize,
                                                               CurveParams->PointColor, HelperLineWidth,
                                                               CurveParams->CurveColor, MVP, Window);
                     }
                     if (SelectedControlPointIndex + 1 < Curve->ControlPointCount)
                     {
                        RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex + 1,
                                                               true, false, CurveParams->PointSize,
                                                               CurveParams->PointColor, HelperLineWidth,
                                                               CurveParams->CurveColor, MVP, Window);
                     }
                  }
                  
                  f32 NormalControlPointRadius = CurveParams->PointSize;
                  color ControlPointColor = CurveParams->PointColor;
                  
                  f32 ControlPointOutlineThicknessScale = 0.0f;
                  color NormalControlPointOutlineColor = {};
                  if (IsSelected)
                  {
                     ControlPointOutlineThicknessScale = Editor->Params.SelectedCurveControlPointOutlineThicknessScale;
                     NormalControlPointOutlineColor = Editor->Params.SelectedCurveControlPointOutlineColor;
                  }
                  
                  u64 ControlPointCount = Curve->ControlPointCount;
                  local_position *ControlPoints = Curve->ControlPoints;
                  for (u64 ControlPointIndex = 0;
                       ControlPointIndex < ControlPointCount;
                       ++ControlPointIndex)
                  {
                     local_position ControlPoint = ControlPoints[ControlPointIndex];
                     
                     f32 ControlPointRadius = NormalControlPointRadius;
                     if (ControlPointIndex == ControlPointCount - 1)
                     {
                        ControlPointRadius *= Editor->Params.LastControlPointSizeMultiplier;
                     }
                     
                     color ControlPointOutlineColor = NormalControlPointOutlineColor;
                     if (ControlPointIndex == SelectedControlPointIndex)
                     {
                        ControlPointOutlineColor = Editor->Params.SelectedControlPointOutlineColor;
                     }
                     
                     f32 OutlineThickness = ControlPointOutlineThicknessScale * ControlPointRadius;
                     
                     DrawCircle(ControlPoint,
                                ControlPointRadius,
                                ControlPointColor,
                                MVP,
                                Window,
                                OutlineThickness,
                                ControlPointOutlineColor);
                  }
               }
               
               
               curve_degree_lowering_state *Lowering = &Editor->DegreeLowering;
               if (Lowering->Entity == Entity)
               {
                  UpdateAndRenderDegreeLowering(Lowering, VP, Editor->RenderData.Window);
               }
               
               de_casteljau_visual_state *Visualization = &Editor->DeCasteljauVisual;
               if (Visualization->IsVisualizing && Visualization->CurveEntity == Entity)
               {
                  UpdateAndRenderDeCasteljauVisual(Visualization, VP, Editor->RenderData.Window);
               }
               
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
               
               curve_splitting_state *Splitting = &Editor->CurveSplitting;
               if (Splitting->IsSplitting && Splitting->SplitCurveEntity == Entity)
               {
                  UpdateAndRenderCurveSplitting(Editor, VP);
               }
            } break;
            
            case Entity_Image: {
               image *Image = &Entity->Image;
               
               sf::Sprite Sprite;
               Sprite.setTexture(Image->Texture);
               
               sf::Vector2u TextureSize = Image->Texture.getSize();
               Sprite.setOrigin(0.5f*TextureSize.x, 0.5f*TextureSize.y);
               
               v2f32 Scale = GlobalImageScaleFactor * Entity->Scale;
               // NOTE(hbr): Flip vertically because SFML images are flipped.
               Sprite.setScale(Scale.X, -Scale.Y);
               
               v2f32 Position = Entity->Position;
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
   CameraUpdate(&Editor->RenderData.Camera, Input->MouseWheelDelta, DeltaTime);
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
      }
      if (Action.Type != UserAction_None)
      {
         ExecuteUserAction(Editor, Action);
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
   if (PressedWithKey(Input->Keys[Key_E], 0))
   {
      AddNotificationF(Editor, Notification_Error, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
   }
   
   if (Editor->SelectedEntity)
   {
      RenderSelectedEntityUI(Editor);
   }
   
   if (Editor->UI_Config.ViewListOfEntitiesWindow)
   {
      RenderListOfEntitiesWindow(Editor);
   }
   
   if (Editor->UI_Config.ViewParametersWindow)
   {
      RenderSettingsWindow(Editor);
   }
   
   if (Editor->UI_Config.ViewDiagnosticsWindow)
   {
      RenderDiagnosticsWindow(Editor, DeltaTime);
   }
   
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
         .translate(-V2F32ToVector2f(Data->Camera.Position));
      // NOTE(hbr): Data->Camera Space -> Clip Space
      sf::Transform ProjectionAnimate =
         sf::Transform()
         .scale(1.0f / (0.5f * Data->FrustumSize * Data->AspectRatio),
                1.0f / (0.5f * Data->FrustumSize));
      VP = ProjectionAnimate * ViewAnimate;
   }
   
   UpdateAndRenderEntities(Editor, DeltaTime, VP);
   
   RenderRotationIndicator(Editor, VP);
   
   // NOTE(hbr): Update menu bar here, because world has to already be rendered
   // and UI not, because we might save our project into image file and we
   // don't want to include UI render into our image.
   UpdateAndRenderMenuBar(Editor, Input);
}

internal void
SetNormalizedDeviceCoordinatesView(sf::RenderWindow *RenderWindow)
{
   sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
   sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
   sf::View View = sf::View(Center, Size);
   
   RenderWindow->setView(View);
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
   InitProfiler();
   
   arena *PermamentArena = AllocArena();
   
   sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
   sf::ContextSettings ContextSettings = sf::ContextSettings();
   ContextSettings.antialiasingLevel = 4;
   sf::RenderWindow Window(VideoMode, WINDOW_TITLE, sf::Style::Default, ContextSettings);
   
   SetNormalizedDeviceCoordinatesView(&Window);
   
#if 1
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
               .Position = V2F32(0.0f, 0.0f),
               .Rotation = Rotation2DZero(),
               .Zoom = 1.0f,
               .ZoomSensitivity = 0.05f,
               .ReachingTargetSpeed = 10.0f,
            },
            .FrustumSize = 2.0f,
         };
         Editor->FrameStats = MakeFrameStats();
         Editor->MovingPointArena = AllocArena();
         Editor->DeCasteljauVisual.Arena = AllocArena();
         Editor->DegreeLowering.Arena = AllocArena();
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