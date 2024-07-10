#include "editor.h"

#include "editor_base.cpp"
#include "editor_os.cpp"
#include "editor_profiler.cpp"
#include "editor_math.cpp"

#include "editor_input.cpp"
#include "editor_entity.cpp"
#include "editor_debug.cpp"
#include "editor_draw.cpp"

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
      Camera->Position = LerpV2F32(Camera->Position, Camera->PositionTarget, T);
      Camera->Zoom = LerpF32(Camera->Zoom, Camera->ZoomTarget, T);
      
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
CameraToWorldSpace(camera_position Position, coordinate_system_data Data)
{
   v2f32 Rotated = RotateAround(Position,
                                V2F32(0.0f, 0.0f),
                                Data.Camera.Rotation);
   v2f32 Scaled = Rotated / Data.Camera.Zoom;
   v2f32 Translated = Scaled + Data.Camera.Position;
   
   world_position WorldPosition = Translated;
   return WorldPosition;
}

internal camera_position
WorldToCameraSpace(world_position Position, coordinate_system_data Data)
{
   v2f32 Translated = Position - Data.Camera.Position;
   v2f32 Scaled = Data.Camera.Zoom * Translated;
   v2f32 Rotated = RotateAround(Scaled,
                                V2F32(0.0f, 0.0f),
                                Rotation2DInverse(Data.Camera.Rotation));
   
   camera_position CameraPosition = Rotated;
   return CameraPosition;
}

internal camera_position
ScreenToCameraSpace(screen_position Position, coordinate_system_data Data)
{
   v2f32 ClipPosition = V2F32FromVec(Data.Window->mapPixelToCoords(V2S32ToVector2i(Position)));
   
   f32 FrustumExtent = 0.5f * Data.Projection.FrustumSize;
   f32 CameraPositionX = ClipPosition.X * Data.Projection.AspectRatio * FrustumExtent;
   f32 CameraPositionY = ClipPosition.Y * FrustumExtent;
   
   camera_position CameraPosition = V2F32(CameraPositionX, CameraPositionY);
   return CameraPosition;
}

internal world_position
ScreenToWorldSpace(screen_position Position, coordinate_system_data Data)
{
   camera_position CameraPosition = ScreenToCameraSpace(Position, Data);
   world_position WorldPosition = CameraToWorldSpace(CameraPosition, Data);
   
   return WorldPosition;
}

internal f32
ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, coordinate_system_data Data)
{
   f32 CameraSpaceDistance = ClipSpaceDistance * 0.5f * Data.Projection.FrustumSize;
   f32 WorldSpaceDistance = CameraSpaceDistance / Data.Camera.Zoom;
   
   return WorldSpaceDistance;
}

internal collision
CheckCollisionWith(entity *Entities,
                   world_position CheckPosition,
                   f32 CollisionTolerance,
                   editor_params EditorParams,
                   check_collision_with_flags CheckCollisionWithFlags)
{
   collision Result = {};
   
   temp_arena Temp = TempArena(0);
   
   sorted_entity_array SortedEntities = SortEntitiesBySortingLayer(Temp.Arena, Entities);
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
                      Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier &&
                      Curve->CurveParams.CurveShape.BezierType == Bezier_Cubic)
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

internal char const *
AnimationToString(animation_type Animation)
{
   switch (Animation)
   {
      case Animation_Smooth: return "Smooth";
      case Animation_Linear: return "Linear";
      case Animation_Count: InvalidPath;
   }
   
   return "invalid";
}

internal editor_state
CreateEditorState(pool *EntityPool,
                  u64 CurveCounter, camera Camera,
                  arena *DeCasteljauVisualizationArena,
                  arena *DegreeLoweringArena,
                  arena *MovingPointArena,
                  arena *CurveAnimationArena,
                  f32 CurveAnimationSpeed)
{
   editor_state Result = {};
   Result.EntityPool = EntityPool;
   Result.EverIncreasingCurveCounter = CurveCounter;
   Result.Camera = Camera;
   Result.DeCasteljauVisualization.Arena = DeCasteljauVisualizationArena;
   Result.DegreeLowering.Arena = DegreeLoweringArena;
   Result.MovingPointArena = MovingPointArena;
   Result.CurveAnimation.Arena = CurveAnimationArena;
   Result.CurveAnimation.AnimationSpeed = CurveAnimationSpeed;
   
   return Result;
}

internal editor_state
CreateDefaultEditorState(pool *EntityPool,
                         arena *DeCasteljauVisualizationArena,
                         arena *DegreeLoweringArena,
                         arena *MovingPointArena,
                         arena *CurveAnimationArena)
{
   camera Camera = {
      .Position = V2F32(0.0f, 0.0f),
      .Rotation = Rotation2DZero(),
      .Zoom = 1.0f,
      .ZoomSensitivity = 0.05f,
      .ReachingTargetSpeed = 10.0f,
   };
   
   editor_state Result = CreateEditorState(EntityPool, 0,
                                           Camera,
                                           DeCasteljauVisualizationArena,
                                           DegreeLoweringArena,
                                           MovingPointArena,
                                           CurveAnimationArena,
                                           CURVE_DEFAULT_ANIMATION_SPEED);
   return Result;
}

internal void
DeallocEditorState(editor_state *EditorState)
{
   ListIter(Entity, EditorState->EntitiesHead, entity)
   {
      EntityDestroy(Entity);
      ReleaseChunk(EditorState->EntityPool, Entity);
   }
   EditorState->EntitiesHead = 0;
   EditorState->EntitiesTail = 0;
   EditorState->NumEntities = 0;
}

internal entity *
AllocAndAddEntity(editor_state *State)
{
   allocated_entity Alloc = AllocEntity();
   entity *Entity = Alloc.Entity;
   if (Entity)
   {
      // NOTE(hbr): Ugly C++ thing we have to do because of constructors/destructors.
      new (&Entity->Image.Texture) sf::Texture();
      Entity->Curve.ComputationArena = Alloc.EntityArena;
      DLLPushBack(State->EntitiesHead, State->EntitiesTail, Entity);
      ++State->NumEntities;
   }
   
   // TODO(hbr): Remove
#if 0
   entity *Entity = RequestChunkNonZero(State->EntityPool, entity);
#endif
   
   return Entity;
}

internal void
DeallocAndRemoveEntity(editor_state *State, entity *Entity)
{
   if (State->SplittingBezierCurve.IsSplitting &&
       State->SplittingBezierCurve.SplitCurveEntity == Entity)
   {
      State->SplittingBezierCurve.IsSplitting = false;
      State->SplittingBezierCurve.SplitCurveEntity = 0;
   }
   
   if (State->DeCasteljauVisualization.IsVisualizing &&
       State->DeCasteljauVisualization.CurveEntity == Entity)
   {
      State->DeCasteljauVisualization.IsVisualizing = false;
      State->DeCasteljauVisualization.CurveEntity = 0;
   }
   
   if (State->DegreeLowering.Entity == Entity)
   {
      State->DegreeLowering.Entity = 0;
   }
   
   if (State->CurveAnimation.FromCurveEntity == Entity ||
       State->CurveAnimation.ToCurveEntity == Entity)
   {
      State->CurveAnimation.Stage = AnimateCurveAnimation_None;
   }
   
   if (State->CurveCombining.CombineCurveEntity == Entity)
   {
      State->CurveCombining.IsCombining = false;
   }
   if (State->CurveCombining.TargetCurveEntity == Entity)
   {
      State->CurveCombining.TargetCurveEntity = 0;
   }
   
   EntityDestroy(Entity);
   DLLRemove(State->EntitiesHead, State->EntitiesTail, Entity);
   --State->NumEntities;
   ReleaseChunk(State->EntityPool, Entity);
}

internal void
DeselectCurrentEntity(editor_state *State)
{
   entity *Entity = State->SelectedEntity;
   if (Entity)
   {
      Entity->Flags &= ~EntityFlag_Selected;
   }
   State->SelectedEntity = 0;
}

internal void
SelectEntity(editor_state *State, entity *Entity)
{
   if (State->SelectedEntity)
   {
      DeselectCurrentEntity(State);
   }
   if (Entity)
   {
      Entity->Flags |= EntityFlag_Selected;
   }
   State->SelectedEntity = Entity;
}

internal notification
CreateNotification(string Title, color TitleColor, string Text, f32 PosY)
{
   notification Result = {};
   Result.Title = DuplicateStr(Title);
   Result.TitleColor = TitleColor;
   Result.Text = DuplicateStr(Text);
   Result.PosY = PosY;
   if (Result.Text.Count > 0)
   {
      Result.Text.Data[0] = ToUpper(Result.Text.Data[0]);
   }
   
   return Result;
}

internal void
DestroyNotification(notification *Notification)
{
   FreeStr(&Notification->Title);
   FreeStr(&Notification->Text);
}

internal void
AddNotificationF(notifications *Notifs, notification_type Type, char const *Fmt, ...)
{
   va_list Args;
   DeferBlock(va_start(Args, Fmt), va_end(Args))
   {
      temp_arena Temp = TempArena(0);
      
      if (Notifs->NotifCount < ArrayCount(Notifs->Notifs))
      {
         char const *TitleStr = 0;
         color Color = {};
         switch (Type)
         {
            case Notification_Success: {
               TitleStr = "Success";
               Color = GreenColor;
            } break;
            
            case Notification_Error: {
               TitleStr = "Error";
               Color = RedColor;
            } break;
            
            case Notification_Warning: {
               TitleStr = "Warning";
               Color = YellowColor;
            } break;
         }
         
         string Title = StrLitArena(Temp.Arena, TitleStr);
         
         // NOTE(hbr): Calculate desginated position
         sf::Vector2u WindowSize = Notifs->Window->getSize();
         f32 Padding = NotificationWindowPaddingScale * WindowSize.x;
         f32 PosY = WindowSize.y - Padding;
         for (u64 NotifIndex = 0; NotifIndex < Notifs->NotifCount; ++NotifIndex)
         {
            notification *Notif = Notifs->Notifs + NotifIndex;
            PosY -= Notif->NotifWindowHeight + Padding;
         }
         
         string Text = StrFV(Temp.Arena, Fmt, Args);
         
         Notifs->Notifs[Notifs->NotifCount] = CreateNotification(Title, Color, Text, PosY);;
         Notifs->NotifCount += 1;
      }
      
      EndTemp(Temp);
   }
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
      Editor->Window->setTitle(SaveProjectFileName.Data);
      
      EndTemp(Temp);
   }
   else
   {
      Editor->SaveProjectFormat = SaveProjectFormat_None;
      Editor->ProjectSavePath = {};
      Editor->Window->setTitle(WINDOW_TITLE);
   }
}

internal b32
ScreenPointsAreClose(screen_position A, screen_position B,
                     coordinate_system_data CoordinateSystemData)
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
                  coordinate_system_data CoordinateSystemData)
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
                  coordinate_system_data CoordinateSystemData)
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

internal b32
AreCurvesCompatibleForCombining(curve *CurveA, curve *CurveB)
{
   b32 Result = false;
   
   curve_shape ShapeA = CurveA->CurveParams.CurveShape;
   curve_shape ShapeB = CurveB->CurveParams.CurveShape;
   if (ShapeA.InterpolationType == ShapeB.InterpolationType)
   {
      switch (ShapeA.InterpolationType)
      {
         case Interpolation_Polynomial: { Result = true; } break;
         case Interpolation_CubicSpline: { Result = (ShapeA.CubicSplineType == ShapeB.CubicSplineType); } break;
         case Interpolation_Bezier: { Result = (ShapeA.BezierType == ShapeB.BezierType); } break;
      }
   }
   
   return Result;
}

internal editor_mode
CreateMovingEntityMode(entity *Entity)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_Entity;
   Result.Moving.Entity = Entity;
   
   return Result;
}

internal editor_mode
CreateMovingCameraMode(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_Camera;
   
   return Result;
}

internal editor_mode
CreateMovingCurvePointMode(entity *CurveEntity, u64 PointIndex,
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

// TODO(hbr): Merge [RotatingCurve] with [RotatingImage] internals (at least)
internal editor_mode
CreateRotatingCurveMode(entity *CurveEntity, screen_position RotationCenter)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Rotating;
   Result.Rotating.Entity = CurveEntity;
   Result.Rotating.RotationCenter = RotationCenter;
   
   return Result;
}

internal editor_mode
CreateRotatingImageMode(entity *ImageEntity)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Rotating;
   Result.Rotating.Entity = ImageEntity;
   
   return Result;
}

internal editor_mode
CreateRotatingCameraMode(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Rotating;
   
   return Result;
}

internal name_string
GenerateNewNameForCurve(editor_state *State)
{
   // NOTE(hbr): Simple approach, not bullet-proof but ppb good enough
   name_string Result = NameStrF("curve(%lu)", State->EverIncreasingCurveCounter++);
   return Result;
}

internal editor_state
EditorStateNormalModeUpdate(editor_state State, user_action Action,
                            coordinate_system_data CoordinateSystemData,
                            editor_params EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_ButtonClicked: {
         world_position ClickPosition = ScreenToWorldSpace(Action.Click.ClickPosition, CoordinateSystemData);
         switch (Action.Click.Button)
         {
            case Button_Left: {
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                                                    CoordinateSystemData);
               
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
                  Collision = CheckCollisionWith(State.EntitiesHead,
                                                 ClickPosition, CollisionTolerance,
                                                 EditorParams,  CheckCollisionWithFlags);
               }
               
               b32 SkipNormalModeHandling = false;
               if (State.SplittingBezierCurve.IsSplitting)
               {
                  entity *SplitEntity = State.SplittingBezierCurve.SplitCurveEntity;
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
                     Result.SplittingBezierCurve.T = Clamp(T, 0.0f, 1.0f); // NOTE(hbr): Be safe
                     SelectEntity(&Result, SplitEntity);
                     SkipNormalModeHandling = true;
                  }
               }
               
               if (State.CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget &&
                   Collision.Entity &&
                   Collision.Entity != State.CurveAnimation.FromCurveEntity)
               {
                  Result.CurveAnimation.ToCurveEntity = Collision.Entity;
                  SkipNormalModeHandling = true;
               }
               
               if (State.CurveCombining.IsCombining && Collision.Entity)
               {
                  // TODO(hbr): Isn't [Assert] needed here, that it is [Collision_Curve]????
                  curve *CollisionCurve = &Collision.Entity->Curve;
                  curve *CombineCurve = &State.CurveCombining.CombineCurveEntity->Curve;
                  curve *TargetCurve = &State.CurveCombining.TargetCurveEntity->Curve;
                  
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
                     u64 CollisionPointIndex = Collision.PointIndex;
                     if (CollisionCurve == CombineCurve)
                     {
                        if (CollisionPointIndex == 0)
                        {
                           if (!State.CurveCombining.CombineCurveLastControlPoint)
                           {
                              Swap(Result.CurveCombining.CombineCurveEntity,
                                   Result.CurveCombining.TargetCurveEntity,
                                   entity *);
                              Swap(Result.CurveCombining.CombineCurveLastControlPoint,
                                   Result.CurveCombining.TargetCurveFirstControlPoint,
                                   b32);
                              Result.CurveCombining.CombineCurveLastControlPoint =
                                 !Result.CurveCombining.CombineCurveLastControlPoint;
                              Result.CurveCombining.TargetCurveFirstControlPoint =
                                 !Result.CurveCombining.TargetCurveFirstControlPoint;
                           }
                           else
                           {
                              Result.CurveCombining.CombineCurveLastControlPoint = false;
                           }
                        }
                        else if (CollisionPointIndex == CombineCurve->ControlPointCount - 1)
                        {
                           if (State.CurveCombining.CombineCurveLastControlPoint)
                           {
                              Swap(Result.CurveCombining.CombineCurveEntity,
                                   Result.CurveCombining.TargetCurveEntity,
                                   entity *);
                              Swap(Result.CurveCombining.CombineCurveLastControlPoint,
                                   Result.CurveCombining.TargetCurveFirstControlPoint,
                                   b32);
                              Result.CurveCombining.CombineCurveLastControlPoint =
                                 !Result.CurveCombining.CombineCurveLastControlPoint;
                              Result.CurveCombining.TargetCurveFirstControlPoint =
                                 !Result.CurveCombining.TargetCurveFirstControlPoint;
                           }
                           else
                           {
                              Result.CurveCombining.CombineCurveLastControlPoint = true;
                           }
                        }
                     }
                     else if (CollisionCurve == TargetCurve)
                     {
                        if (CollisionPointIndex == 0)
                        {
                           Result.CurveCombining.TargetCurveFirstControlPoint = true;
                        }
                        else if (CollisionPointIndex == TargetCurve->ControlPointCount - 1)
                        {
                           Result.CurveCombining.TargetCurveFirstControlPoint = false;
                        }
                     }
                  }
                  
                  if (CollisionCurve != CombineCurve &&
                      AreCurvesCompatibleForCombining(CombineCurve, CollisionCurve))
                  {
                     Result.CurveCombining.TargetCurveEntity = Collision.Entity;
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
                           SelectEntity(&Result, Collision.Entity);
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
                           
                           u64 InsertPointIndex = CurveInsertControlPoint(Collision.Entity, ClickPosition,
                                                                          InsertAfterPointIndex,
                                                                          EditorParams.DefaultControlPointWeight);
                           SelectControlPoint(Collision.Entity, InsertPointIndex);
                           SelectEntity(&Result, Collision.Entity);
                        } break;
                     }
                  }
                  else
                  {
                     entity *AddCurveEntity = 0;
                     if (State.SelectedEntity &&
                         State.SelectedEntity->Type == Entity_Curve)
                     {
                        AddCurveEntity = State.SelectedEntity;
                     }
                     else
                     {
                        entity *Entity = AllocAndAddEntity(&Result);
                        world_position Position = V2F32(0.0f, 0.0f);
                        v2f32 Scale = V2F32(1.0f, 1.0f);
                        rotation_2d Rotation = Rotation2DZero();
                        name_string Name = GenerateNewNameForCurve(&Result);
                        InitCurveEntity(Entity,
                                        Position, Scale, Rotation,
                                        Name,
                                        EditorParams.CurveDefaultParams);
                        
                        AddCurveEntity = Entity;
                     }
                     Assert(AddCurveEntity);
                     
                     u64 AddedPointIndex = AppendCurveControlPoint(AddCurveEntity, ClickPosition, EditorParams.DefaultControlPointWeight);
                     SelectControlPoint(AddCurveEntity, AddedPointIndex);
                     SelectEntity(&Result, AddCurveEntity);
                  }
               }
            } break;
            
            case Button_Right: {
               check_collision_with_flags CheckCollisionWithFlags =
                  CheckCollisionWith_ControlPoints | CheckCollisionWith_Images;
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                                                    CoordinateSystemData);
               collision Collision = CheckCollisionWith(State.EntitiesHead,
                                                        ClickPosition, CollisionTolerance,
                                                        EditorParams, CheckCollisionWithFlags);
               
               if (Collision.Entity)
               {
                  switch (Collision.Entity->Type)
                  {
                     case Entity_Curve: {
                        switch (Collision.Type)
                        {
                           case CurveCollision_ControlPoint: {
                              RemoveCurveControlPoint(Collision.Entity, Collision.PointIndex);
                              SelectEntity(&Result, Collision.Entity);
                           } break;
                           
                           case CurveCollision_CubicBezierPoint: { NotImplemented; } break;
                           case CurveCollision_CurvePoint: InvalidPath;
                        }
                     } break;
                     
                     case Entity_Image: {
                        DeallocAndRemoveEntity(&Result, Collision.Entity);
                     } break;
                  }
               }
               else
               {
                  DeselectCurrentEntity(&Result);
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
                                                                   CoordinateSystemData);
         
         switch (DragButton)
         {
            case Button_Left: {
               f32 CollisionTolerance =
                  ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                              CoordinateSystemData);
               
               b32 SkipCheckingNormalModeCollisions = false;
               if (State.SplittingBezierCurve.IsSplitting)
               {
                  if (State.SplittingBezierCurve.DraggingSplitPoint)
                  {
                     SkipCheckingNormalModeCollisions = true;
                  }
                  else
                  {
                     entity *CurveEntity = State.SplittingBezierCurve.SplitCurveEntity;
                     world_position SplitPoint = State.SplittingBezierCurve.SplitPoint;
                     f32 SplitPointRadius =
                        ClipSpaceLengthToWorldSpace(EditorParams.BezierSplitPoint.RadiusClipSpace,
                                                    CoordinateSystemData);
                     
                     if (PointCollision(SplitPoint, DragFromWorldPosition, SplitPointRadius + CollisionTolerance))
                     {
                        Result.SplittingBezierCurve.DraggingSplitPoint = true;
                        SelectEntity(&Result, CurveEntity);
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
                  collision Collision = CheckCollisionWith(State.EntitiesHead,
                                                           DragFromWorldPosition,
                                                           CollisionTolerance,
                                                           EditorParams,
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
                                 
                                 Result.Mode = CreateMovingCurvePointMode(Collision.Entity, Collision.PointIndex,
                                                                          IsBezierPoint, State.MovingPointArena);
                              } break;
                              
                              case CurveCollision_CurvePoint: {
                                 Result.Mode = CreateMovingEntityMode(Collision.Entity);
                              } break;
                           }
                        } break;
                        
                        case Entity_Image: {
                           Result.Mode = CreateMovingEntityMode(Collision.Entity);
                        } break;
                     }
                     
                     SelectEntity(&Result, Collision.Entity);
                  }
                  else
                  {
                     Result.Mode = CreateMovingCameraMode(); 
                  }
               }
            } break;
            
            case Button_Right: {
               // TODO(hbr): Merge [Entity] cases here and maybe also [Camera] case.
               entity *Entity = State.SelectedEntity;
               if (Entity)
               {
                  switch (Entity->Type)
                  {
                     case Entity_Curve: { Result.Mode = CreateRotatingCurveMode(Entity, DragFromScreenPosition); } break;
                     case Entity_Image: { Result.Mode = CreateRotatingImageMode(Entity); } break;
                  }
               }
               else
               {
                  Result.Mode = CreateRotatingCameraMode();
               }
            } break;
            
            case Button_Middle: { Result.Mode = CreateRotatingCameraMode(); } break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Drag.Button)
         {
            case Button_Left: { Result.SplittingBezierCurve.DraggingSplitPoint = false; } break;
            case Button_Right:
            case Button_Middle: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_MouseMove: {
         splitting_bezier_curve *Split = &State.SplittingBezierCurve;
         
         // NOTE(hbr): Pretty involved logic to move point along the curve and have pleasant experience
         if (State.SplittingBezierCurve.DraggingSplitPoint)
         {
            world_position FromPosition = ScreenToWorldSpace(Action.MouseMove.FromPosition, CoordinateSystemData);
            world_position ToPosition = ScreenToWorldSpace(Action.MouseMove.ToPosition, CoordinateSystemData);
            
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
            
            Result.SplittingBezierCurve.T = T;
         }
      } break;
      
      case UserAction_None: {} break;
   }
   
   return Result;
}

