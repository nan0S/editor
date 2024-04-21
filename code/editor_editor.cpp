//- Camera, Projection and Coordinate Spaces
function void
CameraSetZoom(camera *Camera, f32 Zoom)
{
   Camera->Zoom = Zoom;
   Camera->ReachingTarget = false;
}

function void
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

function void
CameraMove(camera *Camera, v2f32 Translation)
{
   Camera->Position += Translation;
   Camera->ReachingTarget = false;
}

function void
CameraRotate(camera *Camera, rotation_2d Rotation)
{
   Camera->Rotation = CombineRotations2D(Camera->Rotation, Rotation);
   Camera->ReachingTarget = false;
}

function world_position
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

function camera_position
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

function camera_position
ScreenToCameraSpace(screen_position Position, coordinate_system_data Data)
{
   v2f32 ClipPosition = V2F32FromVec(Data.Window->mapPixelToCoords(V2S32ToVector2i(Position)));
   
   f32 FrustumExtent = 0.5f * Data.Projection.FrustumSize;
   f32 CameraPositionX = ClipPosition.X * Data.Projection.AspectRatio * FrustumExtent;
   f32 CameraPositionY = ClipPosition.Y * FrustumExtent;
   
   camera_position CameraPosition = V2F32(CameraPositionX, CameraPositionY);
   return CameraPosition;
}

function world_position
ScreenToWorldSpace(screen_position Position, coordinate_system_data Data)
{
   camera_position CameraPosition = ScreenToCameraSpace(Position, Data);
   world_position WorldPosition = CameraToWorldSpace(CameraPosition, Data);
   
   return WorldPosition;
}

function f32
ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, coordinate_system_data Data)
{
   f32 CameraSpaceDistance = ClipSpaceDistance * 0.5f * Data.Projection.FrustumSize;
   f32 WorldSpaceDistance = CameraSpaceDistance / Data.Camera.Zoom;
   
   return WorldSpaceDistance;
}

