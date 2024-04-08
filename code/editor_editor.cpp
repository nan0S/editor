//- Parameters and constants
function editor_parameters
EditorParametersMake(f32 RotationIndicatorRadiusClipSpace,
                     f32 RotationIndicatorOutlineThicknessFraction,
                     color RotationIndicatorFillColor,
                     color RotationIndicatorOutlineColor,
                     f32 BezierSplitPointRadiusClipSpace,
                     f32 BezierSplitPointOutlineThicknessFraction,
                     color BezierSplitPointFillColor,
                     color BezierSplitPointOutlineColor,
                     color BackgroundColor,
                     f32 CollisionToleranceClipSpace,
                     f32 LastControlPointSizeMultiplier,
                     f32 SelectedCurveControlPointOutlineThicknessScale,
                     color SelectedCurveControlPointOutlineColor,
                     color SelectedControlPointOutlineColor,
                     f32 CubicBezierHelperLineWidthClipSpace,
                     curve_params CurveDefaultParams,
                     f32 DefaultControlPointWeight)
{
   editor_parameters Result = {};
   
   Result.RotationIndicator.RadiusClipSpace = RotationIndicatorRadiusClipSpace;
   Result.RotationIndicator.OutlineThicknessFraction = RotationIndicatorOutlineThicknessFraction;
   Result.RotationIndicator.FillColor = RotationIndicatorFillColor;
   Result.RotationIndicator.OutlineColor = RotationIndicatorOutlineColor;
   
   Result.BezierSplitPoint.RadiusClipSpace = BezierSplitPointRadiusClipSpace;
   Result.BezierSplitPoint.OutlineThicknessFraction = BezierSplitPointOutlineThicknessFraction;
   Result.BezierSplitPoint.FillColor = BezierSplitPointFillColor;
   Result.BezierSplitPoint.OutlineColor = BezierSplitPointOutlineColor;
   
   Result.BackgroundColor = BackgroundColor;
   Result.CollisionToleranceClipSpace = CollisionToleranceClipSpace;
   Result.LastControlPointSizeMultiplier = LastControlPointSizeMultiplier;
   Result.SelectedCurveControlPointOutlineThicknessScale = SelectedCurveControlPointOutlineThicknessScale;
   Result.SelectedCurveControlPointOutlineColor = SelectedCurveControlPointOutlineColor;
   Result.SelectedControlPointOutlineColor = SelectedControlPointOutlineColor;
   Result.CubicBezierHelperLineWidthClipSpace = CubicBezierHelperLineWidthClipSpace;
   
   Result.CurveDefaultParams = CurveDefaultParams;
   Result.DefaultControlPointWeight = DefaultControlPointWeight;
   
   return Result;
}

//- Camera, Projection and Coordinate Spaces
internal void
SetNormalizedDeviceCoordinatesView(sf::RenderWindow *RenderWindow)
{
   sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
   sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
   sf::View View = sf::View(Center, Size);
   
   RenderWindow->setView(View);
}