internal editor_state
EditorStateMovingModeUpdate(editor_state State, user_action Action,
                            coordinate_system_data CoordinateSystemData)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         world_position From = ScreenToWorldSpace(Action.MouseMove.FromPosition, CoordinateSystemData);
         world_position To = ScreenToWorldSpace(Action.MouseMove.ToPosition, CoordinateSystemData);
         v2f32 Translation = To - From;
         
         auto *Moving = &State.Mode.Moving;
         if (Moving->Type == MovingMode_CurvePoint)
         {
            translate_control_point_flags TranslateFlags = 0;
            if (State.Mode.Moving.IsBezierPoint)       TranslateFlags |= TranslateControlPoint_BezierPoint;
            if (!Action.Input->Keys[Key_LeftCtrl].Pressed)  TranslateFlags |= TranslateControlPoint_MatchBezierTwinDirection;
            if (!Action.Input->Keys[Key_LeftShift].Pressed) TranslateFlags |= TranslateControlPoint_MatchBezierTwinDirection;
            
            TranslateCurveControlPoint(State.Mode.Moving.Entity,
                                       State.Mode.Moving.PointIndex,
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
                  CameraMove(&Result.Camera, -Translation);
               } break;
               
               case MovingMode_CurvePoint: InvalidPath;
            }
         }
         
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Release.Button)
         {
            case Button_Left:
            case Button_Middle: { Result.Mode = EditorModeNormal(); } break;
            case Button_Right: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag:
      case UserAction_None: {} break;
   }
   
   return Result;
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
                          coordinate_system_data CoordinateSystemData,
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

internal editor_state
EditorStateRotatingModeUpdate(editor_state State, user_action Action,
                              coordinate_system_data CoordinateSystemData,
                              editor_params EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         entity *Entity = State.Mode.Rotating.Entity;
         
         // TODO(hbr): Merge cases below
         camera_position RotationCenter = {};
         if (Entity)
         {
            switch (Entity->Type)
            {
               case Entity_Curve: {
                  RotationCenter = ScreenToCameraSpace(State.Mode.Rotating.RotationCenter,
                                                       CoordinateSystemData);
               } break;
               case Entity_Image: {
                  RotationCenter = WorldToCameraSpace(Entity->Position, CoordinateSystemData);
               } break;
            }
         }
         else
         {
            RotationCenter = WorldToCameraSpace(State.Camera.Position, CoordinateSystemData);
         }
         
         stable_rotation StableRotation =
            CalculateRotationIfStable(Action.MouseMove.FromPosition,
                                      Action.MouseMove.ToPosition,
                                      RotationCenter,
                                      CoordinateSystemData,
                                      EditorParams);
         
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
                     rotation_2d NewRotation = CombineRotations2D(Entity->Rotation,
                                                                  StableRotation.Rotation);
                     Entity->Rotation = NewRotation;
                  } break;
               }
            }
            else
            {
               rotation_2d InverseRotation = Rotation2DInverse(StableRotation.Rotation);
               CameraRotate(&Result.Camera, InverseRotation);
            }
         }
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Right: { Result.Mode = EditorModeNormal(); } break;
            case Button_Left:
            case Button_Middle: {} break;
            case Button_Count: InvalidPath;
         }
      } break;
      
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag:
      case UserAction_None: {} break;
   }
   
   return Result;
}

internal editor_state
EditorStateUpdate(editor_state State, user_action Action,
                  coordinate_system_data CoordinateSystemData,
                  editor_params EditorParams)
{
   editor_state Result = {};
   switch (State.Mode.Type)
   {
      case EditorMode_Normal:   { Result = EditorStateNormalModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
      case EditorMode_Moving:   { Result = EditorStateMovingModeUpdate(State, Action, CoordinateSystemData); } break;
      case EditorMode_Rotating: { Result = EditorStateRotatingModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
   }
   
   return Result;
}

internal void
RenderPoint(v2f32 Position,
            render_point_data PointData,
            coordinate_system_data CoordinateSystemData,
            sf::Transform Transform,
            sf::RenderWindow *RenderWindow)
{
   f32 TotalRadius = ClipSpaceLengthToWorldSpace(PointData.RadiusClipSpace,
                                                 CoordinateSystemData);
   f32 OutlineThickness = PointData.OutlineThicknessFraction * TotalRadius;
   f32 InsideRadius = TotalRadius - OutlineThickness;
   
   DrawCircle(Position, InsideRadius, PointData.FillColor,
              Transform, RenderWindow, OutlineThickness,
              PointData.OutlineColor);
}

internal void
RenderRotationIndicator(editor_state *EditorState, editor_params *EditorParams,
                        coordinate_system_data CoordinateSystemData,
                        sf::Transform Transform, sf::RenderWindow *Window)
{
   if (EditorState->Mode.Type == EditorMode_Rotating)
   {
      // TODO(hbr): Merge cases here.
      entity *Entity = EditorState->Mode.Rotating.Entity;
      world_position RotationIndicatorPosition = {};
      if (Entity)
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               RotationIndicatorPosition = ScreenToWorldSpace(EditorState->Mode.Rotating.RotationCenter,
                                                              CoordinateSystemData);
            } break;
            
            case Entity_Image: {
               RotationIndicatorPosition = Entity->Position;
            } break;
         }
      }
      else
      {
         RotationIndicatorPosition = EditorState->Camera.Position;
      }
      
      RenderPoint(RotationIndicatorPosition,
                  EditorParams->RotationIndicator,
                  CoordinateSystemData, Transform,
                  Window);
   }
}

#define SAVED_PROJECT_MAGIC_VALUE 0xdeadc0dedeadc0deull
struct saved_project_header
{
   u64 MagicValue;
   
   u64 NumEntities;
   u64 CurveCounter;
   
   camera Camera;
   
   editor_params EditorParameters;
   
   ui_config UI_Config;
   
   // TODO(hbr): Should it really belong here?
   f32 CurveAnimationSpeed;
};

struct saved_project_curve
{
   curve_params CurveParams;
   u64 ControlPointCount;
   u64 SelectedControlPointIndex;
};

struct saved_project_image
{
   u64 ImagePathSize;
};

struct saved_project_entity
{
   entity_type Type;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   name_string Name;
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   union {
      saved_project_curve Curve;
      saved_project_image Image;
   };
};

internal saved_project_header
SavedProjectHeader(u64 MagicValue, u64 NumEntities,
                   u64 CurveCounter, camera Camera,
                   editor_params EditorParameters,
                   ui_config UI_Config, f32 CurveAnimationSpeed)
{
   saved_project_header Result = {};
   Result.MagicValue = MagicValue;
   Result.NumEntities = NumEntities;
   Result.CurveCounter = CurveCounter;
   Result.Camera = Camera;
   Result.EditorParameters = EditorParameters;
   Result.UI_Config = UI_Config;
   Result.CurveAnimationSpeed = CurveAnimationSpeed;
   
   return Result;
}

internal saved_project_curve
SavedProjectCurve(curve_params CurveParams,
                  u64 ControlPointCount,
                  u64 SelectedControlPointIndex)
{
   saved_project_curve Result = {};
   Result.CurveParams = CurveParams;
   Result.ControlPointCount = ControlPointCount;
   Result.SelectedControlPointIndex = SelectedControlPointIndex;
   
   return Result;
}

internal saved_project_image
SavedProjectImage(u64 ImagePathSize)
{
   saved_project_image Result = {};
   Result.ImagePathSize = ImagePathSize;
   
   return Result;
}

internal saved_project_entity
SavedProjectEntity(entity *Entity)
{
   saved_project_entity Result = {};
   Result.Type = Entity->Type;
   Result.Position;
   Result.Scale = Entity->Scale;
   Result.Rotation = Entity->Rotation;
   Result.Name = Entity->Name;
   Result.SortingLayer = Entity->SortingLayer;
   Result.Flags = Entity->Flags;
   
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         Result.Curve = SavedProjectCurve(Curve->CurveParams,
                                          Curve->ControlPointCount,
                                          Curve->SelectedControlPointIndex);
      } break;
      
      case Entity_Image: {
         Result.Image = SavedProjectImage(Entity->Image.FilePath.Count);
      } break;
   }
   
   return Result;
}

// TODO(hbr): Take more things by pointer in general
internal error_string
SaveProjectInFile(arena *Arena, editor Editor, string SaveFilePath)
{
   temp_arena Temp = TempArena(Arena);
   
   string_list SaveData = {};
   
   saved_project_header Header = SavedProjectHeader(SAVED_PROJECT_MAGIC_VALUE,
                                                    Editor.State.NumEntities,
                                                    Editor.State.EverIncreasingCurveCounter,
                                                    Editor.State.Camera,
                                                    Editor.Parameters,
                                                    Editor.UI_Config,
                                                    Editor.State.CurveAnimation.AnimationSpeed);
   string HeaderData = Str(Temp.Arena, Cast(char const *)&Header, SizeOf(Header));
   StrListPush(Temp.Arena, &SaveData, HeaderData);
   
   ListIter(Entity, Editor.State.EntitiesHead, entity)
   {
      saved_project_entity Saved = SavedProjectEntity(Entity);
      string SavedData = Str(Temp.Arena, Cast(char const *)&Saved, SizeOf(Saved));
      StrListPush(Temp.Arena, &SaveData, SavedData);
      
      switch (Entity->Type)
      {
         case Entity_Curve: {
            curve *Curve = &Entity->Curve;
            
            u64 ControlPointCount = Curve->ControlPointCount;
            
            string ControlPoints = Str(Temp.Arena,
                                       Cast(char const *)Curve->ControlPoints,
                                       ControlPointCount * SizeOf(Curve->ControlPoints[0]));
            StrListPush(Temp.Arena, &SaveData, ControlPoints);
            
            string ControlPointWeights = Str(Temp.Arena,
                                             Cast(char const *)Curve->ControlPointWeights,
                                             ControlPointCount * SizeOf(Curve->ControlPointWeights[0]));
            StrListPush(Temp.Arena, &SaveData, ControlPointWeights);
            
            string CubicBezierPoints = Str(Temp.Arena,
                                           Cast(char const *)Curve->CubicBezierPoints,
                                           3 * ControlPointCount * SizeOf(Curve->CubicBezierPoints[0]));
            StrListPush(Temp.Arena, &SaveData, CubicBezierPoints);
         } break;
         
         case Entity_Image: {
            StrListPush(Temp.Arena, &SaveData, Entity->Image.FilePath);
         } break;
      }
   }
   
   b32 Success = OS_WriteDataListToFile(SaveFilePath, SaveData);
   error_string Error = {};
   // TODO(hbr): return boolean instead of error string
   if (!Success)
   {
      Error = StrF(Arena, "save failed");
   }
   
   EndTemp(Temp);
   
   return Error;
}

internal void *
ExpectSizeInData(u64 Expected, void **Data, u64 *BytesLeft)
{
   void *Result = 0;
   if (*BytesLeft >= Expected)
   {
      Result = *Data;
      *Data = Cast(u8 *)*Data + Expected;
      *BytesLeft -= Expected;
   }
   
   return Result;
}
#define ExpectTypeInData(Type, Data, BytesLeft) Cast(Type *)ExpectSizeInData(SizeOf(Type), Data, BytesLeft)
#define ExpectArrayInData(Count, Type, Data, BytesLeft) Cast(Type *)ExpectSizeInData((Count) * SizeOf(Type), Data, BytesLeft)

// TODO(hbr): Remove this type as well, just save [editor].
struct load_project_result
{
   editor_state EditorState;
   editor_params EditorParameters;
   ui_config UI_Config;
   
   error_string Error;
   string_list Warnings;
};
// TODO(hbr): Error doesn't have to be so specific, maybe just "project file is corrupted or something like that"
internal load_project_result
LoadProjectFromFile(arena *Arena,
                    string ProjectFilePath,
                    pool *EntityPool,
                    arena *DeCasteljauVisualizationArena,
                    arena *DegreeLoweringArena, arena *MovingPointArena,
                    arena *CurveAnimationArena)
{
   // TODO(hbr): Shorten error messages, maybe even don't return
   load_project_result Result = {};
   error_string Error = {};
   string_list Warnings = {};
   
   editor_state *EditorState = &Result.EditorState;
   editor_params *EditorParameters = &Result.EditorParameters;
   ui_config *UI_Config = &Result.UI_Config;
   
   temp_arena Temp = TempArena(Arena);
   
   error_string ReadError = {};
   string ProjectData = OS_ReadEntireFile(Temp.Arena, ProjectFilePath);
   if (ProjectData.Data)
   {
      void *Data = ProjectData.Data;
      u64 BytesLeft = ProjectData.Count;
      b32 Corrupted = false;
      
      saved_project_header *Header = ExpectTypeInData(saved_project_header, &Data, &BytesLeft);
      if (Header && Header->MagicValue)
      {
         *EditorParameters = Header->EditorParameters;
         *UI_Config = Header->UI_Config;
         *EditorState = CreateEditorState(EntityPool,
                                          Header->CurveCounter,
                                          Header->Camera,
                                          DeCasteljauVisualizationArena,
                                          DegreeLoweringArena,
                                          MovingPointArena,
                                          CurveAnimationArena,
                                          Header->CurveAnimationSpeed);
         
         u64 NumEntities = Header->NumEntities;
         for (u64 EntityIndex = 0;
              EntityIndex < NumEntities;
              ++EntityIndex)
         {
            saved_project_entity *SavedEntity = ExpectTypeInData(saved_project_entity, &Data, &BytesLeft);
            if (SavedEntity)
            {
               entity *Entity = AllocAndAddEntity(EditorState);
               InitEntity(Entity,
                          SavedEntity->Position, SavedEntity->Scale, SavedEntity->Rotation,
                          SavedEntity->Name,
                          SavedEntity->SortingLayer,
                          SavedEntity->RenamingFrame,
                          SavedEntity->Flags,
                          SavedEntity->Type);
               
               switch (SavedEntity->Type)
               {
                  case Entity_Curve: {
                     saved_project_curve *SavedCurve = &SavedEntity->Curve;
                     u64 ControlPointCount = SavedCurve->ControlPointCount;
                     local_position *ControlPoints = ExpectArrayInData(ControlPointCount, local_position, &Data, &BytesLeft);
                     f32 *ControlPointWeights = ExpectArrayInData(ControlPointCount, f32, &Data, &BytesLeft);
                     local_position *CubicBezierPoints = ExpectArrayInData(3 * ControlPointCount, local_position, &Data, &BytesLeft);
                     
                     if (ControlPoints && ControlPointWeights && CubicBezierPoints)
                     {
                        InitCurve(&Entity->Curve,
                                  SavedCurve->CurveParams, SavedCurve->SelectedControlPointIndex,
                                  ControlPointCount, ControlPoints, ControlPointWeights, CubicBezierPoints);
                     }
                     else
                     {
                        Corrupted = true;
                     }
                  } break;
                  
                  case Entity_Image: {
                     saved_project_image *SavedImage = &SavedEntity->Image;
                     image *Image = &Entity->Image;
                     
                     char *ImagePathStr = ExpectArrayInData(SavedImage->ImagePathSize, char, &Data, &BytesLeft);
                     if (ImagePathStr)
                     {
                        string ImagePath = Str(Temp.Arena, ImagePathStr, SavedImage->ImagePathSize);
                        
                        string LoadTextureError = {};
                        sf::Texture LoadedTexture = LoadTextureFromFile(Temp.Arena, ImagePath, &LoadTextureError);
                        
                        if (!IsError(LoadTextureError))
                        {
                           InitImage(Image, ImagePath, &LoadedTexture);
                        }
                        else
                        {
                           string Warning = StrF(Arena, "image texture load failed: %s", LoadTextureError);
                           StrListPush(Arena, &Warnings, Warning);
                        }
                     }
                     else
                     {
                        Corrupted = true;
                     }
                  } break;
               }
               
               if (SavedEntity->Flags & EntityFlag_Selected)
               {
                  SelectEntity(EditorState, Entity);
               }
            }
            else
            {
               Corrupted = true;
            }
            
            if (Corrupted || IsError(Error))
            {
               break;
            }
         }
      }
      else
      {
         Corrupted = true;
      }
      
      if (Corrupted)
      {
         Error = StrC(Arena, "project file corrupted");
      }
      
      if (!IsError(Error))
      {
         if (BytesLeft > 0)
         {
            string Warning = StrC(Arena, "project file has unexpected trailing data");
            StrListPush(Arena, &Warnings, Warning);
         }
      }
   }
   else
   {
      Error = StrF(Arena, "failed to read project file");
   }
   
   if (IsError(Error))
   {
      DeallocEditorState(EditorState);
   }
   
   Result.Error = Error;
   Result.Warnings = Warnings;
   
   EndTemp(Temp);
   
   return Result;
}

