// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->Params.Interpolation == Interpolation_CubicSpline &&
                 Curve->Params.CubicSpline   == CubicSpline_Periodic);
 return IsLooped;
}

struct points_soa
{
 f32 *Xs;
 f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2 *Points, u32 NumPoints)
{
 points_soa Result = {};
 Result.Xs = PushArrayNonZero(Arena, NumPoints, f32);
 Result.Ys = PushArrayNonZero(Arena, NumPoints, f32);
 for (u32 I = 0; I < NumPoints; ++I)
 {
  Result.Xs[I] = Points[I].X;
  Result.Ys[I] = Points[I].Y;
 }
 
 return Result;
}

internal void
CalcNewtonPolynomialLinePoints(v2 *ControlPoints,
                               u32 ControlPointCount,
                               f32 *Ti,
                               u32 NumOutputLinePoints,
                               v2 *OutputLinePoints)
{
 if (ControlPointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena,
                                             ControlPoints,
                                             ControlPointCount);
  
  f32 *Beta_X = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  f32 *Beta_Y = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  NewtonBetaFast(Beta_X, Ti, SOA.Xs, ControlPointCount);
  NewtonBetaFast(Beta_Y, Ti, SOA.Ys, ControlPointCount);
  
  f32 Begin = Ti[0];
  f32 End = Ti[ControlPointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (NumOutputLinePoints - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < NumOutputLinePoints;
       ++OutputIndex)
  {
   f32 X = NewtonEvaluate(T, Beta_X, Ti, ControlPointCount);
   f32 Y = NewtonEvaluate(T, Beta_Y, Ti, ControlPointCount);
   OutputLinePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcBarycentricPolynomialLinePoints(v2 *ControlPoints,
                                    u32 ControlPointCount,
                                    point_spacing PointSpacing,
                                    f32 *Ti,
                                    u32 NumOutputLinePoints,
                                    v2 *OutputLinePoints)
{
 if (ControlPointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  f32 *Omega = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  switch (PointSpacing)
  {
   case PointSpacing_Equidistant: {
    // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
    BarycentricOmega(Omega, Ti, ControlPointCount);
   } break;
   case PointSpacing_Chebychev: {
    BarycentricOmegaChebychev(Omega, ControlPointCount);
   } break;
   
   case PointSpacing_Count: InvalidPath;
  }
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena,
                                             ControlPoints,
                                             ControlPointCount);
  
  f32 Begin = Ti[0];
  f32 End = Ti[ControlPointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (NumOutputLinePoints - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < NumOutputLinePoints;
       ++OutputIndex)
  {
   f32 X = BarycentricEvaluate(T, Omega, Ti, SOA.Xs, ControlPointCount);
   f32 Y = BarycentricEvaluate(T, Omega, Ti, SOA.Ys, ControlPointCount);
   OutputLinePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcCubicSplineLinePoints(v2 *ControlPoints,
                          u32 ControlPointCount,
                          cubic_spline_type CubicSpline,
                          u32 NumOutputLinePoints,
                          v2 *OutputLinePoints)
{
 if (ControlPointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  if (CubicSpline == CubicSpline_Periodic)
  {
   v2 *OriginalControlPoints = ControlPoints;
   
   ControlPoints = PushArrayNonZero(Temp.Arena, ControlPointCount + 1, v2);
   MemoryCopy(ControlPoints,
              OriginalControlPoints,
              ControlPointCount * SizeOf(ControlPoints[0]));
   ControlPoints[ControlPointCount] = OriginalControlPoints[0];
   
   ControlPointCount += 1;
  }
  
  f32 *Ti = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  EquidistantPoints(Ti, ControlPointCount);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, ControlPoints, ControlPointCount);
  
  f32 *Mx = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  f32 *My = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  switch (CubicSpline)
  {
   case CubicSpline_Natural: {
    CubicSplineNaturalM(Mx, Ti, SOA.Xs, ControlPointCount);
    CubicSplineNaturalM(My, Ti, SOA.Ys, ControlPointCount);
   } break;
   
   case CubicSpline_Periodic: {
    CubicSplinePeriodicM(Mx, Ti, SOA.Xs, ControlPointCount);
    CubicSplinePeriodicM(My, Ti, SOA.Ys, ControlPointCount);
   } break;
   
   case CubicSpline_Count: InvalidPath;
  }
  
  f32 Begin = Ti[0];
  f32 End = Ti[ControlPointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (NumOutputLinePoints - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < NumOutputLinePoints;
       ++OutputIndex)
  {
   
   f32 X = CubicSplineEvaluate(T, Mx, Ti, SOA.Xs, ControlPointCount);
   f32 Y = CubicSplineEvaluate(T, My, Ti, SOA.Ys, ControlPointCount);
   OutputLinePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcNormalBezierLinePoints(v2 *ControlPoints,
                           u32 ControlPointCount,
                           u32 NumOutputLinePoints,
                           v2 *OutputLinePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (NumOutputLinePoints - 1);
 for (u32 OutputIndex = 0;
      OutputIndex < NumOutputLinePoints;
      ++OutputIndex)
 {
  
  OutputLinePoints[OutputIndex] = BezierCurveEvaluate(T,
                                                      ControlPoints,
                                                      ControlPointCount);
  
  T += Delta;
 }
}

internal void
CalcRegularBezierLinePoints(v2 *ControlPoints,
                            f32 *ControlPointWeights,
                            u32 ControlPointCount,
                            u32 NumOutputLinePoints,
                            v2 *OutputLinePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (NumOutputLinePoints - 1);
 for (u32 OutputIndex = 0;
      OutputIndex < NumOutputLinePoints;
      ++OutputIndex)
 {
  
  OutputLinePoints[OutputIndex] = BezierCurveEvaluateWeighted(T,
                                                              ControlPoints,
                                                              ControlPointWeights,
                                                              ControlPointCount);
  
  T += Delta;
 }
}

struct cubic_bezier_curve_segment
{
 u32 PointCount;
 v2 *Points;
};
internal cubic_bezier_curve_segment
GetCubicBezierCurveSegment(cubic_bezier_point *BezierPoints, u32 PointCount, u32 SegmentIndex)
{
 cubic_bezier_curve_segment Result = {};
 if (SegmentIndex < PointCount)
 {
  Result.PointCount = (SegmentIndex < PointCount - 1 ? 4 : 1);
  Result.Points = &BezierPoints[SegmentIndex].P1;
 }
 
 return Result;
}

internal void
CalcCubicBezierLinePoints(cubic_bezier_point *CubicBezierPoints,
                          u32 ControlPointCount,
                          u32 NumOutputLinePoints,
                          v2 *OutputLinePoints)
{
 if (ControlPointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta = 1.0f / (NumOutputLinePoints - 1);
  for (u32 OutputIndex = 0;
       OutputIndex < NumOutputLinePoints;
       ++OutputIndex)
  {
   f32 Expanded_T = (ControlPointCount - 1) * T;
   u32 SegmentIndex = Cast(u32)Expanded_T;
   f32 Segment_T = Expanded_T - SegmentIndex;
   
   Assert(SegmentIndex < ControlPointCount);
   Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
   
   cubic_bezier_curve_segment Segment = GetCubicBezierCurveSegment(CubicBezierPoints, ControlPointCount, SegmentIndex);
   OutputLinePoints[OutputIndex] = BezierCurveEvaluate(Segment_T,
                                                       Segment.Points,
                                                       Segment.PointCount);
   
   T += Delta;
  }
 }
}

internal void
EvaluateCurve(curve *Curve, u32 LinePointCount, v2 *LinePoints)
{
 temp_arena Temp = TempArena(0);
 curve_params *CurveParams = &Curve->Params;
 switch (CurveParams->Interpolation)
 {
  case Interpolation_Polynomial: {
   polynomial_interpolation_params *Polynomial = &CurveParams->Polynomial;
   
   f32 *Ti = PushArrayNonZero(Temp.Arena, Curve->ControlPointCount, f32);
   switch (Polynomial->PointSpacing)
   {
    case PointSpacing_Equidistant: { EquidistantPoints(Ti, Curve->ControlPointCount); } break;
    case PointSpacing_Chebychev:   { ChebyshevPoints(Ti, Curve->ControlPointCount);   } break;
    case PointSpacing_Count: InvalidPath;
   }
   
   switch (Polynomial->Type)
   {
    case PolynomialInterpolation_Barycentric: {
     CalcBarycentricPolynomialLinePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                         Polynomial->PointSpacing, Ti,
                                         LinePointCount, LinePoints);
    } break;
    
    case PolynomialInterpolation_Newton: {
     CalcNewtonPolynomialLinePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                    Ti, LinePointCount, LinePoints);
    } break;
    
    case PolynomialInterpolation_Count: InvalidPath;
   }
  } break;
  
  case Interpolation_CubicSpline: {
   CalcCubicSplineLinePoints(Curve->ControlPoints, Curve->ControlPointCount,
                             CurveParams->CubicSpline,
                             LinePointCount, LinePoints);
  } break;
  
  case Interpolation_Bezier: {
   switch (CurveParams->Bezier)
   {
    case Bezier_Regular: {
     CalcRegularBezierLinePoints(Curve->ControlPoints, Curve->ControlPointWeights,
                                 Curve->ControlPointCount, LinePointCount, LinePoints);
    } break;
    
    case Bezier_Cubic: {
     CalcCubicBezierLinePoints(Curve->CubicBezierPoints, Curve->ControlPointCount,
                               LinePointCount, LinePoints);
    } break;
    
    case Bezier_Count: InvalidPath;
   }
  } break;
  
  case Interpolation_Count: InvalidPath;
 }
 EndTemp(Temp);
}

internal void
ActuallyRecomputeCurve(entity *Entity)
{
 curve *Curve = GetCurve(Entity);
 ClearArena(Entity->Arena);
 
 u32 LinePointCount = 0;
 if (Curve->ControlPointCount > 0)
 {
  LinePointCount = (Curve->ControlPointCount - 1) * Curve->Params.PointCountPerSegment + 1;
 }
 Curve->LinePointCount = LinePointCount;
 Curve->LinePoints = PushArrayNonZero(Entity->Arena, LinePointCount, v2);
 EvaluateCurve(Curve, Curve->LinePointCount, Curve->LinePoints);
 
 Curve->LineVertices = ComputeVerticesOfThickLine(Entity->Arena, Curve->LinePointCount, Curve->LinePoints,  Curve->Params.LineWidth, IsCurveLooped(Curve));
 Curve->PolylineVertices = ComputeVerticesOfThickLine(Entity->Arena, Curve->ControlPointCount, Curve->ControlPoints, Curve->Params.PolylineWidth, false);
 
 Curve->ConvexHullPoints = PushArrayNonZero(Entity->Arena, Curve->ControlPointCount, v2);
 Curve->ConvexHullCount = CalcConvexHull(Curve->ControlPointCount, Curve->ControlPoints, Curve->ConvexHullPoints);
 Curve->ConvexHullVertices = ComputeVerticesOfThickLine(Entity->Arena, Curve->ConvexHullCount, Curve->ConvexHullPoints, Curve->Params.ConvexHullWidth, true);
}

internal void
SetTrackingPointFraction(curve_point_tracking_state *Tracking, f32 Fraction)
{
 Tracking->Fraction = Clamp01(Fraction);
 Tracking->NeedsRecomputationThisFrame = true;
}

internal void
BeginCurvePointTracking(curve *Curve, b32 IsSplitting)
{
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 Tracking->Active = true;
 Tracking->IsSplitting = IsSplitting;
 ClearArena(Tracking->Arena);
 SetTrackingPointFraction(Tracking, 0.0f);
}

internal void
SetCurveControlPoints(entity *Entity,
                      u32 ControlPointCount,
                      v2 *ControlPoints,
                      f32 *ControlPointWeights,
                      cubic_bezier_point *CubicBezierPoints)
{
 curve *Curve = GetCurve(Entity);
 Assert(ControlPointCount <= MAX_CONTROL_POINT_COUNT);
 ControlPointCount = Min(ControlPointCount, MAX_CONTROL_POINT_COUNT);
 
 ArrayCopy(Curve->ControlPoints, ControlPoints, ControlPointCount);
 ArrayCopy(Curve->ControlPointWeights, ControlPointWeights, ControlPointCount);
 ArrayCopy(Curve->CubicBezierPoints, CubicBezierPoints, ControlPointCount);
 
 Curve->ControlPointCount = ControlPointCount;
 // TODO(hbr): Make sure this is expected, maybe add to internal arguments
 UnselectControlPoint(Curve);
 
 RecomputeCurve(Entity);
}

internal void
SetCurveControlPoint(entity *Entity, u32 PointIndex, v2 Point, f32 Weight)
{
 curve *Curve = GetCurve(Entity);
 if (PointIndex < Curve->ControlPointCount)
 {
  v2 Translation = Point - Curve->ControlPoints[PointIndex];
  
  Curve->ControlPoints[PointIndex] = Point;
  Curve->ControlPointWeights[PointIndex] = Weight;
  
  cubic_bezier_point *BezierPoints = Curve->CubicBezierPoints + PointIndex;
  BezierPoints->P0 += Translation;
  BezierPoints->P1 += Translation;
  BezierPoints->P2 += Translation;
  
  RecomputeCurve(Entity);
 }
}