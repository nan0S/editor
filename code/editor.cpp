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
RotateCamera(camera *Camera, v2 By)
{
   Camera->Rotation = CombineRotations2D(Camera->Rotation, By);
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
SetEntityModelTransform(render_group *Group, entity *Entity)
{
   SetModelTransform(Group, GetEntityModelTransform(Entity), Entity->SortingLayer);
}

internal v2
ClipToWorld(v2 Clip, render_group *RenderGroup)
{
   v2 World = Unproject(&RenderGroup->ProjXForm, Clip);
   return World;
}
//////////////////////

#if 0

internal void
UpdateCamera(camera *Camera, platform_input *Input)
{
   
}

internal void
MoveCamera(camera *Camera, v2 Translation)
{
   Camera->Position += Translation;
   Camera->ReachingTarget = false;
}

#endif

internal world_position
CameraToWorldSpace(camera_position Position, render_group *Group)
{
   world_position Result = Unproject(&Group->WorldToCamera, Position);
   return Result;
}

internal camera_position
WorldToCameraSpace(world_position Position, render_group *Group)
{
   camera_position Result = Project(&Group->WorldToCamera, Position);
   return Result;
}

internal camera_position
ScreenToCameraSpace(screen_position Position, render_group *Group)
{
   v2 Clip = Unproject(&Group->ClipToScreen, V2(Position.X, Position.Y));
   camera_position Result = Unproject(&Group->CameraToClip, Clip);
   return Result;
}

internal world_position
ScreenToWorldSpace(screen_position Position, render_group *Group)
{
   camera_position CameraPosition = ScreenToCameraSpace(Position, Group);
   world_position WorldPosition = CameraToWorldSpace(CameraPosition, Group);
   
   return WorldPosition;
}

internal v2
ClipToWorldSpace(v2 Clip, render_group *Group)
{
   v2 Camera = Unproject(&Group->CameraToClip, Clip);
   v2 World = CameraToWorldSpace(Camera, Group);
   return World;
}

// NOTE(hbr): Distance in space [-AspectRatio, AspectRatio] x [-1, 1]
internal f32
ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, render_group *Group)
{
   f32 Result = UnprojectLength(&Group->WorldToCamera, V2(ClipSpaceDistance, 0)).X;
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
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               if (AreCurvePointsVisible(Curve))
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
                     local_position BezierPoint = GetCubicBezierPoint(Curve, BezierIndex);
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
                  if (PointCollision(LocalAtP, Tracking->TrackedPoint, PointRadius + Tolerance))
                  {
                     Result.Entity = Entity;
                     Result.Flags |= Collision_TrackedPoint;
                  }
               }
               
               {
                  local_position *CurvePoints = Curve->CurvePoints;
                  u64 CurvePointCount = Curve->CurvePointCount;
                  f32 CurveWidth = 2.0f * Tolerance + Curve->CurveParams.CurveWidth;
                  
                  for (u64 CurvePointIndex = 0;
                       CurvePointIndex + 1 < CurvePointCount;
                       ++CurvePointIndex)
                  {
                     v2 P1 = CurvePoints[CurvePointIndex];
                     v2 P2 = CurvePoints[CurvePointIndex + 1];
                     if (SegmentCollision(LocalAtP, P1, P2, CurveWidth))
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
               sf::Vector2u TextureSize = Image->Texture.getSize();
               f32 SizeX = Abs(GlobalImageScaleFactor * Entity->Scale.X * TextureSize.x);
               f32 SizeY = Abs(GlobalImageScaleFactor * Entity->Scale.Y * TextureSize.y);
               v2 Extents = 0.5f * V2(SizeX + Tolerance, SizeY + Tolerance);
               
               if (-Extents.X <= LocalAtP.X && LocalAtP.X <= Extents.X &&
                   -Extents.Y <= LocalAtP.Y && LocalAtP.Y <= Extents.Y)
               {
                  Result.Entity = Entity;
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
         
         arena *EntityArena = Entity->Arena;
         arena *PointTrackingArena = Entity->Curve.PointTracking.Arena;
         arena *DegreeLoweringArena = Entity->Curve.DegreeLowering.Arena;
         StructZero(Entity);
         Entity->Arena = EntityArena;
         Entity->Curve.PointTracking.Arena = PointTrackingArena;
         Entity->Curve.DegreeLowering.Arena = DegreeLoweringArena;
         
         Entity->Flags |= EntityFlag_Active;
         ClearArena(Entity->Arena);
         
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
#if 0
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
#endif
}

internal b32
ScreenPointsAreClose(screen_position A, screen_position B, render_group *Group)
{
   camera_position C = ScreenToCameraSpace(A, Group);
   camera_position D = ScreenToCameraSpace(B, Group);
   
   local f32 CameraSpaceEpsilon = 0.01f;
   b32 Result = NormSquared(C - D) <= Square(CameraSpaceEpsilon);
   
   return Result;
}

internal editor_mode
MakeMovingEntityMode(entity *Target)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Target = Target;
   Result.Moving.Type = MovingMode_Entity;
   
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

internal vertex_array
CopyLineVertices(arena *Arena, vertex_array Vertices)
{
   vertex_array Result = Vertices;
   Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, vertex);
   ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
   
   return Result;
}

internal editor_mode
MakeMovingCurvePointMode(arena *Arena, entity *TargetCurve, curve_point_index Index)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Target = TargetCurve;
   Result.Moving.Type = MovingMode_CurvePoint;
   Result.Moving.MovingPointIndex = Index;
   
   ClearArena(Arena);
   curve *Curve = GetCurve(TargetCurve);
   Result.Moving.OriginalCurveVertices = CopyLineVertices(Arena, Curve->CurveVertices);
   
   return Result;
}