internal error_string
SaveProjectInFormat(arena *Arena,
                    save_project_format SaveProjectFormat,
                    string SaveProjectFilePath,
                    editor *Editor)
{
   error_string Error = {};
   
   switch (SaveProjectFormat)
   {
      case SaveProjectFormat_ProjectFile: {
         Error = SaveProjectInFile(Arena, *Editor, SaveProjectFilePath);
      } break;
      
      case SaveProjectFormat_ImageFile: {
         sf::RenderWindow *Window = Editor->Window;
         sf::Image Screenshot = Window->capture();
         
         bool SaveSuccess = Screenshot.saveToFile(SaveProjectFilePath.Data);
         if (!SaveSuccess)
         {
            Error = StrF(Arena,
                         "failed to save project into file %s as an image",
                         SaveProjectFilePath);
         }
      } break;
      
      case SaveProjectFormat_None: InvalidPath;
   }
   
   return Error;
}

internal name_string
CreateNameForCopiedEntity(name_string OriginalName)
{
   name_string Result = NameStrF("%s (copy)", OriginalName.Data);
   return Result;
}

internal void
DuplicateEntity(entity *Entity, editor *Editor)
{
   entity *Copy = AllocAndAddEntity(&Editor->State);
   InitEntityFromEntity(Copy, Entity);
   Copy->Name = CreateNameForCopiedEntity(Entity->Name);
   SelectEntity(&Editor->State, Copy);
   
   coordinate_system_data CoordinateSystemData {
      .Window = Editor->Window,
      .Camera = Editor->State.Camera,
      .Projection = Editor->Projection
   };
   f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, CoordinateSystemData);
   v2f32 SlightTranslation = V2F32(SlightTranslationX, 0.0f);
   Copy->Position += SlightTranslation;
}

template<typename enum_type>
internal enum_type
EnumComboUI(char const *Label,
            enum_type EnumTypes[],
            char const *EnumNames[],
            s32 NumEnums,
            enum_type CurrentEnum,
            bool *SomeParameterChanged)
{
   s32 CurrentIndex = 0;
   bool FoundEnum = false;
   for (; CurrentIndex < NumEnums;
        ++CurrentIndex)
   {
      if (EnumTypes[CurrentIndex] == CurrentEnum)
      {
         FoundEnum = true;
         break;
      }
   }
   Assert(FoundEnum);
   
   if (SomeParameterChanged)
      *SomeParameterChanged |= ImGui::Combo(Label, &CurrentIndex, EnumNames, NumEnums);
   
   Assert(CurrentIndex < NumEnums);
   enum_type Result = EnumTypes[CurrentIndex];
   
   return Result;
}

// NOTE(hbr): This should really be some highly local internal, don't expect to reuse.
// It's highly specific.
internal bool
ResetContextMenuItemSelected(char const *PopupLabel)
{
   bool ResetSelected = false;
   
   if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
   {
      ImGui::OpenPopup(PopupLabel);
   }
   if (ImGui::BeginPopup(PopupLabel, ImGuiWindowFlags_NoMove))
   {
      ImGui::MenuItem("Reset", 0, &ResetSelected);
      ImGui::EndPopup();
   }
   
   return ResetSelected;
}

// NOTE(hbr): This should really be some highly local internal, don't expect to reuse.
// It's highly specific.
struct drag_speed_and_format
{
   f32 DragSpeed;
   char const *DragFormat;
};
internal drag_speed_and_format
CalculateDragSpeedAndFormatFromDefaultValue(f32 DefaultValue)
{
   f32 DragSpeed = DefaultValue / 20;
   char const *DragFormat = 0;
   if (DragSpeed >= 1.0f)
   {
      DragFormat = "%.0f";
   }
   else if (DragSpeed >= 0.1f)
   {
      DragFormat = "%.1f";
   }
   else if (DragSpeed >= 0.01f)
   {
      DragFormat = "%.2f";
   }
   else if (DragSpeed >= 0.001f)
   {
      DragFormat = "%.3f";
   }
   else if (DragSpeed >= 0.0001f)
   {
      DragFormat = "%.4f";
   }
   else
   {
      DragFormat = "%f";
   }
   
   drag_speed_and_format Result = {};
   Result.DragSpeed = DragSpeed;
   Result.DragFormat = DragFormat;
   
   return Result;
}

enum change_curve_params_ui_mode_type
{
   ChangeCurveParamsUIMode_DefaultCurve,
   CurveShapeUIMode_SelectedCurve,
};
struct change_curve_params_ui_mode
{
   change_curve_params_ui_mode_type Type;
   
   curve_params *DefaultCurveParams;
   f32 *DefaultControlPointWeight;
   
   curve *SelectedCurve;
};
internal change_curve_params_ui_mode
ChangeCurveShapeUIModeDefaultCurve(curve_params *DefaultCurveParams,
                                   f32 *DefaultControlPointWeight)
{
   change_curve_params_ui_mode Result = {};
   Result.Type = ChangeCurveParamsUIMode_DefaultCurve;
   Result.DefaultCurveParams = DefaultCurveParams;
   Result.DefaultControlPointWeight = DefaultControlPointWeight;
   
   return Result;
}
internal change_curve_params_ui_mode
ChangeCurveShapeUIModeSelectedCurve(curve_params *DefaultCurveParams,
                                    f32 *DefaultControlPointWeight,
                                    curve *SelectedCurve)
{
   change_curve_params_ui_mode Result = {};
   Result.Type = CurveShapeUIMode_SelectedCurve;
   Result.DefaultCurveParams = DefaultCurveParams;
   Result.DefaultControlPointWeight = DefaultControlPointWeight;
   Result.SelectedCurve = SelectedCurve;
   
   return Result;
}

internal bool
RenderChangeCurveParametersUI(char const *PushID,
                              change_curve_params_ui_mode UIMode)
{
   bool SomeCurveParameterChanged = false;
   
   curve_params DefaultParams = {};
   curve_params *ChangeParams = 0;
   switch (UIMode.Type)
   {
      case ChangeCurveParamsUIMode_DefaultCurve: {
         DefaultParams = CURVE_DEFAULT_CURVE_PARAMS;
         ChangeParams = UIMode.DefaultCurveParams;
      } break;
      
      case CurveShapeUIMode_SelectedCurve: {
         DefaultParams = *UIMode.DefaultCurveParams;
         ChangeParams = &UIMode.SelectedCurve->CurveParams;
      } break;
   }
   
   DeferBlock(ImGui::PushID(PushID), ImGui::PopID())
   {   
      ImGui::SeparatorText("Curve");
      DeferBlock(ImGui::PushID("Curve"), ImGui::PopID())
      {
         curve_shape DefaultCurveShape = DefaultParams.CurveShape;
         curve_shape *CurveShape = &ChangeParams->CurveShape;
         
         {
            local interpolation_type InterpolationTypes[] = {
               Interpolation_CubicSpline,
               Interpolation_Polynomial,
               Interpolation_Bezier,
            };
            local char const *InterpolationTypeNames[] = {
               "Cubic Spline",
               "Polynomial",
               "Bezier",
            };
            StaticAssert(ArrayCount(InterpolationTypes) ==
                         ArrayCount(InterpolationTypeNames),
                         CheckInterpolationTypeArrayLengths);
            
            CurveShape->InterpolationType =
               EnumComboUI("Interpolation Type",
                           InterpolationTypes,
                           InterpolationTypeNames,
                           ArrayCount(InterpolationTypes),
                           CurveShape->InterpolationType,
                           &SomeCurveParameterChanged);
            
            if (ResetContextMenuItemSelected("InterpolationTypeReset"))
            {
               CurveShape->InterpolationType = DefaultCurveShape.InterpolationType;
               SomeCurveParameterChanged = true;
            }
         }
         
         switch (CurveShape->InterpolationType)
         {
            case Interpolation_Polynomial: {
               polynomial_interpolation_params *PolynomialInterpolationParams =
                  &CurveShape->PolynomialInterpolationParams;
               
               local polynomial_interpolation_type PolynomialInterpolationTypes[] = {
                  PolynomialInterpolation_Barycentric,
                  PolynomialInterpolation_Newton
               };
               local char const *PolynomialInterpolationNames[] = {
                  "Barycentric",
                  "Newton"
               };
               StaticAssert(ArrayCount(PolynomialInterpolationTypes) ==
                            ArrayCount(PolynomialInterpolationNames),
                            CheckPolynomialInterpolationTypeArrayLengths);
               
               PolynomialInterpolationParams->Type =
                  EnumComboUI("Polynomial Type",
                              PolynomialInterpolationTypes,
                              PolynomialInterpolationNames,
                              ArrayCount(PolynomialInterpolationTypes),
                              PolynomialInterpolationParams->Type,
                              &SomeCurveParameterChanged);
               
               if (ResetContextMenuItemSelected("PolynomialTypeReset"))
               {
                  PolynomialInterpolationParams->Type =
                     DefaultCurveShape.PolynomialInterpolationParams.Type;
                  SomeCurveParameterChanged = true;
               }
               
               local points_arrangement PointsArrangementTypes[] = {
                  PointsArrangement_Chebychev,
                  PointsArrangement_Equidistant
               };
               local char const *PointsArrangementNames[] = {
                  "Chebychev",
                  "Equidistant"
               };
               StaticAssert(ArrayCount(PointsArrangementTypes) ==
                            ArrayCount(PointsArrangementNames),
                            CheckPointsArrangementTypeArrayLengths);
               
               PolynomialInterpolationParams->PointsArrangement =
                  EnumComboUI("Points Arrangement",
                              PointsArrangementTypes,
                              PointsArrangementNames,
                              ArrayCount(PointsArrangementTypes),
                              PolynomialInterpolationParams->PointsArrangement,
                              &SomeCurveParameterChanged);
               
               if (ResetContextMenuItemSelected("PointsArrangementReset"))
               {
                  PolynomialInterpolationParams->PointsArrangement =
                     DefaultCurveShape.PolynomialInterpolationParams.PointsArrangement;
                  SomeCurveParameterChanged = true;
               }
            } break;
            
            case Interpolation_CubicSpline: {
               local cubic_spline_type CubicSplineTypes[] = {
                  CubicSpline_Natural,
                  CubicSpline_Periodic
               };
               local char const *CubicSplineNames[] = {
                  "Natural",
                  "Periodic"
               };
               StaticAssert(ArrayCount(CubicSplineTypes) ==
                            ArrayCount(CubicSplineNames),
                            CheckCubicSplineTypeArrayLengths);
               
               CurveShape->CubicSplineType =
                  EnumComboUI("Spline Types",
                              CubicSplineTypes,
                              CubicSplineNames,
                              ArrayCount(CubicSplineTypes),
                              CurveShape->CubicSplineType,
                              &SomeCurveParameterChanged);
               
               if (ResetContextMenuItemSelected("SplineTypeReset"))
               {
                  CurveShape->CubicSplineType = DefaultCurveShape.CubicSplineType;
                  SomeCurveParameterChanged = true;
               }
            } break;
            
            case Interpolation_Bezier: {
               local bezier_type BezierTypes[] = {
                  Bezier_Normal,
                  Bezier_Weighted,
                  Bezier_Cubic,
               };
               local char const *BezierNames[] = {
                  "Normal",
                  "Weighted",
                  "Cubic",
               };
               StaticAssert(ArrayCount(BezierTypes) ==
                            ArrayCount(BezierNames),
                            CheckBezierTypeArrayLenghts);
               
               CurveShape->BezierType =
                  EnumComboUI("Bezier Types",
                              BezierTypes,
                              BezierNames,
                              ArrayCount(BezierTypes),
                              CurveShape->BezierType,
                              &SomeCurveParameterChanged);
               
               if (ResetContextMenuItemSelected("BezierTypeReset"))
               {
                  CurveShape->BezierType = DefaultCurveShape.BezierType;
                  SomeCurveParameterChanged = true;
               }
            } break;
         }
      }
      
      ImGui::NewLine();
      ImGui::SeparatorText("Line");
      DeferBlock(ImGui::PushID("Line"), ImGui::PopID())
      {
         {
            color *CurveColor = &ChangeParams->CurveColor;
            
            SomeCurveParameterChanged |= ImGui::ColorEdit4("Color", CurveColor);
            
            if (ResetContextMenuItemSelected("CurveColorReset"))
            {
               *CurveColor = DefaultParams.CurveColor;
               SomeCurveParameterChanged = true;
            }
         }
         {
            f32 *CurveWidth = &ChangeParams->CurveWidth;
            drag_speed_and_format DragParams =
               CalculateDragSpeedAndFormatFromDefaultValue(DefaultParams.CurveWidth);
            
            SomeCurveParameterChanged |= ImGui::DragFloat("Width",
                                                          CurveWidth,
                                                          DragParams.DragSpeed,
                                                          0.0f, FLT_MAX,
                                                          DragParams.DragFormat,
                                                          ImGuiSliderFlags_NoRoundToFormat);
            
            if (ResetContextMenuItemSelected("CurveWidthReset"))
            {
               *CurveWidth = DefaultParams.CurveWidth;
               SomeCurveParameterChanged = true;
            }
         }
         {
            u64 *CurvePointCountPerSegment = &ChangeParams->CurvePointCountPerSegment;
            
            int CurvePointCountPerSegmentAsInt = Cast(int)(*CurvePointCountPerSegment);
            SomeCurveParameterChanged |= ImGui::SliderInt("Detail",
                                                          &CurvePointCountPerSegmentAsInt,
                                                          1,    // v_min
                                                          2000, // v_max
                                                          "%d", // v_format
                                                          ImGuiSliderFlags_Logarithmic);
            *CurvePointCountPerSegment = Cast(u64)CurvePointCountPerSegmentAsInt;
            
            if (ResetContextMenuItemSelected("DetailReset"))
            {
               *CurvePointCountPerSegment = DefaultParams.CurvePointCountPerSegment;
               SomeCurveParameterChanged = true;
            }
         }
      }
      
      
      ImGui::NewLine();
      ImGui::SeparatorText("Control Points");
      DeferBlock(ImGui::PushID("ControlPoints"), ImGui::PopID())
      {
         {
            b32 *PointsDisabled = &ChangeParams->PointsDisabled;
            
            bool PointsDisabledAsBool = *PointsDisabled;
            ImGui::Checkbox("Points Disabled", &PointsDisabledAsBool);
            *PointsDisabled = PointsDisabledAsBool;
            
            if (ResetContextMenuItemSelected("PointsDisabledReset"))
            {
               *PointsDisabled = DefaultParams.PointsDisabled;
            }
         }
         {
            color *PointColor = &ChangeParams->PointColor;
            
            SomeCurveParameterChanged |= ImGui::ColorEdit4("Color", PointColor);
            
            if (ResetContextMenuItemSelected("PointColorReset"))
            {
               *PointColor = DefaultParams.PointColor;
               SomeCurveParameterChanged = true;
            }
         }
         {
            f32 *PointSize = &ChangeParams->PointSize;
            drag_speed_and_format DragParams =
               CalculateDragSpeedAndFormatFromDefaultValue(DefaultParams.PointSize);
            
            SomeCurveParameterChanged |= ImGui::DragFloat("Size",
                                                          PointSize,
                                                          DragParams.DragSpeed,
                                                          0.0f, FLT_MAX,
                                                          DragParams.DragFormat,
                                                          ImGuiSliderFlags_NoRoundToFormat);
            
            if (ResetContextMenuItemSelected("PointSizeReset"))
            {
               *PointSize = DefaultParams.PointSize;
               SomeCurveParameterChanged = true;
            }
         }
         {
            local f32 ControlPointWeightDragSpeed = 0.005f;
            local f32 ControlPointMinWeight = EPS_F32;
            local f32 ControlPointMaxWeight = FLT_MAX;
            local char const *ControlPointWeightFormat = "%.2f";
            
            switch (UIMode.Type)
            {
               case ChangeCurveParamsUIMode_DefaultCurve: {
                  ImGui::DragFloat("Point Weight",
                                   UIMode.DefaultControlPointWeight,
                                   ControlPointWeightDragSpeed,
                                   ControlPointMinWeight,
                                   ControlPointMaxWeight,
                                   ControlPointWeightFormat);
                  
                  if (ResetContextMenuItemSelected("DefaultControlPointWeightReset"))
                  {
                     *UIMode.DefaultControlPointWeight = CURVE_DEFAULT_CONTROL_POINT_WEIGHT;
                  }
               } break;
               
               case CurveShapeUIMode_SelectedCurve: {
                  if (ChangeParams->CurveShape.InterpolationType == Interpolation_Bezier &&
                      ChangeParams->CurveShape.BezierType == Bezier_Weighted)
                  {
                     ImGui::SeparatorText("Weights");
                     
                     curve *SelectedCurve = UIMode.SelectedCurve;
                     f32 DefaultControlPointWeight = *UIMode.DefaultControlPointWeight;
                     
                     u64 ControlPointCount = SelectedCurve->ControlPointCount;
                     f32 *ControlPointWeights = SelectedCurve->ControlPointWeights;
                     u64 SelectedControlPointIndex = SelectedCurve->SelectedControlPointIndex;
                     
                     if (SelectedControlPointIndex < ControlPointCount)
                     {
                        char ControlPointWeightLabel[128];
                        Fmt(ArrayCount(ControlPointWeightLabel), ControlPointWeightLabel,
                            "Selected Point (%llu)", SelectedControlPointIndex);
                        
                        f32 *SelectedControlPointWeight = &ControlPointWeights[SelectedControlPointIndex];
                        
                        SomeCurveParameterChanged |=
                           ImGui::DragFloat(ControlPointWeightLabel,
                                            SelectedControlPointWeight,
                                            ControlPointWeightDragSpeed,
                                            ControlPointMinWeight,
                                            ControlPointMaxWeight,
                                            ControlPointWeightFormat);
                        
                        if (ResetContextMenuItemSelected("SelectedPointWeightReset"))
                        {
                           *SelectedControlPointWeight = DefaultControlPointWeight;
                        }
                        
                        ImGui::Spacing();
                     }
                     
                     if (ImGui::TreeNode("All Points"))
                     {
                        for (u64 ControlPointIndex = 0;
                             ControlPointIndex < ControlPointCount;
                             ++ControlPointIndex)
                        {
                           DeferBlock(ImGui::PushID(Cast(int)ControlPointIndex), ImGui::PopID())
                           {                              
                              char ControlPointWeightLabel[128];
                              Fmt(ArrayCount(ControlPointWeightLabel), ControlPointWeightLabel,
                                  "Point (%llu)", ControlPointIndex);
                              
                              f32 *ControlPointWeight = &ControlPointWeights[ControlPointIndex];
                              
                              SomeCurveParameterChanged |=
                                 ImGui::DragFloat(ControlPointWeightLabel,
                                                  ControlPointWeight,
                                                  ControlPointWeightDragSpeed,
                                                  ControlPointMinWeight,
                                                  ControlPointMaxWeight,
                                                  ControlPointWeightFormat);
                              
                              if (ResetContextMenuItemSelected("PointWeightReset"))
                              {
                                 *ControlPointWeight = DefaultControlPointWeight;
                              }
                           }
                        }
                        
                        ImGui::TreePop();
                     }
                  }
               } break;
            }
         }
         if (ChangeParams->CurveShape.InterpolationType == Interpolation_Bezier &&
             ChangeParams->CurveShape.BezierType == Bezier_Cubic)
         {
            b32 *CubicBezierHelpersDisabled = &ChangeParams->CubicBezierHelpersDisabled;
            
            bool CubicBezierHelpersDisabledAsBool = *CubicBezierHelpersDisabled;
            ImGui::Checkbox("Helpers Disabled", &CubicBezierHelpersDisabledAsBool);
            *CubicBezierHelpersDisabled = CubicBezierHelpersDisabledAsBool;
            
            if (ResetContextMenuItemSelected("HelpersDisabledReset"))
            {
               *CubicBezierHelpersDisabled = DefaultParams.CubicBezierHelpersDisabled;
            }
         }
      }
      
      ImGui::NewLine();
      ImGui::SeparatorText("Polyline");
      DeferBlock(ImGui::PushID("Polyline"), ImGui::PopID())
      {
         {
            b32 *PolylineEnabled = &ChangeParams->PolylineEnabled;
            
            bool PolylineEnabledAsBool = *PolylineEnabled;
            ImGui::Checkbox("Enabled", &PolylineEnabledAsBool);
            *PolylineEnabled = PolylineEnabledAsBool;
            
            if (ResetContextMenuItemSelected("PolylineEnabled"))
            {
               *PolylineEnabled = DefaultParams.PolylineEnabled;
            }
         }
         {
            color *PolylineColor = &ChangeParams->PolylineColor;
            
            SomeCurveParameterChanged |= ImGui::ColorEdit4("Color", PolylineColor);
            
            if (ResetContextMenuItemSelected("PolylineColorReset"))
            {
               *PolylineColor = DefaultParams.PolylineColor;
               SomeCurveParameterChanged = true;
            }
         }
         {
            f32 *PolylineWidth = &ChangeParams->PolylineWidth;
            drag_speed_and_format DragParams =
               CalculateDragSpeedAndFormatFromDefaultValue(DefaultParams.PolylineWidth);
            
            SomeCurveParameterChanged |= ImGui::DragFloat("Width",
                                                          PolylineWidth,
                                                          DragParams.DragSpeed,
                                                          0.0f, FLT_MAX,
                                                          DragParams.DragFormat,
                                                          ImGuiSliderFlags_NoRoundToFormat);
            
            if (ResetContextMenuItemSelected("PolylineWidthReset"))
            {
               *PolylineWidth = DefaultParams.PolylineWidth;
               SomeCurveParameterChanged = true;
            }
         }
      }
      
      ImGui::NewLine();
      ImGui::SeparatorText("Convex Hull");
      DeferBlock(ImGui::PushID("ConvexHull"), ImGui::PopID())
      {
         {
            b32 *ConvexHullEnabled = &ChangeParams->ConvexHullEnabled;
            
            bool ConvexHullEnabledAsBool = *ConvexHullEnabled;
            ImGui::Checkbox("Enabled", &ConvexHullEnabledAsBool);
            *ConvexHullEnabled = ConvexHullEnabledAsBool;
            
            if (ResetContextMenuItemSelected("ConvexHullEnabledReset"))
            {
               *ConvexHullEnabled = DefaultParams.ConvexHullEnabled;
            }
         }
         {
            color *ConvexHullColor = &ChangeParams->ConvexHullColor;
            
            SomeCurveParameterChanged |= ImGui::ColorEdit4("Color", ConvexHullColor);
            
            if (ResetContextMenuItemSelected("ConvexHullColorReset"))
            {
               *ConvexHullColor = DefaultParams.ConvexHullColor;
               SomeCurveParameterChanged = true;
            }
         }
         {
            f32 *ConvexHullWidth = &ChangeParams->ConvexHullWidth;
            drag_speed_and_format DragParams =
               CalculateDragSpeedAndFormatFromDefaultValue(DefaultParams.ConvexHullWidth);
            
            SomeCurveParameterChanged |= ImGui::DragFloat("Width",
                                                          ConvexHullWidth,
                                                          DragParams.DragSpeed,
                                                          0.0f, FLT_MAX,
                                                          DragParams.DragFormat,
                                                          ImGuiSliderFlags_NoRoundToFormat);
            
            if (ResetContextMenuItemSelected("ConvexHullWidthReset"))
            {
               *ConvexHullWidth = DefaultParams.ConvexHullWidth;
               SomeCurveParameterChanged = true;
            }
         }
      }
      
      // NOTE(hbr): De Casteljau section logic (somewhat complex, maybe refactor).
      b32 ShowDeCasteljauGradientPicking = false;
      b32 ShowDeCasteljauVisualizationParamtersPicking = false;
      switch (UIMode.Type)
      {
         case ChangeCurveParamsUIMode_DefaultCurve: {
            ShowDeCasteljauGradientPicking = true;
         } break;
         
         case CurveShapeUIMode_SelectedCurve: {
            if (ChangeParams->CurveShape.InterpolationType == Interpolation_Bezier)
            {
               ShowDeCasteljauGradientPicking = true;
               ShowDeCasteljauVisualizationParamtersPicking = true;
            }
         } break;
      }
      
      if (ChangeParams->CurveShape.InterpolationType == Interpolation_Bezier)
      {
         switch (ChangeParams->CurveShape.BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: {
               ImGui::NewLine();
               ImGui::SeparatorText("De Casteljau's Visualization");
               DeferBlock(ImGui::PushID("DeCasteljauVisualization"), ImGui::PopID())
               {
                  color *GradientA = &ChangeParams->DeCasteljau.GradientA;
                  SomeCurveParameterChanged |= ImGui::ColorEdit4("Gradient A", GradientA);
                  if (ResetContextMenuItemSelected("DeCasteljauVisualizationGradientAReset"))
                  {
                     *GradientA = DefaultParams.DeCasteljau.GradientA;
                     SomeCurveParameterChanged = true;
                  }
                  
                  color *GradientB = &ChangeParams->DeCasteljau.GradientB;
                  SomeCurveParameterChanged |= ImGui::ColorEdit4("Gradient B", GradientB);
                  if (ResetContextMenuItemSelected("DeCasteljauVisualizationGradientBReset"))
                  {
                     *GradientB = DefaultParams.DeCasteljau.GradientB;
                     SomeCurveParameterChanged = true;
                  }
               }
            } break;
            
            case Bezier_Cubic: {} break;
         }
      }
   }
   
   return SomeCurveParameterChanged;
}

