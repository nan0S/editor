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

internal sort_entry_array
SortEntities(arena *Arena, entity_array Entities)
{
 sort_entry_array SortArray = AllocSortEntryArray(Arena, Entities.Count, SortOrder_Descending);
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  // NOTE(hbr): Equal sorting layer images should be below curves
  f32 Offset = 0.0f;
  switch (Entity->Type)
  {
   case Entity_Image: {Offset = 0.5f;} break;
   case Entity_Curve: {Offset = 0.0f;} break;
   case Entity_Count: InvalidPath; break;
  }
  
  AddSortEntry(&SortArray, Entity->SortingLayer + Offset, EntityIndex);
 }
 
 Sort(SortArray.Entries, SortArray.Count, SortFlag_Stable);
 
 return SortArray;
}

internal collision
CheckCollisionWithEntities(entity_array Entities, v2 AtP, f32 Tolerance)
{
 ProfileFunctionBegin();
 
 collision Result = {};
 temp_arena Temp = TempArena(0);
 sort_entry_array Sorted = SortEntities(Temp.Arena, Entities);
 
 for (u64 SortedIndex = 0;
      SortedIndex < Sorted.Count;
      ++SortedIndex)
 {
  u64 InverseIndex = Sorted.Count-1 - SortedIndex;
  u32 EntityIndex = Sorted.Entries[InverseIndex].Index;
  entity *Entity = Entities.Entities[EntityIndex];
  
  if (IsEntityVisible(Entity))
  {
   v2 LocalAtP = WorldToLocalEntityPosition(Entity, AtP);
   // TODO(hbr): Note that Tolerance becomes wrong when we want to compute everything in
   // local space, fix that
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     u32 ControlPointCount = Curve->Points.ControlPointCount;
     v2 *ControlPoints = Curve->Points.ControlPoints;
     curve_params *Params = &Curve->Params;
     point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
     
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
        point_draw_info PointInfo = GetCurveControlPointDrawInfo(Entity, ControlPointHandleFromIndex(PointIndex));
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
        Result.CurvePoint = CurvePointFromControlPoint(ControlPointHandleFromIndex(MinPointIndex));
       }
      }
      
      //- bezier "control" points
      {
       visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
       f32 MinSignedDistance = F32_INF;
       cubic_bezier_point_handle MinPoint = {};
       
       for (u32 Index = 0;
            Index < VisibleBeziers.Count;
            ++Index)
       {
        f32 PointRadius = GetCurveCubicBezierPointRadius(Curve);
        cubic_bezier_point_handle Bezier = VisibleBeziers.Indices[Index];
        v2 BezierPoint = GetCubicBezierPoint(Curve, Bezier);
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, BezierPoint, PointRadius + Tolerance);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPoint = Bezier;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePoint = CurvePointFromCubicBezierPoint(MinPoint);
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
      if ((Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization) && !Result.Entity)
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
        multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                               CurveSamples,
                                                               PointCount,
                                                               Params->LineWidth,
                                                               Tolerance);
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
                                                             Curve->CurveSamples,
                                                             Curve->CurveSampleCount,
                                                             Params->LineWidth,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_CurveLine;
       Result.CurveSampleIndex = Line.PointIndex;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsConvexHullVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->ConvexHullPoints,
                                                             Curve->ConvexHullCount,
                                                             Params->ConvexHullWidth,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsPolylineVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             ControlPoints,
                                                             ControlPointCount,
                                                             Params->PolylineWidth,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     if (Are_B_SplineKnotsVisible(Curve))
     {
      //b_spline_knots Knots = Curve->B_SplineKnots;
      u32 PartitionSize = Curve->B_SplineKnotParams.PartitionSize;
      v2 *PartitionKnotPoints = Curve->B_SplinePartitionKnotPoints;
      point_draw_info KnotPointInfo = Get_B_SplineKnotPointDrawInfo(Entity);
      
      for (u32 KnotIndex = 0;
           KnotIndex < PartitionSize;
           ++KnotIndex)
      {
       v2 Knot = PartitionKnotPoints[KnotIndex];
       if (PointCollision(LocalAtP, Knot, KnotPointInfo.Radius))
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_B_SplineKnot;
        Result.KnotIndex = KnotIndex;
        break;
       }
      }
     }
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     scale2d Dim = Image->Dim;
     if (-Dim.V.X <= LocalAtP.X && LocalAtP.X <= Dim.V.X &&
         -Dim.V.Y <= LocalAtP.Y && LocalAtP.Y <= Dim.V.Y)
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
 
 ProfileEnd();
 
 return Result;
}