function camera
CameraMake(v2f32 Position, rotation_2d Rotation,
           f32 Zoom, f32 ZoomSensitivity, f32 ReachingTargetSpeed)
{
   camera Camera = {};
   Camera.Position = Position;
   Camera.Rotation = Rotation;
   Camera.Zoom = Zoom;
   Camera.ZoomSensitivity = ZoomSensitivity;
   Camera.ReachingTargetSpeed = ReachingTargetSpeed;
   
   return Camera;
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

function void
CameraSetZoom(camera *Camera, f32 Zoom)
{
   Camera->Zoom = Zoom;
   Camera->ReachingTarget = false;
}

internal f32
CalculateAspectRatio(u64 Width, u64 Height)
{
   f32 AspectRatio = cast(f32)Width / Height;
   return AspectRatio;
}

function projection
ProjectionMake(u64 Width, u64 Height, f32 FrustumSize)
{
   projection Projection = {};
   Projection.AspectRatio = CalculateAspectRatio(Width, Height);
   Projection.FrustumSize = FrustumSize;
   
   return Projection;
}

function void
ProjectionOnWindowResize(projection *Projection, u64 NewWidth, u64 NewHeight)
{
   Projection->AspectRatio = CalculateAspectRatio(NewWidth, NewHeight);
}

function coordinate_system_data
CoordinateSystemDataMake(sf::RenderWindow *RenderWindow,
                         camera Camera, projection Projection)
{
   coordinate_system_data Result = {};
   Result.RenderWindow = RenderWindow;
   Result.Camera = Camera;
   Result.Projection = Projection;
   
   return Result;
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
   v2f32 ClipPosition = V2F32FromVec(Data.RenderWindow->mapPixelToCoords(V2S32ToVector2i(Position)));
   
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

//- Collisions
function collision
ControlPointCollision(curve *CollisionCurve, u64 CollisionPointIndex, b32 IsCubicBezierPoint)
{
   collision Result = {};
   Result.Type = Collision_ControlPoint;
   Result.CollisionCurve = CollisionCurve;
   Result.CollisionPointIndex = CollisionPointIndex;
   Result.IsCubicBezierPoint = IsCubicBezierPoint;
   
   return Result;
}

function collision
CurvePointCollision(curve *CollisionCurve, u64 CollisionCurvePointIndex)
{
   collision Result = {};
   Result.Type = Collision_CurvePoint;
   Result.CollisionCurve = CollisionCurve;
   Result.CollisionPointIndex = CollisionCurvePointIndex;
   
   return Result;
}

function collision
ImageCollision(image *CollisionImage)
{
   collision Result = {};
   Result.Type = Collision_Image;
   Result.CollisionImage = CollisionImage;
   
   return Result;
}

function curve_point_index
CheckCollisionWithCurvePoints(curve *Curve,
                              world_position CheckPosition,
                              f32 CollisionTolerance)
{
   u64 Result = U64_MAX;
   
   local_position *CurvePoints = Curve->CurvePoints;
   u64 NumCurvePoints = Curve->NumCurvePoints;
   f32 CurveWidth = 2.0f * CollisionTolerance + Curve->CurveParams.CurveWidth;
   
   local_position CheckPositionLocal = WorldToLocalCurvePosition(Curve, CheckPosition);
   
   for (u64 CurvePointIndex = 0;
        CurvePointIndex+1 < NumCurvePoints;
        ++CurvePointIndex)
   {
      v2f32 P1 = CurvePoints[CurvePointIndex];
      v2f32 P2 = CurvePoints[CurvePointIndex+1];
      
      if (SegmentCollision(CheckPositionLocal, P1, P2, CurveWidth))
      {
         Result = CurvePointIndex;
         goto collision_found_label;
      }
   }
   
   collision_found_label:
   return Result;
}

function collision
CheckCollisionWithControlPoints(curve *Curve,
                                world_position CheckPosition,
                                f32 CollisionTolerance,
                                editor_parameters EditorParams,
                                b32 IncludeCubicBezierPoints)
{
}

function collision
CheckCollisionWith(entity *EntitiesHead, entity *EntitiesTail,
                   world_position CheckPosition,
                   f32 CollisionTolerance,
                   editor_parameters EditorParams,
                   check_collision_with_flags CheckCollisionWithFlags)
{
   collision Result = {};
   
   if (CheckCollisionWithFlags & CheckCollisionWith_ControlPoints)
   {
      // NOTE(hbr): Check collisions in reversed drawing order.
      ListIterRev(Entity, EntitiesTail, entity)
      {
         if (Entity->Type == Entity_Curve)
         {
            curve *Curve = &Entity->Curve;
            
            if (!Curve->CurveParams.Hidden &&
                !Curve->CurveParams.PointsDisabled)
            {
               local_position *ControlPoints = Curve->ControlPoints;
               u64 NumControlPoints = Curve->NumControlPoints;
               
               f32 OutlineThicknessScale = 0.0f;
               if (Curve->IsSelected)
               {
                  OutlineThicknessScale =
                     EditorParams.SelectedCurveControlPointOutlineThicknessScale;
               }
               
               f32 NormalPointSize = Curve->CurveParams.PointSize;
               
               local_position CheckPositionLocal = WorldToLocalCurvePosition(Curve, CheckPosition);
               
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
                     Result = ControlPointCollision(Curve, ControlPointIndex, false);
                     goto collision_found_label;
                  }
               }
               
               if (!Curve->CurveParams.CubicBezierHelpersDisabled &&
                   Curve->SelectedControlPointIndex != U64_MAX &&
                   Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier &&
                   Curve->CurveParams.CurveShape.BezierType == Bezier_Cubic)
               {
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
                              Result = ControlPointCollision(Curve, CheckIndex, true);
                              goto collision_found_label;
                           }
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
         if (Entity->Type == Entity_Curve)
         {
            curve *Curve = &Entity->Curve;
            
            // TODO(hbr): Checking hidden flags should happen inside CheckCollisionWithCurvePoints function
            if (!Curve->CurveParams.Hidden)
            {
               u64 CurvePointIndex = CheckCollisionWithCurvePoints(Curve,
                                                                   CheckPosition,
                                                                   CollisionTolerance);
               
               if (CurvePointIndex != U64_MAX)
               {
                  Result = CurvePointCollision(Curve, CurvePointIndex);
                  goto collision_found_label;
               }
            }
         }
      }
   }
   
   if (CheckCollisionWithFlags & CheckCollisionWith_Images)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      // TODO(hbr): Is there a reason to pass NumEntities as well?
      u64 NumEntities = 0;
      ListIter(Entity, EntitiesHead, entity) { NumEntities += 1; }
      sorting_layer_sorted_images SortedImages = SortingLayerSortedImages(Scratch.Arena,
                                                                          NumEntities,
                                                                          EntitiesHead);
      
      // NOTE(hbr): Check collisions in reversed drawing order.
      for (u64 ImageIndex = SortedImages.NumImages;
           ImageIndex > 0;
           --ImageIndex)
      {
         image *Image = SortedImages.SortedImages[ImageIndex - 1].Image;
         
         if (!Image->Hidden)
         {
            v2f32 Position = Image->Position;
            
            sf::Vector2u TextureSize = Image->Texture.getSize();
            f32 SizeX = Abs(GlobalImageScaleFactor * Image->Scale.X * TextureSize.x);
            f32 SizeY = Abs(GlobalImageScaleFactor * Image->Scale.Y * TextureSize.y);
            v2f32 Extents = 0.5f * V2F32(SizeX + CollisionTolerance,
                                         SizeY + CollisionTolerance);
            
            rotation_2d InverseRotation = Rotation2DInverse(Image->Rotation);
            
            v2f32 CheckPositionInImageSpace = CheckPosition - Position;
            CheckPositionInImageSpace = RotateAround(CheckPositionInImageSpace,
                                                     V2F32(0.0f, 0.0f),
                                                     InverseRotation);
            
            if (-Extents.X <= CheckPositionInImageSpace.X &&
                CheckPositionInImageSpace.X <= Extents.X &&
                -Extents.Y <= CheckPositionInImageSpace.Y &&
                CheckPositionInImageSpace.Y <= Extents.Y)
            {
               Result = ImageCollision(Image);
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
UserActionButtonClicked(button Button, screen_position ClickPosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonClicked;
   Result.Click.Button = Button;
   Result.Click.ClickPosition = ClickPosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
UserActionButtonDrag(button Button, screen_position DragFromPosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonDrag;
   Result.Drag.Button = Button;
   Result.Drag.DragFromPosition = DragFromPosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
UserActionButtonReleased(button Button, screen_position ReleasePosition, user_input *UserInput)
{
   user_action Result = {};
   Result.Type = UserAction_ButtonReleased;
   Result.Release.Button = Button;
   Result.Release.ReleasePosition = ReleasePosition;
   Result.UserInput = UserInput;
   
   return Result;
}

function user_action
UserActionMouseMove(v2s32 FromPosition, screen_position ToPosition, user_input *UserInput)
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

function editor_mode
EditorModeMovingPoint(curve *Curve, u64 PointIndex, b32 CubicBezierPointMoving,
                      arena *Arena)
{
   editor_mode Result = {};
   Result.Type = EditorMode_MovingPoint;
   Result.MovingPoint.Curve = Curve;
   Result.MovingPoint.PointIndex = PointIndex;
   Result.MovingPoint.CubicBezierPointMoving = CubicBezierPointMoving;
   
   ArenaClear(Arena);
   
   line_vertices CurveVertices = Curve->CurveVertices;
   sf::Vertex *SavedCurveVertices = PushArray(Arena,
                                              CurveVertices.NumVertices,
                                              sf::Vertex);
   MemoryCopy(SavedCurveVertices,
              CurveVertices.Vertices,
              CurveVertices.NumVertices * sizeof(SavedCurveVertices[0]));
   for (u64 CurveVertexIndex = 0;
        CurveVertexIndex < CurveVertices.NumVertices;
        ++CurveVertexIndex)
   {
      sf::Vertex *Vertex = SavedCurveVertices + CurveVertexIndex;
      Vertex->color.a = cast(u8)(0.2f * Vertex->color.a);
   }
   Result.MovingPoint.SavedCurveVertices = SavedCurveVertices;
   Result.MovingPoint.SavedNumCurveVertices = CurveVertices.NumVertices;
   Result.MovingPoint.SavedPrimitiveType = CurveVertices.PrimitiveType;
   
   return Result;
}

function editor_mode
EditorModeMovingCamera(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_MovingCamera;
   
   return Result;
}

function editor_mode
EditorModeMovingCurve(curve *Curve)
{
   editor_mode Result = {};
   Result.Type = EditorMode_MovingCurve;
   Result.MovingCurve = Curve;
   
   return Result;
}

function editor_mode
EditorModeRotatingCurve(curve *Curve, screen_position RotationCenter)
{
   editor_mode Result = {};
   Result.Type = EditorMode_RotatingCurve;
   Result.RotatingCurve.Curve = Curve;
   Result.RotatingCurve.RotationCenter = RotationCenter;
   
   return Result;
}

function editor_mode
EditorModeRotatingCamera(void)
{
   editor_mode Result = {};
   Result.Type = EditorMode_RotatingCamera;
   
   return Result;
}

function editor_mode
EditorModeRotatingImage(image *Image)
{
   editor_mode Result = {};
   Result.Type = EditorMode_RotatingImage;
   Result.RotatingImage = Image;
   
   return Result;
}

function editor_mode
EditorModeMovingImage(image *Image)
{
   editor_mode Result = {};
   Result.Type = EditorMode_MovingImage;
   Result.MovingImage = Image;
   
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
      
      case Animation_Count: {} break;
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
      
      case Animation_Count: {} break;
   }
   
   return "invalid";
}

function editor_state
EditorStateMake(pool *EntityPool,
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
EditorStateMakeDefault(pool *EntityPool,
                       arena *DeCasteljauVisualizationArena,
                       arena *DegreeLoweringArena,
                       arena *MovingPointArena,
                       arena *CurveAnimationArena)
{
   world_position DefaultCameraPosition = V2F32(0.0f, 0.0f);
   rotation_2d DefaultCameraRotation = Rotation2DZero();
   f32 DefaultCameraZoom = 1.0f;
   f32 DefaultCameraZoomSensitivity = 0.05f;
   f32 DefaultCameraReachingTargetSpeed = 10.0f;
   camera DefaultCamera = CameraMake(DefaultCameraPosition,
                                     DefaultCameraRotation,
                                     DefaultCameraZoom,
                                     DefaultCameraZoomSensitivity,
                                     DefaultCameraReachingTargetSpeed);
   
   editor_state Result = EditorStateMake(EntityPool, 0,
                                         DefaultCamera,
                                         DeCasteljauVisualizationArena,
                                         DegreeLoweringArena,
                                         MovingPointArena,
                                         CurveAnimationArena,
                                         CURVE_DEFAULT_ANIMATION_SPEED);
   return Result;
}

function void
EditorStateDestroy(editor_state *EditorState)
{
   ListIter(Entity, EditorState->EntitiesHead, entity)
   {
      EntityDestroy(Entity);
      PoolFree(EditorState->EntityPool, Entity);
   }
   EditorState->EntitiesHead = 0;
   EditorState->EntitiesTail = 0;
   EditorState->NumEntities = 0;
}

function entity *
EditorStateAddEntity(editor_state *EditorState, entity *Entity)
{
   entity *NewEntity = PoolAllocStruct(EditorState->EntityPool, entity);
   
   // NOTE(hbr): Ugly C++ thing we have to do because of constructors/destructors.
   MemoryCopy(NewEntity, Entity, sizeof(entity));
   if (Entity->Type == Entity_Image)
   {
      new (&NewEntity->Image.Texture) sf::Texture(Entity->Image.Texture);
   }
   
   DLLPushBack(EditorState->EntitiesHead, EditorState->EntitiesTail, NewEntity);
   ++EditorState->NumEntities;
   
   return NewEntity;
}

function void
EditorStateRemoveEntity(editor_state *EditorState, entity *Entity)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         CurveDestroy(Curve);
         
         if (EditorState->SplittingBezierCurve.IsSplitting &&
             EditorState->SplittingBezierCurve.SplitCurve == Curve)
         {
            EditorState->SplittingBezierCurve.IsSplitting = false;
            EditorState->SplittingBezierCurve.SplitCurve = 0;
         }
         
         if (EditorState->DeCasteljauVisualization.IsVisualizing &&
             EditorState->DeCasteljauVisualization.Curve == Curve)
         {
            EditorState->DeCasteljauVisualization.IsVisualizing = false;
            EditorState->DeCasteljauVisualization.Curve = 0;
         }
         
         if (EditorState->DegreeLowering.IsLowering &&
             EditorState->DegreeLowering.Curve == Curve)
         {
            EditorState->DegreeLowering.IsLowering = false;
            EditorState->DegreeLowering.Curve = 0;
         }
         
         if (EditorState->CurveAnimation.FromCurve == Curve ||
             EditorState->CurveAnimation.ToCurve == Curve)
         {
            EditorState->CurveAnimation.Stage = AnimateCurveAnimation_None;
         }
         
         if (EditorState->CurveCombining.CombineCurve == Curve)
         {
            EditorState->CurveCombining.IsCombining = false;
         }
         if (EditorState->CurveCombining.TargetCurve == Curve)
         {
            EditorState->CurveCombining.TargetCurve = 0;
         }
      } break;
      
      case Entity_Image: {
         image *RemoveImage = &Entity->Image;
         ImageDestroy(RemoveImage);
      } break;
   }
   
   if (EditorState->SelectedEntity == Entity)
   {
      EditorState->SelectedEntity = 0;
   }
   
   DLLRemove(EditorState->EntitiesHead, EditorState->EntitiesTail, Entity);
   --EditorState->NumEntities;
   
   PoolFree(EditorState->EntityPool, Entity);
}

function void
EditorStateSelectEntity(editor_state *EditorState, entity *Entity)
{
   if (EditorState->SelectedEntity)
   {
      EditorStateDeselectCurrentEntity(EditorState);
   }
   
   switch (Entity->Type)
   {
      case Entity_Curve: { Entity->Curve.IsSelected = true; } break;
      case Entity_Image: { Entity->Image.IsSelected = true; } break;
   }
   
   EditorState->SelectedEntity = Entity;
}

function void
EditorStateDeselectCurrentEntity(editor_state *EditorState)
{
   entity *Entity = EditorState->SelectedEntity;
   if (Entity)
   {
      switch (Entity->Type)
      {
         case Entity_Curve: { Entity->Curve.IsSelected = false; } break;
         case Entity_Image: { Entity->Image.IsSelected = false; } break;
      }
   }
   EditorState->SelectedEntity = 0;
}

function notification
NotificationMake(string Title,
                 color TitleColor,
                 string Text,
                 f32 PositionY)
{
   Assert(Title);
   Assert(Text);
   
   notification Notification = {};
   Notification.Title = StringDuplicate(Title);
   Notification.TitleColor = TitleColor;
   Notification.Text = StringDuplicate(Text);
   Notification.PositionY = PositionY;
   
   if (StringSize(Notification.Text) > 0)
   {
      Notification.Text[0] = ToUppercase(Notification.Text[0]);
   }
   
   return Notification;
}

function void
NotificationDestroy(notification *Notification)
{
   StringFree(Notification->Title);
   StringFree(Notification->Text);
   
   Notification->Title = 0;
   Notification->Text = 0;
}

function notification_system
NotificationSystemMake(sf::RenderWindow *Window)
{
   notification_system NotificationSystem = {};
   NotificationSystem.Window = Window;
   
   return NotificationSystem;
}

function void
NotificationSystemAddNotificationFormat(notification_system *NotificationSystem,
                                        notification_type NotificationType,
                                        char const *NotificationTextFormat,
                                        ...)
{
   
   va_list ArgList;
   va_start(ArgList, NotificationTextFormat);
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   string NotificationText = StringMakeFormatV(Scratch.Arena,
                                               NotificationTextFormat,
                                               ArgList);
   
   u64 NumNotifications = NotificationSystem->NumNotifications;
   if (NumNotifications < ArrayCount(NotificationSystem->Notifications))
   {
      char const *NotificationTitleLit = 0;
      color NotificationTitleColor = {};
      switch (NotificationType)
      {
         case Notification_Success: {
            NotificationTitleLit = "Success";
            NotificationTitleColor = GreenColor;
         } break;
         
         case Notification_Error: {
            NotificationTitleLit = "Error";
            NotificationTitleColor = RedColor;
         } break;
         
         case Notification_Warning: {
            NotificationTitleLit = "Warning";
            NotificationTitleColor = YellowColor;
         } break;
      }
      
      string NotificationTitle = StringMakeLitArena(Scratch.Arena, NotificationTitleLit);
      
      // NOTE(hbr): Calculate desginated position
      sf::Vector2u WindowSize = NotificationSystem->Window->getSize();
      f32 NotificationWindowPadding = NotificationWindowPaddingScale * WindowSize.x;
      f32 NotificationDesignatedPositionY = WindowSize.y - NotificationWindowPadding;
      notification *Notifications = NotificationSystem->Notifications;
      
      for (u64 NotificationIndex = 0;
           NotificationIndex < NumNotifications;
           ++NotificationIndex)
      {
         notification *Notification = &Notifications[NotificationIndex];
         NotificationDesignatedPositionY -=
            Notification->NotificationWindowHeight + NotificationWindowPadding;
      }
      
      notification NewNotification = NotificationMake(NotificationTitle,
                                                      NotificationTitleColor,
                                                      NotificationText,
                                                      NotificationDesignatedPositionY);
      
      NotificationSystem->Notifications[NumNotifications] = NewNotification;
      NotificationSystem->NumNotifications += 1;
   }
   
   va_end(ArgList);
}

function ui_config
UI_ConfigMake(b32 ViewSelectedCurveWindow,
              b32 ViewSelectedImageWindow,
              b32 ViewListOfEntitiesWindow,
              b32 ViewParametersWindow,
              b32 ViewDiagnosticsWindow,
              b32 DefaultCurveHeaderCollapsed,
              b32 RotationIndicatorHeaderCollapsed,
              b32 BezierSplitPointHeaderCollapsed,
              b32 OtherSettingsHeaderCollapsed)
{
   ui_config Result = {};
   Result.ViewSelectedCurveWindow = ViewSelectedCurveWindow;
   Result.ViewSelectedImageWindow = ViewSelectedCurveWindow;
   Result.ViewListOfEntitiesWindow = ViewListOfEntitiesWindow;
   Result.ViewParametersWindow = ViewParametersWindow;
   Result.ViewDiagnosticsWindow = ViewDiagnosticsWindow;
   Result.DefaultCurveHeaderCollapsed = DefaultCurveHeaderCollapsed;
   Result.RotationIndicatorHeaderCollapsed = RotationIndicatorHeaderCollapsed;
   Result.BezierSplitPointHeaderCollapsed = BezierSplitPointHeaderCollapsed;
   Result.OtherSettingsHeaderCollapsed = OtherSettingsHeaderCollapsed;
#if EDITOR_PROFILER
   Result.ViewProfilerWindow = true;
#endif
#if EDITOR_DEBUG
   Result.ViewDebugWindow = true;
#endif
   
   return Result;
}

function editor
EditorMake(sf::RenderWindow *Window,
           editor_state State, 
           projection Projection,
           notification_system NotificationSystem,
           editor_parameters Parameters,
           ui_config UI_Config)
{
   editor Result = {};
   Result.Window = Window;
   Result.FrameStats = FrameStatsMake();
   Result.State = State;
   Result.Projection = Projection;
   Result.NotificationSystem = NotificationSystem;
   Result.Parameters = Parameters;
   Result.UI_Config = UI_Config;
   
   return Result;
}

function void
EditorSetSaveProjectPath(editor *Editor,
                         save_project_format SaveProjectFormat,
                         string SaveProjectFilePath)
{
   if (Editor->ProjectSavePath)
   {
      StringFree(Editor->ProjectSavePath);
   }
   
   if (SaveProjectFilePath)
   {
      Editor->SaveProjectFormat = SaveProjectFormat;
      Editor->ProjectSavePath = StringDuplicate(SaveProjectFilePath);
      
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      string SaveProjectFileName = StringChopFileNameWithoutExtension(Scratch.Arena,
                                                                      SaveProjectFilePath);
      Editor->Window->setTitle(SaveProjectFileName);
   }
   else
   {
      Editor->SaveProjectFormat = SaveProjectFormat_None;
      Editor->ProjectSavePath = 0;
      
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
                  Collision.Type = Collision_None;
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
                  curve *SplitCurve = State.SplittingBezierCurve.SplitCurve;
                  
                  b32 ClickedOnSplitCurve = false;
                  f32 T = 0.0f;
                  switch (Collision.Type)
                  {
                     case Collision_None: {} break;
                     
                     case Collision_ControlPoint: {
                        T = cast(f32)Collision.CollisionPointIndex / (SplitCurve->NumControlPoints - 1);
                        ClickedOnSplitCurve = true;
                     } break;
                     
                     case Collision_CurvePoint: {
                        T = cast(f32)Collision.CollisionPointIndex / (SplitCurve->NumCurvePoints - 1);
                        ClickedOnSplitCurve = true;
                     } break;
                     
                     // NOTE(hbr): Impossible
                     case Collision_Image: { Assert(false); } break;
                  }
                  
                  if (ClickedOnSplitCurve)
                  {
                     Result.SplittingBezierCurve.T = Clamp(T, 0.0f, 1.0f); // NOTE(hbr): Be safe
                     EditorStateSelectEntity(&Result, EntityFromCurve(SplitCurve));
                     SkipNormalModeHandling = true;
                  }
               }
               
               if (State.CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget)
               {
                  switch (Collision.Type)
                  {
                     case Collision_None:
                     case Collision_Image: {} break;
                     
                     case Collision_CurvePoint:
                     case Collision_ControlPoint: {
                        curve *Curve = Collision.CollisionCurve;
                        if (Curve != State.CurveAnimation.FromCurve)
                        {
                           Result.CurveAnimation.ToCurve = Curve;
                           SkipNormalModeHandling = true;
                        }
                     } break;
                  }
               }
               
               if (State.CurveCombining.IsCombining)
               {
                  if (Collision.Type != Collision_None)
                  {
                     curve *CollisionCurve = Collision.CollisionCurve;
                     curve *CombineCurve = State.CurveCombining.CombineCurve;
                     curve *TargetCurve = State.CurveCombining.TargetCurve;
                     
                     // NOTE(hbr): Logic to recognize whether first/last point of
                     // combine/target curve was clicked. If so then change which
                     // point should be combined with which one (first with last,
                     // first with first, ...). On top of that if user clicked
                     // on already selected point but in different direction
                     // (target->combine rather than combine->target) then swap
                     // target and combine curves to combine in the other direction.
                     if (Collision.Type == Collision_ControlPoint &&
                         !Collision.IsCubicBezierPoint)
                     {
                        u64 CollisionPointIndex = Collision.CollisionPointIndex;
                        
                        if (CollisionCurve == CombineCurve)
                        {
                           if (CollisionPointIndex == 0)
                           {
                              if (!State.CurveCombining.CombineCurveLastControlPoint)
                              {
                                 Swap(Result.CurveCombining.CombineCurve,
                                      Result.CurveCombining.TargetCurve);
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
                                 Swap(Result.CurveCombining.CombineCurve,
                                      Result.CurveCombining.TargetCurve);
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
                        Result.CurveCombining.TargetCurve = CollisionCurve;
                     }
                     
                     SkipNormalModeHandling = true;
                  }
               }
               
               if (!SkipNormalModeHandling)
               {                  
                  switch (Collision.Type)
                  {
                     case Collision_None: {
                        curve *AddCurve = 0;
                        if (State.SelectedEntity &&
                            State.SelectedEntity->Type == Entity_Curve)
                        {
                           AddCurve = &State.SelectedEntity->Curve;
                        }
                        else
                        {
                           name_string NewCurveName = NameStringFormat("curve(%lu)",
                                                                       Result.EverIncreasingCurveCounter++);
                           
                           curve DefaultCurve = CurveMake(NewCurveName,
                                                          EditorParams.CurveDefaultParams,
                                                          U64_MAX,
                                                          V2F32(0.0f, 0.0f),
                                                          Rotation2DZero());
                           
                           entity Entity = CurveEntity(DefaultCurve);
                           entity *AddEntity = EditorStateAddEntity(&Result, &Entity);
                           AddCurve = &AddEntity->Curve;
                        }
                        Assert(AddCurve);
                        
                        added_point_index AddedPointIndex =
                           CurveAppendControlPoint(AddCurve, ClickPosition,
                                                   EditorParams.DefaultControlPointWeight);
                        AddCurve->SelectedControlPointIndex = AddedPointIndex;
                        
                        EditorStateSelectEntity(&Result, EntityFromCurve(AddCurve));
                     } break;
                     
                     case Collision_ControlPoint: {
                        if (!Collision.IsCubicBezierPoint)
                        {
                           EditorStateSelectEntity(&Result, EntityFromCurve(Collision.CollisionCurve));
                        }
                     } break;
                     
                     case Collision_CurvePoint: {
                        curve *AddCurve = Collision.CollisionCurve;
                        
                        u64 CurvePointIndex = Collision.CollisionPointIndex;
                        f32 Segment = cast(f32)CurvePointIndex / (AddCurve->NumCurvePoints - 1);
                        u64 InsertAfterPointIndex =
                           cast(u64)(Segment * (CurveIsLooped(AddCurve->CurveParams.CurveShape) ?
                                                AddCurve->NumControlPoints :
                                                AddCurve->NumControlPoints - 1));
                        InsertAfterPointIndex = Clamp(InsertAfterPointIndex, 0, AddCurve->NumControlPoints - 1);
                        
                        CurveInsertControlPoint(AddCurve, ClickPosition, InsertAfterPointIndex,
                                                EditorParams.DefaultControlPointWeight);
                        AddCurve->SelectedControlPointIndex = InsertAfterPointIndex + 1;
                        EditorStateSelectEntity(&Result, EntityFromCurve(AddCurve));
                     } break;
                     
                     
                     // NOTE(hbr): Impossible cases due to flags.
                     case Collision_Image: { Assert(false); } break;
                  }
               }
            } break;
            
            case Button_Right: {
               check_collision_with_flags CheckCollisionWithFlags =
                  CheckCollisionWith_ControlPoints |
                  CheckCollisionWith_Images;
               
               f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(EditorParams.CollisionToleranceClipSpace,
                                                                    CoordinateSystemData);
               collision Collision = CheckCollisionWith(State.EntitiesHead, State.EntitiesTail,
                                                        ClickPosition, CollisionTolerance,
                                                        EditorParams, CheckCollisionWithFlags);
               
               switch (Collision.Type)
               {
                  case Collision_None: { EditorStateDeselectCurrentEntity(&Result); } break;
                  
                  case Collision_ControlPoint: {
                     if (!Collision.IsCubicBezierPoint)
                     {                        
                        curve *RemoveFromCurve = Collision.CollisionCurve;
                        u64 RemoveControlPointIndex = Collision.CollisionPointIndex;
                        
                        CurveRemoveControlPoint(RemoveFromCurve, RemoveControlPointIndex);
                        EditorStateSelectEntity(&Result, EntityFromCurve(RemoveFromCurve));
                     }
                  } break;
                  
                  case Collision_Image: {
                     EditorStateRemoveEntity(&Result, EntityFromImage(Collision.CollisionImage));
                  } break;
                  
                  // NOTE(hbr): Impossible case due to flags.
                  case Collision_CurvePoint: { Assert(false); } break;
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
                     curve *SplitCurve = State.SplittingBezierCurve.SplitCurve;
                     
                     world_position SplitPoint = State.SplittingBezierCurve.SplitPoint;
                     f32 SplitPointRadius =
                        ClipSpaceLengthToWorldSpace(EditorParams.BezierSplitPoint.RadiusClipSpace,
                                                    CoordinateSystemData);
                     
                     if (PointCollision(SplitPoint, DragFromWorldPosition, SplitPointRadius + CollisionTolerance))
                     {
                        Result.SplittingBezierCurve.DraggingSplitPoint = true;
                        EditorStateSelectEntity(&Result, EntityFromCurve(SplitCurve));
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
                  
                  switch (Collision.Type)
                  {
                     case Collision_None: { Result.Mode = EditorModeMovingCamera(); } break;
                     
                     case Collision_ControlPoint: {
                        curve *Curve = Collision.CollisionCurve;
                        u64 PointIndex = Collision.CollisionPointIndex;
                        b32 IsCubicBezierPoint = Collision.IsCubicBezierPoint;
                        
                        u64 SelectControlPointIndex = (IsCubicBezierPoint ? Curve->SelectedControlPointIndex : PointIndex);
                        Curve->SelectedControlPointIndex = SelectControlPointIndex;
                        EditorStateSelectEntity(&Result, EntityFromCurve(Curve));
                        Result.Mode = EditorModeMovingPoint(Curve, PointIndex, IsCubicBezierPoint, State.MovingPointArena);
                     } break;
                     
                     case Collision_CurvePoint: {
                        curve *Curve = Collision.CollisionCurve;
                        
                        EditorStateSelectEntity(&Result, EntityFromCurve(Curve));
                        Result.Mode = EditorModeMovingCurve(Curve);
                     } break;
                     
                     case Collision_Image: {
                        image *Image = Collision.CollisionImage;
                        
                        EditorStateSelectEntity(&Result, EntityFromImage(Image));
                        Result.Mode = EditorModeMovingImage(Image);
                     } break;
                  }
               }
            } break;
            
            case Button_Right: {
               entity *Entity = State.SelectedEntity;
               if (Entity)
               {
                  switch (Entity->Type)
                  {
                     case Entity_Curve: { Result.Mode = EditorModeRotatingCurve(&Entity->Curve, DragFromScreenPosition); } break;
                     case Entity_Image: { Result.Mode = EditorModeRotatingImage(&Entity->Image); } break;
                  }
               }
               else
               {
                  Result.Mode = EditorModeRotatingCamera();
               }
            } break;
            
            case Button_Middle: { Result.Mode = EditorModeMovingCamera(); } break;
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
         splitting_bezier_curve Split = State.SplittingBezierCurve;
         
         // NOTE(hbr): Pretty involved logic to move point along the curve and have pleasant experience
         if (State.SplittingBezierCurve.DraggingSplitPoint)
         {
            world_position FromPosition = ScreenToWorldSpace(Action.MouseMove.FromPosition, CoordinateSystemData);
            world_position ToPosition = ScreenToWorldSpace(Action.MouseMove.ToPosition, CoordinateSystemData);
            
            v2f32 Translation = ToPosition - FromPosition;
            
            curve *SplitCurve = Split.SplitCurve;
            u64 NumCurvePoints = SplitCurve->NumCurvePoints;
            local_position *CurvePoints = SplitCurve->CurvePoints;
            
            f32 T = Clamp(Split.T, 0.0f, 1.0f);
            f32 DeltaT = 1.0f / (NumCurvePoints - 1);
            
            {
               u64 SplitCurvePointIndex = ClampTop(cast(u64)FloorF32(T * (NumCurvePoints - 1)), NumCurvePoints - 1);
               while (SplitCurvePointIndex + 1 < NumCurvePoints)
               {
                  f32 NextT = cast(f32)(SplitCurvePointIndex + 1) / (NumCurvePoints - 1);
                  f32 PrevT = cast(f32)SplitCurvePointIndex / (NumCurvePoints - 1);
                  T = Clamp(T, PrevT, NextT);
                  
                  f32 SegmentFraction = (NextT - T) / DeltaT;
                  SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
                  
                  v2f32 CurveSegment =
                     LocalCurvePositionToWorld(SplitCurve, CurvePoints[SplitCurvePointIndex + 1])
                     - LocalCurvePositionToWorld(SplitCurve, CurvePoints[SplitCurvePointIndex]);
                  
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
               u64 SplitCurvePointIndex = ClampTop(cast(u64)CeilF32(T * (NumCurvePoints - 1)), NumCurvePoints - 1);
               while (SplitCurvePointIndex >= 1)
               {
                  f32 NextT = cast(f32)(SplitCurvePointIndex - 1) / (NumCurvePoints - 1);
                  f32 PrevT = cast(f32)SplitCurvePointIndex / (NumCurvePoints - 1);
                  T = Clamp(T, NextT, PrevT);
                  
                  f32 SegmentFraction = (T - NextT) / DeltaT;
                  SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
                  
                  v2f32 CurveSegment =
                     LocalCurvePositionToWorld(SplitCurve, CurvePoints[SplitCurvePointIndex - 1])
                     - LocalCurvePositionToWorld(SplitCurve, CurvePoints[SplitCurvePointIndex]);
                  
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

internal v2f32
GetTranslationInWorldSpace(screen_position FromPosition,
                           screen_position ToPosition,
                           coordinate_system_data CoordinateSystemData)
{
   world_position FromWorldPosition = ScreenToWorldSpace(FromPosition, CoordinateSystemData);
   world_position ToWorldPosition = ScreenToWorldSpace(ToPosition, CoordinateSystemData);
   v2f32 Result = ToWorldPosition - FromWorldPosition;
   
   return Result;
}

internal editor_state
EditorStateMovingPointModeUpdate(editor_state State, user_action Action,
                                 coordinate_system_data CoordinateSystemData)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         v2f32 Translation = GetTranslationInWorldSpace(Action.MouseMove.FromPosition,
                                                        Action.MouseMove.ToPosition,
                                                        CoordinateSystemData);
         
         b32 MatchCubicBezierHelpersDirection = (!Action.UserInput->Keys[Key_LeftCtrl].Pressed);
         b32 MatchCubicBezierHelpersLength = (!Action.UserInput->Keys[Key_LeftShift].Pressed);
         
         CurveTranslatePoint(State.Mode.MovingPoint.Curve,
                             State.Mode.MovingPoint.PointIndex,
                             State.Mode.MovingPoint.CubicBezierPointMoving,
                             MatchCubicBezierHelpersDirection,
                             MatchCubicBezierHelpersLength,
                             Translation);
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Left: { Result.Mode  = EditorModeNormal(); } break;
            case Button_Right:
            case Button_Middle: {} break;
            case Button_Count: { Assert(false); } break;
         }
      } break;
      
      case UserAction_None:
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag: {} break;
   }
   
   return Result;
}

internal editor_state
EditorStateMovingCurveModeUpdate(editor_state State, user_action Action,
                                 coordinate_system_data CoordinateSystemData)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         v2f32 Translation = GetTranslationInWorldSpace(Action.MouseMove.FromPosition,
                                                        Action.MouseMove.ToPosition,
                                                        CoordinateSystemData);
         
         curve *MovingCurve = State.Mode.MovingCurve;
         MovingCurve->Position += Translation;
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Left: { Result.Mode = EditorModeNormal(); } break;
            case Button_Right:
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
EditorStateMovingImageModeUpdate(editor_state State, user_action Action,
                                 coordinate_system_data CoordinateSystemData)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         v2f32 Translation = GetTranslationInWorldSpace(Action.MouseMove.FromPosition,
                                                        Action.MouseMove.ToPosition,
                                                        CoordinateSystemData);
         
         image *MovingImage = State.Mode.MovingImage;
         MovingImage->Position += Translation;
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Left: { Result.Mode  = EditorModeNormal(); } break;
            case Button_Right:
            case Button_Middle: {} break;
            case Button_Count: { Assert(false); } break;
         }
      } break;
      
      case UserAction_None:
      case UserAction_ButtonClicked:
      case UserAction_ButtonDrag: {} break;
   }
   
   return Result;
}

internal editor_state
EditorStateMovingCameraModeUpdate(editor_state State, user_action Action,
                                  coordinate_system_data CoordinateSystemData)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         v2f32 Translation = GetTranslationInWorldSpace(Action.MouseMove.FromPosition,
                                                        Action.MouseMove.ToPosition,
                                                        CoordinateSystemData);
         
         CameraMove(&Result.Camera, -Translation);
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Left: { Result.Mode = EditorModeNormal(); } break;
            // TODO(hbr): Maybe add to the state actual button that was the reason that camera moved
            case Button_Middle: { Result.Mode = EditorModeNormal(); } break;
            case Button_Right: {} break;
            case Button_Count: { Assert(false); } break;
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
CalculateRotationIfStable(screen_position FromPosition,
                          screen_position ToPosition,
                          camera_position RotationCenterInCameraSpace,
                          coordinate_system_data CoordinateSystemData,
                          editor_parameters EditorParams)
{
   stable_rotation Result = {};
   
   world_position FromPositionInWorldSpace   = ScreenToWorldSpace(FromPosition,
                                                                  CoordinateSystemData);
   world_position ToPositionInWorldSpace     = ScreenToWorldSpace(ToPosition,
                                                                  CoordinateSystemData);
   world_position RotationCenterInWorldSpace = CameraToWorldSpace(RotationCenterInCameraSpace,
                                                                  CoordinateSystemData);
   
   f32 RotationDeadDistance = ClipSpaceLengthToWorldSpace(EditorParams.RotationIndicator.RadiusClipSpace,
                                                          CoordinateSystemData);
   
   if (NormSquared(FromPositionInWorldSpace - RotationCenterInWorldSpace) >= Square(RotationDeadDistance) &&
       NormSquared(ToPositionInWorldSpace   - RotationCenterInWorldSpace) >= Square(RotationDeadDistance))
   {
      Result.IsStable = true;
      Result.RotationCenter = RotationCenterInWorldSpace;
      Result.Rotation = Rotation2DFromMovementAroundPoint(FromPositionInWorldSpace,
                                                          ToPositionInWorldSpace,
                                                          RotationCenterInWorldSpace);
   }
   
   return Result;
}

internal editor_state
EditorStateRotatingCurveModeUpdate(editor_state State, user_action Action,
                                   coordinate_system_data CoordinateSystemData,
                                   editor_parameters EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         curve *RotatingCurve = State.Mode.RotatingCurve.Curve;
         
         camera_position RotationCenter =
            ScreenToCameraSpace(State.Mode.RotatingCurve.RotationCenter,
                                CoordinateSystemData);
         
         stable_rotation StableRotation =
            CalculateRotationIfStable(Action.MouseMove.FromPosition,
                                      Action.MouseMove.ToPosition,
                                      RotationCenter,
                                      CoordinateSystemData,
                                      EditorParams);
         
         if (StableRotation.IsStable)
         {
            CurveRotateAround(RotatingCurve,
                              StableRotation.RotationCenter,
                              StableRotation.Rotation);
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
EditorStateRotatingImageModeUpdate(editor_state State, user_action Action,
                                   coordinate_system_data CoordinateSystemData,
                                   editor_parameters EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         image *RotatingImage = State.Mode.RotatingImage;
         
         camera_position RotationCenter =
            WorldToCameraSpace(RotatingImage->Position,
                               CoordinateSystemData);
         
         stable_rotation StableRotation =
            CalculateRotationIfStable(Action.MouseMove.FromPosition,
                                      Action.MouseMove.ToPosition,
                                      RotationCenter,
                                      CoordinateSystemData,
                                      EditorParams);
         
         if (StableRotation.IsStable)
         {
            rotation_2d ImageNewRotation = CombineRotations2D(RotatingImage->Rotation,
                                                              StableRotation.Rotation);
            RotatingImage->Rotation = ImageNewRotation;
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
EditorStateRotatingCameraModeUpdate(editor_state State, user_action Action,
                                    coordinate_system_data CoordinateSystemData,
                                    editor_parameters EditorParams)
{
   editor_state Result = State;
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         camera_position RotationCenter =
            WorldToCameraSpace(State.Camera.Position,
                               CoordinateSystemData);
         
         stable_rotation StableRotation =
            CalculateRotationIfStable(Action.MouseMove.FromPosition,
                                      Action.MouseMove.ToPosition,
                                      RotationCenter,
                                      CoordinateSystemData,
                                      EditorParams);
         
         if (StableRotation.IsStable)
         {
            rotation_2d InverseRotation = Rotation2DInverse(StableRotation.Rotation);
            CameraRotate(&Result.Camera, InverseRotation);
         }
      } break;
      
      case UserAction_ButtonReleased: {
         button ReleaseButton = Action.Release.Button;
         switch (ReleaseButton)
         {
            case Button_Right: { Result.Mode = EditorModeNormal(); } break;
            case Button_Left: {} break;
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
      case EditorMode_Normal:               { Result = EditorStateNormalModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
      case EditorMode_MovingPoint:          { Result = EditorStateMovingPointModeUpdate(State, Action, CoordinateSystemData); } break;
      case EditorMode_MovingCurve:          { Result = EditorStateMovingCurveModeUpdate(State, Action, CoordinateSystemData); } break;
      case EditorMode_MovingImage:          { Result = EditorStateMovingImageModeUpdate(State, Action, CoordinateSystemData); } break;
      case EditorMode_MovingCamera:         { Result = EditorStateMovingCameraModeUpdate(State, Action, CoordinateSystemData); } break;
      case EditorMode_RotatingCurve:        { Result = EditorStateRotatingCurveModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
      case EditorMode_RotatingImage:        { Result = EditorStateRotatingImageModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
      case EditorMode_RotatingCamera:       { Result = EditorStateRotatingCameraModeUpdate(State, Action, CoordinateSystemData, EditorParams); } break;
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
   b32 DrawRotationIndicator = false;
   camera_position RotationIndicatorPosition = {};
   
   switch (EditorState->Mode.Type)
   {
      case EditorMode_RotatingCurve: {
         DrawRotationIndicator = true;
         RotationIndicatorPosition = ScreenToWorldSpace(EditorState->Mode.RotatingCurve.RotationCenter,
                                                        CoordinateSystemData);
      } break;
      
      case EditorMode_RotatingImage: {
         DrawRotationIndicator = true;
         RotationIndicatorPosition = EditorState->Mode.RotatingImage->Position;
      } break;
      
      case EditorMode_RotatingCamera: {
         DrawRotationIndicator = true;
         RotationIndicatorPosition = EditorState->Camera.Position;
      } break;
      
      case EditorMode_Normal:
      case EditorMode_MovingPoint:
      case EditorMode_MovingCamera:
      case EditorMode_MovingCurve:
      case EditorMode_MovingImage: {} break;
   }
   
   if (DrawRotationIndicator)
   {
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
   error_string Error = 0;
   
   switch (SaveProjectFormat)
   {
      case SaveProjectFormat_ProjectFile: {
         Error = SaveProjectInFile(Arena, *Editor, SaveProjectFilePath);
      } break;
      
      case SaveProjectFormat_ImageFile: {
         sf::RenderWindow *Window = Editor->Window;
         sf::Image Screenshot = Window->capture();
         
         bool SaveSuccess = Screenshot.saveToFile(SaveProjectFilePath);
         if (!SaveSuccess)
         {
            Error = StringMakeFormat(Arena,
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
   name_string Result = NameStringFormat("%s (copy)", OriginalName.Str);
   return Result;
}

internal void
CopyAndAddCurve(curve *Curve, editor *Editor)
{
   curve CurveTemplate = CurveCopy(*Curve);
   CurveTemplate.Name = CreateNameForCopiedEntity(Curve->Name);
   
   coordinate_system_data CoordinateSystemData =
      CoordinateSystemDataMake(Editor->Window,
                               Editor->State.Camera,
                               Editor->Projection);
   
   f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, CoordinateSystemData);
   v2f32 SlightTranslation = V2F32(SlightTranslationX, 0.0f);
   CurveTemplate.Position += SlightTranslation;
   
   entity EntityTemplate = CurveEntity(CurveTemplate);
   entity *CopiedEntity = EditorStateAddEntity(&Editor->State, &EntityTemplate);
   EditorStateSelectEntity(&Editor->State, CopiedEntity);
}

internal void
CopyAndAddImage(image *Image, editor *Editor)
{
   image ImageTemplate = ImageCopy(*Image);
   ImageTemplate.Name = CreateNameForCopiedEntity(Image->Name);
   
   coordinate_system_data CoordinateSystemData =
      CoordinateSystemDataMake(Editor->Window,
                               Editor->State.Camera,
                               Editor->Projection);
   
   f32 SlightTranslationX = ClipSpaceLengthToWorldSpace(0.2f, CoordinateSystemData);
   v2f32 SlightTranslation = V2F32(SlightTranslationX, 0.0f);
   ImageTemplate.Position += SlightTranslation;
   
   entity EntityTemplate = ImageEntity(ImageTemplate);
   entity *CopiedEntity = EditorStateAddEntity(&Editor->State, &EntityTemplate);
   EditorStateSelectEntity(&Editor->State, CopiedEntity);
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
            
            int NumCurvePointsPerSegmentAsInt = cast(int)(*NumCurvePointsPerSegment);
            SomeCurveParameterChanged |= ImGui::SliderInt("Detail",
                                                          &NumCurvePointsPerSegmentAsInt,
                                                          1,    // v_min
                                                          2000, // v_max
                                                          "%d", // v_format
                                                          ImGuiSliderFlags_Logarithmic);
            *NumCurvePointsPerSegment = cast(u64)NumCurvePointsPerSegmentAsInt;
            
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
            local f32 ControlPointMinimumWeight = Epsilon32;
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
                           DeferBlock(ImGui::PushID(cast(int)ControlPointIndex), ImGui::PopID())
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

internal void
BeginCurveCombining(curve_combining *Combining, curve *CombineCurve)
{
   Combining->IsCombining = true;
   Combining->CombineCurve = CombineCurve;
   Combining->TargetCurve = 0;
   Combining->CombineCurveLastControlPoint = false;
   Combining->TargetCurveFirstControlPoint = false;
   Combining->CombinationType = CurveCombination_Merge;
}

internal void
BeginSplittingBezierCurve(splitting_bezier_curve *Splitting, curve *Curve)
{
   Splitting->IsSplitting = true;
   Splitting->SplitCurve = Curve;
   Splitting->DraggingSplitPoint = false;
   Splitting->T = 0.0f;
   Splitting->SavedCurveVersion = U64_MAX;
}

internal void
BeginVisualizingDeCasteljauAlgorithm(de_casteljau_visualization *Visualization, curve *Curve)
{
   Visualization->IsVisualizing = true;
   Visualization->T = 0.0f;
   Visualization->Curve = Curve;
   Visualization->SavedCurveVersion = U64_MAX;
   ArenaClear(Visualization->Arena);
}

internal void
BeginAnimatingCurve(transform_curve_animation *Animation, curve *Curve)
{
   Animation->Stage = AnimateCurveAnimation_PickingTarget;
   Animation->FromCurve = Curve;
   Animation->ToCurve = 0;
}

internal void
LowerBezierCurveDegree(bezier_curve_degree_lowering *Lowering, curve *Curve)
{
   u64 NumControlPoints = Curve->NumControlPoints;
   if (NumControlPoints > 0)
   {
      Assert(Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      
      auto Scratch = ScratchArena(Lowering->Arena);
      defer { ReleaseScratch(Scratch); };
      
      local_position *NewControlPoints = PushArray(Scratch.Arena,
                                                   NumControlPoints,
                                                   local_position);
      f32 *NewControlPointWeights = PushArray(Scratch.Arena, NumControlPoints, f32);
      local_position *NewCubicBezierPoints = PushArray(Scratch.Arena,
                                                       3 * (NumControlPoints - 1),
                                                       local_position);
      MemoryCopy(NewControlPoints,
                 Curve->ControlPoints,
                 NumControlPoints * sizeof(NewControlPoints[0]));
      MemoryCopy(NewControlPointWeights,
                 Curve->ControlPointWeights,
                 NumControlPoints * sizeof(NewControlPointWeights[0]));
      
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
         Lowering->Curve = Curve;
         
         ArenaClear(Lowering->Arena);
         
         Lowering->SavedControlPoints = PushArray(Lowering->Arena, NumControlPoints, local_position);
         Lowering->SavedControlPointWeights = PushArray(Lowering->Arena, NumControlPoints, f32);
         Lowering->SavedCubicBezierPoints = PushArray(Lowering->Arena, 3 * NumControlPoints, local_position);
         MemoryCopy(Lowering->SavedControlPoints,
                    Curve->ControlPoints,
                    NumControlPoints * sizeof(Lowering->SavedControlPoints[0]));
         MemoryCopy(Lowering->SavedControlPointWeights,
                    Curve->ControlPointWeights,
                    NumControlPoints * sizeof(Lowering->SavedControlPointWeights[0]));
         MemoryCopy(Lowering->SavedCubicBezierPoints,
                    Curve->CubicBezierPoints,
                    3 * NumControlPoints * sizeof(Lowering->SavedCubicBezierPoints[0]));
         
         line_vertices CurveVertices = Curve->CurveVertices;
         Lowering->NumSavedCurveVertices = CurveVertices.NumVertices;
         Lowering->SavedCurveVertices = PushArray(Lowering->Arena, CurveVertices.NumVertices, sf::Vertex);
         Lowering->SavedPrimitiveType = CurveVertices.PrimitiveType;
         MemoryCopy(Lowering->SavedCurveVertices,
                    CurveVertices.Vertices,
                    CurveVertices.NumVertices * sizeof(Lowering->SavedCurveVertices[0]));
         for (u64 CurveVertexIndex = 0;
              CurveVertexIndex < CurveVertices.NumVertices;
              ++CurveVertexIndex)
         {
            sf::Vertex *Vertex = Lowering->SavedCurveVertices + CurveVertexIndex;
            Vertex->color.a = cast(u8)(0.5f * Vertex->color.a);
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
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   u64 NumControlPoints = Curve->NumControlPoints;
   local_position *ElevatedControlPoints = PushArray(Scratch.Arena,
                                                     NumControlPoints + 1,
                                                     local_position);
   f32 *ElevatedControlPointWeights = PushArray(Scratch.Arena,
                                                NumControlPoints + 1,
                                                f32);
   MemoryCopy(ElevatedControlPoints,
              Curve->ControlPoints,
              NumControlPoints * sizeof(ElevatedControlPoints[0]));
   MemoryCopy(ElevatedControlPointWeights,
              Curve->ControlPointWeights,
              NumControlPoints * sizeof(ElevatedControlPointWeights[0]));
   
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
   
   local_position *ElevatedCubicBezierPoints = PushArray(Scratch.Arena,
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
FocusCameraOnCurve(camera *Camera, curve *Curve,
                   coordinate_system_data CoordinateSystemData)
{
   if (Curve->NumControlPoints > 0)
   {
      f32 Left = F32_INF, Right = -F32_INF;
      f32 Down = F32_INF, Top = -F32_INF;
      for (u64 ControlPointIndex = 0;
           ControlPointIndex < Curve->NumControlPoints;
           ++ControlPointIndex)
      {
         world_position ControlPointWorld =
            LocalCurvePositionToWorld(Curve,
                                      Curve->ControlPoints[ControlPointIndex]);
         v2f32 ControlPointRotated = RotateAround(ControlPointWorld,
                                                  V2F32(0.0f, 0.0f),
                                                  Rotation2DInverse(Camera->Rotation));
         
         if (ControlPointRotated.X < Left)  Left  = ControlPointRotated.X;
         if (ControlPointRotated.X > Right) Right = ControlPointRotated.X;
         if (ControlPointRotated.Y < Down)  Down  = ControlPointRotated.Y;
         if (ControlPointRotated.Y > Top)   Top   = ControlPointRotated.Y;
      }
      
      world_position FocusPosition = 
         RotateAround(V2F32(0.5f * (Left + Right), 0.5f * (Down + Top)),
                      V2F32(0.0f, 0.0f), Camera->Rotation);
      v2f32 Extents = V2F32(0.5f * (Right - Left) + 2.0f * Curve->CurveParams.PointSize,
                            0.5f * (Top - Down) + 2.0f * Curve->CurveParams.PointSize);
      FocusCameraOn(Camera, FocusPosition, Extents, CoordinateSystemData);
   }
}

internal void
FocusCameraOnImage(camera *Camera, image *Image,
                   coordinate_system_data CoordinateSystemData)
{
   f32 ScaleX = Abs(Image->Scale.X);
   f32 ScaleY = Abs(Image->Scale.Y);
   if (ScaleX != 0.0f && ScaleY != 0.0f)
   {
      sf::Vector2u TextureSize = Image->Texture.getSize();
      v2f32 Extents = V2F32(0.5f * GlobalImageScaleFactor * ScaleX * TextureSize.x,
                            0.5f * GlobalImageScaleFactor * ScaleY * TextureSize.y);
      
      FocusCameraOn(Camera, Image->Position, Extents, CoordinateSystemData);
   }
}

internal void
SplitCurveOnControlPoint(curve *Curve, editor_state *EditorState)
{
   Assert(Curve->SelectedControlPointIndex < Curve->NumControlPoints);
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   u64 NumLeftControlPoints = Curve->SelectedControlPointIndex + 1;
   u64 NumRightControlPoints = Curve->NumControlPoints - Curve->SelectedControlPointIndex;
   
   local_position *LeftControlPoints = PushArray(Scratch.Arena,
                                                 NumLeftControlPoints,
                                                 local_position);
   f32 *LeftControlPointWeights = PushArray(Scratch.Arena,
                                            NumLeftControlPoints,
                                            f32);
   local_position *LeftCubicBezierPoints = PushArray(Scratch.Arena,
                                                     3 * NumLeftControlPoints,
                                                     local_position);
   
   MemoryCopy(LeftControlPoints,
              Curve->ControlPoints,
              NumLeftControlPoints * sizeof(LeftControlPoints[0]));
   MemoryCopy(LeftControlPointWeights,
              Curve->ControlPointWeights,
              NumLeftControlPoints * sizeof(LeftControlPointWeights[0]));
   MemoryCopy(LeftCubicBezierPoints,
              Curve->CubicBezierPoints,
              3 * NumLeftControlPoints * sizeof(LeftControlPoints[0]));
   
   local_position *RightControlPoints = PushArray(Scratch.Arena,
                                                  NumRightControlPoints,
                                                  local_position);
   f32 *RightControlPointWeights = PushArray(Scratch.Arena,
                                             NumRightControlPoints,
                                             f32);
   local_position *RightCubicBezierPoints = PushArray(Scratch.Arena,
                                                      3 * NumRightControlPoints,
                                                      local_position);
   
   MemoryCopy(RightControlPoints,
              Curve->ControlPoints + Curve->SelectedControlPointIndex,
              NumRightControlPoints * sizeof(RightControlPoints[0]));
   MemoryCopy(RightControlPointWeights,
              Curve->ControlPointWeights + Curve->SelectedControlPointIndex,
              NumRightControlPoints * sizeof(RightControlPointWeights[0]));
   MemoryCopy(RightCubicBezierPoints,
              Curve->CubicBezierPoints + 3 * Curve->SelectedControlPointIndex,
              3 * NumRightControlPoints * sizeof(RightControlPoints[0]));
   
   // TODO(hbr): Optimize to not do so many copies and allocations
   curve *LeftCurve = Curve;
   entity RightEntityTemplate = CurveEntity(CurveCopy(*LeftCurve));
   entity *RightEntity = EditorStateAddEntity(EditorState, &RightEntityTemplate);
   curve *RightCurve = &RightEntity->Curve;
   
   CurveSetControlPoints(LeftCurve, NumLeftControlPoints, LeftControlPoints,
                         LeftControlPointWeights, LeftCubicBezierPoints);
   CurveSetControlPoints(RightCurve, NumRightControlPoints, RightControlPoints,
                         RightControlPointWeights, RightCubicBezierPoints);
   
   name_string LeftCurveName = NameStringFormat("%s (left)", LeftCurve->Name.Str);
   name_string RightCurveName = NameStringFormat("%s (right)", RightCurve->Name.Str);
   LeftCurve->Name = LeftCurveName;
   RightCurve->Name = RightCurveName;
}

internal void
RenderSelectedCurveUI(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   Assert(Editor->State.SelectedEntity &&
          Editor->State.SelectedEntity->Type == Entity_Curve);
   
   DeferBlock(ImGui::PushID("SelectedCurve"), ImGui::PopID())
   {
      bool ViewSelectedCurveWindowAsBool = cast(bool)Editor->UI_Config.ViewSelectedCurveWindow;
      DeferBlock(ImGui::Begin("Curve Parameters", &ViewSelectedCurveWindowAsBool,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {
         Editor->UI_Config.ViewSelectedCurveWindow = ViewSelectedCurveWindowAsBool;
         
         if (Editor->UI_Config.ViewSelectedCurveWindow)
         {
            curve *SelectedCurve = &Editor->State.SelectedEntity->Curve;
            curve_params *SelectedCurveParams = &SelectedCurve->CurveParams;
            
            bool DeleteCurve                   = false;
            bool CopyCurve                     = false;
            bool SwitchVisibility              = false;
            bool SomeCurveParameterChanged     = false;
            bool ElevateBezierCurve            = false;
            bool LowerBezierCurve              = false;
            bool VisualizeDeCasteljauAlgorithm = false;
            bool SplitBezierCurve              = false;
            bool DeselectCurve                 = false;
            bool AnimateCurve                  = false;
            bool CombineCurve                  = false;
            bool FocusOn                       = false;
            bool SplitOnControlPoint           = false;
            
            ImGui::InputText("Name",
                             SelectedCurve->Name.Str,
                             ArrayCount(SelectedCurve->Name.Str));
            
            {
               local f32 PositionDeltaClipSpace = 0.001f;
               f32 DragPositionSpeed = ClipSpaceLengthToWorldSpace(PositionDeltaClipSpace,
                                                                   CoordinateSystemData);
               ImGui::DragFloat2("Position", SelectedCurve->Position.Es, DragPositionSpeed);
               if (ResetContextMenuItemSelected("PositionReset"))
               {
                  SelectedCurve->Position = V2F32(0.0f, 0.0f);
               }
            }
            {
               f32 CurveRotationRadians = Rotation2DToRadians(SelectedCurve->Rotation);
               ImGui::SliderAngle("Rotation", &CurveRotationRadians);
               if (ResetContextMenuItemSelected("RotationReset"))
               {
                  CurveRotationRadians = 0.0f;
               }
               SelectedCurve->Rotation = Rotation2DFromRadians(CurveRotationRadians);
            }
            {
               change_curve_params_ui_mode UIMode =
                  ChangeCurveShapeUIModeSelectedCurve(&Editor->Parameters.CurveDefaultParams,
                                                      &Editor->Parameters.DefaultControlPointWeight,
                                                      SelectedCurve);
               
               ImGui::NewLine();
               SomeCurveParameterChanged |=
                  RenderChangeCurveParametersUI("SelectedCurveShape", UIMode);
            }
            
            ImGui::NewLine();
            ImGui::SeparatorText("Actions");
            {
               DeleteCurve = ImGui::Button("Delete");
               ImGui::SameLine();
               CopyCurve = ImGui::Button("Copy");
               ImGui::SameLine();
               SwitchVisibility = ImGui::Button(SelectedCurveParams->Hidden ? "Show" : "Hide");
               ImGui::SameLine();
               DeselectCurve = ImGui::Button("Deselect");
               ImGui::SameLine();
               FocusOn = ImGui::Button("Focus");
               
               // TODO(hbr): Maybe pick better name than transform
               AnimateCurve = ImGui::Button("Animate");
               ImGui::SameLine();
               // TODO(hbr): Maybe pick better name than "Combine"
               CombineCurve = ImGui::Button("Combine");
               
               DeferBlock(ImGui::BeginDisabled(SelectedCurve->SelectedControlPointIndex >=
                                               SelectedCurve->NumControlPoints),
                          ImGui::EndDisabled())
               {
                  ImGui::SameLine();
                  SplitOnControlPoint = ImGui::Button("Split on Control Point");
               }
               
               b32 IsBezierNormalOrWeighted =
               (SelectedCurveParams->CurveShape.InterpolationType == Interpolation_Bezier &&
                (SelectedCurveParams->CurveShape.BezierType == Bezier_Normal ||
                 SelectedCurveParams->CurveShape.BezierType == Bezier_Weighted));
               
               DeferBlock(ImGui::BeginDisabled(!IsBezierNormalOrWeighted),
                          ImGui::EndDisabled())
               {
                  DeferBlock(ImGui::BeginDisabled(SelectedCurve->NumControlPoints < 2),
                             ImGui::EndDisabled())
                  {
                     ImGui::SameLine();
                     SplitBezierCurve = ImGui::Button("Split");
                  }
                  
                  ElevateBezierCurve = ImGui::Button("Elevate Degree");
                  
                  DeferBlock(ImGui::BeginDisabled(SelectedCurve->NumControlPoints == 0),
                             ImGui::EndDisabled())
                  {
                     ImGui::SameLine();
                     LowerBezierCurve = ImGui::Button("Lower Degree");
                  }
                  
                  VisualizeDeCasteljauAlgorithm = ImGui::Button("Visualize De Casteljau's Algorithm");
               }
            }
            
            if (DeleteCurve)
            {
               EditorStateRemoveEntity(&Editor->State, Editor->State.SelectedEntity);
            }
            
            if (CopyCurve)
            {
               CopyAndAddCurve(SelectedCurve, Editor);
            }
            
            if (SwitchVisibility)
            {
               SelectedCurveParams->Hidden = !SelectedCurveParams->Hidden;
            }
            
            if (ElevateBezierCurve)
            {
               ElevateBezierCurveDegree(SelectedCurve, &Editor->Parameters);
            }
            
            if (LowerBezierCurve)
            {
               LowerBezierCurveDegree(&Editor->State.DegreeLowering, SelectedCurve);
            }
            
            if (SplitBezierCurve)
            {
               BeginSplittingBezierCurve(&Editor->State.SplittingBezierCurve, SelectedCurve);
            }
            
            if (VisualizeDeCasteljauAlgorithm)
            {
               BeginVisualizingDeCasteljauAlgorithm(&Editor->State.DeCasteljauVisualization,
                                                    SelectedCurve);
            }
            
            if (DeselectCurve)
            {
               EditorStateDeselectCurrentEntity(&Editor->State);
            }
            
            if (AnimateCurve)
            {
               BeginAnimatingCurve(&Editor->State.CurveAnimation, SelectedCurve);
            }
            
            if (CombineCurve)
            {
               BeginCurveCombining(&Editor->State.CurveCombining, SelectedCurve);
            }
            
            if (FocusOn)
            {
               FocusCameraOnCurve(&Editor->State.Camera, SelectedCurve, CoordinateSystemData);
            }
            
            if (SplitOnControlPoint)
            {
               SplitCurveOnControlPoint(SelectedCurve, &Editor->State);
            }
            
            if (SomeCurveParameterChanged)
            {
               CurveRecompute(SelectedCurve);
            }
         }
      }
   }
}

internal void
RenderSelectedImageUI(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   Assert(Editor->State.SelectedEntity &&
          Editor->State.SelectedEntity->Type == Entity_Image);
   
   DeferBlock(ImGui::PushID("SelectedImage"), ImGui::PopID())
   {
      bool ViewSelectedImageWindowAsBool = cast(bool)Editor->UI_Config.ViewSelectedImageWindow;
      DeferBlock(ImGui::Begin("Image Parameters", &ViewSelectedImageWindowAsBool,
                              ImGuiWindowFlags_AlwaysAutoResize),
                 ImGui::End())
      {
         Editor->UI_Config.ViewSelectedImageWindow = ViewSelectedImageWindowAsBool;
         
         if (Editor->UI_Config.ViewSelectedImageWindow)
         {
            image *SelectedImage = &Editor->State.SelectedEntity->Image;
            
            bool Delete           = false;
            bool Copy             = false;
            bool SwitchVisibility = false;
            bool FocusOn          = false;
            
            name_string *ImageName = &SelectedImage->Name;
            
            f32 ImageRotationRadians = Rotation2DToRadians(SelectedImage->Rotation);
            
            sf::Vector2u ImageTextureSize = SelectedImage->Texture.getSize();
            v2f32 SelectedImageScale = SelectedImage->Scale;
            f32 ImageWidth = SelectedImageScale.X * ImageTextureSize.x;
            f32 ImageHeight = SelectedImageScale.Y * ImageTextureSize.y;
            f32 ImageScale = 1.0f;
            
            int ImageSortingLayerAsInt = cast(int)SelectedImage->SortingLayer;
            
            b32 IsImageHidden = SelectedImage->Hidden;
            
            ImGui::InputText("Name",
                             ImageName->Str,
                             ArrayCount(ImageName->Str));
            
            ImGui::NewLine();
            ImGui::SeparatorText("Image");
            
            ImGui::SliderAngle("Rotation", &ImageRotationRadians);
            if (ResetContextMenuItemSelected("RotationReset"))
            {
               ImageRotationRadians = 0.0f;
            }
            
            {            
               local f32 ImageDimensionsDragSpeed = 1.0f;
               local char const *ImageDimensionsDragFormat = "%.1f";
               
               ImGui::DragFloat("Width", &ImageWidth,
                                ImageDimensionsDragSpeed,
                                0.0f, 0.0f,
                                ImageDimensionsDragFormat);
               if (ResetContextMenuItemSelected("WidthReset"))
               {
                  ImageWidth = cast(f32)ImageTextureSize.x;
               }
               
               ImGui::DragFloat("Height", &ImageHeight,
                                ImageDimensionsDragSpeed,
                                0.0f, 0.0f,
                                ImageDimensionsDragFormat);
               if (ResetContextMenuItemSelected("HeightReset"))
               {
                  ImageHeight = cast(f32)ImageTextureSize.y;
               }
            }
            
            ImGui::DragFloat("Scale", &ImageScale,
                             0.001f,      // v_speed
                             0.0f,        // v_min
                             FLT_MAX,     // v_max
                             "Drag Me!"); // format
            if (ResetContextMenuItemSelected("ScaleReset"))
            {
               ImageWidth = cast(f32)ImageTextureSize.x;
               ImageHeight = cast(f32)ImageTextureSize.y;
            }
            
            ImGui::SliderInt("Sorting Layer", &ImageSortingLayerAsInt, -100, 100);
            if (ResetContextMenuItemSelected("SortingLayerReset"))
            {
               ImageSortingLayerAsInt = 0;
            }
            
            ImGui::NewLine();
            ImGui::SeparatorText("Actions");
            Delete = ImGui::Button("Delete");
            ImGui::SameLine();
            Copy = ImGui::Button("Copy");
            ImGui::SameLine();
            SwitchVisibility = ImGui::Button(IsImageHidden ? "Show" : "Hide");
            ImGui::SameLine();
            FocusOn = ImGui::Button("Focus");
            
            if (Delete)
            {
               EditorStateRemoveEntity(&Editor->State, Editor->State.SelectedEntity);
            }
            
            if (Copy)
            {
               CopyAndAddImage(SelectedImage, Editor);
            }
            
            if (FocusOn)
            {
               FocusCameraOnImage(&Editor->State.Camera, SelectedImage, CoordinateSystemData);
            }
            
            SelectedImage->Rotation = Rotation2DFromRadians(ImageRotationRadians);
            
            f32 ImageNewScaleX = ImageScale * ImageWidth / ImageTextureSize.x;
            f32 ImageNewScaleY = ImageScale * ImageHeight / ImageTextureSize.y;
            SelectedImage->Scale = V2F32(ImageNewScaleX, ImageNewScaleY);
            
            SelectedImage->SortingLayer = cast(s64)ImageSortingLayerAsInt;
            
            SelectedImage->Hidden = (SwitchVisibility ? !IsImageHidden : IsImageHidden);
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
UpdateAndRenderNotificationSystem(notification_system *NotificationSystem,
                                  f32 DeltaTime, sf::RenderWindow *Window)
{
   DeferBlock(ImGui::PushID("NotificationSystem"), ImGui::PopID())
   {
      sf::Vector2u WindowSize = Window->getSize();
      f32 NotificationWindowPadding = NotificationWindowPaddingScale * WindowSize.x;
      f32 NotificationWindowPositionX = WindowSize.x - NotificationWindowPadding;
      f32 NotificationWindowDesignatedPositionY = WindowSize.y - NotificationWindowPadding;
      
      local ImGuiWindowFlags NotificationWindowFlags =
         ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoDecoration |
         ImGuiWindowFlags_NoNavFocus |
         ImGuiWindowFlags_NoFocusOnAppearing;
      
      local ImGuiCond NotificationWindowPositionCondition = ImGuiCond_Always;
      local ImVec2 NotificationWindowPivot = ImVec2(1.0f, 1.0f);
      
      // NOTE(hbr): Scale in respect to window size
      local f32 NotificationWindowWidthScale = 0.1f;
      local f32 NotificationWindowMovementSpeed = 20.0f;
      
      f32 NotificationWindowWidth = NotificationWindowWidthScale * WindowSize.x;
      ImVec2 NotificationWindowMinSize = ImVec2(NotificationWindowWidth, 0.0f);
      ImVec2 NotificationWindowMaxSize = ImVec2(NotificationWindowWidth, FLT_MAX);
      
      notification *Notifications = NotificationSystem->Notifications;
      notification *FreeSpace = Notifications;
      u64 NumNotificationsRemoved = 0;
      
      for (u64 NotificationIndex = 0;
           NotificationIndex < NotificationSystem->NumNotifications;
           ++NotificationIndex)
      {
         notification *Notification = &Notifications[NotificationIndex];
         
         b32 ShouldBeRemoved = false;
         
         f32 NotificationLifeTime = Notification->LifeTime + DeltaTime;
         Notification->LifeTime= NotificationLifeTime;
         
         if (NotificationLifeTime < NOTIFICATION_TOTAL_LIFE_TIME)
         {
            color NotificationTitleColor = Notification->TitleColor;
            
            // NOTE(hbr): Interpolate position
            f32 NotificationWindowCurrentPositionY = Notification->PositionY;
            f32 NotificationWindowNextPositionY =
               LerpF32(NotificationWindowCurrentPositionY,
                       NotificationWindowDesignatedPositionY,
                       1.0f - PowF32(2.0f, -NotificationWindowMovementSpeed * DeltaTime));
            if (ApproxEq32(NotificationWindowDesignatedPositionY,
                           NotificationWindowNextPositionY))
            {
               NotificationWindowNextPositionY = NotificationWindowDesignatedPositionY;
            }
            Notification->PositionY = NotificationWindowNextPositionY;
            
            ImVec2 NotificationWindowPosition =
               ImVec2(NotificationWindowPositionX, NotificationWindowNextPositionY);
            
            ImGui::SetNextWindowPos(NotificationWindowPosition,
                                    NotificationWindowPositionCondition,
                                    NotificationWindowPivot);
            
            ImGui::SetNextWindowSizeConstraints(NotificationWindowMinSize,
                                                NotificationWindowMaxSize);
            
            f32 NotificationWindowFadeFraction = 0.0f;
            if (NotificationLifeTime <= NOTIFICATION_FADE_IN_TIME)
            {
               NotificationWindowFadeFraction =
                  NotificationLifeTime / NOTIFICATION_FADE_IN_TIME;
            }
            else if (NotificationLifeTime <= NOTIFICATION_FADE_IN_TIME +
                     NOTIFICATION_PROPER_LIFE_TIME)
            {
               NotificationWindowFadeFraction = 1.0f;
            }
            else if (NotificationLifeTime <= NOTIFICATION_FADE_IN_TIME +
                     NOTIFICATION_PROPER_LIFE_TIME +
                     NOTIFICATION_FADE_OUT_TIME)
            {
               NotificationWindowFadeFraction =
                  1.0f - (NotificationLifeTime -
                          NOTIFICATION_FADE_IN_TIME -
                          NOTIFICATION_PROPER_LIFE_TIME) /
                  NOTIFICATION_FADE_OUT_TIME;
            }
            else
            {
               NotificationWindowFadeFraction = 0.0f;
            }
            // NOTE(hbr): Quadratic interpolation instead of linear.
            NotificationWindowFadeFraction = 1.0f - Square(1.0f - NotificationWindowFadeFraction);
            Assert(-Epsilon32 <= NotificationWindowFadeFraction &&
                   NotificationWindowFadeFraction <= 1.0f + Epsilon32);
            
            f32 NotificationWindowAlpha = NotificationWindowFadeFraction;
            
            char NotificationWindowLabel[128];
            sprintf(NotificationWindowLabel, "Notification#%llu", NotificationIndex);
            
            DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_Alpha, NotificationWindowAlpha),
                       ImGui::PopStyleVar())
            {
               DeferBlock(ImGui::Begin(NotificationWindowLabel, 0, NotificationWindowFlags),
                          ImGui::End())
               {
                  ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
                  
                  if (ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(0) ||
                                                   ImGui::IsMouseClicked(1)))
                  {
                     ShouldBeRemoved = true;
                  }
                  
                  DeferBlock(ImGui::PushStyleColor(ImGuiCol_Text,
                                                   ImVec4(NotificationTitleColor.R,
                                                          NotificationTitleColor.G,
                                                          NotificationTitleColor.B,
                                                          NotificationWindowAlpha)),
                             ImGui::PopStyleColor(1 /*count*/))
                  {
                     ImGui::Text(Notification->Title);
                  }
                  
                  ImGui::Separator();
                  
                  DeferBlock(ImGui::PushTextWrapPos(0.0f), ImGui::PopTextWrapPos())
                  {
                     ImGui::TextWrapped(Notification->Text);
                  }
                  
                  f32 NotificationWindowHeight = ImGui::GetWindowHeight();
                  Notification->NotificationWindowHeight = NotificationWindowHeight;
                  
                  NotificationWindowDesignatedPositionY -=
                     NotificationWindowHeight + NotificationWindowPadding;
               }
            }
         }
         else
         {
            ShouldBeRemoved = true;
         }
         
         if (ShouldBeRemoved)
         {
            NumNotificationsRemoved += 1;
            NotificationDestroy(Notification);
         }
         else
         {
            *FreeSpace++ = *Notification;
         }
      }
      NotificationSystem->NumNotifications -= NumNotificationsRemoved;
   }
}

internal void
RenderListOfEntitiesWindow(editor *Editor, coordinate_system_data CoordinateSystemData)
{
   bool ViewListOfEntitiesWindowAsBool = Editor->UI_Config.ViewListOfEntitiesWindow;
   
   DeferBlock(ImGui::Begin("Entities", &ViewListOfEntitiesWindowAsBool), ImGui::End())
   {
      Editor->UI_Config.ViewListOfEntitiesWindow = cast(b32)ViewListOfEntitiesWindowAsBool;
      
      if (Editor->UI_Config.ViewListOfEntitiesWindow)
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
                     curve *Curve = &Entity->Curve;
                     
                     DeferBlock(ImGui::PushID(cast(int)CurveIndex), ImGui::PopID())
                     {
                        b32 IsSelected = Curve->IsSelected;
                        b32 IsHidden = Curve->CurveParams.Hidden;
                        
                        bool DeselectCurveSelected = false;
                        bool DeleteCurveSelected = false;
                        bool CopyCurveSelected = false;
                        bool SwitchVisilitySelected = false;
                        bool RenameCurveSelected = false;
                        bool FocusSelected = false;
                        
                        if (Curve->IsRenamingFrame)
                        {
                           if (Curve->IsRenamingFrame == ImGui::GetFrameCount())
                           {
                              ImGui::SetKeyboardFocusHere(0);
                           }
                           
                           // NOTE(hbr): Make sure text starts rendering at the same position
                           DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0}),
                                      ImGui::PopStyleVar())
                           {
                              ImGui::InputText("", Curve->Name.Str, ArrayCount(Curve->Name.Str));
                           }
                           
                           b32 ClickedOutside = (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0));
                           b32 PressedEnter = ImGui::IsKeyPressed(ImGuiKey_Enter);
                           if (ClickedOutside || PressedEnter)
                           {
                              Curve->IsRenamingFrame = 0;
                           }
                        }
                        else
                        {
                           if (ImGui::Selectable(Curve->Name.Str, IsSelected))
                           {
                              EditorStateSelectEntity(EditorState, Entity);
                           }
                           
                           b32 DoubleClickedOrSelectedAndClicked = ((IsSelected && ImGui::IsMouseClicked(0)) ||
                                                                    ImGui::IsMouseDoubleClicked(0));
                           if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
                           {
                              Curve->IsRenamingFrame = ImGui::GetFrameCount() + 1;
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
                              ImGui::MenuItem(IsHidden ? "Show" : "Hide", 0, &SwitchVisilitySelected);
                              if (IsSelected) ImGui::MenuItem("Deselect", 0, &DeselectCurveSelected);
                              ImGui::MenuItem("Focus", 0, &FocusSelected);
                              
                              ImGui::EndPopup();
                           }
                        }
                        
                        if (RenameCurveSelected)
                        {
                           Curve->IsRenamingFrame = ImGui::GetFrameCount() + 1;
                        }
                        
                        if (DeleteCurveSelected)
                        {
                           EditorStateRemoveEntity(EditorState, Entity);
                        }
                        
                        if (CopyCurveSelected)
                        {
                           CopyAndAddCurve(Curve, Editor);
                        }
                        
                        if (SwitchVisilitySelected)
                        {
                           Curve->CurveParams.Hidden = !IsHidden;
                        }
                        
                        if (DeselectCurveSelected)
                        {
                           EditorStateDeselectCurrentEntity(EditorState);
                        }
                        
                        if (FocusSelected)
                        {
                           FocusCameraOnCurve(&EditorState->Camera, Curve, CoordinateSystemData);
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
                     image *Image = &Entity->Image;
                     
                     DeferBlock(ImGui::PushID(cast(int)ImageIndex), ImGui::PopID())
                     {
                        b32 IsSelected = (EditorState->SelectedEntity == Entity);
                        b32 IsHidden = Image->Hidden;
                        
                        bool DeselectImageSelected = false;
                        bool DeleteImageSelected = false;
                        bool CopyImageSelected = false;
                        bool SwitchVisilitySelected = false;
                        bool RenameImageSelected = false;
                        bool FocusSelected = false;
                        
                        if (Image->IsRenamingFrame)
                        {
                           if (Image->IsRenamingFrame == ImGui::GetFrameCount())
                           {
                              ImGui::SetKeyboardFocusHere(0);
                           }
                           
                           // NOTE(hbr): Make sure text starts rendering at the same position
                           DeferBlock(ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0}),
                                      ImGui::PopStyleVar())
                           {                        
                              ImGui::InputText("", Image->Name.Str, ArrayCount(Image->Name.Str));
                           }
                           
                           b32 ClickedOutside = (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0));
                           b32 PressedEnter = ImGui::IsKeyPressed(ImGuiKey_Enter);
                           if (ClickedOutside || PressedEnter)
                           {
                              Image->IsRenamingFrame = 0;
                           }
                        }
                        else
                        {
                           if (ImGui::Selectable(Image->Name.Str, IsSelected))
                           {
                              EditorStateSelectEntity(EditorState, Entity);
                           }
                           
                           b32 DoubleClickedOrSelectedAndClicked = ((IsSelected && ImGui::IsMouseClicked(0)) ||
                                                                    ImGui::IsMouseDoubleClicked(0));
                           if (ImGui::IsItemHovered() && DoubleClickedOrSelectedAndClicked)
                           {
                              Image->IsRenamingFrame = ImGui::GetFrameCount() + 1;
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
                              ImGui::MenuItem(IsHidden ? "Show" : "Hide", 0, &SwitchVisilitySelected);
                              if (IsSelected) ImGui::MenuItem("Deselect", 0, &DeselectImageSelected);
                              ImGui::MenuItem("Focus", 0, &FocusSelected);
                              
                              ImGui::EndPopup();
                           }
                        }
                        
                        if (RenameImageSelected)
                        {
                           Image->IsRenamingFrame = ImGui::GetFrameCount() + 1;
                        }
                        
                        if (DeleteImageSelected)
                        {
                           EditorStateRemoveEntity(EditorState, Entity);
                        }
                        
                        if (CopyImageSelected)
                        {
                           CopyAndAddImage(Image, Editor);
                        }
                        
                        if (SwitchVisilitySelected)
                        {
                           Image->Hidden = !IsHidden;
                        }
                        
                        if (DeselectImageSelected)
                        {
                           EditorStateDeselectCurrentEntity(EditorState);
                        }
                        
                        if (FocusSelected)
                        {
                           FocusCameraOnImage(&EditorState->Camera, Image, CoordinateSystemData);
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
   ImVec2 FileDialogMaxSize = ImVec2(cast(f32)WindowSize.x, cast(f32)WindowSize.y);
   
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
                             cast(bool)Editor->UI_Config.ViewListOfEntitiesWindow))
         {
            Editor->UI_Config.ViewListOfEntitiesWindow = !Editor->UI_Config.ViewListOfEntitiesWindow;
         }
         
         if (ImGui::MenuItem("Selected Curve", 0,
                             cast(bool)Editor->UI_Config.ViewSelectedCurveWindow))
         {
            Editor->UI_Config.ViewSelectedCurveWindow = !Editor->UI_Config.ViewSelectedCurveWindow;
         }
         
         if (ImGui::MenuItem("Selected Image", 0,
                             cast(bool)Editor->UI_Config.ViewSelectedImageWindow))
         {
            Editor->UI_Config.ViewSelectedImageWindow = !Editor->UI_Config.ViewSelectedImageWindow;
         }
         
         if (ImGui::MenuItem("Parameters", 0,
                             cast(bool)Editor->UI_Config.ViewParametersWindow))
         {
            Editor->UI_Config.ViewParametersWindow = !Editor->UI_Config.ViewParametersWindow;
         }
         
#if EDITOR_PROFILER
         if (ImGui::MenuItem("Profiler", 0,
                             cast(bool)Editor->UI_Config.ViewProfilerWindow))
         {
            Editor->UI_Config.ViewProfilerWindow = !Editor->UI_Config.ViewProfilerWindow;
         }
#endif
         
#if EDITOR_DEBUG
         if (ImGui::MenuItem("Debug", 0,
                             cast(bool)Editor->UI_Config.ViewDebugWindow))
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
                             cast(bool)Editor->UI_Config.ViewDiagnosticsWindow))
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
      if (Editor->ProjectSavePath)
      {
         auto Scratch = ScratchArena(0);
         defer { ReleaseScratch(Scratch); };
         
         error_string SaveProjectInFormatError = SaveProjectInFormat(Scratch.Arena,
                                                                     Editor->SaveProjectFormat,
                                                                     Editor->ProjectSavePath,
                                                                     Editor);
         
         if (SaveProjectInFormatError)
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Error,
                                                    SaveProjectInFormatError);
         }
         else
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Success,
                                                    "project successfully saved in %s",
                                                    Editor->ProjectSavePath);
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
               if (Editor->ProjectSavePath)
               {
                  auto Scratch = ScratchArena(0);
                  defer { ReleaseScratch(Scratch); };
                  
                  string SaveProjectFilePath = Editor->ProjectSavePath;
                  save_project_format SaveProjectFormat = Editor->SaveProjectFormat;
                  
                  error_string SaveProjectInFormatError = SaveProjectInFormat(Scratch.Arena,
                                                                              SaveProjectFormat,
                                                                              SaveProjectFilePath,
                                                                              Editor);
                  
                  if (SaveProjectInFormatError)
                  {
                     NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                             Notification_Error,
                                                             "failed to discard current project: %s",
                                                             SaveProjectInFormatError);
                     
                     Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
                  }
                  else
                  {
                     NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                             Notification_Success,
                                                             "project sucessfully saved in %s",
                                                             SaveProjectFilePath);
                     
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
         auto Scratch = ScratchArena(0);
         defer { ReleaseScratch(Scratch); };
         
         std::string const &SelectedPath = FileDialog->GetFilePathName();
         std::string const &SelectedFilter = FileDialog->GetCurrentFilter();
         string SaveProjectFilePath = StringMake(Scratch.Arena,
                                                 SelectedPath.c_str(),
                                                 SelectedPath.size());
         string SaveProjectExtension = StringMake(Scratch.Arena,
                                                  SelectedFilter.c_str(),
                                                  SelectedFilter.size());
         
         string Project = StringMakeLitArena(Scratch.Arena, PROJECT_FILE_EXTENSION_SELECTION);
         string JPG     = StringMakeLitArena(Scratch.Arena, JPG_FILE_EXTENSION_SELECTION);
         string PNG     = StringMakeLitArena(Scratch.Arena, PNG_FILE_EXTENSION_SELECTION);
         
         string AddExtension = 0;
         save_project_format SaveProjectFormat = SaveProjectFormat_None;
         if (StringsAreEqual(SaveProjectExtension, Project))
         {
            AddExtension = StringMakeLitArena(Scratch.Arena, SAVED_PROJECT_FILE_EXTENSION);
            SaveProjectFormat = SaveProjectFormat_ProjectFile;
         }
         else if (StringsAreEqual(SaveProjectExtension, JPG))
         {
            AddExtension = StringMakeLitArena(Scratch.Arena, ".jpg");
            SaveProjectFormat = SaveProjectFormat_ImageFile;
         }
         else if (StringsAreEqual(SaveProjectExtension, PNG))
         {
            AddExtension = StringMakeLitArena(Scratch.Arena, ".png");
            SaveProjectFormat = SaveProjectFormat_ImageFile;
         }
         
         string SaveProjectFilePathWithExtension = SaveProjectFilePath;
         if (AddExtension)
         {
            if (!StringHasSuffix(SaveProjectFilePathWithExtension, AddExtension))
            {
               SaveProjectFilePathWithExtension = StringMakeFormat(Scratch.Arena,
                                                                   "%s%s",
                                                                   SaveProjectFilePathWithExtension,
                                                                   AddExtension);
            }
         }
         
         error_string SaveProjectInFormatError = SaveProjectInFormat(Scratch.Arena,
                                                                     SaveProjectFormat,
                                                                     SaveProjectFilePathWithExtension,
                                                                     Editor);
         
         if (SaveProjectInFormatError)
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Error,
                                                    SaveProjectInFormatError);
            
            Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
         }
         else
         {
            EditorSetSaveProjectPath(Editor, SaveProjectFormat, SaveProjectFilePathWithExtension);
            
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Success,
                                                    "project sucessfully saved in %s",
                                                    SaveProjectFilePathWithExtension);
            
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
            EditorStateMakeDefault(Editor->State.EntityPool,
                                   Editor->State.DeCasteljauVisualization.Arena,
                                   Editor->State.DegreeLowering.Arena,
                                   Editor->State.MovingPointArena,
                                   Editor->State.CurveAnimation.Arena);
         
         EditorStateDestroy(&Editor->State);
         Editor->State = NewEditorState;
         
         EditorSetSaveProjectPath(Editor, SaveProjectFormat_None, 0);
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
         auto Scratch = ScratchArena(0);
         defer { ReleaseScratch(Scratch); };
         
         std::string const &SelectedPath = FileDialog->GetFilePathName();
         string OpenProjectFilePath = StringMake(Scratch.Arena,
                                                 SelectedPath.c_str(),
                                                 SelectedPath.size());
         
         load_project_result LoadResult = LoadProjectFromFile(Scratch.Arena,
                                                              Editor->State.EntityPool,
                                                              Editor->State.DeCasteljauVisualization.Arena,
                                                              Editor->State.DegreeLowering.Arena,
                                                              Editor->State.MovingPointArena,
                                                              Editor->State.CurveAnimation.Arena,
                                                              OpenProjectFilePath);
         if (LoadResult.Error)
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Error,
                                                    "failed to open new project: %s",
                                                    LoadResult.Error);
         }
         else
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Success,
                                                    "project successfully loaded from %s",
                                                    OpenProjectFilePath);
            
            EditorStateDestroy(&Editor->State);
            Editor->State = LoadResult.EditorState;
            Editor->Parameters = LoadResult.EditorParameters;
            Editor->UI_Config = LoadResult.UI_Config;
            
            EditorSetSaveProjectPath(Editor,
                                     SaveProjectFormat_ProjectFile,
                                     OpenProjectFilePath);
         }
         
         ListIter(Warning, LoadResult.Warnings.Head, string_list_node)
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Warning,
                                                    Warning->String);
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
         auto Scratch = ScratchArena(0);
         defer { ReleaseScratch(Scratch); };
         
         std::string const &SelectedPath = FileDialog->GetFilePathName();
         string NewImageFilePath = StringMake(Scratch.Arena,
                                              SelectedPath.c_str(),
                                              SelectedPath.size());
         
         string LoadTextureError = 0;
         sf::Texture LoadedTexture = LoadTextureFromFile(Scratch.Arena,
                                                         NewImageFilePath,
                                                         &LoadTextureError);
         
         if (LoadTextureError)
         {
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Error,
                                                    "failed to load image: %s",
                                                    LoadTextureError);
         }
         else
         {
            std::string const &SelectedFileName = FileDialog->GetCurrentFileName();
            string ImageFileName = StringMake(Scratch.Arena,
                                              SelectedFileName.c_str(),
                                              SelectedFileName.size());
            
            StringRemoveExtension(ImageFileName);
            name_string NewImageName = NameStringFromString(ImageFileName);
            world_position NewImagePosition = V2F32(0.0f, 0.0f);
            v2f32 NewImageScale = V2F32(1.0f, 1.0f);
            rotation_2d NewImageRotation = Rotation2DZero();
            s64 NewImageSortingLayer = 0;
            b32 NewImageHidden = false;
            
            image NewImage = ImageMake(NewImageName,
                                       NewImagePosition,
                                       NewImageScale,
                                       NewImageRotation,
                                       NewImageSortingLayer,
                                       NewImageHidden,
                                       NewImageFilePath,
                                       LoadedTexture);
            
            editor_state *EditorState = &Editor->State;
            entity NewEntity = ImageEntity(NewImage);
            
            entity *AddedEntity = EditorStateAddEntity(EditorState, &NewEntity);
            EditorStateSelectEntity(EditorState, AddedEntity);
            NotificationSystemAddNotificationFormat(&Editor->NotificationSystem,
                                                    Notification_Success,
                                                    "successfully loaded image from %s",
                                                    NewImageFilePath);
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
      Editor->UI_Config.ViewParametersWindow = cast(b32)ViewParametersWindowAsBool;
      
      if (Editor->UI_Config.ViewParametersWindow)
      {   
         local ImGuiTreeNodeFlags SectionFlags = ImGuiTreeNodeFlags_DefaultOpen;
         editor_parameters *Params = &Editor->Parameters;
         
         {
            b32 *HeaderCollapsed = &Editor->UI_Config.DefaultCurveHeaderCollapsed;
            ImGui::SetNextItemOpen(*HeaderCollapsed);
            
            *HeaderCollapsed = cast(b32)ImGui::CollapsingHeader("Default Curve", SectionFlags);
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
            *HeaderCollapsed = cast(b32)ImGui::CollapsingHeader("Rotation Indicator", SectionFlags);
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
            *HeaderCollapsed = cast(b32)ImGui::CollapsingHeader("Bezier Split Point", SectionFlags);
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
            *HeaderCollapsed = cast(b32)ImGui::CollapsingHeader("Other Settings", SectionFlags);
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
   bool ViewDiagnosticsWindowAsBool = cast(bool)Editor->UI_Config.ViewDiagnosticsWindow;
   DeferBlock(ImGui::Begin("Diagnostics", &ViewDiagnosticsWindowAsBool),
              ImGui::End())
   {
      Editor->UI_Config.ViewDiagnosticsWindow = cast(b32)ViewDiagnosticsWindowAsBool;
      
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
   bool ViewDebugWindowAsBool = cast(bool)Editor->UI_Config.ViewDebugWindow;
   DeferBlock(ImGui::Begin("Debug", &ViewDebugWindowAsBool), ImGui::End())
   {
      Editor->UI_Config.ViewDebugWindow = cast(b32)ViewDebugWindowAsBool;
      
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
   curve *SplitCurve = Splitting->SplitCurve;
   
   if (Splitting->IsSplitting)
   {
      b32 CurveQualifiesForSplitting = false;
      curve_shape CurveShape = SplitCurve->CurveParams.CurveShape;
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
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      u64 NumControlPoints = SplitCurve->NumControlPoints;
      curve_shape CurveShape = SplitCurve->CurveParams.CurveShape;
      
      f32 *ControlPointWeights = SplitCurve->ControlPointWeights;
      switch (CurveShape.BezierType)
      {
         case Bezier_Normal: {
            ControlPointWeights = PushArray(Scratch.Arena, NumControlPoints, f32);
            for (u64 I = 0; I < NumControlPoints; ++I)
            {
               ControlPointWeights[I] = 1.0f;
            }
         } break;
         
         case Bezier_Weighted: {} break;
         case Bezier_Cubic: { Assert(false); } break;
      }
      
      local_position *LeftControlPoints = PushArray(Scratch.Arena,
                                                    NumControlPoints,
                                                    local_position);
      local_position *RightControlPoints = PushArray(Scratch.Arena,
                                                     NumControlPoints,
                                                     local_position);
      
      f32 *LeftControlPointWeights  = PushArray(Scratch.Arena,
                                                NumControlPoints,
                                                f32);
      f32 *RightControlPointWeights = PushArray(Scratch.Arena,
                                                NumControlPoints,
                                                f32);
      
      BezierCurveSplit(Splitting->T,
                       SplitCurve->ControlPoints,
                       ControlPointWeights,
                       NumControlPoints,
                       LeftControlPoints,
                       LeftControlPointWeights,
                       RightControlPoints,
                       RightControlPointWeights);
      
      local_position *LeftCubicBezierPoints = PushArray(Scratch.Arena,
                                                        3 * NumControlPoints,
                                                        local_position);
      local_position *RightCubicBezierPoints = PushArray(Scratch.Arena,
                                                         3 * NumControlPoints,
                                                         local_position);
      
      BezierCubicCalculateAllControlPoints(LeftControlPoints,
                                           NumControlPoints,
                                           LeftCubicBezierPoints + 1);
      BezierCubicCalculateAllControlPoints(RightControlPoints,
                                           NumControlPoints,
                                           RightCubicBezierPoints + 1);
      
      // TODO(hbr): Optimize here not to do so many copies, maybe merge with split on control point code
      curve *LeftSplit = SplitCurve;
      curve RightCurveTemplate = CurveCopy(*SplitCurve);
      entity RightEntityTemplate = CurveEntity(RightCurveTemplate);
      entity *RightSplitEntity = EditorStateAddEntity(EditorState, &RightEntityTemplate);
      curve *RightSplit = &RightSplitEntity->Curve;
      
      CurveSetControlPoints(LeftSplit, NumControlPoints,
                            LeftControlPoints, LeftControlPointWeights,
                            LeftCubicBezierPoints);
      CurveSetControlPoints(RightSplit, NumControlPoints,
                            RightControlPoints, RightControlPointWeights,
                            RightCubicBezierPoints);
      
      name_string LeftSplitName = NameStringFormat("%s (left)", LeftSplit->Name.Str);
      name_string RightSplitName = NameStringFormat("%s (right)", RightSplit->Name.Str);
      LeftSplit->Name = LeftSplitName;
      RightSplit->Name = RightSplitName;
      
      Splitting->IsSplitting = false;
   }
   
   if (Splitting->IsSplitting)
   {
      if (T_ParamterChanged ||
          Splitting->SavedCurveVersion != SplitCurve->CurveVersion)
      {
         
         local_position SplitPoint = {};
         switch (SplitCurve->CurveParams.CurveShape.BezierType)
         {
            case Bezier_Normal: {
               SplitPoint = BezierCurveEvaluateFast(Splitting->T,
                                                    SplitCurve->ControlPoints,
                                                    SplitCurve->NumControlPoints);
            } break;
            
            case Bezier_Weighted: {
               SplitPoint = BezierWeightedCurveEvaluateFast(Splitting->T,
                                                            SplitCurve->ControlPoints,
                                                            SplitCurve->ControlPointWeights,
                                                            SplitCurve->NumControlPoints);
            } break;
            
            case Bezier_Cubic: { Assert(false); } break;
         }
         Splitting->SplitPoint = LocalCurvePositionToWorld(SplitCurve, SplitPoint);
      }
      
      if (!SplitCurve->CurveParams.Hidden)
      {
         RenderPoint(Splitting->SplitPoint,
                     EditorParams->BezierSplitPoint,
                     CoordinateSystemData, Transform,
                     Window);
      }
   }
}

internal void
RenderImages(u64 NumEntities, entity *EntitiesList, sf::Transform Transform,
             sf::RenderWindow *Window)
{
   TimeFunction;
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   sorting_layer_sorted_images SortedImages =
      SortingLayerSortedImages(Scratch.Arena, NumEntities, EntitiesList);
   
   for (u64 ImageIndex = 0;
        ImageIndex < SortedImages.NumImages;
        ++ImageIndex)
   {
      image *Image = SortedImages.SortedImages[ImageIndex].Image;
      
      if (!Image->Hidden)
      {
         sf::Sprite Sprite;
         Sprite.setTexture(Image->Texture);
         
         sf::Vector2u TextureSize = Image->Texture.getSize();
         Sprite.setOrigin(0.5f*TextureSize.x, 0.5f*TextureSize.y);
         
         v2f32 Scale = GlobalImageScaleFactor * Image->Scale;
         // NOTE(hbr): Flip vertically because SFML images are flipped.
         Sprite.setScale(Scale.X, -Scale.Y);
         
         v2f32 Position = Image->Position;
         Sprite.setPosition(Position.X, Position.Y);
         
         f32 AngleRotation = Rotation2DToDegrees(Image->Rotation);
         Sprite.setRotation(AngleRotation);
         
         Window->draw(Sprite, Transform);
      }
   }
}

internal void
UpdateAndRenderDeCasteljauVisualization(de_casteljau_visualization *Visualization,
                                        sf::Transform Transform,
                                        sf::RenderWindow *Window)
{
   curve *Curve = Visualization->Curve;
   
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
         auto Scratch = ScratchArena(Visualization->Arena);
         defer { ReleaseScratch(Scratch); };
         
         u64 NumControlPoints = Curve->NumControlPoints;
         u64 NumIterations = NumControlPoints;
         color *IterationColors = PushArray(Visualization->Arena, NumIterations, color);
         line_vertices *LineVerticesPerIteration = PushArrayZero(Visualization->Arena,
                                                                 NumIterations,
                                                                 line_vertices);
         u64 NumAllIntermediatePoints = NumControlPoints * NumControlPoints;
         local_position *AllIntermediatePoints = PushArray(Visualization->Arena,
                                                           NumAllIntermediatePoints,
                                                           local_position);
         
         // NOTE(hbr): Deal with cases at once.
         f32 *ControlPointWeights = Curve->ControlPointWeights;
         switch (Curve->CurveParams.CurveShape.BezierType)
         {
            case Bezier_Normal: {
               ControlPointWeights = PushArray(Scratch.Arena, NumControlPoints, f32);
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
         f32 *ThrowAwayWeights = PushArray(Scratch.Arena, NumAllIntermediatePoints, f32);
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
   
   if (Visualization->IsVisualizing && !Visualization->Curve->CurveParams.Hidden)
   {
      sf::Transform Model = CurveGetAnimate(Visualization->Curve);
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
      
      f32 PointSize = Visualization->Curve->CurveParams.PointSize;
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
   
   curve *Curve = Lowering->Curve;
   
   if (Lowering->IsLowering)
   {
      if (Lowering->SavedCurveVersion != Curve->CurveVersion)
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
      
      Assert(Curve->CurveParams.CurveShape.InterpolationType == Interpolation_Bezier);
      b32 IncludeWeights = false;
      switch (Curve->CurveParams.CurveShape.BezierType)
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
      Assert(Lowering->LowerDegree.MiddlePointIndex < Curve->NumControlPoints);
      
      if (P_MixChanged || W_MixChanged)
      {
         local_position NewControlPoint =
            Lowering->P_Mix * Lowering->LowerDegree.P_I + (1 - Lowering->P_Mix) * Lowering->LowerDegree.P_II;
         f32 NewControlPointWeight =
            Lowering->W_Mix * Lowering->LowerDegree.W_I + (1 - Lowering->W_Mix) * Lowering->LowerDegree.W_II;
         
         CurveSetControlPoint(Curve,
                              Lowering->LowerDegree.MiddlePointIndex,
                              NewControlPoint,
                              NewControlPointWeight);
         
         Lowering->SavedCurveVersion = Curve->CurveVersion;
      }
      
      if (Revert)
      {
         CurveSetControlPoints(Curve,
                               Curve->NumControlPoints + 1,
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
      sf::Transform Model = CurveGetAnimate(Lowering->Curve);
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
   
   ListIter(Entity, EntitiesList, entity)
   {
      if (Entity->Type == Entity_Curve)
      {
         curve *Curve = &Entity->Curve;
         curve_params CurveParams = Curve->CurveParams;
         
         if (!CurveParams.Hidden)
         {
            sf::Transform Model = CurveGetAnimate(Curve);
            sf::Transform MVP = VP * Model;
            
            if (EditorMode->Type == EditorMode_MovingPoint &&
                EditorMode->MovingPoint.Curve == Curve)
            {
               Window->draw(EditorMode->MovingPoint.SavedCurveVertices,
                            EditorMode->MovingPoint.SavedNumCurveVertices,
                            EditorMode->MovingPoint.SavedPrimitiveType,
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
               b32 IsSelected = Curve->IsSelected;
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
            ImGui::TextColored(CURVE_NAME_HIGHLIGHT_COLOR, Animation->FromCurve->Name.Str);
            ImGui::SameLine();
            ImGui::Text("with");
            ImGui::SameLine();
            char const *ComboPreview = (Animation->ToCurve ? Animation->ToCurve->Name.Str : "");
            if (ImGui::BeginCombo("##AnimationTarget", ComboPreview))
            {
               ListIter(Entity, EntitiesList, entity)
               {
                  if (Entity->Type == Entity_Curve)
                  {
                     curve *Curve = &Entity->Curve;
                     
                     if (Curve != Animation->FromCurve)
                     {
                        b32 IsSelected = (Animation->ToCurve == Curve);
                        if (ImGui::Selectable(Curve->Name.Str, IsSelected))
                        {
                           Animation->ToCurve = Curve;
                        }
                     }
                  }
               }
               
               ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Text("(choose from list or click on curve).");
            
            if (Animation->ToCurve)
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
            Assert(Animation->ToCurve);
            
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
            
            int AnimationAsInt = cast(int)Animation->Animation;
            for (int AnimationType = 0;
                 AnimationType < Animation_Count;
                 ++AnimationType)
            {
               if (AnimationType > 0)
               {
                  ImGui::SameLine();
               }
               
               ImGui::RadioButton(AnimationToString(cast(animation_type)AnimationType),
                                  &AnimationAsInt,
                                  AnimationType);
            }
            Animation->Animation = cast(animation_type)AnimationAsInt;
            
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
            auto Scratch = ScratchArena(Animation->Arena);
            defer { ReleaseScratch(Scratch); };
            
            curve *FromCurve = Animation->FromCurve;
            curve *ToCurve = Animation->ToCurve;
            u64 NumCurvePoints = FromCurve->NumCurvePoints;
            
            b32 ToCurvePointsRecalculated = false;
            if (NumCurvePoints != Animation->NumCurvePoints ||
                Animation->SavedToCurveVersion != ToCurve->CurveVersion)
            {
               ArenaClear(Animation->Arena);
               
               Animation->NumCurvePoints = NumCurvePoints;
               Animation->ToCurvePoints = PushArray(Animation->Arena, NumCurvePoints, v2f32);
               CurveEvaluate(ToCurve, NumCurvePoints, Animation->ToCurvePoints);
               Animation->SavedToCurveVersion = ToCurve->CurveVersion;
               
               ToCurvePointsRecalculated = true;
            }
            
            if (ToCurvePointsRecalculated || BlendChanged)
            {               
               v2f32 *ToCurvePoints = Animation->ToCurvePoints;
               v2f32 *FromCurvePoints = FromCurve->CurvePoints;
               v2f32 *AnimatedCurvePoints = PushArray(Scratch.Arena, NumCurvePoints, v2f32);
               
               f32 Blend = CalculateAnimation(Animation->Animation, Animation->Blend);
               f32 T = 0.0f;
               f32 Delta_T = 1.0f / (NumCurvePoints - 1);
               for (u64 VertexIndex = 0;
                    VertexIndex < NumCurvePoints;
                    ++VertexIndex)
               {
                  v2f32 From = LocalCurvePositionToWorld(FromCurve,
                                                         FromCurvePoints[VertexIndex]);
                  v2f32 To = LocalCurvePositionToWorld(ToCurve,
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
   curve *From = Combining->CombineCurve;
   curve *To = Combining->TargetCurve;
   
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
                  LocalCurvePositionToWorld(To, To->ControlPoints[M - 1]) -
                  LocalCurvePositionToWorld(From, From->ControlPoints[0]);
            }
         }
         
      } break;
      
      case CurveCombination_Count: { Assert(false); } break;
   }
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   // NOTE(hbr): Allocate buffers and copy control points into them
   v2f32 *CombinedControlPoints = PushArray(Scratch.Arena, CombinedNumControlPoints, v2f32);
   f32 *CombinedControlPointWeights = PushArray(Scratch.Arena, CombinedNumControlPoints, f32);
   v2f32 *CombinedCubicBezierPoints = PushArray(Scratch.Arena, 3 * CombinedNumControlPoints, v2f32);
   
   MemoryCopy(CombinedControlPoints,
              To->ControlPoints,
              M * sizeof(CombinedControlPoints[0]));
   MemoryCopy(CombinedControlPointWeights,
              To->ControlPointWeights,
              M * sizeof(CombinedControlPointWeights[0]));
   MemoryCopy(CombinedCubicBezierPoints,
              To->CubicBezierPoints,
              3 * M * sizeof(CombinedCubicBezierPoints[0]));
   
   {
      for (u64 I = StartIndex; I < N; ++I)
      {
         world_position FromControlPoint = LocalCurvePositionToWorld(From, From->ControlPoints[I]);
         local_position ToControlPoint = WorldToLocalCurvePosition(To, FromControlPoint + Translation);
         CombinedControlPoints[M - StartIndex + I] = ToControlPoint;
      }
      
      for (u64 I = 3 * StartIndex; I < 3 * N; ++I)
      {
         world_position FromCubicBezierPoint = LocalCurvePositionToWorld(From, From->CubicBezierPoints[I]);
         local_position ToCubicBezierPoint = WorldToLocalCurvePosition(To, FromCubicBezierPoint + Translation); 
         CombinedCubicBezierPoints[3 * (M - StartIndex) + I] = ToCubicBezierPoint;
      }
      
      MemoryCopy(CombinedControlPointWeights + M,
                 From->ControlPointWeights + StartIndex,
                 (N - StartIndex) * sizeof(CombinedControlPointWeights[0]));
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
            v2f32 FixedControlPoint = cast(f32)M/N * (P - Q) + P;
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
            v2f32 Fixed_T = cast(f32)M/N * (P - Q) + P;
            // NOTE(hbr): Second derivative equal
            v2f32 Fixed_U = cast(f32)(N * (N-1))/(M * (M-1)) * (P - 2.0f * Q + R) + 2.0f * Fixed_T - P;
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
   EditorStateRemoveEntity(EditorState, EntityFromCurve(From));
   // TODO(hbr): Maybe select To curve
}

internal void
UpdateAndRenderCurveCombining(editor_state *EditorState,
                              sf::Transform Transform,
                              sf::RenderWindow *Window)
{
   curve_combining *Combining = &EditorState->CurveCombining;
   
   if (Combining->IsCombining &&
       Combining->TargetCurve &&
       !AreCurvesCompatibleForCombining(Combining->CombineCurve,
                                        Combining->TargetCurve))
   {
      Combining->TargetCurve = 0;
   }
   
   if (Combining->IsCombining &&
       !IsCombinationTypeAllowed(Combining->CombineCurve, Combining->CombinationType))
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
            ImGui::TextColored(CURVE_NAME_HIGHLIGHT_COLOR, Combining->CombineCurve->Name.Str);
            ImGui::SameLine();
            ImGui::Text("with");
            ImGui::SameLine();
            char const *ComboPreview = (Combining->TargetCurve ? Combining->TargetCurve->Name.Str : "");
            if (ImGui::BeginCombo("##CombiningTarget", ComboPreview))
            {
               ListIter(Entity, EditorState->EntitiesHead, entity)
               {
                  if (Entity->Type == Entity_Curve)
                  {
                     curve *Curve = &Entity->Curve;
                     
                     if (Curve != Combining->CombineCurve &&
                         AreCurvesCompatibleForCombining(Combining->CombineCurve, Curve))
                     {
                        b32 IsSelected = (Curve == Combining->TargetCurve);
                        if (ImGui::Selectable(Curve->Name.Str, IsSelected))
                        {
                           Combining->TargetCurve = Curve;
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
               
               int CombinationTypeAsInt = cast(int)Combining->CombinationType;
               for (int Combination = 0;
                    Combination < CurveCombination_Count;
                    ++Combination)
               {
                  curve_combination_type CombinationType = cast(curve_combination_type)Combination;
                  
                  ImGui::SameLine();
                  ImGui::BeginDisabled(!IsCombinationTypeAllowed(Combining->CombineCurve, CombinationType));
                  ImGui::RadioButton(CombinationTypeToString(CombinationType),
                                     &CombinationTypeAsInt,
                                     Combination);
                  ImGui::EndDisabled();
               }
               Combining->CombinationType = cast(curve_combination_type)CombinationTypeAsInt;
            }
            
            if (Combining->TargetCurve)
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
      curve *CombineCurve = Combining->CombineCurve;
      curve *TargetCurve = Combining->TargetCurve;
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
         
         world_position BeginPoint = LocalCurvePositionToWorld(CombineCurve, CombineControlPoint);
         world_position TargetPoint = LocalCurvePositionToWorld(TargetCurve, TargetControlPoint);
         
         f32 LineWidth = 0.5f * CombineCurve->CurveParams.CurveWidth;
         color Color = CombineCurve->CurveParams.CurveColor;
         Color.A = cast(u8)(0.5f * Color.A);
         
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
   UpdateAndRenderNotificationSystem(&Editor->NotificationSystem, DeltaTime, Editor->Window);
   
   {
      TimeBlock("editor state update");
      
      coordinate_system_data CoordinateSystemData =
         CoordinateSystemDataMake(Editor->Window,
                                  Editor->State.Camera,
                                  Editor->Projection);
      
      editor_parameters EditorParams = Editor->Parameters;
      
      // NOTE(hbr): Editor state update
      editor_state NewState = Editor->State;
      for (u64 ButtonIndex = 0;
           ButtonIndex < ArrayCount(UserInput.Buttons);
           ++ButtonIndex)
      {
         button Button = cast(button)ButtonIndex;
         button_state ButtonState = UserInput.Buttons[ButtonIndex];
         
         if (ClickedWithButton(ButtonState, CoordinateSystemData))
         {
            user_action Action = UserActionButtonClicked(Button, ButtonState.ReleasePosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
         
         if (DraggedWithButton(ButtonState, UserInput.MousePosition, CoordinateSystemData))
         {
            user_action Action = UserActionButtonDrag(Button, UserInput.MouseLastPosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
         
         if (ReleasedButton(ButtonState))
         {
            user_action Action = UserActionButtonReleased(Button, ButtonState.ReleasePosition, &UserInput);
            NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
         }
      }
      if (UserInput.MouseLastPosition != UserInput.MousePosition)
      {
         user_action Action = UserActionMouseMove(UserInput.MouseLastPosition, UserInput.MousePosition, &UserInput);
         NewState = EditorStateUpdate(NewState, Action, CoordinateSystemData, EditorParams);
      }
      Editor->State = NewState;
   }
   
#if EDITOR_DEBUG
   {
      notification_system *NotificationSystem = &Editor->NotificationSystem;
      if (PressedWithKey(UserInput.Keys[Key_S], 0))
      {
         NotificationSystemAddNotificationFormat(NotificationSystem,
                                                 Notification_Success,
                                                 "example successful notification message");
      }
      if (PressedWithKey(UserInput.Keys[Key_E], 0))
      {
         NotificationSystemAddNotificationFormat(NotificationSystem,
                                                 Notification_Error,
                                                 "example error notification message");
      }
      if (PressedWithKey(UserInput.Keys[Key_W], 0))
      {
         NotificationSystemAddNotificationFormat(NotificationSystem,
                                                 Notification_Warning,
                                                 "example warning notification message");
      }
   }
#endif
   
   coordinate_system_data CoordinateSystemData = 
      CoordinateSystemDataMake(Editor->Window,
                               Editor->State.Camera,
                               Editor->Projection);
   
   if (Editor->State.SelectedEntity)
   {
      switch (Editor->State.SelectedEntity->Type)
      {
         case Entity_Curve: {
            if (Editor->UI_Config.ViewSelectedCurveWindow)
            {
               RenderSelectedCurveUI(Editor, CoordinateSystemData);
            }
         } break;
         
         case Entity_Image: {
            if (Editor->UI_Config.ViewSelectedImageWindow)
            {
               RenderSelectedImageUI(Editor, CoordinateSystemData);
            }
         } break;
      }
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
   RenderImages(Editor->State.NumEntities, Editor->State.EntitiesHead,
                VP, Editor->Window);
   
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

int main()
{
   SetGlobalContext(Gigabytes(16));
   
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
   
#if 1
   char const *VertexShader =
   R"(

void main()
{
                  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
gl_FrontColor = vec4(0, 1, 0, 1);
}

)";
   
#endif
   sf::Shader Shader;
   Shader.loadFromMemory(VertexShader, sf::Shader::Type::Vertex);
   
   if (Window.isOpen())
   {
      bool ImGuiInitSuccess = ImGui::SFML::Init(Window, true);
      if (ImGuiInitSuccess)
      {
         pool *EntityPool = PoolMakeForType(Gigabytes(1), entity);
         arena *DeCasteljauVisualizationArena = ArenaMake(Gigabytes(1));
         arena *DegreeLoweringArena = ArenaMake(Gigabytes(1));
         arena *MovingPointArena = ArenaMake(Gigabytes(1));
         arena *CurveAnimationArena = ArenaMake(Gigabytes(1));
         editor_state InitialEditorState = EditorStateMakeDefault(EntityPool,
                                                                  DeCasteljauVisualizationArena,
                                                                  DegreeLoweringArena,
                                                                  MovingPointArena,
                                                                  CurveAnimationArena);
         
         f32 DefaultFrustumSize = 2.0f;
         projection InitialProjection = ProjectionMake(VideoMode.width,
                                                       VideoMode.height,
                                                       DefaultFrustumSize);
         
         notification_system NotificationSystem = NotificationSystemMake(&Window);
         
         editor_parameters InitialEditorParameters =
            EditorParametersMake(ROTATION_INDICATOR_DEFAULT_RADIUS_CLIP_SPACE,
                                 ROTATION_INDICATOR_DEFAULT_OUTLINE_THICKNESS_FRACTION,
                                 ROTATION_INDICATOR_DEFAULT_FILL_COLOR,
                                 ROTATION_INDICATOR_DEFAULT_OUTLINE_COLOR,
                                 BEZIER_SPLIT_POINT_DEFAULT_RADIUS_CLIP_SPACE,
                                 BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_THICKNESS_FRACTION,
                                 BEZIER_SPLIT_POINT_DEFAULT_FILL_COLOR,
                                 BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_COLOR,
                                 DEFAULT_BACKGROUND_COLOR,
                                 DEFAULT_COLLLISION_TOLERANCE_CLIP_SPACE,
                                 LAST_CONTROL_POINT_DEFAULT_SIZE_MULTIPLIER,
                                 SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_THICKNESS_SCALE,
                                 SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_COLOR,
                                 SELECTED_CONTROL_POINT_DEFAULT_OUTLINE_COLOR,
                                 CUBIC_BEZIER_HELPER_LINE_DEFAULT_WIDTH_CLIP_SPACE,
                                 CURVE_DEFAULT_CURVE_PARAMS,
                                 CURVE_DEFAULT_CONTROL_POINT_WEIGHT);
         
         ui_config InitialUI_Config = UI_ConfigMake(true, true,
                                                    true, false, false,
                                                    false, false, false, false);
         
         editor Editor = EditorMake(&Window,
                                    InitialEditorState,
                                    InitialProjection,
                                    NotificationSystem,
                                    InitialEditorParameters,
                                    InitialUI_Config);
         
         sf::Vector2i MousePosition = sf::Mouse::getPosition(Window);
         user_input UserInput = UserInputMake(V2S32FromVec(MousePosition),
                                              VideoMode.width,
                                              VideoMode.height);
         
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
            
#if 0
            sf::RectangleShape Rectangle;
            Rectangle.setSize(sf::Vector2f(0.5f, 0.5f));
            Rectangle.setOrigin(0.5f * Rectangle.getScale());
            Rectangle.setPosition(0.0f, 0.0f);
            Window.draw(Rectangle, Shader);
#endif
            
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

StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors),
             MakeSureNumberOfProfilePointsFitsIntoArray);