// TODO(hbr): Probably use initialize list here and below
internal void
BeginCurveCombining(curve_combining *Combining, entity *CombineCurveEntity)
{
   Combining->IsCombining = true;
   Combining->CombineCurveEntity = CombineCurveEntity;
   Combining->TargetCurveEntity = 0;
   Combining->CombineCurveLastControlPoint = false;
   Combining->TargetCurveFirstControlPoint = false;
   Combining->CombinationType = CurveCombination_Merge;
}

internal void
BeginSplittingBezierCurve(splitting_bezier_curve *Splitting, entity *CurveEntity)
{
   Splitting->IsSplitting = true;
   Splitting->SplitCurveEntity = CurveEntity;
   Splitting->DraggingSplitPoint = false;
   Splitting->T = 0.0f;
   Splitting->SavedCurveVersion = U64_MAX;
}

internal void
BeginVisualizingDeCasteljauAlgorithm(de_casteljau_visualization *Visualization, entity *CurveEntity)
{
   Visualization->IsVisualizing = true;
   Visualization->T = 0.0f;
   Visualization->CurveEntity = CurveEntity;
   Visualization->SavedCurveVersion = U64_MAX;
   ClearArena(Visualization->Arena);
}

internal void
BeginAnimatingCurve(transform_curve_animation *Animation, entity *CurveEntity)
{
   Animation->Stage = AnimateCurveAnimation_PickingTarget;
   Animation->FromCurveEntity = CurveEntity;
   Animation->ToCurveEntity = 0;
}

// TODO(hbr): Instead of copying all the verices and control points and weights manually, maybe just
// wrap control points, weights and cubicbezier poiints in a structure and just assign data here.
// In other words - refactor this function
internal void
LowerBezierCurveDegree(bezier_curve_degree_lowering *Lowering, entity *Entity)
{
   curve *Curve = GetCurve(Entity);
   u64 PointCount = Curve->ControlPointCount;
   if (PointCount > 0)
   {
      Assert(Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      temp_arena Temp = TempArena(Lowering->Arena);
      
      local_position *LowerPoints = PushArrayNonZero(Temp.Arena, PointCount, local_position);
      f32 *LowerWeights = PushArrayNonZero(Temp.Arena, PointCount, f32);
      local_position *LowerBeziers = PushArrayNonZero(Temp.Arena, 3 * (PointCount - 1), local_position);
      
      MemoryCopy(LowerPoints, Curve->ControlPoints, PointCount * SizeOf(LowerPoints[0]));
      MemoryCopy(LowerWeights, Curve->ControlPointWeights, PointCount * SizeOf(LowerWeights[0]));
      
      bezier_lower_degree LowerDegree = {};
      b32 IncludeWeights = false;
      // NOTE(hbr): We cannot merge those two cases, because weights might be already modified.
      switch (Curve->CurveParams.CurveShape.BezierType)
      {
         case Bezier_Normal: {
            LowerDegree = BezierCurveLowerDegree(LowerPoints, PointCount);
         } break;
         
         case Bezier_Weighted: {
            IncludeWeights = true;
            LowerDegree = BezierWeightedCurveLowerDegree(LowerPoints, LowerWeights, PointCount);
         } break;
         
         case Bezier_Cubic: InvalidPath;
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
      SetCurveControlPoints(Curve, PointCount - 1, LowerPoints, LowerWeights, LowerBeziers);
      
      Lowering->SavedCurveVersion = Curve->CurveVersion;
      
      EndTemp(Temp);
   }
}

internal void
ElevateBezierCurveDegree(curve *Curve, editor_params *EditorParams)
{
   Assert(Curve->CurveParams.CurveShape.InterpolationType ==
          Interpolation_Bezier);
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
   
   switch (Curve->CurveParams.CurveShape.BezierType)
   {
      case Bezier_Normal: {
         BezierCurveElevateDegree(ElevatedControlPoints,
                                  ControlPointCount);
         ElevatedControlPointWeights[ControlPointCount] =
            EditorParams->DefaultControlPointWeight;
      } break;
      
      case Bezier_Weighted: {
         BezierWeightedCurveElevateDegree(ElevatedControlPoints,
                                          ElevatedControlPointWeights,
                                          ControlPointCount);
      } break;
      
      case Bezier_Cubic: InvalidPath;
   }
   
   local_position *ElevatedCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                3 * (ControlPointCount + 1),
                                                                local_position);
   BezierCubicCalculateAllControlPoints(ElevatedControlPoints,
                                        ControlPointCount + 1,
                                        ElevatedCubicBezierPoints + 1);
   
   SetCurveControlPoints(Curve,
                         ControlPointCount + 1,
                         ElevatedControlPoints,
                         ElevatedControlPointWeights,
                         ElevatedCubicBezierPoints);
   
   EndTemp(Temp);
}

internal void
FocusCameraOn(camera *Camera, world_position Position, v2f32 Extents,
              coordinate_system_data CoordinateSystemData)
{
   Camera->PositionTarget = Position;
   
   local f32 Margin = 0.7f;
   f32 ZoomX = 0.5f * (1.0f - Margin) *
      CoordinateSystemData.Projection.FrustumSize *
      CoordinateSystemData.Projection.AspectRatio / Extents.X;
   f32 ZoomY = 0.5f * (1.0f - Margin) *
      CoordinateSystemData.Projection.FrustumSize / Extents.Y;
   Camera->ZoomTarget = Min(ZoomX, ZoomY);
   
   Camera->ReachingTarget = true;
}

internal void
FocusCameraOnEntity(camera *Camera, entity *Entity, coordinate_system_data CoordinateSystemData)
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
                                                        Rotation2DInverse(Camera->Rotation));
               
               if (ControlPointRotated.X < Left)  Left  = ControlPointRotated.X;
               if (ControlPointRotated.X > Right) Right = ControlPointRotated.X;
               if (ControlPointRotated.Y < Down)  Down  = ControlPointRotated.Y;
               if (ControlPointRotated.Y > Top)   Top   = ControlPointRotated.Y;
            }
            
            world_position FocusPosition =  RotateAround(V2F32(0.5f * (Left + Right), 0.5f * (Down + Top)),
                                                         V2F32(0.0f, 0.0f), Camera->Rotation);
            v2f32 Extents = V2F32(0.5f * (Right - Left) + 2.0f * Curve->CurveParams.PointSize,
                                  0.5f * (Top - Down) + 2.0f * Curve->CurveParams.PointSize);
            FocusCameraOn(Camera, FocusPosition, Extents, CoordinateSystemData);
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
            FocusCameraOn(Camera, Entity->Position, Extents, CoordinateSystemData);
         }
      } break;
   }
}

internal void
SplitCurveOnControlPoint(entity *CurveEntity, editor_state *EditorState)
{
   curve *Curve = &CurveEntity->Curve;
   Assert(Curve->SelectedControlPointIndex < Curve->ControlPointCount);
   
   temp_arena Temp = TempArena(0);
   
   u64 NumLeftControlPoints = Curve->SelectedControlPointIndex + 1;
   u64 NumRightControlPoints = Curve->ControlPointCount - Curve->SelectedControlPointIndex;
   
   local_position *LeftControlPoints = PushArrayNonZero(Temp.Arena,
                                                        NumLeftControlPoints,
                                                        local_position);
   f32 *LeftControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                   NumLeftControlPoints,
                                                   f32);
   local_position *LeftCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                            3 * NumLeftControlPoints,
                                                            local_position);
   
   // TODO(hbr): Isn't here a good place to optimize memory copies?
   MemoryCopy(LeftControlPoints,
              Curve->ControlPoints,
              NumLeftControlPoints * SizeOf(LeftControlPoints[0]));
   MemoryCopy(LeftControlPointWeights,
              Curve->ControlPointWeights,
              NumLeftControlPoints * SizeOf(LeftControlPointWeights[0]));
   MemoryCopy(LeftCubicBezierPoints,
              Curve->CubicBezierPoints,
              3 * NumLeftControlPoints * SizeOf(LeftControlPoints[0]));
   
   local_position *RightControlPoints = PushArrayNonZero(Temp.Arena,
                                                         NumRightControlPoints,
                                                         local_position);
   f32 *RightControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                    NumRightControlPoints,
                                                    f32);
   local_position *RightCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                             3 * NumRightControlPoints,
                                                             local_position);
   
   MemoryCopy(RightControlPoints,
              Curve->ControlPoints + Curve->SelectedControlPointIndex,
              NumRightControlPoints * SizeOf(RightControlPoints[0]));
   MemoryCopy(RightControlPointWeights,
              Curve->ControlPointWeights + Curve->SelectedControlPointIndex,
              NumRightControlPoints * SizeOf(RightControlPointWeights[0]));
   MemoryCopy(RightCubicBezierPoints,
              Curve->CubicBezierPoints + 3 * Curve->SelectedControlPointIndex,
              3 * NumRightControlPoints * SizeOf(RightControlPoints[0]));
   
   // TODO(hbr): Optimize to not do so many copies and allocations
   // TODO(hbr): Maybe mint internals that create duplicate curve but without allocating and with allocating.
   entity *LeftCurveEntity = CurveEntity;
   LeftCurveEntity->Name = NameStrF("%s (left)", CurveEntity->Name.Data);
   SetCurveControlPoints(&LeftCurveEntity->Curve, NumLeftControlPoints, LeftControlPoints,
                         LeftControlPointWeights, LeftCubicBezierPoints);
   
   entity *RightCurveEntity = AllocAndAddEntity(EditorState);
   InitEntityFromEntity(RightCurveEntity, LeftCurveEntity);
   RightCurveEntity->Name = NameStrF("%s (right)", CurveEntity->Name.Data);
   SetCurveControlPoints(&RightCurveEntity->Curve, NumRightControlPoints, RightControlPoints,
                         RightControlPointWeights, RightCubicBezierPoints);
   
   EndTemp(Temp);
}