internal editor_mode
MakeRotatingMode(entity *Target, screen_position Center)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Rotating;
   Result.Target = Target;
   Result.Rotating.Center = Center;
   
   return Result;
}

internal editor_mode
MakeMovingTrackedPointMode(entity *Target)
{
   editor_mode Result = {};
   Result.Type = EditorMode_Moving;
   Result.Target = Target;
   Result.Moving.Type = MovingMode_TrackedPoint;
   
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
   
   EndCurvePoints(&RightEntity->Curve);
   EndCurvePoints(&LeftEntity->Curve);
   
   EndTemp(Temp);
}

#if 0
internal void
ExecuteUserActionNormalMode(editor *Editor, user_action Action, render_group *Group, editor_input *Input)
{
   temp_arena Temp = TempArena(0);
   switch (Action.Type)
   {
      case UserAction_ButtonClicked: {
         world_position ClickPosition = ScreenToWorldSpace(Action.Click.ClickPosition, Group);
         switch (Action.Click.Button)
         {
            case Button_Left: {
               collision Collision = {};
               if (Input->Keys[Key_LeftShift].Pressed)
               {
                  // NOTE(hbr): Force no collision when shift if pressed, so that user can
                  // add control point wherever he wants
               }
               else
               {
                  Collision = CheckCollisionWith(&Editor->Entities, ClickPosition,
                                                 Group->CollisionTolerance,
                                                 Collision_CurvePoint | Collision_CurveLine);
               }
               
               entity *FocusEntity = 0;
               b32 DoFocusCurvePoint = false;
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
                     DoFocusCurvePoint = true;
                     FocusCurvePointIndex = Collision.CurvePointIndex;
                  }
                  else if (Collision.Flags & Collision_CurveLine)
                  {
                     control_point_index Index = CurvePointIndexToControlPointIndex(Curve, Collision.CurveLinePointIndex);
                     u64 InsertAt = Index.Index + 1;
                     InsertControlPoint(Collision.Entity, ClickPosition, InsertAt);
                     DoFocusCurvePoint = true;
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
                     InitCurve(FocusEntity, Editor->CurveDefaultParams);
                  }
                  Assert(FocusEntity);
                  
                  DoFocusCurvePoint = true;
                  control_point_index Appended = AppendControlPoint(FocusEntity, ClickPosition);
                  FocusCurvePointIndex = MakeCurvePointIndexFromControlPointIndex(Appended);
               }
               
               SelectEntity(Editor, FocusEntity);
               if (DoFocusCurvePoint)
               {
                  curve *FocusCurve = GetCurve(FocusEntity);
                  SelectControlPointFromCurvePointIndex(FocusCurve, FocusCurvePointIndex);
               }
            } break;
            
            case Button_Right: {
               
               collision Collision = CheckCollisionWith(&Editor->Entities,
                                                        ClickPosition,
                                                        Group->CollisionTolerance,
                                                        Collision_CurvePoint |
                                                        Collision_TrackedPoint |
                                                        Collision_Image);
               
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
         world_position DragFromWorldPosition = ScreenToWorldSpace(DragFromScreenPosition, Group);
         
         switch (DragButton)
         {
            case Button_Left: {
               collision Collision = CheckCollisionWith(&Editor->Entities,
                                                        DragFromWorldPosition,
                                                        Group->CollisionTolerance,
                                                        Collision_CurvePoint |
                                                        Collision_TrackedPoint |
                                                        Collision_CurveLine |
                                                        Collision_Image);
               
               if (Collision.Entity)
               {
                  if (Collision.Flags & Collision_TrackedPoint)
                  {
                     Editor->Mode = MakeMovingTrackedPointMode(Collision.Entity);
                  }
                  else if (Collision.Flags & Collision_CurvePoint)
                  {
                     curve *Curve = GetCurve(Collision.Entity);
                     SelectControlPointFromCurvePointIndex(Curve, Collision.CurvePointIndex);
                     Editor->Mode = MakeMovingCurvePointMode(Editor->MovingPointArena,
                                                             Collision.Entity,
                                                             Collision.CurvePointIndex);
                  }
                  else if ((Collision.Flags & Collision_CurveLine) ||
                           (Collision.Entity->Type == Entity_Image))
                  {
                     Editor->Mode = MakeMovingEntityMode(Collision.Entity);
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
ExecuteUserActionMoveMode(editor *Editor, user_action Action, render_group *Group, editor_input *Input)
{
   editor_mode *Mode = &Editor->Mode;
   Assert(Mode->Type == EditorMode_Moving);
   editor_moving_mode *Moving = &Mode->Moving;
   
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         
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


internal void
ExecuteUserActionRotateMode(editor *Editor, user_action Action, render_group *Group, editor_input *Input)
{
   editor_mode *Mode = &Editor->Mode;
   Assert(Mode->Type == EditorMode_Rotating);
   editor_rotating_mode *Rotating = &Mode->Rotating;
   
   switch (Action.Type)
   {
      case UserAction_MouseMove: {
         entity *Target = Mode->Target;
         
         world_position RotationCenter;
         if (Target)
         {
            if (Target->Type == Entity_Curve)
            {
               curve *Curve = &Target->Curve;
               local_position Sum = {};
               for (u64 PointIndex = 0;
                    PointIndex < Curve->ControlPointCount;
                    ++PointIndex)
               {
                  Sum += Curve->ControlPoints[PointIndex];
               }
               Sum.X = SafeDiv0(Sum.X, Curve->ControlPointCount);
               Sum.Y = SafeDiv0(Sum.Y, Curve->ControlPointCount);
               RotationCenter = LocalEntityPositionToWorld(Target, Sum);
            }
            else
            {
               RotationCenter = Target->Position;
            }
         }
         else
         {
            RotationCenter = Editor->Camera.Position;
         }
         
         world_position From = ScreenToWorldSpace(Action.MouseMove.FromPosition, Group);
         world_position To = ScreenToWorldSpace(Action.MouseMove.ToPosition, Group);
         // NOTE(hbr): Only rotate when rotation is "stable"
         f32 DeadDistance = ClipSpaceLengthToWorldSpace(ROTATION_INDICATOR_RADIUS_CLIP_SPACE, Group);
         if (NormSquared(From - RotationCenter) >= Square(DeadDistance) &&
             NormSquared(To   - RotationCenter) >= Square(DeadDistance))
         {
            rotation_2d Rotation = Rotation2DFromMovementAroundPoint(From, To, RotationCenter);
            if (Target)
            {
               RotateEntityAround(Target, Rotation, RotationCenter);
            }
            else
            {
               RotateCamera(&Editor->Camera, Rotation2DInverse(Rotation));
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
ExecuteUserAction(editor *Editor, user_action Action, render_group *Group, editor_input *Input)
{
   switch (Editor->Mode.Type)
   {
      case EditorMode_Normal:   { ExecuteUserActionNormalMode(Editor, Action, Group, Input); } break;
      case EditorMode_Moving:   { ExecuteUserActionMoveMode(Editor, Action, Group, Input); } break;
      case EditorMode_Rotating: { ExecuteUserActionRotateMode(Editor, Action, Group, Input); } break;
   }
}
#endif

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
         
         Lowering->SavedCurveVertices = CopyLineVertices(Lowering->Arena, Curve->CurveVertices);
         
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
         local_position *Points = Curve->ControlPoints;
         for (u64 PointIndex = 0;
              PointIndex < PointCount;
              ++PointIndex)
         {
            world_position P = LocalEntityPositionToWorld(Entity, Points[PointIndex]);
            // TODO(hbr): Maybe include also point size
            AddPointAABB(&AABB, P);
         }
      } break;
      
      case Entity_Image: {
         // TODO(hbr): Use LocalEntityPositionToWorld here instead
         f32 ScaleX = Entity->Scale.X;
         f32 ScaleY = Entity->Scale.Y;
         
         sf::Vector2u TextureSize = Entity->Image.Texture.getSize();
         f32 HalfWidth = 0.5f * GlobalImageScaleFactor * ScaleX * TextureSize.x;
         f32 HalfHeight = 0.5f * GlobalImageScaleFactor * ScaleY * TextureSize.y;
         
         AddPointAABB(&AABB, Entity->Position + V2( HalfWidth,  HalfHeight));
         AddPointAABB(&AABB, Entity->Position + V2(-HalfWidth,  HalfHeight));
         AddPointAABB(&AABB, Entity->Position + V2( HalfWidth, -HalfHeight));
         AddPointAABB(&AABB, Entity->Position + V2(-HalfWidth, -HalfHeight));
      } break;
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
               UI_DragFloat2F(Entity->Position.E, 0.0f, 0.0f, 0, "Position");
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
               curve_params DefaultParams = Editor->CurveDefaultParams;
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

internal void
UpdateAndRenderNotifications(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
   temp_arena Temp = TempArena(0);
   
   UI_Label(StrLit("Notifications"))
   {
      v2u WindowDim = RenderGroup->Frame->WindowDim;
      f32 Padding = 0.01f * WindowDim.X;
      f32 TargetPosY = WindowDim.Y - Padding;
      
      f32 WindowWidth = 0.1f * WindowDim.X;
      ImVec2 WindowMinSize = ImVec2(WindowWidth, 0.0f);
      ImVec2 WindowMaxSize = ImVec2(WindowWidth, FLT_MAX);
      
      notification *FreeNotification = Editor->Notifications;
      u64 RemoveCount = 0;
      for (u64 NotificationIndex = 0;
           NotificationIndex < Editor->NotificationCount;
           ++NotificationIndex)
      {
         notification *Notification = Editor->Notifications + NotificationIndex;
         Notification->LifeTime += Input->dtForFrame;
         
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
         f32 NextPosY = Lerp(CurrentPosY, TargetPosY, 1.0f - PowF32(2.0f, -MoveSpeed * Input->dtForFrame));
         if (ApproxEq32(TargetPosY, NextPosY))
         {
            NextPosY = TargetPosY;
         }
         Notification->ScreenPosY = NextPosY;
         
         ImVec2 WindowPosition = ImVec2(WindowDim.X - Padding, NextPosY);
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
               v4 TitleColor = {};
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
UpdateAndRenderMenuBar(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
   b32 NewProject    = false;
   b32 OpenProject   = false;
   b32 Quit          = false;
   b32 SaveProject   = false;
   b32 SaveProjectAs = false;
   b32 Import        = false;
   
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
   
   if (UI_BeginMainMenuBar())
   {
      if (UI_BeginMenuF("File"))
      {
         NewProject    = UI_MenuItemF(0, "Ctrl+N",       "New");
         Import        = UI_MenuItemF(0, 0,              "Import");
         OpenProject   = UI_MenuItemF(0, "Ctrl+O",       "Open");
         SaveProject   = UI_MenuItemF(0, "Ctrl+S",       "Save");
         SaveProjectAs = UI_MenuItemF(0, "Shift+Ctrl+S", "Save As");
         Quit          = UI_MenuItemF(0, "Q/Escape",     "Quit");
         UI_EndMenu();
      }
      
      if (UI_BeginMenuF("View"))
      {
         ui_config *Config = &Editor->UI_Config;
         UI_MenuItemF(&Config->ViewListOfEntitiesWindow, 0, "Entity List");
         UI_MenuItemF(&Config->ViewSelectedEntityWindow, 0, "Selected Enstity");
         UI_EndMenu();
      }
      
      if (UI_BeginMenuF("Settings"))
      {
         if (UI_BeginMenuF("Camera"))
         {
            camera *Camera = &Editor->Camera;
            if (UI_MenuItemF(0, 0, "Reset Position"))
            {
               TranslateCamera(Camera, -Camera->P);
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
         UI_ColorPickerF(&Editor->BackgroundColor, "Background Color");
         if (ResetCtxMenu("BackgroundColorReset"))
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
RenderDiagnosticsWindow(editor *Editor, platform_input *Input)
{
   ui_config *Config = &Editor->UI_Config;
   if (Config->ViewDiagnosticsWindow)
   {
      if (UI_BeginWindowF(&Config->ViewDiagnosticsWindow, "Diagnostics"))
      {
         frame_stats *Stats = &Editor->FrameStats;
         UI_TextF("%20s: %.2f ms", "Frame time", 1000.0f * Input->dtForFrame);
         UI_TextF("%20s: %.0f", "FPS", Stats->FPS);
         UI_TextF("%20s: %.2f ms", "Min frame time", 1000.0f * Stats->MinFrameTime);
         UI_TextF("%20s: %.2f ms", "Max frame time", 1000.0f * Stats->MaxFrameTime);
         UI_TextF("%20s: %.2f ms", "Average frame time", 1000.0f * Stats->AvgFrameTime);
      }
      UI_EndWindow();
   }
}

internal void
UpdateAndRenderPointTracking(render_group *Group, editor *Editor, entity *Entity)
{
   TimeFunction;
   
   curve *Curve = &Entity->Curve;
   curve_params *CurveParams = &Curve->CurveParams;
   curve_point_tracking_state *Tracking = &Curve->PointTracking;
   temp_arena Temp = TempArena(Tracking->Arena);
   
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
                  Tracking->TrackedPoint = BezierCurveEvaluateWeighted(Tracking->Fraction,
                                                                       Curve->ControlPoints,
                                                                       Curve->ControlPointWeights,
                                                                       Curve->ControlPointCount);
               }
               
               f32 Radius = GetCurveTrackedPointRadius(Curve);
               v4 Color = V4(0.0f, 1.0f, 0.0f, 0.5f);
               f32 OutlineThickness = 0.3f * Radius;
               v4 OutlineColor = DarkenColor(Color, 0.5f);
               PushCircle(Group,
                          Tracking->TrackedPoint, Radius, Color,
                          GetCurvePartZOffset(CurvePart_Special),
                          OutlineThickness, OutlineColor);
            }
         }
         else
         {
            b32 IsWindowOpen = true;
            DeferBlock(UI_BeginWindowF(&IsWindowOpen, "De Casteljau Algorithm '%S'", Entity->Name),
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
                  f32 LineWidth = Curve->CurveParams.CurveWidth;
                  
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
                  Tracking->TrackedPoint = Intermediate.P[Intermediate.TotalPointCount - 1];
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
                     local_position Point = Tracking->Intermediate.P[PointIndex];
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
      SetEntityModelTransform(RenderGroup, Entity);
      
      v4 Color = Curve->CurveParams.CurveColor;
      Color.A *= 0.5f;
      
      PushVertexArray(RenderGroup,
                      Lowering->SavedCurveVertices.Vertices,
                      Lowering->SavedCurveVertices.VertexCount,
                      Lowering->SavedCurveVertices.Primitive,
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
               v4 AnimatedCurveColor = LerpColor(FromCurve->CurveParams.CurveColor,
                                                 ToCurve->CurveParams.CurveColor,
                                                 Blend);
               
               // NOTE(hbr): If there is no recalculation we can reuse previous buffer without
               // any calculation, because there is already enough space.
               vertex_array_allocation Allocation = (ToCurvePointsRecalculated ?
                                                     LineVerticesAllocationArena(Animation->Arena) :
                                                     LineVerticesAllocationNone(Animation->AnimatedCurveVertices.Vertices));
               Animation->AnimatedCurveVertices =
                  ComputeVerticesOfThickLine(CurvePointCount, AnimatedCurvePoints,
                                             AnimatedCurveWidth, AnimatedCurveColor,
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
UpdateAndRenderCurveCombining(render_group *Group, editor *Editor)
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
      v4 Color = SourceCurve->CurveParams.CurveColor;
      Color.A = Cast(u8)(0.5f * Color.A);
      
      v2 LineDirection = WithPoint - SourcePoint;
      Normalize(&LineDirection);
      
      f32 TriangleSide = 10.0f * LineWidth;
      f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
      world_position BaseVertex = WithPoint - TriangleHeight * LineDirection;
      PushLine(Group, SourcePoint, BaseVertex, LineWidth, Color, GetCurvePartZOffset(CurvePart_Special));
      
      v2 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
      world_position LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
      world_position RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
      PushTriangle(Group, LeftVertex, RightVertex, WithPoint, Color, GetCurvePartZOffset(CurvePart_Special));
   }
   
   EndTemp(Temp);
}

internal void
UpdateAndRenderEntities(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
   TimeFunction;
   
   temp_arena Temp = TempArena(0);
   
   entity_array EntityArray = {};
   EntityArray.Entities = Editor->Entities.Entities;
   EntityArray.Count = MAX_ENTITY_COUNT;
   sorted_entries Sorted = SortEntities(Temp.Arena, EntityArray);
   
#if 0
   UpdateAnimateCurveAnimation(Editor, Input, RenderGroup);
#endif
   
   for (u64 EntryIndex = 0;
        EntryIndex < Sorted.Count;
        ++EntryIndex)
   {
      entity *Entity = EntityArray.Entities + Sorted.Entries[EntryIndex].Index;
      if (IsEntityVisible(Entity))
      {
         SetEntityModelTransform(RenderGroup, Entity);
         f32 BaseZOffset = Cast(f32)Entity->SortingLayer;
         
         switch (Entity->Type)
         {
            case Entity_Curve: {
               curve *Curve = &Entity->Curve;
               curve_params *CurveParams = &Curve->CurveParams;
               
               if (Curve->RecomputeRequested)
               {
                  ActuallyRecomputeCurve(Entity);
               }
               
               if (Editor->Mode.Type == EditorMode_Moving &&
                   Editor->Mode.Moving.Type == MovingMode_CurvePoint &&
                   Editor->Mode.Target == Entity)
               {
                  v4 Color = Curve->CurveParams.CurveColor;
                  Color.A *= 0.5f;
                  PushVertexArray(RenderGroup,
                                  Editor->Mode.Moving.OriginalCurveVertices.Vertices,
                                  Editor->Mode.Moving.OriginalCurveVertices.VertexCount,
                                  Editor->Mode.Moving.OriginalCurveVertices.Primitive,
                                  Color,
                                  GetCurvePartZOffset(CurvePart_LineShadow));
               }
               
               PushVertexArray(RenderGroup,
                               Curve->CurveVertices.Vertices,
                               Curve->CurveVertices.VertexCount,
                               Curve->CurveVertices.Primitive,
                               Curve->CurveParams.CurveColor,
                               GetCurvePartZOffset(CurvePart_Line));
               
               if (CurveParams->PolylineEnabled)
               {
                  PushVertexArray(RenderGroup,
                                  Curve->PolylineVertices.Vertices,
                                  Curve->PolylineVertices.VertexCount,
                                  Curve->PolylineVertices.Primitive,
                                  Curve->CurveParams.PolylineColor,
                                  GetCurvePartZOffset(CurvePart_Special));
               }
               
               if (CurveParams->ConvexHullEnabled)
               {
                  PushVertexArray(RenderGroup,
                                  Curve->ConvexHullVertices.Vertices,
                                  Curve->ConvexHullVertices.VertexCount,
                                  Curve->ConvexHullVertices.Primitive,
                                  Curve->CurveParams.ConvexHullColor,
                                  GetCurvePartZOffset(CurvePart_Special));
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
                     
                     PushLine(RenderGroup, BezierPoint, CenterPoint, HelperLineWidth, CurveParams->CurveColor,
                              GetCurvePartZOffset(CurvePart_Line));
                     PushCircle(RenderGroup, BezierPoint, BezierPointRadius, CurveParams->PointColor,
                                GetCurvePartZOffset(CurvePart_ControlPoint));
                  }
                  
                  u64 ControlPointCount = Curve->ControlPointCount;
                  local_position *ControlPoints = Curve->ControlPoints;
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
                  v4 Color = Curve->CurveParams.CurveColor;
                  PushVertexArray(RenderGroup,
                                  Animation->AnimatedCurveVertices.Vertices,
                                  Animation->AnimatedCurveVertices.VertexCount,
                                  Animation->AnimatedCurveVertices.Primitive,
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
               
               // TODO(hbr): Uncomment that
               //Window->draw(Sprite, VP);
            }break;
         }
         
         ResetModelTransform(RenderGroup);
      }
   }
   
   EndTemp(Temp);
}

internal frame_stats
MakeFrameStats(void)
{
   frame_stats Result = {};
   Result.Calculation.MinFrameTime = INF_F32;
   Result.Calculation.MaxFrameTime = -INF_F32;
   
   return Result;
}

internal void
FrameStatsUpdate(frame_stats *Stats, f32 FrameTime)
{
   Stats->Calculation.FrameCount += 1;
   Stats->Calculation.SumFrameTime += FrameTime;
   Stats->Calculation.MinFrameTime = Min(Stats->Calculation.MinFrameTime, FrameTime);
   Stats->Calculation.MaxFrameTime = Max(Stats->Calculation.MaxFrameTime, FrameTime);
   
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

internal void
DebugUpdateAndRender(editor *Editor)
{
   ui_config *UI = &Editor->UI_Config;
   if (UI->ViewDebugWindow)
   {
      DeferBlock(UI_BeginWindowF(&UI->ViewDebugWindow, "Debug"), UI_EndWindow())
      {
         ImGui::Text("Number of entities = %lu", Editor->EntityCount);
         
         if (Editor->SelectedEntity && Editor->SelectedEntity->Type == Entity_Curve)
         {
            curve *Curve = &Editor->SelectedEntity->Curve;
            ImGui::Text("Number of control points = %lu", Curve->ControlPointCount);
            ImGui::Text("Number of curve points = %lu", Curve->CurvePointCount);
         }
         
         ImGui::Text("Min Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MinFrameTime);
         ImGui::Text("Max Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MaxFrameTime);
         ImGui::Text("Average Frame Time = %.3fms", 1000.0f * Editor->FrameStats.AvgFrameTime);
      }
   }
   
#if BUILD_DEBUG
   ImGui::ShowDemoWindow();
#endif
}

internal void
MovePointAlongCurve(entity *Entity, v2 *TranslateInOut, f32 *FractionInOut)
{
   v2 Translate = *TranslateInOut;
   f32 Fraction = *FractionInOut;
   
   curve *Curve = SafeGetCurve(Entity);
   u64 CurvePointCount = Curve->CurvePointCount;
   local_position *CurvePoints = Curve->CurvePoints;
   f32 DeltaFraction = 1.0f / (CurvePointCount - 1);
   
   {
      u64 SplitCurvePointIndex = ClampTop(Cast(u64)FloorF32(Fraction * (CurvePointCount - 1)), CurvePointCount - 1);
      while (SplitCurvePointIndex + 1 < CurvePointCount)
      {
         f32 NextFraction = Cast(f32)(SplitCurvePointIndex + 1) / (CurvePointCount - 1);
         f32 PrevFraction = Cast(f32)SplitCurvePointIndex / (CurvePointCount - 1);
         Fraction = Clamp(Fraction, PrevFraction, NextFraction);
         
         f32 SegmentFraction = (NextFraction - Fraction) / DeltaFraction;
         SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
         
         v2 CurveSegment =
            LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex + 1])
            - LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex]);
         
         f32 CurveSegmentLength = Norm(CurveSegment);
         f32 InvCurveSegmentLength = 1.0f / CurveSegmentLength;
         
         f32 ProjectionSegmentFraction = ClampBot(Dot(CurveSegment, Translate) *
                                                  InvCurveSegmentLength * InvCurveSegmentLength,
                                                  0.0f);
         
         if (SegmentFraction <= ProjectionSegmentFraction)
         {
            Fraction = NextFraction;
            Translate -= SegmentFraction * CurveSegment;
            SplitCurvePointIndex += 1;
         }
         else
         {
            Fraction += ProjectionSegmentFraction * DeltaFraction;
            Translate -= ProjectionSegmentFraction * CurveSegment;
            break;
         }
      }
   }
   
   {
      u64 SplitCurvePointIndex = ClampTop(Cast(u64)CeilF32(Fraction * (CurvePointCount - 1)), CurvePointCount - 1);
      while (SplitCurvePointIndex >= 1)
      {
         f32 NextFraction = Cast(f32)(SplitCurvePointIndex - 1) / (CurvePointCount - 1);
         f32 PrevFraction = Cast(f32)SplitCurvePointIndex / (CurvePointCount - 1);
         Fraction = Clamp(Fraction, NextFraction, PrevFraction);
         
         f32 SegmentFraction = (Fraction - NextFraction) / DeltaFraction;
         SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
         
         v2 CurveSegment =
            LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex - 1])
            - LocalEntityPositionToWorld(Entity, CurvePoints[SplitCurvePointIndex]);
         
         f32 CurveSegmentLength = Norm(CurveSegment);
         f32 InvCurveSegmentLength = 1.0f / CurveSegmentLength;
         
         f32 ProjectionSegmentFraction = ClampBot(Dot(CurveSegment, Translate) *
                                                  InvCurveSegmentLength * InvCurveSegmentLength,
                                                  0.0f);
         
         if (SegmentFraction <= ProjectionSegmentFraction)
         {
            Fraction = NextFraction;
            Translate -= SegmentFraction * CurveSegment;
            SplitCurvePointIndex -= 1;
         }
         else
         {
            Fraction -= ProjectionSegmentFraction * DeltaFraction;
            Translate -= ProjectionSegmentFraction * CurveSegment;
            break;
         }
      }
   }
   
   *TranslateInOut = Translate;
   *FractionInOut = Fraction;
}

#if 0
internal void
MovePointBackwardAlongCurve(entity *Entity, v2 *TranslateInOut, f32 *FractionInOut)
{
   v2 Translate = *TranslateInOut;
   f32 Fraction = *FractionInOut;
   
   curve *Curve = SafeGetCurve(Entity);
   u64 CurvePointCount = Curve->CurvePointCount;
   f32 DeltaFraction = 1.0f / (CurvePointCount - 1);
   local_position *CurvePoints = Curve->CurvePoints;
   
   u64 SplitCurvePointIndex = ClampTop(Cast(u64)CeilF32(Fraction * (CurvePointCount - 1)), CurvePointCount - 1);
   while ()
   {
      
      f32 SegmentFraction = (Fraction - NextFraction) / DeltaFraction;
      SegmentFraction = Clamp(SegmentFraction, 0.0f, 1.0f);
      
      v2 CurveSegment = CurvePoints[] - CurvePoints[SplitCurvePointIndex];
      
      f32 CurveSegmentLength = Norm(CurveSegment);
      f32 InvCurveSegmentLength = 1.0f / CurveSegmentLength;
      
      f32 ProjectionSegmentFraction = ClampBot(Dot(CurveSegment, Translate) *
                                               InvCurveSegmentLength * InvCurveSegmentLength,
                                               0.0f);
      
      if (SegmentFraction <= ProjectionSegmentFraction)
      {
         Fraction = NextFraction;
         Translate -= SegmentFraction * CurveSegment;
         SplitCurvePointIndex -= 1;
      }
      else
      {
         Fraction -= ProjectionSegmentFraction * DeltaFraction;
         Translate -= ProjectionSegmentFraction * CurveSegment;
         break;
      }
   }
   
   *TranslateInOut = Translate;
   *FractionInOut = Fraction;
}
#endif

EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender)
{
   TimeFunction;
   
   Assert(Memory->PermamentMemorySize >= SizeOf(editor));
   editor *Editor = Cast(editor *)Memory->PermamentMemory;
   if (!Editor->Initialized)
   {
      Editor->BackgroundColor = Editor->DefaultBackgroundColor = RGBA_Color(21, 21, 21);
      Editor->CollisionToleranceClip = 0.02f;
      f32 CurveWidth = 0.009f;
      v4 PolylineColor = RGBA_Color(16, 31, 31, 200);
      Editor->CurveDefaultParams = {
         .CurveColor = RGBA_Color(21, 69, 98),
         .CurveWidth = CurveWidth,
         .PointColor = RGBA_Color(0, 138, 138, 148),
         .PointRadius = 0.014f,
         .PolylineColor = PolylineColor,
         .PolylineWidth = CurveWidth,
         .ConvexHullColor = PolylineColor,
         .ConvexHullWidth = CurveWidth,
         .CurvePointCountPerSegment = 50,
      };
      
      Editor->Camera = {
         .P = V2(0.0f, 0.0f),
         .Rotation = Rotation2DZero(),
         .Zoom = 1.0f,
         .ZoomSensitivity = 0.05f,
         .ReachingTargetSpeed = 10.0f,
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
      
      Editor->Initialized = true;
      
      Editor->LeftClick.OriginalVerticesArena = AllocArena();
   }
   
   // NOTE(hbr): Beginning profiling frame is inside the loop, because we have only
   // space for one frame and rendering has to be done between ImGui::SFML::Update
   // and ImGui::SFML::Render calls
#if EDITOR_PROFILER
   FrameProfilePoint(&Editor->UI_Config.ViewProfilerWindow);
#endif
   
   camera *Camera = &Editor->Camera;
   render_group RenderGroup_ =  BeginRenderGroup(Frame,
                                                 Camera->P, Camera->Rotation, Camera->Zoom,
                                                 Editor->BackgroundColor,
                                                 Editor->CollisionToleranceClip);
   render_group *RenderGroup = &RenderGroup_;
   Editor->RenderGroup = RenderGroup;
   
   FrameStatsUpdate(&Editor->FrameStats, Input->dtForFrame);
   
   UpdateAndRenderNotifications(Editor, Input, RenderGroup);
   
   //- process events
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
         LeftClick->LastMouseP = MouseP;
         
         collision Collision = {};
         if (Input->Pressed[PlatformKey_LeftAlt])
         {
            // NOTE(hbr): Force no collision, so that user can add control point wherever they want
         }
         else
         {
            Collision = CheckCollisionWith(MAX_ENTITY_COUNT,
                                           Editor->Entities.Entities,
                                           MouseP,
                                           RenderGroup->CollisionTolerance);
         }
         
         b32 DoSelectCurvePoint = false;
         if (Collision.Entity)
         {
            LeftClick->TargetEntity = Collision.Entity;
            
            // NOTE(hbr): just move entity if ctrl is pressed
            if (Input->Pressed[PlatformKey_LeftCtrl])
            {
               LeftClick->Mode = EditorLeftClick_MovingEntity;
            }
            else
            {
               curve *Curve = SafeGetCurve(Collision.Entity);
               curve_point_tracking_state *Tracking = &Curve->PointTracking;
               if (Tracking->Active && (Collision.Flags & Collision_CurveLine))
               {
                  LeftClick->Mode = EditorLeftClick_MovingTrackingPoint;
                  f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveLinePointIndex, (Curve->CurvePointCount- 1));
                  SetTrackingPointFraction(Tracking, Fraction);
               }
               else if (Collision.Flags & Collision_CurvePoint)
               {
                  DoSelectCurvePoint = true;
                  LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
                  LeftClick->CurvePointIndex = Collision.CurvePointIndex;
               }
               else if (Collision.Flags & Collision_CurveLine)
               {
                  control_point_index Index = CurveLinePointIndexToControlPointIndex(Curve, Collision.CurveLinePointIndex);
                  u64 InsertAt = Index.Index + 1;
                  InsertControlPoint(Collision.Entity, MouseP, InsertAt);
                  DoSelectCurvePoint = true;
                  LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
                  LeftClick->CurvePointIndex = CurvePointIndexFromControlPointIndex(MakeControlPointIndex(InsertAt));
               }
            }
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
               string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EntityCounter++);
               InitEntity(Entity, V2(0.0f, 0.0f), V2(1.0f, 1.0f), Rotation2DZero(), Name, 0);
               InitCurve(Entity, Editor->CurveDefaultParams);
               EndTemp(Temp);
               
               LeftClick->TargetEntity = Entity;
            }
            Assert(LeftClick->TargetEntity);
            
            DoSelectCurvePoint = true;
            LeftClick->CurvePointIndex = CurvePointIndexFromControlPointIndex(AppendControlPoint(LeftClick->TargetEntity, MouseP));
            LeftClick->Mode = EditorLeftClick_MovingCurvePoint;
         }
         
         SelectEntity(Editor, LeftClick->TargetEntity);
         if (DoSelectCurvePoint)
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
         v2 TranslateLocal =
            WorldToLocalEntityPosition(Entity, MouseP) -
            WorldToLocalEntityPosition(Entity, LeftClick->LastMouseP);
         
         
         switch (LeftClick->Mode)
         {
            case EditorLeftClick_MovingTrackingPoint: {
               
               curve *Curve = SafeGetCurve(Entity);
               curve_point_tracking_state *Tracking = &Curve->PointTracking;
               f32 Fraction = Tracking->Fraction;
               
#if 1
               if (!LeftClick->OriginalVerticesCaptured)
               {
                  arena *Arena = LeftClick->OriginalVerticesArena;
                  ClearArena(Arena);
                  LeftClick->OriginalLineVertices = CopyLineVertices(Arena, Curve->CurveVertices);
                  LeftClick->OriginalVerticesCaptured = true;
               }
               
               MovePointAlongCurve(Entity, &Translate, &Fraction);
#else
               curve_point_tracking_state *Tracking = &Entity->Curve.PointTracking;
               f32 Fraction = Tracking->Fraction;
               MovePointAlongCurve(Entity, &TranslateLocal, &Fraction, false);
               MovePointAlongCurve(Entity, &TranslateLocal, &Fraction, true);
#endif
               SetTrackingPointFraction(Tracking, Fraction);
            }break;
            
            case EditorLeftClick_MovingCurvePoint: {
               translate_curve_point_flags Flags = 0;
               if (Input->Pressed[PlatformKey_LeftShift])
               {
                  Flags |= TranslateCurvePoint_MatchBezierTwinDirection;
               }
               if (Input->Pressed[PlatformKey_LeftCtrl])
               {
                  Flags |= TranslateCurvePoint_MatchBezierTwinLength;
               }
               SetCurvePoint(LeftClick->TargetEntity, LeftClick->CurvePointIndex, MouseP, Flags);
            }break;
            
            case EditorLeftClick_MovingEntity: {
               Entity->Position += Translate;
            }break;
         }
         
         LeftClick->LastMouseP = MouseP;
      }
      
      //- right click events processing
      if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_RightMouseButton && !RightClick->Active)
      {
         Eat = true;
         
         collision Collision = CheckCollisionWith(MAX_ENTITY_COUNT,
                                                  Editor->Entities.Entities,
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
               DeselectCurrentEntity(Editor);
            }
         }
      }
      
      //- camera control events processing
      if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_MiddleMouseButton)
      {
         Eat = true;
         MiddleClick->Active = true;
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
         
         v2 LastMouseP = ClipToWorld(MiddleClick->ClipSpaceLastMouseP, RenderGroup);
         v2 Translate = MouseP - LastMouseP;
         TranslateCamera(&Editor->Camera, -Translate);
         MiddleClick->ClipSpaceLastMouseP = Event->ClipSpaceMouseP;
      }
      
      if (!Eat && Event->Type == PlatformEvent_Scroll)
      {
         Eat = true;
         ZoomCamera(&Editor->Camera, Event->ScrollDelta);
      }
      
      //- some general input handling
      if (!Eat && Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_Escape)
      {
         Eat = true;
         Input->QuitRequested = true;
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
   
   if (RightClick->Active)
   {
      v4 Color = V4(0.5f, 0.5f, 0.5f, 0.3f);
      PushCircle(RenderGroup, RightClick->ClickP, RenderGroup->CollisionTolerance, Color, 0.0f);
   }
   
#if 0
   {
      TimeBlock("editor state update");
      
      for (u64 ButtonIndex = 0;
           ButtonIndex < Button_Count;
           ++ButtonIndex)
      {
         button Button = Cast(button)ButtonIndex;
         button_state *ButtonState = &Input->Buttons[ButtonIndex];
         
         user_action Action = {};
         if (ClickedWithButton(ButtonState, RenderGroup))
         {
            Action.Type = UserAction_ButtonClicked;
            Action.Click.Button = Button;
            // TODO(hbr): I know, I know. But should it be ReleasePosition???
            Action.Click.ClickPosition = ButtonState->ReleasePosition;
         }
         else if (DraggedWithButton(ButtonState, Input->MousePosition, RenderGroup))
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
            ExecuteUserAction(Editor, Action, RenderGroup, Input);
         }
      }
      
      if ((Input->MouseLastPosition != Input->MousePosition) ||
          (Editor->Mode.Type != EditorMode_Normal))
      {
         user_action Action = {};
         Action.Type = UserAction_MouseMove;
         Action.MouseMove.FromPosition = Input->MouseLastPosition;
         Action.MouseMove.ToPosition = Input->MousePosition;
         ExecuteUserAction(Editor, Action, RenderGroup, Input);
      }
   }
#endif
   
#if 0   
   // TODO(hbr): Remove this
   if (KeyPressed(Input, Key_E, 0))
   {
      AddNotificationF(Editor, Notification_Error, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
   }
#endif
   
   RenderSelectedEntityUI(Editor);
   RenderListOfEntitiesWindow(Editor);
   RenderDiagnosticsWindow(Editor, Input);
   DebugUpdateAndRender(Editor);
   UpdateAndRenderEntities(Editor, Input, RenderGroup);
   
   // NOTE(hbr): Render rotation indicator if rotating
   if (Editor->Mode.Type == EditorMode_Rotating)
   {
      // TODO(hbr): Merge cases here.
      entity *Entity = Editor->Mode.Target;
      v2 RotationIndicatorP = {};
      if (Entity)
      {
         switch (Entity->Type)
         {
            case Entity_Curve: {
               RotationIndicatorP = ScreenToWorldSpace(Editor->Mode.Rotating.Center, RenderGroup);
            } break;
            
            case Entity_Image: {
               RotationIndicatorP = Entity->Position;
            } break;
         }
      }
      else
      {
         RotationIndicatorP = Editor->Camera.P;
      }
      
      ResetModelTransform(RenderGroup);
      
#define ROTATION_INDICATOR_RADIUS_CLIP_SPACE 0.1f
      f32 Radius = ClipSpaceLengthToWorldSpace(ROTATION_INDICATOR_RADIUS_CLIP_SPACE, RenderGroup);
      v4 Color = RGBA_Color(30, 56, 87, 80);
      f32 OutlineThickness = 0.1f * Radius;
      v4 OutlineColor = RGBA_Color(255, 255, 255, 24);
      PushCircle(RenderGroup,
                 RotationIndicatorP,
                 Radius - OutlineThickness,
                 Color, 0.0f,
                 OutlineThickness, OutlineColor);
   }
   
   // NOTE(hbr): Update menu bar here, because world has to already be rendered
   // and UI not, because we might save our project into image file and we
   // don't want to include UI render into our image.
   UpdateAndRenderMenuBar(Editor, Input, RenderGroup);
   
#if  0
   UI_MenuItemF(&Config->ViewProfilerWindow,     0, "Profiler");
   UI_MenuItemF(&Config->ViewDebugWindow,        0, "Debug");
   UI_MenuItemF(&Config->ViewDiagnosticsWindow,  0, "Diagnostics");
#endif
   
}

// NOTE(hbr): Specifically after every file is included. Works in Unity build only.
StaticAssert(__COUNTER__ < ArrayCount(profiler::Anchors), ProfileAnchorsFitIntoArray);