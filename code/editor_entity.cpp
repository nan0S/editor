// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->CurveParams.InterpolationType == Interpolation_CubicSpline &&
                 Curve->CurveParams.CubicSplineType   == CubicSpline_Periodic);
 return IsLooped;
}

struct points_soa
{
 f32 *Xs;
 f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2 *Points, u64 NumPoints)
{
 points_soa Result = {};
 Result.Xs = PushArrayNonZero(Arena, NumPoints, f32);
 Result.Ys = PushArrayNonZero(Arena, NumPoints, f32);
 for (u64 I = 0; I < NumPoints; ++I)
 {
  Result.Xs[I] = Points[I].X;
  Result.Ys[I] = Points[I].Y;
 }
 
 return Result;
}

internal void
CalcNewtonPolynomialCurvePoints(v2 *ControlPoints,
                                u64 ControlPointCount,
                                f32 *Ti,
                                u64 NumOutputCurvePoints,
                                v2 *OutputCurvePoints)
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
  f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
  
  for (u64 OutputIndex = 0;
       OutputIndex < NumOutputCurvePoints;
       ++OutputIndex)
  {
   f32 X = NewtonEvaluate(T, Beta_X, Ti, ControlPointCount);
   f32 Y = NewtonEvaluate(T, Beta_Y, Ti, ControlPointCount);
   OutputCurvePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcBarycentricPolynomialCurvePoints(v2 *ControlPoints,
                                     u64 ControlPointCount,
                                     points_arrangement PointsArrangement,
                                     f32 *Ti,
                                     u64 NumOutputCurvePoints,
                                     v2 *OutputCurvePoints)
{
 if (ControlPointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  f32 *Omega = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
  switch (PointsArrangement)
  {
   case PointsArrangement_Equidistant: {
    // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
    BarycentricOmega(Omega, Ti, ControlPointCount);
   } break;
   case PointsArrangement_Chebychev: {
    BarycentricOmegaChebychev(Omega, ControlPointCount);
   } break;
   
   case PointsArrangement_Count: InvalidPath;
  }
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena,
                                             ControlPoints,
                                             ControlPointCount);
  
  f32 Begin = Ti[0];
  f32 End = Ti[ControlPointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
  
  for (u64 OutputIndex = 0;
       OutputIndex < NumOutputCurvePoints;
       ++OutputIndex)
  {
   f32 X = BarycentricEvaluate(T, Omega, Ti, SOA.Xs, ControlPointCount);
   f32 Y = BarycentricEvaluate(T, Omega, Ti, SOA.Ys, ControlPointCount);
   OutputCurvePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcCubicSplineCurvePoints(v2 *ControlPoints,
                           u64 ControlPointCount,
                           cubic_spline_type CubicSplineType,
                           u64 NumOutputCurvePoints,
                           v2 *OutputCurvePoints)
{
 if (ControlPointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  if (CubicSplineType == CubicSpline_Periodic)
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
  switch (CubicSplineType)
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
  f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
  
  for (u64 OutputIndex = 0;
       OutputIndex < NumOutputCurvePoints;
       ++OutputIndex)
  {
   
   f32 X = CubicSplineEvaluate(T, Mx, Ti, SOA.Xs, ControlPointCount);
   f32 Y = CubicSplineEvaluate(T, My, Ti, SOA.Ys, ControlPointCount);
   OutputCurvePoints[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcNormalBezierCurvePoints(v2 *ControlPoints,
                            u64 ControlPointCount,
                            u64 NumOutputCurvePoints,
                            v2 *OutputCurvePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
 for (u64 OutputIndex = 0;
      OutputIndex < NumOutputCurvePoints;
      ++OutputIndex)
 {
  
  OutputCurvePoints[OutputIndex] = BezierCurveEvaluate(T,
                                                       ControlPoints,
                                                       ControlPointCount);
  
  T += Delta;
 }
}

internal void
CalcRegularBezierCurvePoints(v2 *ControlPoints,
                             f32 *ControlPointWeights,
                             u64 ControlPointCount,
                             u64 NumOutputCurvePoints,
                             v2 *OutputCurvePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
 for (u64 OutputIndex = 0;
      OutputIndex < NumOutputCurvePoints;
      ++OutputIndex)
 {
  
  OutputCurvePoints[OutputIndex] = BezierCurveEvaluateWeighted(T,
                                                               ControlPoints,
                                                               ControlPointWeights,
                                                               ControlPointCount);
  
  T += Delta;
 }
}

struct cubic_bezier_curve_segment
{
 u64 PointCount;
 v2 *Points;
};
internal cubic_bezier_curve_segment
GetCubicBezierCurveSegment(cubic_bezier_point *BezierPoints, u64 PointCount, u64 SegmentIndex)
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
CalcCubicBezierCurvePoints(cubic_bezier_point *CubicBezierPoints,
                           u64 ControlPointCount,
                           u64 NumOutputCurvePoints,
                           v2 *OutputCurvePoints)
{
 if (ControlPointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
  for (u64 OutputIndex = 0;
       OutputIndex < NumOutputCurvePoints;
       ++OutputIndex)
  {
   f32 Expanded_T = (ControlPointCount - 1) * T;
   u64 SegmentIndex = Cast(u64)Expanded_T;
   f32 Segment_T = Expanded_T - SegmentIndex;
   
   Assert(SegmentIndex < ControlPointCount);
   Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
   
   cubic_bezier_curve_segment Segment = GetCubicBezierCurveSegment(CubicBezierPoints, ControlPointCount, SegmentIndex);
   OutputCurvePoints[OutputIndex] = BezierCurveEvaluate(Segment_T,
                                                        Segment.Points,
                                                        Segment.PointCount);
   
   T += Delta;
  }
 }
}

internal void
EvaluateCurve(curve *Curve, u64 CurvePointCount, v2 *CurvePoints)
{
 temp_arena Temp = TempArena(0);
 curve_params *CurveParams = &Curve->CurveParams;
 switch (CurveParams->InterpolationType)
 {
  case Interpolation_Polynomial: {
   polynomial_interpolation_params *Polynomial = &CurveParams->PolynomialInterpolationParams;
   
   f32 *Ti = PushArrayNonZero(Temp.Arena, Curve->ControlPointCount, f32);
   switch (Polynomial->PointsArrangement)
   {
    case PointsArrangement_Equidistant: { EquidistantPoints(Ti, Curve->ControlPointCount); } break;
    case PointsArrangement_Chebychev:   { ChebyshevPoints(Ti, Curve->ControlPointCount);   } break;
    case PointsArrangement_Count: InvalidPath;
   }
   
   switch (Polynomial->Type)
   {
    case PolynomialInterpolation_Barycentric: {
     CalcBarycentricPolynomialCurvePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                          Polynomial->PointsArrangement, Ti,
                                          CurvePointCount, CurvePoints);
    } break;
    
    case PolynomialInterpolation_Newton: {
     CalcNewtonPolynomialCurvePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                     Ti, CurvePointCount, CurvePoints);
    } break;
    
    case PolynomialInterpolation_Count: InvalidPath;
   }
  } break;
  
  case Interpolation_CubicSpline: {
   CalcCubicSplineCurvePoints(Curve->ControlPoints, Curve->ControlPointCount,
                              CurveParams->CubicSplineType,
                              CurvePointCount, CurvePoints);
  } break;
  
  case Interpolation_Bezier: {
   switch (CurveParams->BezierType)
   {
    case Bezier_Regular: {
     CalcRegularBezierCurvePoints(Curve->ControlPoints, Curve->ControlPointWeights,
                                  Curve->ControlPointCount, CurvePointCount, CurvePoints);
    } break;
    
    case Bezier_Cubic: {
     CalcCubicBezierCurvePoints(Curve->CubicBezierPoints, Curve->ControlPointCount,
                                CurvePointCount, CurvePoints);
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
 TimeFunction;
 
 temp_arena Temp = TempArena(Entity->Arena);
 
 curve *Curve = GetCurve(Entity);
 ClearArena(Entity->Arena);
 
 u64 CurvePointCount = 0;
 if (Curve->ControlPointCount > 0)
 {
  CurvePointCount = (Curve->ControlPointCount - 1) * Curve->CurveParams.CurvePointCountPerSegment + 1;
 }
 Curve->CurvePointCount = CurvePointCount;
 Curve->CurvePoints = PushArrayNonZero(Entity->Arena, CurvePointCount, v2);
 EvaluateCurve(Curve, Curve->CurvePointCount, Curve->CurvePoints);
 
 Curve->CurveVertices = ComputeVerticesOfThickLine(Entity->Arena, Curve->CurvePointCount, Curve->CurvePoints,  Curve->CurveParams.CurveWidth, IsCurveLooped(Curve));
 Curve->PolylineVertices = ComputeVerticesOfThickLine(Entity->Arena, Curve->ControlPointCount, Curve->ControlPoints, Curve->CurveParams.PolylineWidth, false);
 
 v2 *ConvexHullPoints = PushArrayNonZero(Temp.Arena, Curve->ControlPointCount, v2);
 u64 NumConvexHullPoints = CalcConvexHull(Curve->ControlPointCount, Curve->ControlPoints, ConvexHullPoints);
 Curve->ConvexHullVertices = ComputeVerticesOfThickLine(Entity->Arena, NumConvexHullPoints, ConvexHullPoints, Curve->CurveParams.ConvexHullWidth, true);
 
 EndTemp(Temp);
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

internal v2
LocalEntityPositionToWorld(entity *Entity, v2 P)
{
 v2 Result = RotateAround(P, V2(0.0f, 0.0f), Entity->Rotation) + Entity->P;
 return Result;
}

internal void
SetCurveControlPoints(entity *Entity,
                      u64 ControlPointCount,
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
SetCurveControlPoint(entity *Entity, u64 PointIndex, v2 Point, f32 Weight)
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