// TODO(hbr): Do a pass oveer this internal to simplify the logic maybe (and simplify how the UI looks like in real life)
// TODO(hbr): Maybe also shorten some labels used to pass to ImGUI
internal void
RenderSelectedEntityUI(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   entity *Entity = Editor->State.SelectedEntity;
   Assert(Entity);
   
   DeferBlock(ImGui::PushID("SelectedEntity"), ImGui::PopID())
   {
      bool ViewWindow = Cast(bool)Editor->UI_Config.ViewSelectedEntityWindow;
      DeferBlock(ImGui::Begin("Entity Parameters", &ViewWindow, ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {
         Editor->UI_Config.ViewSelectedEntityWindow = Cast(b32)ViewWindow;
         
         if (ViewWindow)
         {
            curve *Curve = 0;
            image *Image = 0;
            switch (Entity->Type)
            {
               case Entity_Curve: { Curve = &Entity->Curve; } break;
               case Entity_Image: { Image = &Entity->Image; } break;
            }
            
            ImGui::InputText("Name", Entity->Name.Data, ArrayCount(Entity->Name.Data));
            
            if (Image)
            {
               ImGui::NewLine();
               ImGui::SeparatorText("Image");
            }
            
            if (Curve)
            {
               local f32 PositionDeltaClipSpace = 0.001f;
               f32 DragPositionSpeed = ClipSpaceLengthToWorldSpace(PositionDeltaClipSpace,
                                                                   CoordinateSystemData);
               ImGui::DragFloat2("Position", Entity->Position.Es, DragPositionSpeed);
               if (ResetContextMenuItemSelected("PositionReset"))
               {
                  Entity->Position = V2F32(0.0f, 0.0f);
               }
            }
            
            {
               f32 RotationRadians = Rotation2DToRadians(Entity->Rotation);
               ImGui::SliderAngle("Rotation", &RotationRadians);
               if (ResetContextMenuItemSelected("RotationReset"))
               {
                  RotationRadians = 0.0f;
               }
               Entity->Rotation = Rotation2DFromRadians(RotationRadians);
            }
            
            if (Image)
            {
               sf::Vector2u ImageTextureSize = Image->Texture.getSize();
               v2f32 SelectedImageScale = Entity->Scale;
               f32 ImageWidth = SelectedImageScale.X * ImageTextureSize.x;
               f32 ImageHeight = SelectedImageScale.Y * ImageTextureSize.y;
               f32 ImageScale = 1.0f;
               
               local f32 ImageDimensionsDragSpeed = 1.0f;
               local char const *ImageDimensionsDragFormat = "%.1f";
               
               ImGui::DragFloat("Width", &ImageWidth,
                                ImageDimensionsDragSpeed,
                                0.0f, 0.0f,
                                ImageDimensionsDragFormat);
               if (ResetContextMenuItemSelected("WidthReset"))
               {
                  ImageWidth = Cast(f32)ImageTextureSize.x;
               }
               
               ImGui::DragFloat("Height", &ImageHeight,
                                ImageDimensionsDragSpeed,
                                0.0f, 0.0f,
                                ImageDimensionsDragFormat);
               if (ResetContextMenuItemSelected("HeightReset"))
               {
                  ImageHeight = Cast(f32)ImageTextureSize.y;
               }
               
               ImGui::DragFloat("Scale", &ImageScale,
                                0.001f,      // v_speed
                                0.0f,        // v_min
                                FLT_MAX,     // v_max
                                "Drag Me!"); // format
               if (ResetContextMenuItemSelected("ScaleReset"))
               {
                  ImageWidth = Cast(f32)ImageTextureSize.x;
                  ImageHeight = Cast(f32)ImageTextureSize.y;
               }
               
               f32 ImageNewScaleX = ImageScale * ImageWidth / ImageTextureSize.x;
               f32 ImageNewScaleY = ImageScale * ImageHeight / ImageTextureSize.y;
               Entity->Scale = V2F32(ImageNewScaleX, ImageNewScaleY);
            }
            
            {
               int LayerInt = Cast(int)Entity->SortingLayer;
               ImGui::SliderInt("Sorting Layer", &LayerInt, -100, 100);
               if (ResetContextMenuItemSelected("SortingLayerReset"))
               {
                  LayerInt = 0;
               }
               Entity->SortingLayer = Cast(s64)LayerInt;
            }
            
            bool SomeCurveParameterChanged = false;
            if (Curve)
            {
               change_curve_params_ui_mode UIMode =
                  ChangeCurveShapeUIModeSelectedCurve(&Editor->Parameters.CurveDefaultParams,
                                                      &Editor->Parameters.DefaultControlPointWeight,
                                                      Curve);
               ImGui::NewLine();
               SomeCurveParameterChanged |= RenderChangeCurveParametersUI("SelectedCurveShape", UIMode);
            }
            
            bool Delete                        = false;
            bool Copy                          = false;
            bool SwitchVisibility              = false;
            bool Deselect                      = false;
            bool FocusOn                       = false;
            bool ElevateBezierCurve            = false;
            bool LowerBezierCurve              = false;
            bool VisualizeDeCasteljauAlgorithm = false;
            bool SplitBezierCurve              = false;
            bool AnimateCurve                  = false;
            bool CombineCurve                  = false;
            bool SplitOnControlPoint           = false;
            
            ImGui::NewLine();
            ImGui::SeparatorText("Actions");
            {
               Delete = ImGui::Button("Delete");
               ImGui::SameLine();
               Copy = ImGui::Button("Copy");
               ImGui::SameLine();
               SwitchVisibility = ImGui::Button((Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide");
               ImGui::SameLine();
               Deselect = ImGui::Button("Deselect");
               ImGui::SameLine();
               FocusOn = ImGui::Button("Focus");
               
               if (Curve)
               {
                  // TODO(hbr): Maybe pick better name than transform
                  AnimateCurve = ImGui::Button("Animate");
                  ImGui::SameLine();
                  // TODO(hbr): Maybe pick better name than "Combine"
                  CombineCurve = ImGui::Button("Combine");
                  
                  DeferBlock(ImGui::BeginDisabled(Curve->SelectedControlPointIndex >=
                                                  Curve->ControlPointCount),
                             ImGui::EndDisabled())
                  {
                     ImGui::SameLine();
                     SplitOnControlPoint = ImGui::Button("Split on Control Point");
                  }
                  
                  curve_params *CurveParams = &Curve->CurveParams;
                  b32 IsBezierNormalOrWeighted =
                  (CurveParams->CurveShape.InterpolationType == Interpolation_Bezier &&
                   (CurveParams->CurveShape.BezierType == Bezier_Normal ||
                    CurveParams->CurveShape.BezierType == Bezier_Weighted));
                  
                  DeferBlock(ImGui::BeginDisabled(!IsBezierNormalOrWeighted),
                             ImGui::EndDisabled())
                  {
                     DeferBlock(ImGui::BeginDisabled(Curve->ControlPointCount < 2),
                                ImGui::EndDisabled())
                     {
                        ImGui::SameLine();
                        SplitBezierCurve = ImGui::Button("Split");
                     }
                     
                     ElevateBezierCurve = ImGui::Button("Elevate Degree");
                     
                     DeferBlock(ImGui::BeginDisabled(Curve->ControlPointCount == 0),
                                ImGui::EndDisabled())
                     {
                        ImGui::SameLine();
                        LowerBezierCurve = ImGui::Button("Lower Degree");
                     }
                     
                     VisualizeDeCasteljauAlgorithm = ImGui::Button("Visualize De Casteljau's Algorithm");
                  }
               }
            }
            
            if (Delete)
            {
               DeallocAndRemoveEntity(&Editor->State, Entity);
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
               DeselectCurrentEntity(&Editor->State);
            }
            
            if (FocusOn)
            {
               FocusCameraOnEntity(&Editor->State.Camera, Entity, CoordinateSystemData);
            }
            
            if (ElevateBezierCurve)
            {
               ElevateBezierCurveDegree(Curve, &Editor->Parameters);
            }
            
            if (LowerBezierCurve)
            {
               LowerBezierCurveDegree(&Editor->State.DegreeLowering, Entity);
            }
            
            if (SplitBezierCurve)
            {
               BeginSplittingBezierCurve(&Editor->State.SplittingBezierCurve, Entity);
            }
            
            if (VisualizeDeCasteljauAlgorithm)
            {
               BeginVisualizingDeCasteljauAlgorithm(&Editor->State.DeCasteljauVisualization, Entity);
            }
            
            if (AnimateCurve)
            {
               BeginAnimatingCurve(&Editor->State.CurveAnimation, Entity);
            }
            
            if (CombineCurve)
            {
               BeginCurveCombining(&Editor->State.CurveCombining, Entity);
            }
            
            if (SplitOnControlPoint)
            {
               SplitCurveOnControlPoint(Entity, &Editor->State);
            }
            
            if (SomeCurveParameterChanged)
            {
               RecomputeCurve(Curve);
            }
         }
      }
   }
}

internal void
RenderUIForRenderPointData(char const *Label,
                           render_point_data *RenderPointData,
                           f32 DefaultRadiusClipSpace,
                           f32 DefaultOutlineThicknessFraction,
                           color DefaultFillColor,
                           color DefaultOutlineColor)
{
   DeferBlock(ImGui::PushID(Label), ImGui::PopID())
   {      
      {
         drag_speed_and_format Drag =
            CalculateDragSpeedAndFormatFromDefaultValue(DefaultRadiusClipSpace);
         
         ImGui::DragFloat("Radius",
                          &RenderPointData->RadiusClipSpace,
                          Drag.DragSpeed,
                          0.0f, FLT_MAX,
                          Drag.DragFormat,
                          ImGuiSliderFlags_NoRoundToFormat);
         
         if (ResetContextMenuItemSelected("RadiusReset"))
         {
            RenderPointData->RadiusClipSpace = DefaultRadiusClipSpace;
         }
      }
      
      {   
         drag_speed_and_format Drag =
            CalculateDragSpeedAndFormatFromDefaultValue(DefaultOutlineThicknessFraction);
         
         ImGui::DragFloat("Outline Thickness",
                          &RenderPointData->OutlineThicknessFraction,
                          Drag.DragSpeed,
                          0.0f, 1.0f,
                          Drag.DragFormat);
         
         if (ResetContextMenuItemSelected("OutlineThicknessReset"))
         {
            RenderPointData->OutlineThicknessFraction =
               DefaultOutlineThicknessFraction;
         }
      }
      
      ImGui::ColorEdit4("Fill Color", &RenderPointData->FillColor);
      if (ResetContextMenuItemSelected("FillColorReset"))
      {
         RenderPointData->FillColor = DefaultFillColor;
      }
      
      ImGui::ColorEdit4("Outline Color", &RenderPointData->OutlineColor);
      if (ResetContextMenuItemSelected("OutlineColorReset"))
      {
         RenderPointData->OutlineColor = DefaultOutlineColor;
      }
   }
}

internal void
UpdateAndRenderNotifications(notifications *Notifs, f32 DeltaTime, sf::RenderWindow *Window)
{
   DeferBlock(ImGui::PushID("Notifications"), ImGui::PopID())
   {
      sf::Vector2u WindowSize = Window->getSize();
      f32 Padding = NotificationWindowPaddingScale * WindowSize.x;
      f32 DesignatedPosY = WindowSize.y - Padding;
      
      f32 WindowWidth = 0.1f * WindowSize.x;
      ImVec2 WindowMinSize = ImVec2(WindowWidth, 0.0f);
      ImVec2 WindowMaxSize = ImVec2(WindowWidth, FLT_MAX);
      
      notification *FreeSpace = Notifs->Notifs;
      u64 RemoveCount = 0;
      for (u64 NotifIndex = 0; NotifIndex < Notifs->NotifCount; ++NotifIndex)
      {
         notification *Notif = Notifs->Notifs + NotifIndex;
         
         b32 ShouldBeRemoved = false;
         
         f32 LifeTime = Notif->LifeTime + DeltaTime;
         Notif->LifeTime = LifeTime;
         
         if (LifeTime < NOTIFICATION_TOTAL_LIFE_TIME)
         {
            // NOTE(hbr): Interpolate position
            f32 CurrentPosY = Notif->PosY;
            local f32 MoveSpeed = 20.0f;
            f32 NextPosY = LerpF32(CurrentPosY, DesignatedPosY,
                                   1.0f - PowF32(2.0f, -MoveSpeed * DeltaTime));
            if (ApproxEq32(DesignatedPosY, NextPosY))
            {
               NextPosY = DesignatedPosY;
            }
            Notif->PosY = NextPosY;
            
            ImVec2 WindowPosition = ImVec2(WindowSize.x - Padding, NextPosY);
            ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
            ImGui::SetNextWindowSizeConstraints(WindowMinSize, WindowMaxSize);
            
            f32 Fade = 0.0f;
            if (LifeTime <= NOTIFICATION_FADE_IN_TIME)
            {
               Fade = LifeTime / NOTIFICATION_FADE_IN_TIME;
            }
            else if (LifeTime <= NOTIFICATION_FADE_IN_TIME + NOTIFICATION_PROPER_LIFE_TIME)
            {
               Fade = 1.0f;
            }
            else if (LifeTime <= NOTIFICATION_FADE_IN_TIME + NOTIFICATION_PROPER_LIFE_TIME +
                     NOTIFICATION_FADE_OUT_TIME)
            {
               Fade = 1.0f - (LifeTime - NOTIFICATION_FADE_IN_TIME - NOTIFICATION_PROPER_LIFE_TIME) /
                  NOTIFICATION_FADE_OUT_TIME;
            }
            else
            {
               Fade = 0.0f;
            }
            // NOTE(hbr): Quadratic interpolation instead of linear.
            Fade = 1.0f - Square(1.0f - Fade);
            Assert(-EPS_F32 <= Fade && Fade <= 1.0f + EPS_F32);
            
            char Label[128];
            Fmt(ArrayCount(Label), Label, "Notification#%llu", NotifIndex);
            
            DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Fade),
                       ImGui::PopStyleVar())
            {
               DeferBlock(ImGui::Begin(Label, 0,
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
                  
                  DeferBlock(ImGui::PushStyleColor(ImGuiCol_Text,
                                                   ImVec4(Notif->TitleColor.R,
                                                          Notif->TitleColor.G,
                                                          Notif->TitleColor.B,
                                                          Fade)),
                             ImGui::PopStyleColor(1 /*count*/))
                  {
                     ImGui::Text(Notif->Title.Data);
                  }
                  
                  ImGui::Separator();
                  
                  DeferBlock(ImGui::PushTextWrapPos(0.0f), ImGui::PopTextWrapPos())
                  {
                     ImGui::TextWrapped(Notif->Text.Data);
                  }
                  
                  f32 WindowHeight = ImGui::GetWindowHeight();
                  Notif->NotifWindowHeight = WindowHeight;
                  
                  DesignatedPosY -= WindowHeight + Padding;
               }
            }
         }
         else
         {
            ShouldBeRemoved = true;
         }
         
         if (ShouldBeRemoved)
         {
            RemoveCount += 1;
            DestroyNotification(Notif);
         }
         else
         {
            *FreeSpace++ = *Notif;
         }
      }
      Notifs->NotifCount -= RemoveCount;
   }
}

internal void
RenderListOfEntitiesEntityRow(entity *Entity, editor *Editor, coordinate_system_data CoordinateSystemData)
{
   bool Deselect       = false;
   bool Delete         = false;
   bool Copy           = false;
   bool SwitchVisility = false;
   bool Rename         = false;
   bool Focus          = false;
   
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
         ImGui::InputText("", Entity->Name.Data, ArrayCount(Entity->Name.Data));
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
      
      if (ImGui::Selectable(Entity->Name.Data, Selected))
      {
         SelectEntity(&Editor->State, Entity);
      }
      
      b32 DoubleClickedOrSelectedAndClicked = ((Selected && ImGui::IsMouseClicked(0)) ||
                                               ImGui::IsMouseDoubleClicked(0));
      if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
      {
         Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
      }
      
      local char const *ContextMenuLabel = "EntityContextMenu";
      if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
      {
         ImGui::OpenPopup(ContextMenuLabel);
      }
      if (ImGui::BeginPopup(ContextMenuLabel))
      {
         ImGui::MenuItem("Rename", 0, &Rename);
         ImGui::MenuItem("Delete", 0, &Delete);
         ImGui::MenuItem("Copy", 0, &Copy);
         ImGui::MenuItem((Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide", 0, &SwitchVisility);
         if (Selected) ImGui::MenuItem("Deselect", 0, &Deselect);
         ImGui::MenuItem("Focus", 0, &Focus);
         
         ImGui::EndPopup();
      }
   }
   
   if (Rename)
   {
      Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
   }
   
   if (Delete)
   {
      DeallocAndRemoveEntity(&Editor->State, Entity);
   }
   
   if (Copy)
   {
      DuplicateEntity(Entity, Editor);
   }
   
   if (SwitchVisility)
   {
      Entity->Flags ^= EntityFlag_Hidden;
   }
   
   if (Deselect)
   {
      DeselectCurrentEntity(&Editor->State);
   }
   
   if (Focus)
   {
      FocusCameraOnEntity(&Editor->State.Camera, Entity, CoordinateSystemData);
   }
}

internal void
RenderListOfEntitiesWindow(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   bool ViewWindow = Cast(bool)Editor->UI_Config.ViewListOfEntitiesWindow;
   
   ImGui::Begin("Entities", &ViewWindow);
   Editor->UI_Config.ViewListOfEntitiesWindow = Cast(b32)ViewWindow;
   
   if (ViewWindow)
   {
      local ImGuiTreeNodeFlags CollapsingHeaderFlags = ImGuiTreeNodeFlags_DefaultOpen;
      
      if (ImGui::CollapsingHeader("Curves", CollapsingHeaderFlags))
      {
         ImGui::PushID("CurveEntities");
         
         u64 CurveIndex = 0;
         ListIter(Entity, Editor->State.EntitiesHead, entity)
         {
            if (Entity->Type == Entity_Curve)
            {
               ImGui::PushID(Cast(int)CurveIndex);
               RenderListOfEntitiesEntityRow(Entity, Editor, CoordinateSystemData);
               ImGui::PopID();
               
               CurveIndex += 1;
            }
         }
         
         ImGui::PopID();
      }
      
      ImGui::Spacing();
      if (ImGui::CollapsingHeader("Images", CollapsingHeaderFlags))
      {
         ImGui::PushID("ImageEntities");
         
         u64 ImageIndex = 0;
         ListIter(Entity, Editor->State.EntitiesHead, entity)
         {
            if (Entity->Type == Entity_Image)
            {
               ImGui::PushID(Cast(int)ImageIndex);
               RenderListOfEntitiesEntityRow(Entity, Editor, CoordinateSystemData);
               ImGui::PopID();
               
               ImageIndex += 1;
            }
         }
         
         ImGui::PopID();
      }
   }
   
   ImGui::End();
}

internal void
UpdateAndRenderMenuBar(editor *Editor, user_input *Input)
{
   bool NewProjectSelected = false;
   bool OpenProjectSelected = false;
   bool QuitEditorSelected = false;
   bool SaveProjectSelected = false;
   bool SaveProjectAsSelected = false;
   bool LoadImage = false;
   
   bool ResetCameraPositionSelected = false;
   bool ResetCameraRotationSelected = false;
   bool ResetCameraZoomSelected = false;
   
   local char const *SaveAsLabel = "SaveAsWindow";
   local char const *SaveAsTitle = "Save As";
   local char const *ConfirmCloseCurrentProjectLabel = "ConfirmCloseCurrentProject";
   local char const *OpenNewProjectLabel = "OpenNewProject";
   local char const *OpenNewProjectTitle = "Open";
   local char const *LoadImageLabel = "LoadImage";
   local char const *LoadImageTitle = "Load Image";
   
   local ImGuiWindowFlags ConfirmCloseWindowFlags =
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_AlwaysAutoResize;
   local ImGuiWindowFlags FileDialogWindowFlags =
      ImGuiWindowFlags_NoCollapse;
   
   sf::Vector2u WindowSize = Editor->Window->getSize();
   ImVec2 HalfWindowSize = ImVec2(0.5f * WindowSize.x, 0.5f * WindowSize.y);
   ImVec2 FileDialogMinSize = HalfWindowSize;
   ImVec2 FileDialogMaxSize = ImVec2(Cast(f32)WindowSize.x, Cast(f32)WindowSize.y);
   
   auto FileDialog = ImGuiFileDialog::Instance();
   
   if (ImGui::BeginMainMenuBar())
   {
      if (ImGui::BeginMenu("Project"))
      {
         ImGui::MenuItem("New",     "Ctrl+N",       &NewProjectSelected);
         ImGui::MenuItem("Open",    "Ctrl+O",       &OpenProjectSelected);
         ImGui::MenuItem("Save",    "Ctrl+S",       &SaveProjectSelected);
         ImGui::MenuItem("Save As", "Shift+Ctrl+S", &SaveProjectAsSelected);
         ImGui::MenuItem("Quit",    "Q/Escape",     &QuitEditorSelected);
         
         ImGui::EndMenu();
      }
      
      ImGui::MenuItem("Load Image", 0, &LoadImage);
      
      if (ImGui::BeginMenu("View"))
      {
         if (ImGui::MenuItem("List of Entities", 0,
                             Cast(bool)Editor->UI_Config.ViewListOfEntitiesWindow))
         {
            Editor->UI_Config.ViewListOfEntitiesWindow = !Editor->UI_Config.ViewListOfEntitiesWindow;
         }
         
         if (ImGui::MenuItem("Selected Entity", 0,
                             Cast(bool)Editor->UI_Config.ViewSelectedEntityWindow))
         {
            Editor->UI_Config.ViewSelectedEntityWindow = !Editor->UI_Config.ViewSelectedEntityWindow;
         }
         
         if (ImGui::MenuItem("Parameters", 0,
                             Cast(bool)Editor->UI_Config.ViewParametersWindow))
         {
            Editor->UI_Config.ViewParametersWindow = !Editor->UI_Config.ViewParametersWindow;
         }
         
#if EDITOR_PROFILER
         if (ImGui::MenuItem("Profiler", 0,
                             Cast(bool)Editor->UI_Config.ViewProfilerWindow))
         {
            Editor->UI_Config.ViewProfilerWindow = !Editor->UI_Config.ViewProfilerWindow;
         }
#endif
         
#if BUILD_DEBUG
         if (ImGui::MenuItem("Debug", 0,
                             Cast(bool)Editor->UI_Config.ViewDebugWindow))
         {
            Editor->UI_Config.ViewDebugWindow = !Editor->UI_Config.ViewDebugWindow;
         }
#endif
         
         if (ImGui::BeginMenu("Camera"))
         {
            ImGui::MenuItem("Reset Position", 0, &ResetCameraPositionSelected);
            ImGui::MenuItem("Reset Rotation", 0, &ResetCameraRotationSelected);
            ImGui::MenuItem("Reset Zoom", 0, &ResetCameraZoomSelected);
            
            ImGui::EndMenu();
         }
         
         if (ImGui::MenuItem("Diagnostics", 0,
                             Cast(bool)Editor->UI_Config.ViewDiagnosticsWindow))
         {
            Editor->UI_Config.ViewDiagnosticsWindow = !Editor->UI_Config.ViewDiagnosticsWindow;
         }
         
         ImGui::EndMenu();
      }
      
      // TODO(hbr): Complete help menu
      if (ImGui::BeginMenu("Help"))
      {
         ImGui::EndMenu();
      }
      
      ImGui::EndMainMenuBar();
   }
   
   if (NewProjectSelected || PressedWithKey(Input->Keys[Key_N], Modifier_Ctrl))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_NewProject;
   }
   
   if (OpenProjectSelected || PressedWithKey(Input->Keys[Key_O], Modifier_Ctrl))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_OpenProject;
   }
   
   if (QuitEditorSelected ||
       PressedWithKey(Input->Keys[Key_Q], 0) ||
       PressedWithKey(Input->Keys[Key_ESC], 0))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_Quit;
   }
   
   if (SaveProjectSelected || PressedWithKey(Input->Keys[Key_S], Modifier_Ctrl))
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
            AddNotificationF(&Editor->Notifications,
                             Notification_Error,
                             SaveProjectInFormatError.Data);
         }
         else
         {
            AddNotificationF(&Editor->Notifications,
                             Notification_Success,
                             "project successfully saved in %s",
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
   
   if (SaveProjectAsSelected || PressedWithKey(Input->Keys[Key_S],
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
   
   {
      camera *Camera = &Editor->State.Camera;
      if (ResetCameraPositionSelected)
      {
         CameraMove(Camera, -Camera->Position);
      }
      if (ResetCameraRotationSelected)
      {
         CameraRotate(Camera, Rotation2DInverse(Camera->Rotation));
      }
      if (ResetCameraZoomSelected)
      {
         CameraSetZoom(Camera, 1.0f);
      }
   }
   
   action_to_do ActionToDo = ActionToDo_Nothing;
   
   // NOTE(hbr): Open "Are you sure you want to exit?" popup
   {   
      // NOTE(hbr): Center window.
      ImGui::SetNextWindowPos(HalfWindowSize, ImGuiCond_Always, ImVec2(0.5f,0.5f));
      if (ImGui::BeginPopupModal(ConfirmCloseCurrentProjectLabel,
                                 0, ConfirmCloseWindowFlags))
      {
         ImGui::Text("You are about to discard current project. Save it?");
         ImGui::Separator();
         
         bool Yes = ImGui::Button("Yes");
         ImGui::SameLine();
         bool No = ImGui::Button("No");
         ImGui::SameLine();
         bool Cancel = ImGui::Button("Cancel");
         
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
                     AddNotificationF(&Editor->Notifications,
                                      Notification_Error,
                                      "failed to discard current project: %s",
                                      SaveProjectInFormatError.Data);
                     
                     Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
                  }
                  else
                  {
                     AddNotificationF(&Editor->Notifications,
                                      Notification_Success,
                                      "project sucessfully saved in %s",
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
            ImGui::CloseCurrentPopup();
         
         ImGui::EndPopup();
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
            AddNotificationF(&Editor->Notifications,
                             Notification_Error,
                             SaveProjectInFormatError.Data);
            
            Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
         }
         else
         {
            EditorSetSaveProjectPath(Editor, SaveProjectFormat, SaveProjectFilePathWithExtension);
            
            AddNotificationF(&Editor->Notifications,
                             Notification_Success,
                             "project sucessfully saved in %s",
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
         editor_state NewEditorState =
            CreateDefaultEditorState(Editor->State.EntityPool,
                                     Editor->State.DeCasteljauVisualization.Arena,
                                     Editor->State.DegreeLowering.Arena,
                                     Editor->State.MovingPointArena,
                                     Editor->State.CurveAnimation.Arena);
         
         DeallocEditorState(&Editor->State);
         Editor->State = NewEditorState;
         
         EditorSetSaveProjectPath(Editor, SaveProjectFormat_None, {});
      } break;
      
      case ActionToDo_OpenProject: {
         Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
         FileDialog->OpenModal(OpenNewProjectLabel, OpenNewProjectTitle,
                               SAVED_PROJECT_FILE_EXTENSION, ".");
      } break;
      
      case ActionToDo_Quit: {
         Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
         Editor->Window->close();
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
         
         load_project_result LoadResult = LoadProjectFromFile(Temp.Arena,
                                                              OpenProjectFilePath,
                                                              Editor->State.EntityPool,
                                                              Editor->State.DeCasteljauVisualization.Arena,
                                                              Editor->State.DegreeLowering.Arena,
                                                              Editor->State.MovingPointArena,
                                                              Editor->State.CurveAnimation.Arena);
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
            
            DeallocEditorState(&Editor->State);
            Editor->State = LoadResult.EditorState;
            Editor->Parameters = LoadResult.EditorParameters;
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
            
            entity *Entity = AllocAndAddEntity(&Editor->State);
            InitImageEntity(Entity,
                            V2F32(0.0f, 0.0f),
                            V2F32(1.0f, 1.0f),
                            Rotation2DZero(),
                            StrToNameStr(ImageFileName),
                            NewImageFilePath,
                            &LoadedTexture);
            
            SelectEntity(&Editor->State, Entity);
            
            AddNotificationF(&Editor->Notifications,
                             Notification_Success,
                             "successfully loaded image from %s",
                             NewImageFilePath.Data);
         }
         
         EndTemp(Temp);
      }
      
      FileDialog->Close();
   }
}

internal void
RenderParamtersWindow(editor *Editor)
{
   bool ViewParametersWindowAsBool = Editor->UI_Config.ViewParametersWindow;
   DeferBlock(ImGui::Begin("Parameters", &ViewParametersWindowAsBool,
                           ImGuiWindowFlags_AlwaysAutoResize),
              ImGui::End())
   {
      Editor->UI_Config.ViewParametersWindow = Cast(b32)ViewParametersWindowAsBool;
      
      if (Editor->UI_Config.ViewParametersWindow)
      {   
         local ImGuiTreeNodeFlags SectionFlags = ImGuiTreeNodeFlags_DefaultOpen;
         editor_params *Params = &Editor->Parameters;
         
         {
            b32 *HeaderCollapsed = &Editor->UI_Config.DefaultCurveHeaderCollapsed;
            ImGui::SetNextItemOpen(*HeaderCollapsed);
            
            *HeaderCollapsed = Cast(b32)ImGui::CollapsingHeader("Default Curve", SectionFlags);
            if (!(*HeaderCollapsed))
            {
               change_curve_params_ui_mode UIMode =
                  ChangeCurveShapeUIModeDefaultCurve(&Params->CurveDefaultParams,
                                                     &Params->DefaultControlPointWeight);
               
               RenderChangeCurveParametersUI("DefaultCurve", UIMode);
            }
         }
         {   
            b32 *HeaderCollapsed = &Editor->UI_Config.RotationIndicatorHeaderCollapsed;
            ImGui::SetNextItemOpen(*HeaderCollapsed);
            
            ImGui::Spacing();
            *HeaderCollapsed = Cast(b32)ImGui::CollapsingHeader("Rotation Indicator", SectionFlags);
            if (!(*HeaderCollapsed))
            {
               RenderUIForRenderPointData("RotationIndicator",
                                          &Params->RotationIndicator,
                                          ROTATION_INDICATOR_DEFAULT_RADIUS_CLIP_SPACE,
                                          ROTATION_INDICATOR_DEFAULT_OUTLINE_THICKNESS_FRACTION,
                                          ROTATION_INDICATOR_DEFAULT_FILL_COLOR,
                                          ROTATION_INDICATOR_DEFAULT_OUTLINE_COLOR);
            }
         }
         {
            b32 *HeaderCollapsed = &Editor->UI_Config.BezierSplitPointHeaderCollapsed;
            ImGui::SetNextItemOpen(*HeaderCollapsed);
            
            ImGui::Spacing();
            *HeaderCollapsed = Cast(b32)ImGui::CollapsingHeader("Bezier Split Point", SectionFlags);
            if (!(*HeaderCollapsed))
            {
               RenderUIForRenderPointData("BezierSplitPoint",
                                          &Params->BezierSplitPoint,
                                          BEZIER_SPLIT_POINT_DEFAULT_RADIUS_CLIP_SPACE,
                                          BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_THICKNESS_FRACTION,
                                          BEZIER_SPLIT_POINT_DEFAULT_FILL_COLOR,
                                          BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_COLOR);
            }
         }
         {
            b32 *HeaderCollapsed = &Editor->UI_Config.OtherSettingsHeaderCollapsed;
            ImGui::SetNextItemOpen(*HeaderCollapsed);
            
            ImGui::Spacing();
            *HeaderCollapsed = Cast(b32)ImGui::CollapsingHeader("Other Settings", SectionFlags);
            if (!(*HeaderCollapsed))
            {
               DeferBlock(ImGui::PushID("OtherSettings"), ImGui::PopID())
               {
                  ImGui::ColorEdit4("Background Color", &Params->BackgroundColor);
                  
                  if (ResetContextMenuItemSelected("BackgroundColorReset"))
                  {
                     Params->BackgroundColor = DEFAULT_BACKGROUND_COLOR;
                  }
                  
                  //-
                  {
                     drag_speed_and_format Drag =
                        CalculateDragSpeedAndFormatFromDefaultValue(DEFAULT_COLLLISION_TOLERANCE_CLIP_SPACE);
                     
                     ImGui::DragFloat("Collision Tolerance",
                                      &Params->CollisionToleranceClipSpace,
                                      Drag.DragSpeed,
                                      0.0f, 1.0f,
                                      Drag.DragFormat);
                     
                     if (ResetContextMenuItemSelected("CollisionToleranceReset"))
                     {
                        Params->CollisionToleranceClipSpace = DEFAULT_COLLLISION_TOLERANCE_CLIP_SPACE;
                     }
                  }
                  
                  //-
                  {
                     drag_speed_and_format Drag =
                        CalculateDragSpeedAndFormatFromDefaultValue(LAST_CONTROL_POINT_DEFAULT_SIZE_MULTIPLIER);
                     
                     ImGui::DragFloat("Last Control Point Size",
                                      &Params->LastControlPointSizeMultiplier,
                                      Drag.DragSpeed,
                                      0.0f, FLT_MAX,
                                      Drag.DragFormat);
                     
                     if (ResetContextMenuItemSelected("LastControlPointSizeReset"))
                     {
                        Params->LastControlPointSizeMultiplier = LAST_CONTROL_POINT_DEFAULT_SIZE_MULTIPLIER;
                     }
                  }
                  
                  //-
                  {
                     drag_speed_and_format Drag =
                        CalculateDragSpeedAndFormatFromDefaultValue(SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_THICKNESS_SCALE);
                     
                     ImGui::DragFloat("Selected Curve Control Point Outline Thickness",
                                      &Params->SelectedCurveControlPointOutlineThicknessScale,
                                      Drag.DragSpeed,
                                      0.0f, FLT_MAX,
                                      Drag.DragFormat);
                     
                     if (ResetContextMenuItemSelected("SelectedCurveControlPointOutlineThicknessReset"))
                     {
                        Params->SelectedCurveControlPointOutlineThicknessScale =
                           SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_THICKNESS_SCALE;
                     }
                  }
                  
                  //-
                  ImGui::ColorEdit4("Selected Curve Control Point Outline Color",
                                    &Params->SelectedCurveControlPointOutlineColor);
                  
                  if (ResetContextMenuItemSelected("SelectedCurveControlPointOutlineColorReset"))
                  {
                     Params->SelectedCurveControlPointOutlineColor = SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_COLOR;
                  }
                  
                  //-
                  ImGui::ColorEdit4("Selected Control Point Outline Color",
                                    &Params->SelectedControlPointOutlineColor);
                  
                  if (ResetContextMenuItemSelected("SelectedControlPointOutlineColorReset"))
                  {
                     Params->SelectedControlPointOutlineColor = SELECTED_CONTROL_POINT_DEFAULT_OUTLINE_COLOR;
                  }
                  
                  //-
                  {
                     drag_speed_and_format Drag =
                        CalculateDragSpeedAndFormatFromDefaultValue(CUBIC_BEZIER_HELPER_LINE_DEFAULT_WIDTH_CLIP_SPACE);
                     
                     ImGui::DragFloat("Cubic Bezier Helper Line Width",
                                      &Params->CubicBezierHelperLineWidthClipSpace,
                                      Drag.DragSpeed,
                                      0.0f, FLT_MAX,
                                      Drag.DragFormat);
                     
                     if (ResetContextMenuItemSelected("CubicBezierHelperLineWidthReset"))
                     {
                        Params->CubicBezierHelperLineWidthClipSpace =
                           CUBIC_BEZIER_HELPER_LINE_DEFAULT_WIDTH_CLIP_SPACE;
                     }
                  }
               }
            }
         }
      }
   }
}

internal void
RenderDiagnosticsWindow(editor *Editor, f32 DeltaTime)
{
   bool ViewDiagnosticsWindowAsBool = Cast(bool)Editor->UI_Config.ViewDiagnosticsWindow;
   DeferBlock(ImGui::Begin("Diagnostics", &ViewDiagnosticsWindowAsBool),
              ImGui::End())
   {
      Editor->UI_Config.ViewDiagnosticsWindow = Cast(b32)ViewDiagnosticsWindowAsBool;
      
      if (Editor->UI_Config.ViewDiagnosticsWindow)
      {      
         frame_stats FrameStats = Editor->FrameStats;
         
         ImGui::Text("%20s: %.2f ms", "Frame time", 1000.0f * DeltaTime);
         ImGui::Text("%20s: %.0f", "FPS", FrameStats.FPS);
         ImGui::Text("%20s: %.2f ms", "Min frame time", 1000.0f * FrameStats.MinFrameTime);
         ImGui::Text("%20s: %.2f ms", "Max frame time", 1000.0f * FrameStats.MaxFrameTime);
         ImGui::Text("%20s: %.2f ms", "Average frame time", 1000.0f * FrameStats.AvgFrameTime);
      }
   }
   
}

internal void
UpdateAndRenderSplittingBezierCurve(editor *Editor,
                                    coordinate_system_data CoordinateSystemData,
                                    sf::Transform Transform)
{
   TimeFunction;
   
   // TODO(hbr): Remove those variables or rename them
   editor_state *EditorState = &Editor->State;
   editor_params *EditorParams = &Editor->Parameters;
   sf::RenderWindow *Window = Editor->Window;
   
   splitting_bezier_curve *Splitting = &EditorState->SplittingBezierCurve;
   entity *CurveEntity = Splitting->SplitCurveEntity;
   curve *Curve = &CurveEntity->Curve;
   
   if (Splitting->IsSplitting)
   {
      b32 CurveQualifiesForSplitting = false;
      curve_shape CurveShape = Curve->CurveParams.CurveShape;
      if (CurveShape.InterpolationType == Interpolation_Bezier)
      {
         switch (CurveShape.BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: { CurveQualifiesForSplitting = true; } break;
            case Bezier_Cubic: {} break;
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
         
         PerformSplit = ImGui::Button("Split");
         ImGui::SameLine();
         Cancel = ImGui::Button("Cancel");
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
      curve_shape CurveShape = Curve->CurveParams.CurveShape;
      
      f32 *ControlPointWeights = Curve->ControlPointWeights;
      switch (CurveShape.BezierType)
      {
         case Bezier_Normal: {
            ControlPointWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
            for (u64 I = 0; I < ControlPointCount; ++I)
            {
               ControlPointWeights[I] = 1.0f;
            }
         } break;
         
         case Bezier_Weighted: {} break;
         case Bezier_Cubic: InvalidPath;
      }
      
      local_position *LeftControlPoints = PushArrayNonZero(Temp.Arena,
                                                           ControlPointCount,
                                                           local_position);
      local_position *RightControlPoints = PushArrayNonZero(Temp.Arena,
                                                            ControlPointCount,
                                                            local_position);
      
      f32 *LeftControlPointWeights  = PushArrayNonZero(Temp.Arena,
                                                       ControlPointCount,
                                                       f32);
      f32 *RightControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                       ControlPointCount,
                                                       f32);
      
      BezierCurveSplit(Splitting->T,
                       Curve->ControlPoints,
                       ControlPointWeights,
                       ControlPointCount,
                       LeftControlPoints,
                       LeftControlPointWeights,
                       RightControlPoints,
                       RightControlPointWeights);
      
      local_position *LeftCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                               3 * ControlPointCount,
                                                               local_position);
      local_position *RightCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                3 * ControlPointCount,
                                                                local_position);
      
      BezierCubicCalculateAllControlPoints(LeftControlPoints,
                                           ControlPointCount,
                                           LeftCubicBezierPoints + 1);
      BezierCubicCalculateAllControlPoints(RightControlPoints,
                                           ControlPointCount,
                                           RightCubicBezierPoints + 1);
      
      // TODO(hbr): Optimize here not to do so many copies, maybe merge with split on control point code
      // TODO(hbr): Isn't this similar to split on curve point
      // TODO(hbr): Maybe remove this duplication of initialzation, maybe not?
      // TODO(hbr): Refactor that code into common internal that splits curve into two curves
      entity *LeftCurveEntity = CurveEntity;
      LeftCurveEntity->Name = NameStrF("%s (left)", LeftCurveEntity->Name.Data);
      SetCurveControlPoints(&LeftCurveEntity->Curve,
                            ControlPointCount,
                            LeftControlPoints, LeftControlPointWeights, LeftCubicBezierPoints);
      
      entity *RightCurveEntity = AllocAndAddEntity(EditorState);
      RightCurveEntity->Name = NameStrF("%s (right)", RightCurveEntity->Name.Data);
      InitEntityFromEntity(RightCurveEntity, LeftCurveEntity);
      SetCurveControlPoints(&RightCurveEntity->Curve,
                            ControlPointCount,
                            RightControlPoints, RightControlPointWeights, RightCubicBezierPoints);
      
      Splitting->IsSplitting = false;
      
      EndTemp(Temp);
   }
   
   if (Splitting->IsSplitting)
   {
      if (T_ParamterChanged ||
          Splitting->SavedCurveVersion != Curve->CurveVersion)
      {
         local_position SplitPoint = {};
         switch (Curve->CurveParams.CurveShape.BezierType)
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
            
            case Bezier_Cubic: InvalidPath;
         }
         Splitting->SplitPoint = LocalEntityPositionToWorld(CurveEntity, SplitPoint);
      }
      
      if (!(CurveEntity->Flags & EntityFlag_Hidden))
      {
         RenderPoint(Splitting->SplitPoint,
                     EditorParams->BezierSplitPoint,
                     CoordinateSystemData, Transform,
                     Window);
      }
   }
}

internal void
UpdateAndRenderDeCasteljauVisualization(de_casteljau_visualization *Visualization,
                                        sf::Transform Transform,
                                        sf::RenderWindow *Window)
{
   entity *CurveEntity = Visualization->CurveEntity;
   curve *Curve = &CurveEntity->Curve;
   
   if (Visualization->IsVisualizing)
   {
      curve_shape CurveShape = Curve->CurveParams.CurveShape;
      
      b32 CurveQualifiesForVisualizing = false;
      if (CurveShape.InterpolationType == Interpolation_Bezier)
      {
         switch (CurveShape.BezierType)
         {
            case Bezier_Normal:
            case Bezier_Weighted: { CurveQualifiesForVisualizing = true; } break;
            case Bezier_Cubic: {} break;
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
         switch (Curve->CurveParams.CurveShape.BezierType)
         {
            case Bezier_Normal: {
               ControlPointWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
               for (u64 I = 0; I < ControlPointCount; ++I)
               {
                  ControlPointWeights[I] = 1.0f;
               }
            } break;
            
            case Bezier_Weighted: {} break;
            case Bezier_Cubic: InvalidPath;
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
UpdateAndRenderDegreeLowering(bezier_curve_degree_lowering *Lowering,
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
      curve *Curve = GetCurve(Lowering->Entity);
      
      bool Ok = false;
      bool Revert = false;
      bool IsDegreeLoweringWindowOpen = true;
      bool P_MixChanged = false;
      bool W_MixChanged = false;
      
      Assert(Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      b32 IncludeWeights = false;
      switch (Curve->CurveParams.CurveShape.BezierType)
      {
         case Bezier_Normal: {} break;
         case Bezier_Weighted: { IncludeWeights = true; } break;
         case Bezier_Cubic: InvalidPath;
      }
      
      DeferBlock(ImGui::Begin("Degree Lowering", &IsDegreeLoweringWindowOpen,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {                 
         ImGui::TextWrapped("Degree lowering failed (curve has higher degree "
                            "than the one you are trying to fit). Tweak parameters"
                            "in order to fit curve manually or revert.");
         
         P_MixChanged = ImGui::SliderFloat("Middle Point Mix", &Lowering->P_Mix, 0.0f, 1.0f);
         if (IncludeWeights)
         {
            W_MixChanged = ImGui::SliderFloat("Middle Point Weight Mix", &Lowering->W_Mix, 0.0f, 1.0f);
         }
      }
      
      Ok = ImGui::Button("OK");
      ImGui::SameLine();
      Revert = ImGui::Button("Revert");
      
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
         SetCurveControlPoints(Curve, Curve->ControlPointCount + 1,
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
   TimeFunction;
   
   // TODO(hbr): Rename to [State] or don't use at all.
   editor_state *EditorState = &Editor->State;
   transform_curve_animation *Animation = &EditorState->CurveAnimation;
   
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
            ImGui::Text("Animate");
            ImGui::SameLine();
            ImGui::TextColored(CURVE_NAME_HIGHLIGHT_COLOR, Animation->FromCurveEntity->Name.Data);
            ImGui::SameLine();
            ImGui::Text("with");
            ImGui::SameLine();
            char const *ComboPreview = (Animation->ToCurveEntity ? Animation->ToCurveEntity->Name.Data : "");
            if (ImGui::BeginCombo("##AnimationTarget", ComboPreview))
            {
               ListIter(Entity, EditorState->EntitiesHead, entity)
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
            ImGui::SameLine();
            ImGui::Text("(choose from list or click on curve).");
            
            if (Animation->ToCurveEntity)
            {
               Animate = ImGui::Button("Animate");
               ImGui::SameLine();
            }
            Cancel = ImGui::Button("Cancel");
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
                  ImGui::SameLine();
               }
               
               ImGui::RadioButton(AnimationToString(Cast(animation_type)AnimationType),
                                  &AnimationAsInt,
                                  AnimationType);
            }
            Animation->Animation = Cast(animation_type)AnimationAsInt;
            
            ImGui::SameLine();
            
            if (ImGui::Button(Animation->IsAnimationPlaying ? "Stop" : "Start"))
            {
               Animation->IsAnimationPlaying = !Animation->IsAnimationPlaying;
            }
            
            BlendChanged = ImGui::SliderFloat("Blend", &Animation->Blend, 0.0f, 1.0f);
            
            {
               drag_speed_and_format Drag =
                  CalculateDragSpeedAndFormatFromDefaultValue(CURVE_DEFAULT_ANIMATION_SPEED);
               ImGui::DragFloat("Speed",
                                &Animation->AnimationSpeed,
                                Drag.DragSpeed,
                                0.0f, FLT_MAX,
                                Drag.DragFormat);
            }
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
                  
                  AnimatedCurvePoints[VertexIndex] = LerpV2F32(From, To, Blend);
                  
                  T += Delta_T;
               }
               
               f32 AnimatedCurveWidth = LerpF32(FromCurve->CurveParams.CurveWidth,
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
}

internal b32
IsCombinationTypeAllowed(curve *CombineCurve, curve_combination_type Combination)
{
   b32 Result = false;
   switch (Combination)
   {
      case CurveCombination_Merge:
      case CurveCombination_C0:
      case CurveCombination_G1: { Result = true; } break;
      
      case CurveCombination_C1:
      case CurveCombination_C2: {
         curve_shape Shape = CombineCurve->CurveParams.CurveShape;
         // TODO(hbr): Maybe include weighted as well and maybe even cubic
         Result = (Shape.InterpolationType == Interpolation_Bezier &&
                   Shape.BezierType == Bezier_Normal);
      } break;
      
      case CurveCombination_Count: InvalidPath;
   }
   
   return Result;
}

internal char const *
CombinationTypeToString(curve_combination_type Combination)
{
   switch (Combination)
   {
      case CurveCombination_Merge: return "Merge";
      case CurveCombination_C0: return "C0";
      case CurveCombination_C1: return "C1";
      case CurveCombination_C2: return "C2";
      case CurveCombination_G1: return "G1";
      
      case CurveCombination_Count: InvalidPath;
   }
   
   return "invalid";
}

internal void
CombineCurves(curve_combining *Combining, editor_state *EditorState)
{
   entity *FromEntity = Combining->CombineCurveEntity;
   entity *ToEntity = Combining->TargetCurveEntity;
   curve *From = &FromEntity->Curve;
   curve *To = &ToEntity->Curve;
   
   u64 N = From->ControlPointCount;
   u64 M = To->ControlPointCount;
   
   Assert(AreCurvesCompatibleForCombining(From, To));
   
   // NOTE(hbr): Prepare control point-related buffers in proper order.
   if (Combining->CombineCurveLastControlPoint)
   {
      ArrayReverse(From->ControlPoints, N, local_position);
      ArrayReverse(From->ControlPointWeights, N, f32);
      ArrayReverse(From->CubicBezierPoints, 3 * N, local_position);
   }
   
   if (Combining->TargetCurveFirstControlPoint)
   {
      ArrayReverse(To->ControlPoints, M, local_position);
      ArrayReverse(To->ControlPointWeights, M, f32);
      ArrayReverse(To->CubicBezierPoints, 3 * M, local_position);
   }
   
   u64 CombinedControlPointCount = M;
   u64 StartIndex = 0;
   v2f32 Translation = {};
   switch (Combining->CombinationType)
   {
      case CurveCombination_Merge: {
         CombinedControlPointCount += N;
         StartIndex = 0;
         Translation = {};
      } break;
      
      case CurveCombination_C0:
      case CurveCombination_C1:
      case CurveCombination_C2:
      case CurveCombination_G1: {
         if (N > 0)
         {
            CombinedControlPointCount += N - 1;
            StartIndex = 1;
            if (M > 0)
            {
               Translation =
                  LocalEntityPositionToWorld(ToEntity, To->ControlPoints[M - 1]) -
                  LocalEntityPositionToWorld(FromEntity, From->ControlPoints[0]);
            }
         }
      } break;
      
      case CurveCombination_Count: InvalidPath;
   }
   
   temp_arena Temp = TempArena(0);
   
   // NOTE(hbr): Allocate buffers and copy control points into them
   v2f32 *CombinedControlPoints = PushArrayNonZero(Temp.Arena, CombinedControlPointCount, v2f32);
   f32 *CombinedControlPointWeights = PushArrayNonZero(Temp.Arena, CombinedControlPointCount, f32);
   v2f32 *CombinedCubicBezierPoints = PushArrayNonZero(Temp.Arena, 3 * CombinedControlPointCount, v2f32);
   
   MemoryCopy(CombinedControlPoints,
              To->ControlPoints,
              M * SizeOf(CombinedControlPoints[0]));
   MemoryCopy(CombinedControlPointWeights,
              To->ControlPointWeights,
              M * SizeOf(CombinedControlPointWeights[0]));
   MemoryCopy(CombinedCubicBezierPoints,
              To->CubicBezierPoints,
              3 * M * SizeOf(CombinedCubicBezierPoints[0]));
   
   {
      for (u64 I = StartIndex; I < N; ++I)
      {
         world_position FromControlPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
         local_position ToControlPoint = WorldToLocalEntityPosition(ToEntity, FromControlPoint + Translation);
         CombinedControlPoints[M - StartIndex + I] = ToControlPoint;
      }
      
      for (u64 I = 3 * StartIndex; I < 3 * N; ++I)
      {
         world_position FromCubicBezierPoint = LocalEntityPositionToWorld(FromEntity, From->CubicBezierPoints[I]);
         local_position ToCubicBezierPoint = WorldToLocalEntityPosition(ToEntity, FromCubicBezierPoint + Translation); 
         CombinedCubicBezierPoints[3 * (M - StartIndex) + I] = ToCubicBezierPoint;
      }
      
      MemoryCopy(CombinedControlPointWeights + M,
                 From->ControlPointWeights + StartIndex,
                 (N - StartIndex) * SizeOf(CombinedControlPointWeights[0]));
   }
   
   // NOTE(hbr): Combine control points properly on the border
   switch (Combining->CombinationType)
   {
      // NOTE(hbr): Nothing to do
      case CurveCombination_Merge:
      case CurveCombination_C0: {} break;
      
      case CurveCombination_C1: {
         if (M >= 2 && N >= 2)
         {
            v2f32 P = CombinedControlPoints[M - 1];
            v2f32 Q = CombinedControlPoints[M - 2];
            
            // NOTE(hbr): First derivative equal
            v2f32 FixedControlPoint = Cast(f32)M/N * (P - Q) + P;
            v2f32 Fix = FixedControlPoint - CombinedControlPoints[M];
            CombinedControlPoints[M] = FixedControlPoint;
            
            CombinedCubicBezierPoints[3 * M + 0] += Fix;
            CombinedCubicBezierPoints[3 * M + 1] += Fix;
            CombinedCubicBezierPoints[3 * M + 2] += Fix;
         }
      } break;
      
      case CurveCombination_C2: {
         if (M >= 3 && N >= 3)
         {
            // TODO(hbr): Merge C1 with C2, maybe G1.
            v2f32 R = CombinedControlPoints[M - 3];
            v2f32 Q = CombinedControlPoints[M - 2];
            v2f32 P = CombinedControlPoints[M - 1];
            
            // NOTE(hbr): First derivative equal
            v2f32 Fixed_T = Cast(f32)M/N * (P - Q) + P;
            // NOTE(hbr): Second derivative equal
            v2f32 Fixed_U = Cast(f32)(N * (N-1))/(M * (M-1)) * (P - 2.0f * Q + R) + 2.0f * Fixed_T - P;
            v2f32 Fix_T = Fixed_T - CombinedControlPoints[M];
            v2f32 Fix_U = Fixed_U - CombinedControlPoints[M + 1];
            CombinedControlPoints[M] = Fixed_T;
            CombinedControlPoints[M + 1] = Fixed_U;
            
            CombinedCubicBezierPoints[3 * M + 0] += Fix_T;
            CombinedCubicBezierPoints[3 * M + 1] += Fix_T;
            CombinedCubicBezierPoints[3 * M + 2] += Fix_T;
            
            CombinedCubicBezierPoints[3 * (M + 1) + 0] += Fix_U;
            CombinedCubicBezierPoints[3 * (M + 1) + 1] += Fix_U;
            CombinedCubicBezierPoints[3 * (M + 1) + 2] += Fix_U;
         }
      } break;
      
      case CurveCombination_G1: {
         if (M >= 2 && N >= 2)
         {
            f32 PreserveLength = Norm(From->ControlPoints[1] - From->ControlPoints[0]);
            
            v2f32 P = CombinedControlPoints[M - 2];
            v2f32 Q = CombinedControlPoints[M - 1];
            v2f32 Direction = P - Q;
            Normalize(&Direction);
            
            v2f32 FixedControlPoint = Q - PreserveLength * Direction;
            v2f32 Fix = FixedControlPoint - CombinedControlPoints[M];
            CombinedControlPoints[M] = FixedControlPoint;
            
            CombinedCubicBezierPoints[3 * M + 0] += Fix;
            CombinedCubicBezierPoints[3 * M + 1] += Fix;
            CombinedCubicBezierPoints[3 * M + 2] += Fix;
         }
      } break;
      
      case CurveCombination_Count: InvalidPath;
   }
   
   SetCurveControlPoints(To,
                         CombinedControlPointCount,
                         CombinedControlPoints,
                         CombinedControlPointWeights,
                         CombinedCubicBezierPoints);
   
   // TODO(hbr): Maybe update name of combined curve
   DeallocAndRemoveEntity(EditorState, FromEntity);
   // TODO(hbr): Maybe select To curve
   
   EndTemp(Temp);
}

internal void
UpdateAndRenderCurveCombining(editor *Editor, sf::Transform Transform)
{
   sf::RenderWindow *Window = Editor->Window;
   // TODO(hbr): Rename to [State] or don't use at all
   editor_state *EditorState = &Editor->State;
   curve_combining *Combining = &EditorState->CurveCombining;
   
   if (Combining->IsCombining &&
       Combining->TargetCurveEntity &&
       !AreCurvesCompatibleForCombining(&Combining->CombineCurveEntity->Curve,
                                        &Combining->TargetCurveEntity->Curve))
   {
      Combining->TargetCurveEntity = 0;
   }
   
   if (Combining->IsCombining &&
       !IsCombinationTypeAllowed(&Combining->CombineCurveEntity->Curve, Combining->CombinationType))
   {
      Combining->CombinationType = CurveCombination_Merge;
   }
   
   if (Combining->IsCombining)
   {
      bool IsWindowOpen = true;
      bool Combine = false;
      bool Cancel = false;
      
      DeferBlock(ImGui::Begin("Combining curves", &IsWindowOpen,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {                 
         
         if (IsWindowOpen)
         {
            ImGui::Text("Combine");
            ImGui::SameLine();
            ImGui::TextColored(CURVE_NAME_HIGHLIGHT_COLOR, Combining->CombineCurveEntity->Name.Data);
            ImGui::SameLine();
            ImGui::Text("with");
            ImGui::SameLine();
            char const *ComboPreview = (Combining->TargetCurveEntity ? Combining->TargetCurveEntity->Name.Data : "");
            if (ImGui::BeginCombo("##CombiningTarget", ComboPreview))
            {
               ListIter(Entity, EditorState->EntitiesHead, entity)
               {
                  if (Entity->Type == Entity_Curve)
                  {
                     curve *Curve = &Entity->Curve;
                     
                     if (Entity != Combining->CombineCurveEntity &&
                         AreCurvesCompatibleForCombining(&Combining->CombineCurveEntity->Curve, Curve))
                     {
                        // TODO(hbr): Revise this whole internal
                        b32 Selected = (Entity == Combining->TargetCurveEntity);
                        if (ImGui::Selectable(Entity->Name.Data, Selected))
                        {
                           Combining->TargetCurveEntity = Entity;
                        }
                     }
                  }
               }
               ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Text("(choose from list or click on curve).");
            
            {
               ImGui::Text("Combination Type");
               
               int CombinationTypeAsInt = Cast(int)Combining->CombinationType;
               for (int Combination = 0;
                    Combination < CurveCombination_Count;
                    ++Combination)
               {
                  curve_combination_type CombinationType = Cast(curve_combination_type)Combination;
                  
                  ImGui::SameLine();
                  ImGui::BeginDisabled(!IsCombinationTypeAllowed(&Combining->CombineCurveEntity->Curve, CombinationType));
                  ImGui::RadioButton(CombinationTypeToString(CombinationType),
                                     &CombinationTypeAsInt,
                                     Combination);
                  ImGui::EndDisabled();
               }
               Combining->CombinationType = Cast(curve_combination_type)CombinationTypeAsInt;
            }
            
            if (Combining->TargetCurveEntity)
            {
               Combine = ImGui::Button("Combine");
               ImGui::SameLine();
            }
            Cancel = ImGui::Button("Cancel");
         }
      }
      
      if (Combine)
      {
         CombineCurves(Combining, EditorState);
      }
      
      if (Combine || !IsWindowOpen || Cancel)
      {
         Combining->IsCombining = false;
      }
   }
   
   {
      entity *CombineCurveEntity = Combining->CombineCurveEntity;
      entity *TargetCurveEntity = Combining->TargetCurveEntity;
      
      curve *CombineCurve = &CombineCurveEntity->Curve;
      curve *TargetCurve = &TargetCurveEntity->Curve;
      
      if (Combining->IsCombining && TargetCurve &&
          CombineCurve->ControlPointCount > 0 &&
          TargetCurve->ControlPointCount > 0)
      {
         local_position CombineControlPoint = (Combining->CombineCurveLastControlPoint ?
                                               CombineCurve->ControlPoints[CombineCurve->ControlPointCount - 1] :
                                               CombineCurve->ControlPoints[0]);
         
         local_position TargetControlPoint = (Combining->TargetCurveFirstControlPoint ?
                                              TargetCurve->ControlPoints[0] :
                                              TargetCurve->ControlPoints[TargetCurve->ControlPointCount - 1]);
         
         world_position BeginPoint = LocalEntityPositionToWorld(CombineCurveEntity, CombineControlPoint);
         world_position TargetPoint = LocalEntityPositionToWorld(TargetCurveEntity, TargetControlPoint);
         
         f32 LineWidth = 0.5f * CombineCurve->CurveParams.CurveWidth;
         color Color = CombineCurve->CurveParams.CurveColor;
         Color.A = Cast(u8)(0.5f * Color.A);
         
         v2f32 LineDirection = TargetPoint - BeginPoint;
         Normalize(&LineDirection);
         
         f32 TriangleSide = 20.0f * LineWidth;
         f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
         world_position BaseVertex = TargetPoint - TriangleHeight * LineDirection;
         
         DrawLine(BeginPoint, BaseVertex, LineWidth, Color, Transform, Window);
         
         v2f32 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
         world_position LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
         world_position RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
         
         DrawTriangle(LeftVertex, RightVertex, TargetPoint, Color, Transform, Window);
      }
   }
}

internal void
UpdateAndRenderEntities(editor *Editor, f32 DeltaTime, coordinate_system_data CoordinateSystemData, sf::Transform VP)
{
   TimeFunction;
   
   temp_arena Temp = TempArena(0);
   
   editor_state *State = &Editor->State;
   sf::RenderWindow *Window = Editor->Window;
   sorted_entity_array SortedEntities = SortEntitiesBySortingLayer(Temp.Arena, State->EntitiesHead);
   
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
               curve_params CurveParams = Curve->CurveParams;
               
               sf::Transform Model = CurveGetAnimate(Entity);
               sf::Transform MVP = VP * Model;
               
               if (State->Mode.Type == EditorMode_Moving &&
                   State->Mode.Moving.Type == MovingMode_CurvePoint &&
                   State->Mode.Moving.Entity == Entity)
               {
                  Window->draw(State->Mode.Moving.SavedCurveVertices,
                               State->Mode.Moving.SavedNumCurveVertices,
                               State->Mode.Moving.SavedPrimitiveType,
                               MVP);
               }
               
               Window->draw(Curve->CurveVertices.Vertices,
                            Curve->CurveVertices.NumVertices,
                            Curve->CurveVertices.PrimitiveType,
                            MVP);
               
               if (CurveParams.PolylineEnabled)
               {
                  Window->draw(Curve->PolylineVertices.Vertices,
                               Curve->PolylineVertices.NumVertices,
                               Curve->PolylineVertices.PrimitiveType,
                               MVP);
               }
               
               if (CurveParams.ConvexHullEnabled)
               {
                  Window->draw(Curve->ConvexHullVertices.Vertices,
                               Curve->ConvexHullVertices.NumVertices,
                               Curve->ConvexHullVertices.PrimitiveType,
                               MVP);
               }
               
               if (!CurveParams.PointsDisabled)
               {
                  b32 IsSelected = (Entity->Flags & EntityFlag_Selected);
                  u64 SelectedControlPointIndex = Curve->SelectedControlPointIndex;
                  
                  if (!CurveParams.CubicBezierHelpersDisabled &&
                      IsSelected && SelectedControlPointIndex != U64_MAX &&
                      CurveParams.CurveShape.InterpolationType == Interpolation_Bezier &&
                      CurveParams.CurveShape.BezierType == Bezier_Cubic)
                  {
                     f32 HelperLineWidth =
                        ClipSpaceLengthToWorldSpace(Editor->Parameters.CubicBezierHelperLineWidthClipSpace,
                                                    CoordinateSystemData);
                     
                     RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex,
                                                            true, true, CurveParams.PointSize,
                                                            CurveParams.PointColor, HelperLineWidth,
                                                            CurveParams.CurveColor, MVP, Window);
                     if (SelectedControlPointIndex >= 1)
                     {
                        RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex - 1,
                                                               false, true, CurveParams.PointSize,
                                                               CurveParams.PointColor, HelperLineWidth,
                                                               CurveParams.CurveColor, MVP, Window);
                     }
                     if (SelectedControlPointIndex + 1 < Curve->ControlPointCount)
                     {
                        RenderCubicBezierPointsWithHelperLines(Curve, SelectedControlPointIndex + 1,
                                                               true, false, CurveParams.PointSize,
                                                               CurveParams.PointColor, HelperLineWidth,
                                                               CurveParams.CurveColor, MVP, Window);
                     }
                  }
                  
                  f32 NormalControlPointRadius = CurveParams.PointSize;
                  color ControlPointColor = CurveParams.PointColor;
                  
                  f32 ControlPointOutlineThicknessScale = 0.0f;
                  color NormalControlPointOutlineColor = {};
                  if (IsSelected)
                  {
                     ControlPointOutlineThicknessScale = Editor->Parameters.SelectedCurveControlPointOutlineThicknessScale;
                     NormalControlPointOutlineColor = Editor->Parameters.SelectedCurveControlPointOutlineColor;
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
                        ControlPointRadius *= Editor->Parameters.LastControlPointSizeMultiplier;
                     }
                     
                     color ControlPointOutlineColor = NormalControlPointOutlineColor;
                     if (ControlPointIndex == SelectedControlPointIndex)
                     {
                        ControlPointOutlineColor = Editor->Parameters.SelectedControlPointOutlineColor;
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
               
               
               bezier_curve_degree_lowering *Lowering = &Editor->State.DegreeLowering;
               if (Lowering->Entity == Entity)
               {
                  UpdateAndRenderDegreeLowering(Lowering, VP, Editor->Window);
               }
               
               de_casteljau_visualization *Visualization = &Editor->State.DeCasteljauVisualization;
               if (Visualization->IsVisualizing && Visualization->CurveEntity == Entity)
               {
                  UpdateAndRenderDeCasteljauVisualization(Visualization, VP, Editor->Window);
               }
               
               transform_curve_animation *Animation = &Editor->State.CurveAnimation;
               if (Animation->Stage == AnimateCurveAnimation_Animating && Animation->FromCurveEntity == Entity)
               {
                  Window->draw(Animation->AnimatedCurveVertices.Vertices,
                               Animation->AnimatedCurveVertices.NumVertices,
                               Animation->AnimatedCurveVertices.PrimitiveType,
                               VP);
               }
               
               curve_combining *Combining = &Editor->State.CurveCombining;
               if (Combining->IsCombining && Combining->CombineCurveEntity == Entity)
               {
                  UpdateAndRenderCurveCombining(Editor, VP);
               }
               
               splitting_bezier_curve *Splitting = &Editor->State.SplittingBezierCurve;
               if (Splitting->IsSplitting && Splitting->SplitCurveEntity == Entity)
               {
                  UpdateAndRenderSplittingBezierCurve(Editor, CoordinateSystemData, VP);
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
   CameraUpdate(&Editor->State.Camera, Input->MouseWheelDelta, DeltaTime);
   UpdateAndRenderNotifications(&Editor->Notifications, DeltaTime, Editor->Window);
   
   {
      TimeBlock("editor state update");
      
      coordinate_system_data CoordinateSystemData {
         .Window = Editor->Window,
         .Camera = Editor->State.Camera,
         .Projection = Editor->Projection
      };
      
      user_action Action = {};
      Action.Input = Input;
      editor_state NewState = Editor->State;
      for (u64 ButtonIndex = 0;
           ButtonIndex < Button_Count;
           ++ButtonIndex)
      {
         button Button = Cast(button)ButtonIndex;
         button_state *ButtonState = &Input->Buttons[ButtonIndex];
         
         if (ClickedWithButton(ButtonState, CoordinateSystemData))
         {
            Action.Type = UserAction_ButtonClicked;
            Action.Click.Button = Button;
            Action.Click.ClickPosition = ButtonState->ReleasePosition;
         }
         else if (DraggedWithButton(ButtonState, Input->MousePosition, CoordinateSystemData))
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
         Editor->State = EditorStateUpdate(Editor->State, Action, CoordinateSystemData, Editor->Parameters);
      }
      
      if (Input->MouseLastPosition != Input->MousePosition)
      {
         Action.Type = UserAction_MouseMove;
         Action.MouseMove.FromPosition = Input->MouseLastPosition;
         Action.MouseMove.ToPosition = Input->MousePosition;
         
         Editor->State = EditorStateUpdate(Editor->State, Action, CoordinateSystemData, Editor->Parameters);
      }
   }
   
   // TODO(hbr): Remove this
   if (PressedWithKey(Input->Keys[Key_E], 0))
   {
      AddNotificationF(&Editor->Notifications, Notification_Error, "siema");
   }
   
   coordinate_system_data CoordinateSystemData {
      .Window = Editor->Window,
      .Camera = Editor->State.Camera,
      .Projection = Editor->Projection
   };
   
   if (Editor->State.SelectedEntity)
   {
      RenderSelectedEntityUI(Editor, CoordinateSystemData);
   }
   
   if (Editor->UI_Config.ViewListOfEntitiesWindow)
   {
      RenderListOfEntitiesWindow(Editor, CoordinateSystemData);
   }
   
   if (Editor->UI_Config.ViewParametersWindow)
   {
      RenderParamtersWindow(Editor);
   }
   
   if (Editor->UI_Config.ViewDiagnosticsWindow)
   {
      RenderDiagnosticsWindow(Editor, DeltaTime);
   }
   
   DebugUpdateAndRender(Editor);
   
   // NOTE(hbr): World Space -> Clip Space
   sf::Transform VP = sf::Transform();
   {
      camera Camera = Editor->State.Camera;
      projection Projection = Editor->Projection;
      
      f32 CameraRotationInDegrees = Rotation2DToDegrees(Camera.Rotation);
      
      // NOTE(hbr): World Space -> Camera/View Space
      sf::Transform ViewAnimate =
         sf::Transform()
         .rotate(-CameraRotationInDegrees)
         .scale(Camera.Zoom, Camera.Zoom)
         .translate(-V2F32ToVector2f(Camera.Position));
      
      // NOTE(hbr): Camera Space -> Clip Space
      sf::Transform ProjectionAnimate =
         sf::Transform()
         .scale(1.0f / (0.5f * Projection.FrustumSize * Projection.AspectRatio),
                1.0f / (0.5f * Projection.FrustumSize));
      
      VP = ProjectionAnimate * ViewAnimate;
   }
   
   UpdateAndRenderEntities(Editor, DeltaTime, CoordinateSystemData, VP);
   
   RenderRotationIndicator(&Editor->State, &Editor->Parameters,
                           CoordinateSystemData, VP, Editor->Window);
   
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

int main()
{
   InitThreadCtx();
   InitProfiler();
   
   sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
   sf::ContextSettings ContextSettings = sf::ContextSettings();
   ContextSettings.antialiasingLevel = 4;
   sf::RenderWindow Window(VideoMode,
                           WINDOW_TITLE,
                           sf::Style::Default,
                           ContextSettings);
   
   SetNormalizedDeviceCoordinatesView(&Window);
   
#if 0
#if not(BUILD_DEBUG)
   Window.setFramerateLimit(60);
#endif
#endif
   
   if (Window.isOpen())
   {
      bool ImGuiInitSuccess = ImGui::SFML::Init(Window, true);
      if (ImGuiInitSuccess)
      {
         pool *EntityPool = AllocPool(entity);
         arena *DeCasteljauVisualizationArena = AllocArena();
         arena *DegreeLoweringArena = AllocArena();
         arena *MovingPointArena = AllocArena();
         arena *CurveAnimationArena = AllocArena();
         editor_state InitialEditorState = CreateDefaultEditorState(EntityPool,
                                                                    DeCasteljauVisualizationArena,
                                                                    DegreeLoweringArena,
                                                                    MovingPointArena,
                                                                    CurveAnimationArena);
         
         f32 AspectRatio = CalculateAspectRatio(VideoMode.width, VideoMode.height);
         projection InitialProjection {
            .AspectRatio = AspectRatio,
            .FrustumSize = 2.0f,
         };
         
         notifications Notifications = {};
         Notifications.Window = &Window;
         
         editor_params InitialEditorParameters {
            .RotationIndicator = {
               .RadiusClipSpace = ROTATION_INDICATOR_DEFAULT_RADIUS_CLIP_SPACE,
               .OutlineThicknessFraction = ROTATION_INDICATOR_DEFAULT_OUTLINE_THICKNESS_FRACTION,
               .FillColor = ROTATION_INDICATOR_DEFAULT_FILL_COLOR,
               .OutlineColor = ROTATION_INDICATOR_DEFAULT_OUTLINE_COLOR,
            },
            .BezierSplitPoint = {
               .RadiusClipSpace = BEZIER_SPLIT_POINT_DEFAULT_RADIUS_CLIP_SPACE,
               .OutlineThicknessFraction = BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_THICKNESS_FRACTION,
               .FillColor = BEZIER_SPLIT_POINT_DEFAULT_FILL_COLOR,
               .OutlineColor = BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_COLOR,
            },
            .BackgroundColor = DEFAULT_BACKGROUND_COLOR,
            .CollisionToleranceClipSpace = DEFAULT_COLLLISION_TOLERANCE_CLIP_SPACE,
            .LastControlPointSizeMultiplier = LAST_CONTROL_POINT_DEFAULT_SIZE_MULTIPLIER,
            .SelectedCurveControlPointOutlineThicknessScale = SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_THICKNESS_SCALE,
            .SelectedCurveControlPointOutlineColor = SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_COLOR,
            .SelectedControlPointOutlineColor = SELECTED_CONTROL_POINT_DEFAULT_OUTLINE_COLOR,
            .CubicBezierHelperLineWidthClipSpace = CUBIC_BEZIER_HELPER_LINE_DEFAULT_WIDTH_CLIP_SPACE,
            .CurveDefaultParams = CURVE_DEFAULT_CURVE_PARAMS,
            .DefaultControlPointWeight = CURVE_DEFAULT_CONTROL_POINT_WEIGHT
         };
         
         ui_config InitialUI_Config {
            .ViewSelectedEntityWindow = true,
            .ViewListOfEntitiesWindow = true,
            .ViewParametersWindow = false,
            .ViewDiagnosticsWindow = false,
            .DefaultCurveHeaderCollapsed = false,
            .RotationIndicatorHeaderCollapsed = false,
            .BezierSplitPointHeaderCollapsed = false,
            .OtherSettingsHeaderCollapsed = false,
#if EDITOR_PROFILER
            .ViewProfilerWindow = true,
#endif
#if BUILD_DEBUG
            .ViewDebugWindow = true,
#endif
         };
         
         editor Editor = {};
         Editor.Window = &Window;
         Editor.FrameStats = CreateFrameStats();
         Editor.State = InitialEditorState;
         Editor.Projection = InitialProjection;
         Editor.Notifications = Notifications;
         Editor.Parameters = InitialEditorParameters;
         Editor.UI_Config = InitialUI_Config;
         
         sf::Vector2i MousePosition = sf::Mouse::getPosition(Window);
         
         user_input Input = {};
         Input.MousePosition = V2S32FromVec(MousePosition);
         Input.MouseLastPosition = V2S32FromVec(MousePosition);
         Input.WindowWidth = VideoMode.width;
         Input.WindowHeight = VideoMode.height;
         
         while (Window.isOpen())
         {
            auto SFMLDeltaTime = Editor.DeltaClock.restart();
            f32 DeltaTime = SFMLDeltaTime.asSeconds();
            
            {
               TimeBlock("ImGui::SFML::Update");
               ImGui::SFML::Update(Window, SFMLDeltaTime);
            }
            
            // NOTE(hbr): Beginning profiling frame is inside the loop, because we have only
            // space for one frame and rendering has to be done between ImGui::SFML::Update
            // and ImGui::SFML::Render calls
#if EDITOR_PROFILER
            FrameProfilePoint(&Editor.UI_Config.ViewProfilerWindow);
#endif
            
            HandleEvents(&Window, &Input);
            Editor.Projection.AspectRatio = CalculateAspectRatio(Input.WindowWidth,
                                                                 Input.WindowHeight);
            
            Window.clear(ColorToSFMLColor(Editor.Parameters.BackgroundColor));
            UpdateAndRender(DeltaTime, &Input, &Editor);
            
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
StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors), MakeSureNumberOfProfilePointsFitsIntoArray);