function collision
CheckCollisionWith(entity *EntitiesHead, entity *EntitiesTail,
                   world_position CheckPosition,
                   f32 CollisionTolerance,
                   editor_parameters EditorParams,
                   check_collision_with_flags CheckCollisionWithFlags)
{
   collision Result = {};
   
   // TODO(hbr): Do one pass over entities instead of multiple ones,
   // it should be more correct to, as everything is sorted according to sorting layer.
   if (CheckCollisionWithFlags & CheckCollisionWith_ControlPoints)
   {
      // NOTE(hbr): Check collisions in reversed drawing order.
      ListIterRev(Entity, EntitiesTail, entity)
      {
         curve *Curve = &Entity->Curve;
         if (Entity->Type == Entity_Curve &&
             !(Entity->Flags & EntityFlag_Hidden) &&
             !Curve->CurveParams.PointsDisabled)
         {
            local_position *ControlPoints = Curve->ControlPoints;
            u64 NumControlPoints = Curve->NumControlPoints;
            f32 NormalPointSize = Curve->CurveParams.PointSize;
            local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
            f32 OutlineThicknessScale = 0.0f;
            if (Entity->Flags & EntityFlag_Selected)
            {
               OutlineThicknessScale = EditorParams.SelectedCurveControlPointOutlineThicknessScale;
            }
            
            for (u64 ControlPointIndex = 0;
                 ControlPointIndex < NumControlPoints;
                 ++ControlPointIndex)
            {
               f32 PointSize = NormalPointSize;
               if (ControlPointIndex == NumControlPoints - 1)
               {
                  PointSize *= EditorParams.LastControlPointSizeMultiplier;
               }
               
               f32 PointSizeWithOutline = (1.0f + OutlineThicknessScale) * PointSize;
               
               if (PointCollision(CheckPositionLocal, ControlPoints[ControlPointIndex],
                                  PointSizeWithOutline + CollisionTolerance))
               {
                  Result = { .Entity = Entity, .Type = CurveCollision_ControlPoint, .PointIndex = ControlPointIndex };
                  goto collision_found_label;
               }
            }
            
            if (!Curve->CurveParams.CubicBezierHelpersDisabled &&
                Curve->SelectedControlPointIndex != U64_MAX &&
                Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier &&
                Curve->CurveParams.CurveShape.BezierType == Bezier_Cubic)
            {
               // TODO(hbr): Maybe don't calculate how many Bezier points there are in all the places, maybe
               u64 SelectedControlPointIndex = Curve->SelectedControlPointIndex;
               local_position *CubicBezierPoints = Curve->CubicBezierPoints;
               f32 CubicBezierPointSize = NormalPointSize + CollisionTolerance;
               u64 NumCubicBezierPoints = 3 * NumControlPoints;
               u64 CenterPoint = 3 * SelectedControlPointIndex + 1;
               
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
                           Result = { .Entity = Entity, .Type = CurveCollision_CubicBezierPoint, .PointIndex = CheckIndex };
                           goto collision_found_label;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
   if (CheckCollisionWithFlags & CheckCollisionWith_CurvePoints)
   {
      // NOTE(hbr): Check collisions in reversed drawing order.
      ListIterRev(Entity, EntitiesTail, entity)
      {
         if (Entity->Type == Entity_Curve && !(Entity->Flags & EntityFlag_Hidden))
         {
            curve *Curve = &Entity->Curve;
            local_position *CurvePoints = Curve->CurvePoints;
            u64 NumCurvePoints = Curve->NumCurvePoints;
            f32 CurveWidth = 2.0f * CollisionTolerance + Curve->CurveParams.CurveWidth;
            local_position CheckPositionLocal = WorldToLocalEntityPosition(Entity, CheckPosition);
            
            for (u64 CurvePointIndex = 0;
                 CurvePointIndex+1 < NumCurvePoints;
                 ++CurvePointIndex)
            {
               v2f32 P1 = CurvePoints[CurvePointIndex];
               v2f32 P2 = CurvePoints[CurvePointIndex+1];
               
               if (SegmentCollision(CheckPositionLocal, P1, P2, CurveWidth))
               {
                  Result = { .Entity = Entity, .Type = CurveCollision_CurvePoint, .PointIndex = CurvePointIndex };
                  goto collision_found_label;
               }
            }
         }
      }
   }
   
   if (CheckCollisionWithFlags & CheckCollisionWith_Images)
   {
      temp_arena Temp = TempArena(0);
      defer { EndTemp(Temp); };
      
      sorted_entity_array SortedEntities = SortEntitiesBySortingLayer(Temp.Arena, EntitiesHead);
      
      // NOTE(hbr): Check collisions in reversed drawing order.
      for (u64 EntityIndex = SortedEntities.EntityCount;
           EntityIndex > 0;
           --EntityIndex)
      {
         entity *Entity = SortedEntities.Entries[EntityIndex - 1].Entity;
         if (Entity->Type == Entity_Image && !(Entity->Flags & EntityFlag_Hidden))
         {
            image *Image = &Entity->Image;
            
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
            
            if (-Extents.X <= CheckPositionInImageSpace.X &&
                CheckPositionInImageSpace.X <= Extents.X &&
                -Extents.Y <= CheckPositionInImageSpace.Y &&
                CheckPositionInImageSpace.Y <= Extents.Y)
            {
               Result = { .Entity = Entity, .Type = Cast(curve_collision_type)0, .PointIndex = 0 };
               goto collision_found_label;
            }
         }
      }
   }
   
   collision_found_label:
   return Result;
}

//- Editor
function user_action
CreateClickedAction(button Button, screen_position ClickPosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonClicked;
   Result.Click.Button = Button;
   Result.Click.ClickPosition = ClickPosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
CreateDragAction(button Button, screen_position DragFromPosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonDrag;
   Result.Drag.Button = Button;
   Result.Drag.DragFromPosition = DragFromPosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
CreateReleasedAction(button Button, screen_position ReleasePosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonReleased;
   Result.Release.Button = Button;
   Result.Release.ReleasePosition = ReleasePosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
CreateMouseMoveAction(v2s32 FromPosition, screen_position ToPosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_MouseMove;
   Result.MouseMove.FromPosition = FromPosition;
   Result.MouseMove.ToPosition = ToPosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function editor_mode
EditorModeNormal(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Normal;
   
   return Result;
}

function f32
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
      case Animation_Count: { InvalidPath; } break;
   }
   
   return Result;
}

function char const *
AnimationToString(animation_type Animation)
{
   switch (Animation)
   {
      case Animation_Smooth: return "Smooth";
      case Animation_Linear: return "Linear";
      case Animation_Count: { InvalidPath; } break;
   }
   
   return "invalid";
}

function editor_state
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

function editor_state
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

function void
DestroyEditorState(editor_state *EditorState)
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

function entity *
AllocateAndAddEntity(editor_state *State)
{
   entity *Entity = PoolAllocStructNonZero(State->EntityPool, entity);
   
   // NOTE(hbr): Ugly C++ thing we have to do because of constructors/destructors.
   new (&Entity->Image.Texture) sf::Texture();
   
   DLLPushBack(State->EntitiesHead, State->EntitiesTail, Entity);
   ++State->NumEntities;
   
   return Entity;
}

function void
DeallocateAndRemoveEntity(editor_state *State, entity *Entity)
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
   
   if (State->DegreeLowering.IsLowering &&
       State->DegreeLowering.CurveEntity == Entity)
   {
      State->DegreeLowering.IsLowering = false;
      State->DegreeLowering.CurveEntity = 0;
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

function void
DeselectCurrentEntity(editor_state *State)
{
   entity *Entity = State->SelectedEntity;
   if (Entity)
   {
      Entity->Flags &= ~EntityFlag_Selected;
   }
   State->SelectedEntity = 0;
}

function void
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

function notification
NotificationMake(string Title, color TitleColor, string Text, f32 PosY)
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

function void
NotificationDestroy(notification *Notification)
{
   FreeStr(&Notification->Title);
   FreeStr(&Notification->Text);
}

function void
AddNotificationF(notifications *Notifs, notification_type Type, char const *Fmt, ...)
{
   va_list Args;
   DeferBlock(va_start(Args, Fmt), va_end(Args))
   {
      temp_arena Temp = TempArena(0);
      defer { EndTemp(Temp); };
      
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
         
         Notifs->Notifs[Notifs->NotifCount] = NotificationMake(Title, Color, Text, PosY);;
         Notifs->NotifCount += 1;
      }
   }
}

function void
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
      defer { EndTemp(Temp); };
      
      string SaveProjectFileName = StringChopFileNameWithoutExtension(Temp.Arena,
                                                                      SaveProjectFilePath);
      Editor->Window->setTitle(SaveProjectFileName.Data);
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
ReleasedButton(button_state ButtonState)
{
   b32 Result = ButtonState.WasPressed && !ButtonState.Pressed;
   return Result;
}

internal b32
ClickedWithButton(button_state ButtonState,
                  coordinate_system_data CoordinateSystemData)
{
   b32 Result = ButtonState.WasPressed && !ButtonState.Pressed &&
      ScreenPointsAreClose(ButtonState.PressPosition,
                           ButtonState.ReleasePosition,
                           CoordinateSystemData);
   
   return Result;
}

internal b32
DraggedWithButton(button_state ButtonState,
                  v2s32 MousePosition,
                  coordinate_system_data CoordinateSystemData)
{
   b32 Result = ButtonState.Pressed &&
      !ScreenPointsAreClose(ButtonState.PressPosition,
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
CreateMovingCurvePointMode(entity *CurveEntity,
                           u64 PointIndex,
                           b32 CubicBezierPointMoving,
                           arena *Arena)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Moving.Type = MovingMode_CurvePoint;
   Result.Moving.Entity = CurveEntity;
   Result.Moving.PointIndex = PointIndex;
   Result.Moving.CubicBezierPointMoving = CubicBezierPointMoving;
   
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

// TODO(hbr): Merge [RotatingCurve] with [RotatingImage] functions (at least)
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
                            editor_parameters EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_ButtonClicked: {
         button ClickButton = Action.Click.Button;
         world_position ClickPosition = ScreenToWorldSpace(Action.Click.ClickPosition,
                                                           CoordinateSystemData);
         
         switch (ClickButton)
         {
            case Button_Left: {
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                                                    CoordinateSystemData);
               
               collision Collision = {};
               if (Action.UserInput->Keys[Key_LeftShift].Pressed)
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
                  Collision = CheckCollisionWith(State.EntitiesHead, State.EntitiesTail,
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
                           u64 NumPoints = ((Collision.Type == CurveCollision_ControlPoint) ? Curve->NumControlPoints : Curve->NumCurvePoints);
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
                                   Result.CurveCombining.TargetCurveEntity);
                              Swap(Result.CurveCombining.CombineCurveLastControlPoint,
                                   Result.CurveCombining.TargetCurveFirstControlPoint);
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
                        else if (CollisionPointIndex == CombineCurve->NumControlPoints - 1)
                        {
                           if (State.CurveCombining.CombineCurveLastControlPoint)
                           {
                              Swap(Result.CurveCombining.CombineCurveEntity,
                                   Result.CurveCombining.TargetCurveEntity);
                              Swap(Result.CurveCombining.CombineCurveLastControlPoint,
                                   Result.CurveCombining.TargetCurveFirstControlPoint);
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
                        else if (CollisionPointIndex == TargetCurve->NumControlPoints - 1)
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
                           entity *CurveEntity = Collision.Entity;
                           curve *Curve = &CurveEntity->Curve;
                           
                           f32 Segment = Cast(f32)Collision.PointIndex/ (Curve->NumCurvePoints - 1);
                           u64 InsertAfterPointIndex =
                              Cast(u64)(Segment * (CurveIsLooped(Curve->CurveParams.CurveShape) ?
                                                   Curve->NumControlPoints :
                                                   Curve->NumControlPoints - 1));
                           InsertAfterPointIndex = Clamp(InsertAfterPointIndex, 0, Curve->NumControlPoints - 1);
                           
                           CurveInsertControlPoint(CurveEntity, ClickPosition, InsertAfterPointIndex,
                                                   EditorParams.DefaultControlPointWeight);
                           Curve->SelectedControlPointIndex = InsertAfterPointIndex + 1;
                           SelectEntity(&Result, CurveEntity);
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
                        entity *Entity = AllocateAndAddEntity(&Result);
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
                     
                     u64 AddedPointIndex = CurveAppendControlPoint(AddCurveEntity, ClickPosition, EditorParams.DefaultControlPointWeight);
                     AddCurveEntity->Curve.SelectedControlPointIndex = AddedPointIndex;
                     SelectEntity(&Result, AddCurveEntity);
                  }
               }
            } break;
            
            case Button_Right: {
               check_collision_with_flags CheckCollisionWithFlags =
                  CheckCollisionWith_ControlPoints | CheckCollisionWith_Images;
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                                                    CoordinateSystemData);
               collision Collision = CheckCollisionWith(State.EntitiesHead, State.EntitiesTail,
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
                              CurveRemoveControlPoint(CurveFromEntity(Collision.Entity), Collision.PointIndex);
                              SelectEntity(&Result, Collision.Entity);
                           } break;
                           
                           case CurveCollision_CubicBezierPoint: {
                              NotImplemented;
                           } break;
                           
                           case CurveCollision_CurvePoint: { InvalidPath; } break;
                        }
                     } break;
                     
                     case Entity_Image: {
                        DeallocateAndRemoveEntity(&Result, Collision.Entity);
                     } break;
                  }
               }
               else
               {
                  DeselectCurrentEntity(&Result);
               }
            } break;
            
            case Button_Middle: {} break;
            case Button_Count: { Assert(false); } break;
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
                                                           State.EntitiesTail,
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
                                 curve *Curve = CurveFromEntity(Collision.Entity);
                                 u64 PointIndex = Collision.PointIndex;
                                 b32 IsCubicBezierPoint = (Collision.Type == CurveCollision_CubicBezierPoint);
                                 
                                 u64 SelectControlPointIndex = (IsCubicBezierPoint ? Curve->SelectedControlPointIndex : PointIndex);
                                 Curve->SelectedControlPointIndex = SelectControlPointIndex;
                                 SelectEntity(&Result, Collision.Entity);
                                 Result.Mode = CreateMovingCurvePointMode(Collision.Entity, PointIndex, IsCubicBezierPoint, State.MovingPointArena);
                              } break;
                              
                              case CurveCollision_CurvePoint: {
                                 SelectEntity(&Result, Collision.Entity);
                                 Result.Mode = CreateMovingEntityMode(Collision.Entity);
                              } break;
                              
                           }
                        } break;
                        
                        case Entity_Image: {
                           entity *Entity = Collision.Entity;
                           SelectEntity(&Result, Entity);
                           Result.Mode = CreateMovingEntityMode(Entity);
                        } break;
                     }
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
            case Button_Count: { Assert(false); } break;
         }
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Drag.Button)
         {
            case Button_Left: { Result.SplittingBezierCurve.DraggingSplitPoint = false; } break;
            case Button_Right:
            case Button_Middle: {} break;
            case Button_Count: { Assert(false); } break;
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
            u64 NumCurvePoints = Curve->NumCurvePoints;
            local_position *CurvePoints = Curve->CurvePoints;
            
            f32 T = Clamp(Split->T, 0.0f, 1.0f);
            f32 DeltaT = 1.0f / (NumCurvePoints - 1);
            
            {
               u64 SplitCurvePointIndex = ClampTop(Cast(u64)FloorF32(T * (NumCurvePoints - 1)), NumCurvePoints - 1);
               while (SplitCurvePointIndex + 1 < NumCurvePoints)
               {
                  f32 NextT = Cast(f32)(SplitCurvePointIndex + 1) / (NumCurvePoints - 1);
                  f32 PrevT = Cast(f32)SplitCurvePointIndex / (NumCurvePoints - 1);
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
               u64 SplitCurvePointIndex = ClampTop(Cast(u64)CeilF32(T * (NumCurvePoints - 1)), NumCurvePoints - 1);
               while (SplitCurvePointIndex >= 1)
               {
                  f32 NextT = Cast(f32)(SplitCurvePointIndex - 1) / (NumCurvePoints - 1);
                  f32 PrevT = Cast(f32)SplitCurvePointIndex / (NumCurvePoints - 1);
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
            b32 MatchCubicBezierHelpersDirection = (!Action.UserInput->Keys[Key_LeftCtrl].Pressed);
            b32 MatchCubicBezierHelpersLength = (!Action.UserInput->Keys[Key_LeftShift].Pressed);
            
            CurveTranslatePoint(State.Mode.Moving.Entity,
                                State.Mode.Moving.PointIndex,
                                State.Mode.Moving.CubicBezierPointMoving,
                                MatchCubicBezierHelpersDirection,
                                MatchCubicBezierHelpersLength,
                                Translation);
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
               
               case MovingMode_CurvePoint: { InvalidPath; } break;
            }
         }
         
      } break;
      
      case UserAction_ButtonReleased: {
         switch (Action.Release.Button)
         {
            case Button_Left:
            case Button_Middle: { Result.Mode = EditorModeNormal(); } break;
            case Button_Right: {} break;
            case Button_Count: { InvalidPath; } break;
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
                          editor_parameters Params)
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
                              editor_parameters EditorParams)
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
            case Button_Count: { Assert(false); } break;
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
                  editor_parameters EditorParams)
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
RenderRotationIndicator(editor_state *EditorState, editor_parameters *EditorParams,
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
      
      case SaveProjectFormat_None: {
         // NOTE(hbr): Impossible, all cases should be handled.
         Assert(false);
      } break;
      
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
   entity *Copy = AllocateAndAddEntity(&Editor->State);
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

// NOTE(hbr): This should really be some highly local function, don't expect to reuse.
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

// NOTE(hbr): This should really be some highly local function, don't expect to reuse.
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
            u64 *NumCurvePointsPerSegment = &ChangeParams->NumCurvePointsPerSegment;
            
            int NumCurvePointsPerSegmentAsInt = Cast(int)(*NumCurvePointsPerSegment);
            SomeCurveParameterChanged |= ImGui::SliderInt("Detail",
                                                          &NumCurvePointsPerSegmentAsInt,
                                                          1,    // v_min
                                                          2000, // v_max
                                                          "%d", // v_format
                                                          ImGuiSliderFlags_Logarithmic);
            *NumCurvePointsPerSegment = Cast(u64)NumCurvePointsPerSegmentAsInt;
            
            if (ResetContextMenuItemSelected("DetailReset"))
            {
               *NumCurvePointsPerSegment = DefaultParams.NumCurvePointsPerSegment;
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
            local f32 ControlPointMinimumWeight = EPS_F32;
            local f32 ControlPointMaximumWeight = FLT_MAX;
            local char const *ControlPointWeightFormat = "%.2f";
            
            switch (UIMode.Type)
            {
               case ChangeCurveParamsUIMode_DefaultCurve: {
                  ImGui::DragFloat("Point Weight",
                                   UIMode.DefaultControlPointWeight,
                                   ControlPointWeightDragSpeed,
                                   ControlPointMinimumWeight,
                                   ControlPointMaximumWeight,
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
                     
                     u64 NumControlPoints = SelectedCurve->NumControlPoints;
                     f32 *ControlPointWeights = SelectedCurve->ControlPointWeights;
                     u64 SelectedControlPointIndex = SelectedCurve->SelectedControlPointIndex;
                     
                     if (SelectedControlPointIndex < NumControlPoints)
                     {
                        char ControlPointWeightLabel[128];
                        sprintf(ControlPointWeightLabel, "Selected Point (%llu)",
                                SelectedControlPointIndex);
                        
                        f32 *SelectedControlPointWeight = &ControlPointWeights[SelectedControlPointIndex];
                        
                        SomeCurveParameterChanged |=
                           ImGui::DragFloat(ControlPointWeightLabel,
                                            SelectedControlPointWeight,
                                            ControlPointWeightDragSpeed,
                                            ControlPointMinimumWeight,
                                            ControlPointMaximumWeight,
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
                             ControlPointIndex < NumControlPoints;
                             ++ControlPointIndex)
                        {
                           DeferBlock(ImGui::PushID(Cast(int)ControlPointIndex), ImGui::PopID())
                           {                              
                              char ControlPointWeightLabel[128];
                              sprintf(ControlPointWeightLabel, "Point (%llu)", ControlPointIndex);
                              
                              f32 *ControlPointWeight = &ControlPointWeights[ControlPointIndex];
                              
                              SomeCurveParameterChanged |=
                                 ImGui::DragFloat(ControlPointWeightLabel,
                                                  ControlPointWeight,
                                                  ControlPointWeightDragSpeed,
                                                  ControlPointMinimumWeight,
                                                  ControlPointMaximumWeight,
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
BeginVisualizingDeCasteljauAlgorithm(de_Casteljau_visualization *Visualization, entity *CurveEntity)
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

internal void
LowerBezierCurveDegree(bezier_curve_degree_lowering *Lowering, entity *CurveEntity)
{
   curve *Curve = &CurveEntity->Curve;
   
   u64 NumControlPoints = Curve->NumControlPoints;
   if (NumControlPoints > 0)
   {
      Assert(Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      
      temp_arena Temp = TempArena(Lowering->Arena);
      defer { EndTemp(Temp); };
      
      local_position *NewControlPoints = PushArrayNonZero(Temp.Arena,
                                                          NumControlPoints,
                                                          local_position);
      f32 *NewControlPointWeights = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
      local_position *NewCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                              3 * (NumControlPoints - 1),
                                                              local_position);
      MemoryCopy(NewControlPoints,
                 Curve->ControlPoints,
                 NumControlPoints * SizeOf(NewControlPoints[0]));
      MemoryCopy(NewControlPointWeights,
                 Curve->ControlPointWeights,
                 NumControlPoints * SizeOf(NewControlPointWeights[0]));
      
      bezier_lower_degree LowerDegree = {};
      b32 IncludeWeights = false;
      switch (Curve->CurveParams.CurveShape.BezierType)
      {
         case Bezier_Normal: {
            LowerDegree = BezierCurveLowerDegree(NewControlPoints, NumControlPoints);
         } break;
         
         case Bezier_Weighted: {
            IncludeWeights = true;
            LowerDegree = BezierWeightedCurveLowerDegree(NewControlPoints,
                                                         NewControlPointWeights,
                                                         NumControlPoints);
         } break;
         
         case Bezier_Cubic: { Assert(false); } break;
      }
      
      if (LowerDegree.Failure)
      {
         Lowering->IsLowering = true;
         Lowering->CurveEntity = CurveEntity;
         
         ClearArena(Lowering->Arena);
         
         Lowering->SavedControlPoints = PushArrayNonZero(Lowering->Arena, NumControlPoints, local_position);
         Lowering->SavedControlPointWeights = PushArrayNonZero(Lowering->Arena, NumControlPoints, f32);
         Lowering->SavedCubicBezierPoints = PushArrayNonZero(Lowering->Arena, 3 * NumControlPoints, local_position);
         MemoryCopy(Lowering->SavedControlPoints,
                    Curve->ControlPoints,
                    NumControlPoints * SizeOf(Lowering->SavedControlPoints[0]));
         MemoryCopy(Lowering->SavedControlPointWeights,
                    Curve->ControlPointWeights,
                    NumControlPoints * SizeOf(Lowering->SavedControlPointWeights[0]));
         MemoryCopy(Lowering->SavedCubicBezierPoints,
                    Curve->CubicBezierPoints,
                    3 * NumControlPoints * SizeOf(Lowering->SavedCubicBezierPoints[0]));
         
         line_vertices CurveVertices = Curve->CurveVertices;
         Lowering->NumSavedCurveVertices = CurveVertices.NumVertices;
         Lowering->SavedCurveVertices = PushArrayNonZero(Lowering->Arena, CurveVertices.NumVertices, sf::Vertex);
         Lowering->SavedPrimitiveType = CurveVertices.PrimitiveType;
         MemoryCopy(Lowering->SavedCurveVertices,
                    CurveVertices.Vertices,
                    CurveVertices.NumVertices * SizeOf(Lowering->SavedCurveVertices[0]));
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
         
         NewControlPoints[LowerDegree.MiddlePointIndex] =
            Lowering->P_Mix * LowerDegree.P_I + (1 - Lowering->P_Mix) * LowerDegree.P_II;
         if (IncludeWeights)
         {
            NewControlPointWeights[LowerDegree.MiddlePointIndex] =
               Lowering->W_Mix * LowerDegree.W_I + (1 - Lowering->W_Mix) * LowerDegree.W_II;
         }
         
         // TODO(hbr): refactor this, it only has to be here because we still modify control points above
         BezierCubicCalculateAllControlPoints(NewControlPoints,
                                              NumControlPoints - 1,
                                              NewCubicBezierPoints + 1);
         
         CurveSetControlPoints(Curve,
                               NumControlPoints - 1,
                               NewControlPoints,
                               NewControlPointWeights,
                               NewCubicBezierPoints);
         
         Lowering->SavedCurveVersion = Curve->CurveVersion;
      }
      else
      {
         BezierCubicCalculateAllControlPoints(NewControlPoints,
                                              NumControlPoints - 1,
                                              NewCubicBezierPoints + 1);
         
         CurveSetControlPoints(Curve,
                               NumControlPoints - 1,
                               NewControlPoints,
                               NewControlPointWeights,
                               NewCubicBezierPoints);
      }
   }
}

internal void
ElevateBezierCurveDegree(curve *Curve, editor_parameters *EditorParams)
{
   Assert(Curve->CurveParams.CurveShape.InterpolationType ==
          Interpolation_Bezier);
   
   temp_arena Temp = TempArena(0);
   defer { EndTemp(Temp); };
   
   u64 NumControlPoints = Curve->NumControlPoints;
   local_position *ElevatedControlPoints = PushArrayNonZero(Temp.Arena,
                                                            NumControlPoints + 1,
                                                            local_position);
   f32 *ElevatedControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                       NumControlPoints + 1,
                                                       f32);
   MemoryCopy(ElevatedControlPoints,
              Curve->ControlPoints,
              NumControlPoints * SizeOf(ElevatedControlPoints[0]));
   MemoryCopy(ElevatedControlPointWeights,
              Curve->ControlPointWeights,
              NumControlPoints * SizeOf(ElevatedControlPointWeights[0]));
   
   switch (Curve->CurveParams.CurveShape.BezierType)
   {
      case Bezier_Normal: {
         BezierCurveElevateDegree(ElevatedControlPoints,
                                  NumControlPoints);
         ElevatedControlPointWeights[NumControlPoints] =
            EditorParams->DefaultControlPointWeight;
      } break;
      
      case Bezier_Weighted: {
         BezierWeightedCurveElevateDegree(ElevatedControlPoints,
                                          ElevatedControlPointWeights,
                                          NumControlPoints);
      } break;
      
      case Bezier_Cubic: { Assert(false); } break;
   }
   
   local_position *ElevatedCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                3 * (NumControlPoints + 1),
                                                                local_position);
   BezierCubicCalculateAllControlPoints(ElevatedControlPoints,
                                        NumControlPoints + 1,
                                        ElevatedCubicBezierPoints + 1);
   
   CurveSetControlPoints(Curve,
                         NumControlPoints + 1,
                         ElevatedControlPoints,
                         ElevatedControlPointWeights,
                         ElevatedCubicBezierPoints);
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
   Camera->ZoomTarget = Minimum(ZoomX, ZoomY);
   
   Camera->ReachingTarget = true;
}

internal void
FocusCameraOnEntity(camera *Camera, entity *Entity, coordinate_system_data CoordinateSystemData)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         if (Curve->NumControlPoints > 0)
         {
            f32 Left = INF_F32, Right = -INF_F32;
            f32 Down = INF_F32, Top = -INF_F32;
            for (u64 ControlPointIndex = 0;
                 ControlPointIndex < Curve->NumControlPoints;
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
   Assert(Curve->SelectedControlPointIndex < Curve->NumControlPoints);
   
   temp_arena Temp = TempArena(0);
   defer { EndTemp(Temp); };
   
   u64 NumLeftControlPoints = Curve->SelectedControlPointIndex + 1;
   u64 NumRightControlPoints = Curve->NumControlPoints - Curve->SelectedControlPointIndex;
   
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
   // TODO(hbr): Maybe mint functions that create duplicate curve but without allocating and with allocating.
   entity *LeftCurveEntity = CurveEntity;
   LeftCurveEntity->Name = NameStrF("%s (left)", CurveEntity->Name.Data);
   CurveSetControlPoints(&LeftCurveEntity->Curve, NumLeftControlPoints, LeftControlPoints,
                         LeftControlPointWeights, LeftCubicBezierPoints);
   
   entity *RightCurveEntity = AllocateAndAddEntity(EditorState);
   InitEntityFromEntity(RightCurveEntity, LeftCurveEntity);
   RightCurveEntity->Name = NameStrF("%s (right)", CurveEntity->Name.Data);
   CurveSetControlPoints(&RightCurveEntity->Curve, NumRightControlPoints, RightControlPoints,
                         RightControlPointWeights, RightCubicBezierPoints);
}

// TODO(hbr): Do a pass oveer this function to simplify the logic maybe (and simplify how the UI looks like in real life)
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
            
            bool Delete           = false;
            bool Copy             = false;
            bool SwitchVisibility = false;
            bool Deselect         = false;
            bool FocusOn          = false;
            
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
                                                  Curve->NumControlPoints),
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
                     DeferBlock(ImGui::BeginDisabled(Curve->NumControlPoints < 2),
                                ImGui::EndDisabled())
                     {
                        ImGui::SameLine();
                        SplitBezierCurve = ImGui::Button("Split");
                     }
                     
                     ElevateBezierCurve = ImGui::Button("Elevate Degree");
                     
                     DeferBlock(ImGui::BeginDisabled(Curve->NumControlPoints == 0),
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
               DeallocateAndRemoveEntity(&Editor->State, Entity);
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
               CurveRecompute(Curve);
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
            else if (LifeTime <= NOTIFICATION_FADE_IN_TIME +
                     NOTIFICATION_PROPER_LIFE_TIME)
            {
               Fade = 1.0f;
            }
            else if (LifeTime <= NOTIFICATION_FADE_IN_TIME +
                     NOTIFICATION_PROPER_LIFE_TIME +
                     NOTIFICATION_FADE_OUT_TIME)
            {
               Fade = 1.0f - (LifeTime -
                              NOTIFICATION_FADE_IN_TIME -
                              NOTIFICATION_PROPER_LIFE_TIME) / NOTIFICATION_FADE_OUT_TIME;
            }
            else
            {
               Fade = 0.0f;
            }
            // NOTE(hbr): Quadratic interpolation instead of linear.
            Fade = 1.0f - Square(1.0f - Fade);
            Assert(-EPS_F32 <= Fade && Fade <= 1.0f + EPS_F32);
            
            char Label[128];
            sprintf(Label, "Notification#%llu", NotifIndex);
            
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
            NotificationDestroy(Notif);
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
RenderListOfEntitiesWindow(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   bool ViewWindow = Cast(bool)Editor->UI_Config.ViewListOfEntitiesWindow;
   
   DeferBlock(ImGui::Begin("Entities", &ViewWindow), ImGui::End())
   {
      Editor->UI_Config.ViewListOfEntitiesWindow = Cast(b32)ViewWindow;
      
      if (ViewWindow)
      {
         local ImGuiTreeNodeFlags CollapsingHeaderFlags = ImGuiTreeNodeFlags_DefaultOpen;
         local char const *RightClickOnEntityContextMenuLabel = "EntityContextMenu";
         editor_state *EditorState = &Editor->State;
         
         if (ImGui::CollapsingHeader("Curves", CollapsingHeaderFlags))
         {
            DeferBlock(ImGui::PushID("CurveEntities"), ImGui::PopID())
            {
               u64 CurveIndex = 0;
               ListIter(Entity, Editor->State.EntitiesHead, entity)
               {
                  if (Entity->Type == Entity_Curve)
                  {
                     DeferBlock(ImGui::PushID(Cast(int)CurveIndex), ImGui::PopID())
                     {
                        bool DeselectCurveSelected = false;
                        bool DeleteCurveSelected = false;
                        bool CopyCurveSelected = false;
                        bool SwitchVisilitySelected = false;
                        bool RenameCurveSelected = false;
                        bool FocusSelected = false;
                        
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
                              SelectEntity(EditorState, Entity);
                           }
                           
                           b32 DoubleClickedOrSelectedAndClicked = ((Selected && ImGui::IsMouseClicked(0)) ||
                                                                    ImGui::IsMouseDoubleClicked(0));
                           if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
                           {
                              Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
                           }
                           
                           if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                           {
                              ImGui::OpenPopup(RightClickOnEntityContextMenuLabel);
                           }
                           if (ImGui::BeginPopup(RightClickOnEntityContextMenuLabel))
                           {
                              ImGui::MenuItem("Rename", 0, &RenameCurveSelected);
                              ImGui::MenuItem("Delete", 0, &DeleteCurveSelected);
                              ImGui::MenuItem("Copy", 0, &CopyCurveSelected);
                              ImGui::MenuItem((Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide", 0, &SwitchVisilitySelected);
                              if (Selected) ImGui::MenuItem("Deselect", 0, &DeselectCurveSelected);
                              ImGui::MenuItem("Focus", 0, &FocusSelected);
                              
                              ImGui::EndPopup();
                           }
                        }
                        
                        if (RenameCurveSelected)
                        {
                           Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
                        }
                        
                        if (DeleteCurveSelected)
                        {
                           DeallocateAndRemoveEntity(EditorState, Entity);
                        }
                        
                        if (CopyCurveSelected)
                        {
                           DuplicateEntity(Entity, Editor);
                        }
                        
                        if (SwitchVisilitySelected)
                        {
                           Entity->Flags ^= EntityFlag_Hidden;
                        }
                        
                        if (DeselectCurveSelected)
                        {
                           DeselectCurrentEntity(EditorState);
                        }
                        
                        if (FocusSelected)
                        {
                           FocusCameraOnEntity(&EditorState->Camera, Entity, CoordinateSystemData);
                        }
                        
                        CurveIndex += 1;
                     }
                  }
               }
            }
         }
         
         ImGui::Spacing();
         if (ImGui::CollapsingHeader("Images", CollapsingHeaderFlags))
         {
            DeferBlock(ImGui::PushID("ImageEntities"), ImGui::PopID())
            {
               u64 ImageIndex = 0;
               ListIter(Entity, Editor->State.EntitiesHead, entity)
               {
                  if (Entity->Type == Entity_Image)
                  {
                     DeferBlock(ImGui::PushID(Cast(int)ImageIndex), ImGui::PopID())
                     {
                        // TODO(hbr): This is not good way to check if selected
                        b32 Selected = (EditorState->SelectedEntity == Entity);
                        b32 Hidden = (Entity->Flags & EntityFlag_Hidden);
                        
                        bool DeselectImageSelected = false;
                        bool DeleteImageSelected = false;
                        bool CopyImageSelected = false;
                        bool SwitchVisilitySelected = false;
                        bool RenameImageSelected = false;
                        bool FocusSelected = false;
                        
                        if (Entity->RenamingFrame)
                        {
                           // TODO(hbr): Remove this Cast
                           if (Entity->RenamingFrame == Cast(u32)ImGui::GetFrameCount())
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
                           if (ImGui::Selectable(Entity->Name.Data, Selected))
                           {
                              SelectEntity(EditorState, Entity);
                           }
                           
                           b32 DoubleClickedOrSelectedAndClicked = ((Selected && ImGui::IsMouseClicked(0)) ||
                                                                    ImGui::IsMouseDoubleClicked(0));
                           if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
                           {
                              Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
                           }
                           
                           if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                           {
                              ImGui::OpenPopup(RightClickOnEntityContextMenuLabel);
                           }
                           if (ImGui::BeginPopup(RightClickOnEntityContextMenuLabel))
                           {
                              ImGui::MenuItem("Rename", 0, &RenameImageSelected);
                              ImGui::MenuItem("Delete", 0, &DeleteImageSelected);
                              ImGui::MenuItem("Copy", 0, &CopyImageSelected);
                              ImGui::MenuItem(Hidden ? "Show" : "Hide", 0, &SwitchVisilitySelected);
                              if (Selected) ImGui::MenuItem("Deselect", 0, &DeselectImageSelected);
                              ImGui::MenuItem("Focus", 0, &FocusSelected);
                              
                              ImGui::EndPopup();
                           }
                        }
                        
                        if (RenameImageSelected)
                        {
                           Entity->RenamingFrame = ImGui::GetFrameCount() + 1;
                        }
                        
                        if (DeleteImageSelected)
                        {
                           DeallocateAndRemoveEntity(EditorState, Entity);
                        }
                        
                        if (CopyImageSelected)
                        {
                           DuplicateEntity(Entity, Editor);
                        }
                        
                        if (SwitchVisilitySelected)
                        {
                           Entity->Flags ^= EntityFlag_Hidden;
                        }
                        
                        if (DeselectImageSelected)
                        {
                           DeselectCurrentEntity(EditorState);
                        }
                        
                        if (FocusSelected)
                        {
                           FocusCameraOnEntity(&EditorState->Camera, Entity, CoordinateSystemData);
                        }
                        
                        ImageIndex += 1;
                     }
                  }
               }
            }
         }
      }
   }
}

internal void
UpdateAndRenderMenuBar(editor *Editor, user_input UserInput)
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
         
#if EDITOR_DEBUG
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
   
   if (NewProjectSelected || PressedWithKey(UserInput.Keys[Key_N], Modifier_Ctrl))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_NewProject;
   }
   
   if (OpenProjectSelected || PressedWithKey(UserInput.Keys[Key_O], Modifier_Ctrl))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_OpenProject;
   }
   
   if (QuitEditorSelected ||
       PressedWithKey(UserInput.Keys[Key_Q], 0) ||
       PressedWithKey(UserInput.Keys[Key_ESC], 0))
   {
      ImGui::OpenPopup(ConfirmCloseCurrentProjectLabel);
      Editor->ActionWaitingToBeDone = ActionToDo_Quit;
   }
   
   if (SaveProjectSelected || PressedWithKey(UserInput.Keys[Key_S], Modifier_Ctrl))
   {
      if (IsValid(Editor->ProjectSavePath))
      {
         temp_arena Temp = TempArena(0);
         defer { EndTemp(Temp); };
         
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
      }
      else
      {
         FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                               SAVE_AS_MODAL_EXTENSION_SELECTION,
                               ".");
      }
   }
   
   if (SaveProjectAsSelected || PressedWithKey(UserInput.Keys[Key_S],
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
                  defer { EndTemp(Temp); };
                  
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
         defer { EndTemp(Temp); };
         
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
         
         DestroyEditorState(&Editor->State);
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
         defer { EndTemp(Temp); };
         
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
            
            DestroyEditorState(&Editor->State);
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
         defer { EndTemp(Temp); };
         
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
            
            entity *Entity = AllocateAndAddEntity(&Editor->State);
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
         editor_parameters *Params = &Editor->Parameters;
         
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
         ImGui::Text("%20s: %.2f ms", "Minimum frame time", 1000.0f * FrameStats.MinFrameTime);
         ImGui::Text("%20s: %.2f ms", "Maximum frame time", 1000.0f * FrameStats.MaxFrameTime);
         ImGui::Text("%20s: %.2f ms", "Average frame time", 1000.0f * FrameStats.AvgFrameTime);
      }
   }
   
}

internal void
RenderDebugWindow(editor *Editor)
{
   bool ViewDebugWindowAsBool = Cast(bool)Editor->UI_Config.ViewDebugWindow;
   DeferBlock(ImGui::Begin("Debug", &ViewDebugWindowAsBool), ImGui::End())
   {
      Editor->UI_Config.ViewDebugWindow = Cast(b32)ViewDebugWindowAsBool;
      
      if (Editor->UI_Config.ViewDebugWindow)
      {
         ImGui::Text("Number of entities = %lu", Editor->State.NumEntities);
         
         if (Editor->State.SelectedEntity &&
             Editor->State.SelectedEntity->Type == Entity_Curve)
         {
            curve *Curve = &Editor->State.SelectedEntity->Curve;
            
            ImGui::Text("Number of control points = %lu", Curve->NumControlPoints);
            ImGui::Text("Number of curve points = %lu", Curve->NumCurvePoints);
         }
         
         ImGui::Text("Minimum Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MinFrameTime);
         ImGui::Text("Maximum Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MaxFrameTime);
         ImGui::Text("Average Frame Time = %.3fms", 1000.0f * Editor->FrameStats.AvgFrameTime);
      }
   }
}

internal void
UpdateAndRenderSplittingBezierCurve(editor_state *EditorState,
                                    editor_parameters *EditorParams,
                                    coordinate_system_data CoordinateSystemData,
                                    sf::Transform Transform, sf::RenderWindow *Window)
{
   TimeFunction;
   
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
      defer { EndTemp(Temp); };
      
      u64 NumControlPoints = Curve->NumControlPoints;
      curve_shape CurveShape = Curve->CurveParams.CurveShape;
      
      f32 *ControlPointWeights = Curve->ControlPointWeights;
      switch (CurveShape.BezierType)
      {
         case Bezier_Normal: {
            ControlPointWeights = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
            for (u64 I = 0; I < NumControlPoints; ++I)
            {
               ControlPointWeights[I] = 1.0f;
            }
         } break;
         
         case Bezier_Weighted: {} break;
         case Bezier_Cubic: { Assert(false); } break;
      }
      
      local_position *LeftControlPoints = PushArrayNonZero(Temp.Arena,
                                                           NumControlPoints,
                                                           local_position);
      local_position *RightControlPoints = PushArrayNonZero(Temp.Arena,
                                                            NumControlPoints,
                                                            local_position);
      
      f32 *LeftControlPointWeights  = PushArrayNonZero(Temp.Arena,
                                                       NumControlPoints,
                                                       f32);
      f32 *RightControlPointWeights = PushArrayNonZero(Temp.Arena,
                                                       NumControlPoints,
                                                       f32);
      
      BezierCurveSplit(Splitting->T,
                       Curve->ControlPoints,
                       ControlPointWeights,
                       NumControlPoints,
                       LeftControlPoints,
                       LeftControlPointWeights,
                       RightControlPoints,
                       RightControlPointWeights);
      
      local_position *LeftCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                               3 * NumControlPoints,
                                                               local_position);
      local_position *RightCubicBezierPoints = PushArrayNonZero(Temp.Arena,
                                                                3 * NumControlPoints,
                                                                local_position);
      
      BezierCubicCalculateAllControlPoints(LeftControlPoints,
                                           NumControlPoints,
                                           LeftCubicBezierPoints + 1);
      BezierCubicCalculateAllControlPoints(RightControlPoints,
                                           NumControlPoints,
                                           RightCubicBezierPoints + 1);
      
      // TODO(hbr): Optimize here not to do so many copies, maybe merge with split on control point code
      // TODO(hbr): Isn't this similar to split on curve point
      // TODO(hbr): Maybe remove this duplication of initialzation, maybe not?
      // TODO(hbr): Refactor that code into common function that splits curve into two curves
      entity *LeftCurveEntity = CurveEntity;
      LeftCurveEntity->Name = NameStrF("%s (left)", LeftCurveEntity->Name.Data);
      CurveSetControlPoints(&LeftCurveEntity->Curve,
                            NumControlPoints,
                            LeftControlPoints, LeftControlPointWeights, LeftCubicBezierPoints);
      
      entity *RightCurveEntity = AllocateAndAddEntity(EditorState);
      RightCurveEntity->Name = NameStrF("%s (right)", RightCurveEntity->Name.Data);
      InitEntityFromEntity(RightCurveEntity, LeftCurveEntity);
      CurveSetControlPoints(&RightCurveEntity->Curve,
                            NumControlPoints,
                            RightControlPoints, RightControlPointWeights, RightCubicBezierPoints);
      
      Splitting->IsSplitting = false;
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
                                                    Curve->NumControlPoints);
            } break;
            
            case Bezier_Weighted: {
               SplitPoint = BezierWeightedCurveEvaluateFast(Splitting->T,
                                                            Curve->ControlPoints,
                                                            Curve->ControlPointWeights,
                                                            Curve->NumControlPoints);
            } break;
            
            case Bezier_Cubic: { Assert(false); } break;
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
RenderImages(entity *Entities, sf::Transform Transform, sf::RenderWindow *Window)
{
   TimeFunction;
   
   temp_arena Temp = TempArena(0);
   defer { EndTemp(Temp); };
   
   sorted_entity_array SortedEntities = SortEntitiesBySortingLayer(Temp.Arena, Entities);
   for (u64 EntityIndex = 0;
        EntityIndex < SortedEntities.EntityCount;
        ++EntityIndex)
   {
      entity *Entity = SortedEntities.Entries[EntityIndex].Entity;
      if (Entity->Type == Entity_Image && !(Entity->Flags & EntityFlag_Hidden))
      {
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
         
         Window->draw(Sprite, Transform);
      }
   }
}

internal void
UpdateAndRenderDeCasteljauVisualization(de_Casteljau_visualization *Visualization,
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
         defer { EndTemp(Temp); };
         
         u64 NumControlPoints = Curve->NumControlPoints;
         u64 NumIterations = NumControlPoints;
         color *IterationColors = PushArrayNonZero(Visualization->Arena, NumIterations, color);
         line_vertices *LineVerticesPerIteration = PushArray(Visualization->Arena,
                                                             NumIterations,
                                                             line_vertices);
         u64 NumAllIntermediatePoints = NumControlPoints * NumControlPoints;
         local_position *AllIntermediatePoints = PushArrayNonZero(Visualization->Arena,
                                                                  NumAllIntermediatePoints,
                                                                  local_position);
         
         // NOTE(hbr): Deal with cases at once.
         f32 *ControlPointWeights = Curve->ControlPointWeights;
         switch (Curve->CurveParams.CurveShape.BezierType)
         {
            case Bezier_Normal: {
               ControlPointWeights = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
               for (u64 I = 0; I < NumControlPoints; ++I)
               {
                  ControlPointWeights[I] = 1.0f;
               }
            } break;
            
            case Bezier_Weighted: {} break;
            case Bezier_Cubic: { Assert(false); } break;
         }
         
         // TODO(hbr): Consider passing 0 as ThrowAwayWeights, but make sure algorithm
         // handles that case
         f32 *ThrowAwayWeights = PushArrayNonZero(Temp.Arena, NumAllIntermediatePoints, f32);
         DeCasteljauAlgorithm(Visualization->T,
                              Curve->ControlPoints,
                              ControlPointWeights,
                              NumControlPoints,
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
            
            u64 NumPoints = NumControlPoints - Iteration;
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
   
   // TODO(hbr): Try to save [CurveEntity] and/or [Curve] into local variable
   
   if (Lowering->IsLowering)
   {
      if (Lowering->SavedCurveVersion != Lowering->CurveEntity->Curve.CurveVersion)
      { 
         Lowering->IsLowering = false;
      }
   }
   
   if (Lowering->IsLowering)
   {
      bool Ok = false;
      bool Revert = false;
      bool IsDegreeLoweringWindowOpen = true;
      bool P_MixChanged = false;
      bool W_MixChanged = false;
      
      Assert(Lowering->CurveEntity->Curve.CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      b32 IncludeWeights = false;
      switch (Lowering->CurveEntity->Curve.CurveParams.CurveShape.BezierType)
      {
         case Bezier_Normal: {} break;
         case Bezier_Weighted: { IncludeWeights = true; } break;
         case Bezier_Cubic: { Assert(false); } break;
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
      Assert(Lowering->LowerDegree.MiddlePointIndex < Lowering->CurveEntity->Curve.NumControlPoints);
      
      if (P_MixChanged || W_MixChanged)
      {
         local_position NewControlPoint =
            Lowering->P_Mix * Lowering->LowerDegree.P_I + (1 - Lowering->P_Mix) * Lowering->LowerDegree.P_II;
         f32 NewControlPointWeight =
            Lowering->W_Mix * Lowering->LowerDegree.W_I + (1 - Lowering->W_Mix) * Lowering->LowerDegree.W_II;
         
         CurveSetControlPoint(&Lowering->CurveEntity->Curve,
                              Lowering->LowerDegree.MiddlePointIndex,
                              NewControlPoint,
                              NewControlPointWeight);
         
         Lowering->SavedCurveVersion = Lowering->CurveEntity->Curve.CurveVersion;
      }
      
      if (Revert)
      {
         CurveSetControlPoints(&Lowering->CurveEntity->Curve,
                               Lowering->CurveEntity->Curve.NumControlPoints + 1,
                               Lowering->SavedControlPoints,
                               Lowering->SavedControlPointWeights,
                               Lowering->SavedCubicBezierPoints);
      }
      
      if (Ok || Revert || !IsDegreeLoweringWindowOpen)
      {
         Lowering->IsLowering = false;
      }
   }
   
   if (Lowering->IsLowering)
   {
      sf::Transform Model = CurveGetAnimate(Lowering->CurveEntity);
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

internal void
RenderCurves(entity *EntitiesList,
             editor_mode *EditorMode,
             editor_parameters *EditorParams,
             coordinate_system_data CoordinateSystemData,
             sf::Transform VP, sf::RenderWindow *Window)
{
   TimeFunction;
   
   // TODO(hbr): Add drawing in sort order
   ListIter(Entity, EntitiesList, entity)
   {
      if (Entity->Type == Entity_Curve)
      {
         curve *Curve = &Entity->Curve;
         curve_params CurveParams = Curve->CurveParams;
         
         if (!(Entity->Flags & EntityFlag_Hidden))
         {
            sf::Transform Model = CurveGetAnimate(Entity);
            sf::Transform MVP = VP * Model;
            
            if (EditorMode->Type == EditorMode_Moving &&
                EditorMode->Moving.Type == MovingMode_CurvePoint &&
                EditorMode->Moving.Entity == Entity)
            {
               Window->draw(EditorMode->Moving.SavedCurveVertices,
                            EditorMode->Moving.SavedNumCurveVertices,
                            EditorMode->Moving.SavedPrimitiveType,
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
                     ClipSpaceLengthToWorldSpace(EditorParams->CubicBezierHelperLineWidthClipSpace,
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
                  if (SelectedControlPointIndex + 1 < Curve->NumControlPoints)
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
                  ControlPointOutlineThicknessScale = EditorParams->SelectedCurveControlPointOutlineThicknessScale;
                  NormalControlPointOutlineColor = EditorParams->SelectedCurveControlPointOutlineColor;
               }
               
               u64 NumControlPoints = Curve->NumControlPoints;
               local_position *ControlPoints = Curve->ControlPoints;
               for (u64 ControlPointIndex = 0;
                    ControlPointIndex < NumControlPoints;
                    ++ControlPointIndex)
               {
                  local_position ControlPoint = ControlPoints[ControlPointIndex];
                  
                  f32 ControlPointRadius = NormalControlPointRadius;
                  if (ControlPointIndex == NumControlPoints - 1)
                  {
                     ControlPointRadius *= EditorParams->LastControlPointSizeMultiplier;
                  }
                  
                  color ControlPointOutlineColor = NormalControlPointOutlineColor;
                  if (ControlPointIndex == SelectedControlPointIndex)
                  {
                     ControlPointOutlineColor = EditorParams->SelectedControlPointOutlineColor;
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
         }
      }
   }
}

#define CURVE_NAME_HIGHLIGHT_COLOR ImVec4(1.0f, 0.5, 0.0f, 1.0f)

internal void
UpdateAndRenderAnimateCurveAnimation(transform_curve_animation *Animation,
                                     entity *EntitiesList, f32 DeltaTime,
                                     sf::Transform Transform,
                                     sf::RenderWindow *Window)
{
   TimeFunction;
   
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
               ListIter(Entity, EntitiesList, entity)
               {
                  if (Entity->Type == Entity_Curve)
                  {
                     if (Entity != Animation->FromCurveEntity)
                     {
                        // TODO(hbr): Do a pass over this whole function, [Selected] should not be initialized like this
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
            defer { EndTemp(Temp); };
            
            entity *FromCurveEntity = Animation->FromCurveEntity;
            entity *ToCurveEntity = Animation->ToCurveEntity;
            curve *FromCurve = &FromCurveEntity->Curve;
            curve *ToCurve = &ToCurveEntity->Curve;
            u64 NumCurvePoints = FromCurve->NumCurvePoints;
            
            b32 ToCurvePointsRecalculated = false;
            if (NumCurvePoints != Animation->NumCurvePoints ||
                Animation->SavedToCurveVersion != ToCurve->CurveVersion)
            {
               ClearArena(Animation->Arena);
               
               Animation->NumCurvePoints = NumCurvePoints;
               Animation->ToCurvePoints = PushArrayNonZero(Animation->Arena, NumCurvePoints, v2f32);
               CurveEvaluate(ToCurve, NumCurvePoints, Animation->ToCurvePoints);
               Animation->SavedToCurveVersion = ToCurve->CurveVersion;
               
               ToCurvePointsRecalculated = true;
            }
            
            if (ToCurvePointsRecalculated || BlendChanged)
            {               
               v2f32 *ToCurvePoints = Animation->ToCurvePoints;
               v2f32 *FromCurvePoints = FromCurve->CurvePoints;
               v2f32 *AnimatedCurvePoints = PushArrayNonZero(Temp.Arena, NumCurvePoints, v2f32);
               
               f32 Blend = CalculateAnimation(Animation->Animation, Animation->Blend);
               f32 T = 0.0f;
               f32 Delta_T = 1.0f / (NumCurvePoints - 1);
               for (u64 VertexIndex = 0;
                    VertexIndex < NumCurvePoints;
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
                  CalculateLineVertices(NumCurvePoints, AnimatedCurvePoints,
                                        AnimatedCurveWidth, AnimatedCurveColor,
                                        false, Allocation);
            }
         }
      } break;
   }
   
   if (Animation->Stage == AnimateCurveAnimation_Animating)
   {
      Window->draw(Animation->AnimatedCurveVertices.Vertices,
                   Animation->AnimatedCurveVertices.NumVertices,
                   Animation->AnimatedCurveVertices.PrimitiveType,
                   Transform);
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
      
      case CurveCombination_Count: { Assert(false); } break;
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
      
      case CurveCombination_Count: { Assert(false); } break;
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
   
   u64 N = From->NumControlPoints;
   u64 M = To->NumControlPoints;
   
   Assert(AreCurvesCompatibleForCombining(From, To));
   
   // NOTE(hbr): Prepare control point-related buffers in proper order.
   if (Combining->CombineCurveLastControlPoint)
   {
      ArrayReverse(From->ControlPoints, N);
      ArrayReverse(From->ControlPointWeights, N);
      ArrayReverse(From->CubicBezierPoints, 3 * N);
   }
   
   if (Combining->TargetCurveFirstControlPoint)
   {
      ArrayReverse(To->ControlPoints, M);
      ArrayReverse(To->ControlPointWeights, M);
      ArrayReverse(To->CubicBezierPoints, 3 * M);
   }
   
   u64 CombinedNumControlPoints = M;
   u64 StartIndex = 0;
   v2f32 Translation = {};
   switch (Combining->CombinationType)
   {
      case CurveCombination_Merge: {
         CombinedNumControlPoints += N;
         StartIndex = 0;
         Translation = {};
      } break;
      
      case CurveCombination_C0:
      case CurveCombination_C1:
      case CurveCombination_C2:
      case CurveCombination_G1: {
         if (N > 0)
         {
            CombinedNumControlPoints += N - 1;
            StartIndex = 1;
            if (M > 0)
            {
               Translation =
                  LocalEntityPositionToWorld(ToEntity, To->ControlPoints[M - 1]) -
                  LocalEntityPositionToWorld(FromEntity, From->ControlPoints[0]);
            }
         }
      } break;
      
      case CurveCombination_Count: { Assert(false); } break;
   }
   
   temp_arena Temp = TempArena(0);
   defer { EndTemp(Temp); };
   
   // NOTE(hbr): Allocate buffers and copy control points into them
   v2f32 *CombinedControlPoints = PushArrayNonZero(Temp.Arena, CombinedNumControlPoints, v2f32);
   f32 *CombinedControlPointWeights = PushArrayNonZero(Temp.Arena, CombinedNumControlPoints, f32);
   v2f32 *CombinedCubicBezierPoints = PushArrayNonZero(Temp.Arena, 3 * CombinedNumControlPoints, v2f32);
   
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
      
      case CurveCombination_Count: { Assert(false); } break;
   }
   
   CurveSetControlPoints(To,
                         CombinedNumControlPoints,
                         CombinedControlPoints,
                         CombinedControlPointWeights,
                         CombinedCubicBezierPoints);
   
   // TODO(hbr): Maybe update name of combined curve
   DeallocateAndRemoveEntity(EditorState, FromEntity);
   // TODO(hbr): Maybe select To curve
}

internal void
UpdateAndRenderCurveCombining(editor_state *EditorState,
                              sf::Transform Transform,
                              sf::RenderWindow *Window)
{
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
                        // TODO(hbr): Revise this whole function
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
          CombineCurve->NumControlPoints > 0 &&
          TargetCurve->NumControlPoints > 0)
      {
         local_position CombineControlPoint = (Combining->CombineCurveLastControlPoint ?
                                               CombineCurve->ControlPoints[CombineCurve->NumControlPoints - 1] :
                                               CombineCurve->ControlPoints[0]);
         
         local_position TargetControlPoint = (Combining->TargetCurveFirstControlPoint ?
                                              TargetCurve->ControlPoints[0] :
                                              TargetCurve->ControlPoints[TargetCurve->NumControlPoints - 1]);
         
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

function void
UpdateAndRender(f32 DeltaTime, user_input UserInput, editor *Editor)
{
   TimeFunction;
   
   FrameStatsUpdate(&Editor->FrameStats, DeltaTime);
   CameraUpdate(&Editor->State.Camera, UserInput.MouseWheelDelta, DeltaTime);
   UpdateAndRenderNotifications(&Editor->Notifications, DeltaTime, Editor->Window);
   
   {
      TimeBlock("editor state update");
      
      coordinate_system_data CoordinateSystemData {
         .Window = Editor->Window,
         .Camera = Editor->State.Camera,
         .Projection = Editor->Projection
      };
      
      editor_parameters EditorParams = Editor->Parameters;
      
      // NOTE(hbr): Editor state update
      editor_state NewState = Editor->State;
      for (u64 ButtonIndex = 0;
           ButtonIndex < ArrayCount(UserInput.Buttons);
           ++ButtonIndex)
      {
         button Button = Cast(button)ButtonIndex;
         button_state ButtonState = UserInput.Buttons[ButtonIndex];
         
         if (ClickedWithButton(ButtonState, CoordinateSystemData))
         {
            user_action Action = CreateClickedAction(Button, ButtonState.ReleasePosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
         
         if (DraggedWithButton(ButtonState, UserInput.MousePosition, CoordinateSystemData))
         {
            user_action Action = CreateDragAction(Button, UserInput.MouseLastPosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
         
         if (ReleasedButton(ButtonState))
         {
            user_action Action = CreateReleasedAction(Button, ButtonState.ReleasePosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
      }
      if (UserInput.MouseLastPosition != UserInput.MousePosition)
      {
         user_action Action = CreateMouseMoveAction(UserInput.MouseLastPosition, UserInput.MousePosition, &UserInput);
         NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
      }
      Editor->State = NewState;
   }
   
#if EDITOR_DEBUG
   {
      notifications *Notifications = &Editor->Notifications;
      if (PressedWithKey(UserInput.Keys[Key_S], 0))
      {
         AddNotificationF(Notifications,
                          Notification_Success,
                          "example successful notification message");
      }
      if (PressedWithKey(UserInput.Keys[Key_E], 0))
      {
         AddNotificationF(Notifications,
                          Notification_Error,
                          "example error notification message");
      }
      if (PressedWithKey(UserInput.Keys[Key_W], 0))
      {
         AddNotificationF(Notifications,
                          Notification_Warning,
                          "example warning notification message");
      }
   }
#endif
   
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
   
#if EDITOR_DEBUG
   if (Editor->UI_Config.ViewDebugWindow)
   {
      RenderDebugWindow(Editor);
   }
   ImGui::ShowDemoWindow();
#endif
   
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
   
   // NOTE(hbr): Rendering functions are invoked in order to match expected drawing order.
   RenderImages(Editor->State.EntitiesHead, VP, Editor->Window);
   
   // NOTE(hbr): Render below curves
   UpdateAndRenderDegreeLowering(&Editor->State.DegreeLowering,
                                 VP, Editor->Window);
   
   RenderCurves(Editor->State.EntitiesHead,
                &Editor->State.Mode,
                &Editor->Parameters,
                CoordinateSystemData,
                VP, Editor->Window);
   
   // NOTE(hbr): Render on top of curves
   UpdateAndRenderDeCasteljauVisualization(&Editor->State.DeCasteljauVisualization,
                                           VP, Editor->Window);
   UpdateAndRenderAnimateCurveAnimation(&Editor->State.CurveAnimation,
                                        Editor->State.EntitiesHead,
                                        DeltaTime, VP, Editor->Window);
   UpdateAndRenderCurveCombining(&Editor->State, VP, Editor->Window);
   UpdateAndRenderSplittingBezierCurve(&Editor->State,
                                       &Editor->Parameters,
                                       CoordinateSystemData,
                                       VP,
                                       Editor->Window);
   
   RenderRotationIndicator(&Editor->State, &Editor->Parameters,
                           CoordinateSystemData, VP, Editor->Window);
   
   // NOTE(hbr): Update menu bar here, because world has to already be rendered
   // and UI not, because we might save our project into image file and we
   // don't want to include UI render into our image.
   UpdateAndRenderMenuBar(Editor, UserInput);
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
   InitThreadCtx(Gigabytes(16));
   
   sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
   sf::ContextSettings ContextSettings = sf::ContextSettings();
   ContextSettings.antialiasingLevel = 4;
   sf::RenderWindow Window(VideoMode,
                           WINDOW_TITLE,
                           sf::Style::Default,
                           ContextSettings);
   
   SetNormalizedDeviceCoordinatesView(&Window);
   
   InitializeProfiler();
   
#if 0
#if not(EDITOR_DEBUG)
   Window.setFramerateLimit(60);
#endif
#endif
   
   if (Window.isOpen())
   {
      bool ImGuiInitSuccess = ImGui::SFML::Init(Window, true);
      if (ImGuiInitSuccess)
      {
         pool *EntityPool = AllocatePoolForType(Gigabytes(1), entity);
         arena *DeCasteljauVisualizationArena = AllocateArena(Gigabytes(1));
         arena *DegreeLoweringArena = AllocateArena(Gigabytes(1));
         arena *MovingPointArena = AllocateArena(Gigabytes(1));
         arena *CurveAnimationArena = AllocateArena(Gigabytes(1));
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
         
         editor_parameters InitialEditorParameters {
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
#if EDITOR_DEBUG
            .ViewDebugWindow = true,
#endif
         };
         
         editor Editor = {};
         Editor.Window = &Window;
         Editor.FrameStats = FrameStatsMake();
         Editor.State = InitialEditorState;
         Editor.Projection = InitialProjection;
         Editor.Notifications = Notifications;
         Editor.Parameters = InitialEditorParameters;
         Editor.UI_Config = InitialUI_Config;
         
         sf::Vector2i MousePosition = sf::Mouse::getPosition(Window);
         
         user_input UserInput = {};
         UserInput.MousePosition = V2S32FromVec(MousePosition);
         UserInput.MouseLastPosition = V2S32FromVec(MousePosition);
         UserInput.WindowWidth = VideoMode.width;
         UserInput.WindowHeight = VideoMode.height;
         
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
            
            HandleEvents(&Window, &UserInput);
            Editor.Projection.AspectRatio = CalculateAspectRatio(UserInput.WindowWidth,
                                                                 UserInput.WindowHeight);
            
            Window.clear(ColorToSFMLColor(Editor.Parameters.BackgroundColor));
            UpdateAndRender(DeltaTime, UserInput, &Editor);
            
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
         ReportError("failed to initialize ImGui");
      }
   }
   else
   {
      ReportError("failed to open window");
   }
   
   return 0;
}

// NOTE(hbr): Specifically after every file is included. Works in Unity build only.
StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors), MakeSureNumberOfProfilePointsFitsIntoArray);