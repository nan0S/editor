inline internal entity_snapshot_for_merging
MakeEntitySnapshotForMerging(entity *Entity)
{
 entity_snapshot_for_merging Result = {};
 Result.Entity = Entity;
 if (Entity)
 {
  Result.P = Entity->P;
  Result.Scale = Entity->Scale;
  Result.Rotation = Entity->Rotation;
  Result.Flags = Entity->Flags;
  Result.Version = Entity->Version;
 }
 return Result;
}

inline internal b32
EntityModified(entity_snapshot_for_merging Versioned, entity *Entity)
{
 b32 Result = false;
 if (Versioned.Entity != Entity ||
     (Entity && (!StructsEqual(Entity->P, Versioned.P) ||
                 !StructsEqual(Entity->Scale, Versioned.Scale) ||
                 !StructsEqual(Entity->Rotation, Versioned.Rotation) ||
                 !StructsEqual(Entity->Flags, Versioned.Flags) ||
                 !StructsEqual(Entity->Version, Versioned.Version))))
 {
  Result = true;
 }
 return Result;
}

inline internal entity_handle
MakeEntityHandle(entity *Entity)
{
 entity_handle Handle = {};
 if (Entity)
 {
  Handle.Entity = Entity;
  Handle.Generation = Entity->Generation;
 }
 return Handle;
}

inline internal entity *
EntityFromHandle(entity_handle Handle)
{
 entity *Result = 0;
 if (Handle.Entity && (Handle.Generation == Handle.Entity->Generation))
 {
  Result = Handle.Entity;
 }
 return Result;
}

inline internal curve *
SafeGetCurve(entity *Entity)
{
 Assert(Entity->Type == Entity_Curve);
 return &Entity->Curve;
}

inline internal image *
SafeGetImage(entity *Entity)
{
 Assert(Entity->Type == Entity_Image);
 return &Entity->Image;
}

inline internal curve_point_index
CurvePointIndexFromControlPointIndex(control_point_index Index)
{
 curve_point_index Result = {};
 Result.Type = CurvePoint_ControlPoint;
 Result.ControlPoint = Index;
 
 return Result;
}

inline internal curve_point_index
CurvePointIndexFromBezierPointIndex(cubic_bezier_point_index Index)
{
 curve_point_index Result = {};
 Result.Type = CurvePoint_CubicBezierPoint;
 Result.BezierPoint = Index;
 
 return Result;
}

inline internal cubic_bezier_point_index
CubicBezierPointIndexFromControlPointIndex(control_point_index Index)
{
 cubic_bezier_point_index Result = {};
 Result.Index = 3 * Index.Index + 1;
 
 return Result;
}

inline internal control_point_index
MakeControlPointIndex(u32 Index)
{
 control_point_index Result = {};
 Result.Index = Index;
 return Result;
}

inline internal v2
GetCubicBezierPoint(curve *Curve, cubic_bezier_point_index Index)
{
 v2 Result = {};
 if (Index.Index < 3 * Curve->ControlPointCount)
 {
  v2 *Beziers = Cast(v2 *)Curve->CubicBezierPoints;
  Result = Beziers[Index.Index];
 }
 
 return Result;
}

inline internal v2
GetCenterPointFromCubicBezierPointIndex(curve *Curve, cubic_bezier_point_index Index)
{
 v2 Result = {};
 u32 ControlPointIndex = Index.Index / 3;
 if (ControlPointIndex < Curve->ControlPointCount)
 {
  Result = Curve->ControlPoints[ControlPointIndex];
 }
 
 return Result;
}

inline internal control_point_index
CurveLinePointIndexToControlPointIndex(curve *Curve, u32 CurveLinePointIndex)
{
 Assert(!IsCurveTotalSamplesMode(Curve));
 u32 Index = SafeDiv0(CurveLinePointIndex, Curve->Params.SamplesPerControlPoint);
 Assert(Index < Curve->ControlPointCount);
 control_point_index Result = {};
 Result.Index = ClampTop(Index, Curve->ControlPointCount - 1);
 
 return Result;
}

internal b32
AreCurvePointsVisible(curve *Curve)
{
 b32 Result = (!Curve->Params.PointsDisabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsPolylineVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->PolylineEnabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsConvexHullVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->ConvexHullEnabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
UsesControlPoints(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_B_Spline: {Result = true;}break;
  
  case Curve_Parametric: {}break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

internal b32
IsEntityVisible(entity *Entity)
{
 b32 Result = !(Entity->Flags & EntityFlag_Hidden);
 return Result;
}

internal void
SetEntityVisibility(entity *Entity, b32 Visible)
{
 if (Visible)
 {
  Entity->Flags &= ~EntityFlag_Hidden;
 }
 else
 {
  Entity->Flags |= EntityFlag_Hidden;
 }
}

internal b32
IsEntitySelected(entity *Entity)
{
 b32 Result = (Entity->Flags & EntityFlag_Selected);
 return Result;
}

internal f32
GetCurveTrackedPointRadius(curve *Curve)
{
 f32 Result = 1.5f * Curve->Params.LineWidth;
 return Result;
}

internal void
AddVisibleCubicBezierPoint(visible_cubic_bezier_points *Visible, cubic_bezier_point_index Index)
{
 Assert(Visible->Count < ArrayCount(Visible->Indices));
 Visible->Indices[Visible->Count] = Index;
 ++Visible->Count;
}

inline internal b32
PrevCubicBezierPoint(curve *Curve, cubic_bezier_point_index *Index)
{
 b32 Result = false;
 if (Index->Index > 0)
 {
  --Index->Index;
  Result = true;
 }
 return Result;
}

inline internal b32
NextCubicBezierPoint(curve *Curve, cubic_bezier_point_index *Index)
{
 b32 Result = false;
 if (Index->Index + 1 < 3 * Curve->ControlPointCount)
 {
  ++Index->Index;
  Result = true;
 }
 return Result;
}

internal visible_cubic_bezier_points
GetVisibleCubicBezierPoints(entity *Entity)
{
 visible_cubic_bezier_points Result = {};
 
 curve *Curve = SafeGetCurve(Entity);
 if (IsEntitySelected(Entity) &&
     IsControlPointSelected(Curve) &&
     Curve->Params.Type == Curve_Bezier &&
     Curve->Params.Bezier == Bezier_Cubic)
 {
  cubic_bezier_point_index StartIndex = CubicBezierPointIndexFromControlPointIndex(Curve->SelectedIndex);
  
  cubic_bezier_point_index PrevIndex = StartIndex;
  if (PrevCubicBezierPoint(Curve, &PrevIndex))
  {
   AddVisibleCubicBezierPoint(&Result, PrevIndex);
   if (PrevCubicBezierPoint(Curve, &PrevIndex))
   {
    AddVisibleCubicBezierPoint(&Result, PrevIndex);
   }
  }
  
  cubic_bezier_point_index NextIndex = StartIndex;
  if (NextCubicBezierPoint(Curve, &NextIndex))
  {
   AddVisibleCubicBezierPoint(&Result, NextIndex);
   if (NextCubicBezierPoint(Curve, &NextIndex))
   {
    AddVisibleCubicBezierPoint(&Result, NextIndex);
   }
  }
 }
 
 return Result;
}

internal b32
IsCurveEligibleForPointTracking(curve *Curve)
{
 curve_params *Params = &Curve->Params;
 curve_type Interpolation = Params->Type;
 bezier_type BezierVariant = Params->Bezier;
 b32 Result = (Interpolation == Curve_Bezier && BezierVariant == Bezier_Regular);
 
 return Result;
}

internal b32
CurveHasWeights(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_Bezier &&
               Curve->Params.Bezier == Bezier_Regular);
 return Result;
}

internal b32
IsCurveTotalSamplesMode(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_B_Spline: {}break;
  
  case Curve_Parametric: {
   Result = true;
  }break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

inline internal b32
IsCurve(entity *Entity)
{
 b32 Result = (Entity->Type == Entity_Curve);
 return Result;
}

internal b32
IsCurveReversed(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 b32 Result = (UsesControlPoints(Curve) && (Entity->Flags & EntityFlag_CurveAppendFront));
 return Result;
}

internal b32
IsRegularBezierCurve(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_Bezier && Curve->Params.Bezier == Bezier_Regular);
 return Result;
}

internal b32
Are_B_SplineKnotsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_B_Spline && Curve->Params.B_Spline.ShowPartitionKnotPoints);
 return Result;
}

internal curve_merge_compatibility
AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method)
{
 b32 Compatible = false;
 string WhyIncompatible = NilStr;
 
 curve_params *Params0 = &Curve0->Params;
 curve_params *Params1 = &Curve1->Params;
 
 string IncompatibleTypes = StrLit("cannot merge curves of different types");
 
#define Concat_C0_G1_Str " can only be merged using [Concat], [C0] or [G1] methods"
#define Concat_Str       " can only be merged using [Concat] method"
 
 b32 Concat_C0_G1 = (Method == CurveMerge_Concat ||
                     Method == CurveMerge_C0 ||
                     Method == CurveMerge_G1);
 b32 Concat = (Method == CurveMerge_Concat);
 
 if (Params0->Type == Params1->Type)
 {
  switch (Params0->Type)
  {
   case Curve_Polynomial: {
    if (Concat_C0_G1) Compatible = true;
    else WhyIncompatible = StrLit("polynomial curves" Concat_C0_G1_Str);
   }break;
   
   case Curve_CubicSpline: {
    if (Params0->CubicSpline == Params1->CubicSpline)
    {
     if (Concat_C0_G1) Compatible = true;
     else WhyIncompatible = StrLit("cubic spline curves" Concat_C0_G1_Str);
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_Bezier: {
    if (Params0->Bezier == Params1->Bezier)
    {
     switch (Params0->Bezier)
     {
      case Bezier_Regular: {Compatible = true;}break;
      case Bezier_Cubic: {
       if (Concat_C0_G1) Compatible = true;
       else WhyIncompatible = StrLit("cubic bezier curves" Concat_C0_G1_Str);
      }break;
      case Bezier_Count: InvalidPath;
     }
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_B_Spline: {
    if (Concat) Compatible = true;
    else WhyIncompatible = StrLit("B-spline curves" Concat_Str);
   }break;
   
   case Curve_Parametric: {WhyIncompatible = StrLit("parametric curves cannot be merged");}break;
   
   case Curve_Count: InvalidPath;
  }
 }
 else
 {
  WhyIncompatible = IncompatibleTypes;
 }
 
 curve_merge_compatibility Result = {};
 Result.Compatible = Compatible;
 Result.WhyIncompatible = WhyIncompatible;
 
 return Result;
}

internal entity_colors
ExtractEntityColors(entity *Entity)
{
 entity_colors Colors = {};
 
 curve_params *Params = &Entity->Curve.Params;
 Colors.CurveLineColor = Params->LineColor;
 Colors.CurvePointColor = Params->PointColor;
 Colors.CurvePolylineColor = Params->PolylineColor;
 Colors.CurveConvexHullColor = Params->ConvexHullColor;
 
 return Colors;
}

internal void
ApplyColorsToEntity(entity *Entity, entity_colors Colors)
{
 curve_params *Params = &Entity->Curve.Params;
 Params->LineColor = Colors.CurveLineColor;
 Params->PointColor = Colors.CurvePointColor;
 Params->PolylineColor = Colors.CurvePolylineColor;
 Params->ConvexHullColor = Colors.CurveConvexHullColor;
}

internal point_info
GetEntityPointInfo(entity *Entity, f32 BaseRadius, v4 BaseColor)
{
 point_info Result = {};
 
 Result.Radius = BaseRadius;
 
 f32 OutlineScale = 0.0f;
 if (IsEntitySelected(Entity))
 {
  OutlineScale = 0.55f;
 }
 Result.OutlineThickness = OutlineScale * Result.Radius;
 
 Result.Color = BaseColor;
 Result.OutlineColor = BrightenColor(Result.Color, 0.3f);
 
 return Result;
}

internal point_info
GetCurveControlPointInfo(entity *Entity, u32 PointIndex)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 
 point_info Result = GetEntityPointInfo(Entity, Params->PointRadius, Params->PointColor);
 
 if (( (Entity->Flags & EntityFlag_CurveAppendFront) && PointIndex == 0) ||
     (!(Entity->Flags & EntityFlag_CurveAppendFront) && PointIndex == Curve->ControlPointCount-1))
 {
  Result.Radius *= 1.5f;
 }
 
 if (PointIndex == Curve->SelectedIndex.Index)
 {
  Result.OutlineColor = BrightenColor(Result.OutlineColor, 0.5f);
 }
 
 return Result;
}

internal point_info
Get_B_SplineKnotPointInfo(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 b_spline_params *B_Spline = &Params->B_Spline;
 
 point_info Result = GetEntityPointInfo(Entity, B_Spline->KnotPointRadius, B_Spline->KnotPointColor);
 return Result;
}

internal f32
GetCurveCubicBezierPointRadius(curve *Curve)
{
 // TODO(hbr): Make bezier points smaller
 f32 Result = Curve->Params.PointRadius;
 return Result;
}

internal string
GetEntityName(entity *Entity)
{
 string Name = StrFromCharBuffer(Entity->NameBuffer);
 return Name;
}

internal void
ExtractControlPointIndexAndBezierOffsetFromCurvePointIndex(curve_point_index Index,
                                                           control_point_index *ControlPointIndex,
                                                           u32 *BezierOffset)
{
 switch (Index.Type)
 {
  case CurvePoint_ControlPoint: {
   ControlPointIndex->Index = Index.ControlPoint.Index;
   *BezierOffset = 1;
  }break;
  case CurvePoint_CubicBezierPoint: {
   ControlPointIndex->Index = Index.BezierPoint.Index / 3;
   *BezierOffset = Index.BezierPoint.Index % 3;
  }break;
 }
 Assert(*BezierOffset < 3);
}

internal void
SetCurvePoint(entity_with_modify_witness *EntityWitness, curve_point_index Index, v2 P, translate_curve_point_flags Flags)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 control_point_index ControlPointIndex;
 u32 BezierOffset;
 ExtractControlPointIndexAndBezierOffsetFromCurvePointIndex(Index, &ControlPointIndex, &BezierOffset);
 
 if (ControlPointIndex.Index < Curve->ControlPointCount)
 {
  cubic_bezier_point *B = Curve->CubicBezierPoints + ControlPointIndex.Index;
  v2 *TranslatePoint = B->Ps + BezierOffset;
  v2 LocalP = WorldToLocalEntityPosition(Entity, P);
  
  if (BezierOffset == 1)
  {
   v2 LocalTranslation = LocalP - B->P1;
   
   B->P0 += LocalTranslation;
   B->P2 += LocalTranslation;
   
   Curve->ControlPoints[ControlPointIndex.Index] += LocalTranslation;
  }
  else
  {
   Assert(BezierOffset == 0 || BezierOffset == 2);
   v2 *Twin = B->Ps + (2 - BezierOffset);
   
   v2 DesiredTwinDirection;
   if (Flags & TranslateCurvePoint_MatchBezierTwinDirection)
   {
    DesiredTwinDirection = B->P1 - LocalP;
   }
   else
   {
    DesiredTwinDirection = *Twin - B->P1;
   }
   Normalize(&DesiredTwinDirection);
   
   v2 DesiredLengthVector;
   if (Flags & TranslateCurvePoint_MatchBezierTwinLength)
   {
    DesiredLengthVector = B->P1 - LocalP;
   }
   else
   {
    DesiredLengthVector = *Twin - B->P1;
   }
   f32 DesiredTwinLength = Norm(DesiredLengthVector);
   
   *Twin = B->P1 + DesiredTwinLength * DesiredTwinDirection;
  }
  
  *TranslatePoint = LocalP;
 }
 
 MarkEntityModified(EntityWitness);
}

internal void
SelectControlPoint(curve *Curve, control_point_index Index)
{
 if (Curve)
 {
  Curve->SelectedIndex = Index;
 }
}

internal void
SelectControlPointFromCurvePointIndex(curve *Curve, curve_point_index Index)
{
 if (Curve)
 {
  control_point_index ControlPointIndex;
  u32 BezierOffset;
  ExtractControlPointIndexAndBezierOffsetFromCurvePointIndex(Index, &ControlPointIndex, &BezierOffset);
  Curve->SelectedIndex = ControlPointIndex;
 }
}

internal void
RecomputeCurveB_SplineKnots(curve *Curve)
{
 u32 ControlPointCount = Curve->ControlPointCount;
 curve_params *CurveParams = &Curve->Params;
 b_spline_params *B_SplineParams = &CurveParams->B_Spline;
 f32 *Knots = Curve->B_SplineKnots;
 
 b_spline_degree_bounds DegreeBounds = B_SplineDegreeBounds(ControlPointCount);
 u32 Degree = Clamp(Curve->B_SplineKnotParams.Degree, DegreeBounds.MinDegree, DegreeBounds.MaxDegree);
 b_spline_knot_params KnotParams = B_SplineKnotsParamsFromDegree(Degree, ControlPointCount);
 
 Assert(KnotParams.KnotCount <= MAX_B_SPLINE_KNOT_COUNT);
 B_SplineBaseKnots(KnotParams, Knots);
 switch (B_SplineParams->Partition)
 {
  case B_SplinePartition_Natural: {B_SplineKnotsNaturalExtension(KnotParams, Knots);}break;
  case B_SplinePartition_Periodic: {B_SplineKnotsPeriodicExtension(KnotParams, Knots);}break;
  case B_SplinePartition_Count: InvalidPath;
 }
 
 Curve->B_SplineKnotParams = KnotParams;
}

internal void
RemoveControlPoint(entity_with_modify_witness *EntityWitness, control_point_index Index)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 if (Index.Index < Curve->ControlPointCount)
 {
#define ArrayRemoveOrdered(Array, At, Count) \
MemoryMove((Array) + (At), \
(Array) + (At) + 1, \
((Count) - (At) - 1) * SizeOf((Array)[0]))
  ArrayRemoveOrdered(Curve->ControlPoints,       Index.Index, Curve->ControlPointCount);
  ArrayRemoveOrdered(Curve->ControlPointWeights, Index.Index, Curve->ControlPointCount);
  ArrayRemoveOrdered(Curve->CubicBezierPoints,   Index.Index, Curve->ControlPointCount);
#undef ArrayRemoveOrdered
  
  --Curve->ControlPointCount;
  
  // NOTE(hbr): Maybe fix selected control point
  if (IsControlPointSelected(Curve))
  {
   if (Index.Index == Curve->SelectedIndex.Index)
   {
    UnselectControlPoint(Curve);
   }
   else if (Index.Index < Curve->SelectedIndex.Index)
   {
    control_point_index Fixed = MakeControlPointIndex(Curve->SelectedIndex.Index - 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
UnselectControlPoint(curve *Curve)
{
 if (Curve)
 {
  Curve->SelectedIndex.Index = U32_MAX;
 }
}

internal b32
IsControlPointSelected(curve *Curve)
{
 b32 Result = false;
 if (Curve)
 {
  Result = (Curve->SelectedIndex.Index < Curve->ControlPointCount);
 }
 return Result;
}

internal control_point_index
AppendControlPoint(entity_with_modify_witness *EntityWitness, v2 Point)
{
 control_point_index Result = {};
 
 entity *Entity = EntityWitness->Entity;
 if (Entity->Flags & EntityFlag_CurveAppendFront)
 {
  Result.Index = 0;
 }
 else
 {
  Result.Index = Entity->Curve.ControlPointCount;
 }
 InsertControlPoint(EntityWitness, Point, Result.Index);
 
 return Result;
}

internal void
InsertControlPoint(entity_with_modify_witness *EntityWitness, v2 Point, u32 At)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 if (Curve &&
     Curve->ControlPointCount < MAX_CONTROL_POINT_COUNT &&
     At <= Curve->ControlPointCount)
 {
  Assert(UsesControlPoints(Curve));
  
  u32 N = Curve->ControlPointCount;
  v2 *P = Curve->ControlPoints;
  f32 *W = Curve->ControlPointWeights;
  cubic_bezier_point *B = Curve->CubicBezierPoints;
  
#define ShiftRightArray(Array, ArrayLength, At, ShiftCount) \
MemoryMove((Array) + ((At)+(ShiftCount)), \
(Array) + (At), \
((ArrayLength) - (At)) * SizeOf((Array)[0]))
  ShiftRightArray(P, N, At, 1);
  ShiftRightArray(W, N, At, 1);
  ShiftRightArray(B, N, At, 1);
#undef ShiftRightArray
  
  P[At] = WorldToLocalEntityPosition(Entity, Point);
  W[At] = 1.0f;
  
  // NOTE(hbr): Cubic bezier point calculation is not really defined
  // for N<=2. At least I tried to "define" it but failed to do something
  // that was super useful. In that case, when the third point is added,
  // just recalculate all bezier points.
  if (N == 2)
  {
   BezierCubicCalculateAllControlPoints(N+1, P, B);
  }
  else
  {
   CalculateBezierCubicPointAt(N+1, P, B, At);
  }
  
  if (IsControlPointSelected(Curve) && Curve->SelectedIndex.Index >= At)
  {
   Curve->SelectedIndex.Index = Curve->SelectedIndex.Index + 1;
  }
  
  ++Curve->ControlPointCount;
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
SetCurveControlPoints(entity_with_modify_witness *EntityWitness,
                      u32 PointCount,
                      v2 *Points,
                      f32 *Weights,
                      cubic_bezier_point *CubicBeziers)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 Assert(PointCount <= MAX_CONTROL_POINT_COUNT);
 PointCount = Min(PointCount, MAX_CONTROL_POINT_COUNT);
 
 ArrayCopy(Curve->ControlPoints, Points, PointCount);
 ArrayCopy(Curve->ControlPointWeights, Weights, PointCount);
 ArrayCopy(Curve->CubicBezierPoints, CubicBeziers, PointCount);
 
 Curve->ControlPointCount = PointCount;
 // TODO(hbr): Make sure this is expected, maybe add to internal arguments
 UnselectControlPoint(Curve);
 
 MarkEntityModified(EntityWitness);
}

// TODO(hbr): Inline this
internal void
SetEntityName(entity *Entity, string Name)
{
 FillCharBuffer(&Entity->NameBuffer, Name);
}

#if 0
internal void
InitEntityPart(entity *Entity, v2 P, string Name)
{
 Entity->P = P;
 Entity->Scale = V2(1, 1);
 Entity->Rotation = Rotation2DZero();
 SetEntityName(Entity, Name);
}

internal void
InitCurveEntity2(entity *Entity, string Name, curve_params Params)
{
 Assert(Entity->Type == Entity_Curve);
 InitEntityPart(Entity, V2(0, 0), Name);
 curve *Curve = &Entity->Curve;
 Curve->Params = Params;
}

internal void
InitCurveEntity(entity *Entity, curve_params Params)
{
 Assert(Entity->Type == Entity_Curve);
 
 curve *Curve = &Entity->Curve;
 Curve->Params = Params;
}

#endif

internal void
InitEntityPart(entity *Entity,
               v2 P, v2 Scale, v2 Rotation,
               string Name,
               i32 SortingLayer,
               entity_flags Flags)
{
 Entity->P = P;
 Entity->Scale = Scale;
 Entity->Rotation = Rotation;
 SetEntityName(Entity, Name);
 Entity->SortingLayer = SortingLayer;
 Entity->Flags = Flags;
}

internal void
InitEntityAsCurve(entity *Entity, string Name, curve_params CurveParams)
{
 InitEntityPart(Entity,
                V2(0, 0), V2(1, 1), Rotation2DZero(),
                Name, 0, 0);
 curve *Curve = SafeGetCurve(Entity);
 Curve->Params = CurveParams;
}

internal void
InitEntityAsImage(entity *Entity, v2 P, u32 Width, u32 Height, string FilePath)
{
 string FileName = PathLastPart(FilePath);
 string FileNameNoExt = StrChopLastDot(FileName);
 InitEntityPart(Entity,
                P, V2(1, 1), Rotation2DZero(),
                FileNameNoExt, 0, 0);
 image *Image = SafeGetImage(Entity);
 Image->Dim = V2(Cast(f32)Width / Height, 1.0f);
}

internal void
SetCurveControlPoint(entity_with_modify_witness *EntityWitness, control_point_index Index, v2 P, f32 Weight)
{
 curve_point_index CurvePointIndex = CurvePointIndexFromControlPointIndex(Index);
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 SetCurvePoint(EntityWitness, CurvePointIndex, P, 0);
 
 if (Index.Index < Curve->ControlPointCount)
 {
  Curve->ControlPointWeights[Index.Index] = Weight;
  MarkEntityModified(EntityWitness);
 }
}

internal curve_points_modify_handle
BeginModifyCurvePoints(entity_with_modify_witness *EntityWitness, u32 RequestedPointCount, modify_curve_points_which_points Which)
{
 curve_points_modify_handle Result = {};
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 u32 ActualPointCount = Min(RequestedPointCount, MAX_CONTROL_POINT_COUNT);
 Result.Curve = Curve;
 Result.PointCount = ActualPointCount;
 Result.ControlPoints = Curve->ControlPoints;
 Result.Weights = Curve->ControlPointWeights;
 Result.CubicBeziers = Curve->CubicBezierPoints;
 Result.Which = Which;
 
 // NOTE(hbr): Assume (which is very reasonable) that the curve is always changing here
 MarkEntityModified(EntityWitness);
 
 return Result;
}

internal void
EndModifyCurvePoints(curve_points_modify_handle Handle)
{
 curve *Curve = Handle.Curve;
 
 Curve->ControlPointCount = Handle.PointCount;
 
 if (Handle.Which <= ModifyCurvePointsWhichPoints_JustControlPoints)
 {
  for (u32 PointIndex = 0;
       PointIndex < Handle.PointCount;
       ++PointIndex)
  {
   Handle.Weights[PointIndex] = 1.0f;
  }
 }
 
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsWithWeights)
 {
  BezierCubicCalculateAllControlPoints(Handle.PointCount,
                                       Handle.ControlPoints,
                                       Handle.CubicBeziers);
 }
 
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers)
 {
  // NOTE(hbr): Nothing to do
 }
}

internal v2
WorldToLocalEntityPosition(entity *Entity, v2 P)
{
 v2 Result = RotateAround(P - Entity->P, V2(0, 0), Rotation2DInverse(Entity->Rotation));
 Result = Hadamard(V2(1.0f / Entity->Scale.X, 1.0f / Entity->Scale.Y), Result);
 return Result;
}

internal v2
LocalEntityPositionToWorld(entity *Entity, v2 P)
{
 // TODO(hbr): At some point at realized that the code line below was not there for a long
 // time. Investigate whether it shoould be there or not ????
 P = Hadamard(P, Entity->Scale);
 v2 Result = RotateAround(P, V2(0.0f, 0.0f), Entity->Rotation) + Entity->P;
 return Result;
}

internal void
SetTrackingPointFraction(entity_with_modify_witness *EntityWitness, f32 Fraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 
 Tracking->Fraction = Clamp01(Fraction);
 MarkEntityModified(EntityWitness);
}

// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->Params.Type == Curve_CubicSpline &&
                 Curve->Params.CubicSpline == CubicSpline_Periodic);
 return IsLooped;
}

struct points_soa
{
 f32 *Xs;
 f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2 *Points, u32 PointCount)
{
 points_soa Result = {};
 Result.Xs = PushArrayNonZero(Arena, PointCount, f32);
 Result.Ys = PushArrayNonZero(Arena, PointCount, f32);
 for (u32 I = 0; I < PointCount; ++I)
 {
  Result.Xs[I] = Points[I].X;
  Result.Ys[I] = Points[I].Y;
 }
 
 return Result;
}

internal void
CalcNormalBezierLinePoints(v2 *ControlPoints,
                           u32 ControlPointCount,
                           u32 OutputLinePointCount,
                           v2 *OutputLinePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (OutputLinePointCount - 1);
 for (u32 OutputIndex = 0;
      OutputIndex < OutputLinePointCount;
      ++OutputIndex)
 {
  
  OutputLinePoints[OutputIndex] = BezierCurveEvaluate(T,
                                                      ControlPoints,
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
CalcBarycentricPolynomial(v2 *Controls, u32 PointCount, point_spacing PointSpacing, f32 *Ti,
                          u32 SampleCount, v2 *OutSamples,
                          f32 Min_T, f32 Max_T)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  f32 *Omega = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (PointSpacing)
  {
   case PointSpacing_Equidistant: {
    // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
    BarycentricOmega(Omega, Ti, PointCount);
   } break;
   case PointSpacing_Chebychev: {
    BarycentricOmegaChebychev(Omega, PointCount);
   } break;
   
   case PointSpacing_Count: InvalidPath;
  }
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 T = Min_T;
  f32 Delta_T = (Max_T - Min_T) / (SampleCount - 1);
  
  for (u32 SampleIndex = 0;
       SampleIndex < SampleCount;
       ++SampleIndex)
  {
   f32 X = BarycentricEvaluate(T, Omega, Ti, SOA.Xs, PointCount);
   f32 Y = BarycentricEvaluate(T, Omega, Ti, SOA.Ys, PointCount);
   OutSamples[SampleIndex] = V2(X, Y);
   
   T += Delta_T;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcNewtonPolynomial(v2 *Controls, u32 PointCount, f32 *Ti,
                     u32 SampleCount, v2 *OutSamples,
                     f32 Min_T, f32 Max_T)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Beta_X = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *Beta_Y = PushArrayNonZero(Temp.Arena, PointCount, f32);
  NewtonBetaFast(Beta_X, Ti, SOA.Xs, PointCount);
  NewtonBetaFast(Beta_Y, Ti, SOA.Ys, PointCount);
  
  f32 T = Min_T;
  f32 Delta_T = (Max_T - Min_T) / (SampleCount - 1);
  
  for (u32 SampleIndex = 0;
       SampleIndex < SampleCount;
       ++SampleIndex)
  {
   f32 X = NewtonEvaluate(T, Beta_X, Ti, PointCount);
   f32 Y = NewtonEvaluate(T, Beta_Y, Ti, PointCount);
   OutSamples[SampleIndex] = V2(X, Y);
   
   T += Delta_T;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcPolynomial(v2 *Controls, u32 PointCount,
               polynomial_interpolation_params Polynomial,
               u32 SampleCount, v2 *OutSamples)
{
 temp_arena Temp = TempArena(0);
 
 point_spacing PointSpacing = Polynomial.PointSpacing;
 
 f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
 switch (PointSpacing)
 {
  case PointSpacing_Equidistant: { EquidistantPoints(Ti, PointCount, 0.0f, 1.0f); } break;
  case PointSpacing_Chebychev:   { ChebyshevPoints(Ti, PointCount);   } break;
  case PointSpacing_Count: InvalidPath;
 }
 
 f32 Min_T = 0;
 f32 Max_T = 0;
 if (PointCount > 0)
 {
  Min_T = Ti[0];
  Max_T = Ti[PointCount - 1];
 }
 
 switch (Polynomial.Type)
 {
  case PolynomialInterpolation_Barycentric: {CalcBarycentricPolynomial(Controls, PointCount, PointSpacing, Ti, SampleCount, OutSamples, Min_T, Max_T);}break;
  case PolynomialInterpolation_Newton:      {CalcNewtonPolynomial(Controls, PointCount, Ti, SampleCount, OutSamples, Min_T, Max_T);}break;
  case PolynomialInterpolation_Count: InvalidPath;
 }
 
 EndTemp(Temp);
}

internal void
CalcCubicSpline(v2 *Controls, u32 PointCount,
                cubic_spline_type Spline,
                u32 SampleCount, v2 *OutSamples)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  if (Spline == CubicSpline_Periodic)
  {
   v2 *OriginalControls = Controls;
   
   Controls = PushArrayNonZero(Temp.Arena, PointCount + 1, v2);
   ArrayCopy(Controls, OriginalControls, PointCount);
   Controls[PointCount] = OriginalControls[0];
   
   ++PointCount;
  }
  
  f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
  EquidistantPoints(Ti, PointCount, 0.0f, 1.0f);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Mx = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *My = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (Spline)
  {
   case CubicSpline_Natural: {
    CubicSplineNaturalM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplineNaturalM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Periodic: {
    CubicSplinePeriodicM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplinePeriodicM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Count: InvalidPath;
  }
  
  f32 Begin = Ti[0];
  f32 End = Ti[PointCount - 1];
  f32 T = Begin;
  f32 Delta = (End - Begin) / (SampleCount - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < SampleCount;
       ++OutputIndex)
  {
   
   f32 X = CubicSplineEvaluate(T, Mx, Ti, SOA.Xs, PointCount);
   f32 Y = CubicSplineEvaluate(T, My, Ti, SOA.Ys, PointCount);
   OutSamples[OutputIndex] = V2(X, Y);
   
   T += Delta;
  }
  
  EndTemp(Temp);
 }
}

internal void
CalcRegularBezier(v2 *Controls, f32 *Weights, u32 PointCount,
                  u32 SampleCount, v2 *OutSamples)
{
 f32 T = 0.0f;
 f32 Delta_T = 1.0f / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  OutSamples[SampleIndex] = BezierCurveEvaluateWeighted(T, Controls, Weights, PointCount);
  T += Delta_T;
 }
}

internal void
CalcCubicBezier(cubic_bezier_point *Beziers, u32 PointCount,
                u32 SampleCount, v2 *OutSamples)
{
 if (PointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta_T = 1.0f / (SampleCount - 1);
  
  for (u32 SampleIndex = 0;
       SampleIndex < SampleCount;
       ++SampleIndex)
  {
   f32 Expanded_T = (PointCount - 1) * T;
   u32 SegmentIndex = Cast(u32)Expanded_T;
   f32 Segment_T = Expanded_T - SegmentIndex;
   
   Assert(SegmentIndex < PointCount);
   Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
   
   cubic_bezier_curve_segment Segment = GetCubicBezierCurveSegment(Beziers, PointCount, SegmentIndex);
   OutSamples[SampleIndex] = BezierCurveEvaluate(Segment_T, Segment.Points, Segment.PointCount);
   
   T += Delta_T;
  }
 }
}

internal void
Calc_B_Spline(v2 *Controls, u32 PointCount,
              b_spline_knot_params KnotParams, f32 *Knots,
              u32 SampleCount, v2 *OutSamples)
{
 temp_arena Temp = TempArena(0);
 
 f32 T = KnotParams.A;
 f32 Delta_T = (KnotParams.B - KnotParams.A) / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  OutSamples[SampleIndex] = B_SplineEvaluate(T, Controls, KnotParams, Knots);
  T += Delta_T;
 }
 
 EndTemp(Temp);
}

internal void
CalcParametric(parametric_curve_params Parametric,
               u32 SampleCount, v2 *OutSamples)
{
 f32 MinT = Parametric.MinT;
 f32 MaxT = Max(Parametric.MinT, Parametric.MaxT);
 Assert(MinT <= MaxT);
 
 parametric_equation_expr *X_Equation = Parametric.X_Equation;
 parametric_equation_expr *Y_Equation = Parametric.Y_Equation;
 
 f32 T = MinT;
 f32 DeltaT = (MaxT - MinT) / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex	< SampleCount;
      ++SampleIndex)
 {
  f32 X = ParametricEquationEvalWithT(X_Equation, T);
  f32 Y = ParametricEquationEvalWithT(Y_Equation, T);
  
  v2 Point = V2(X, Y);
  OutSamples[SampleIndex] = Point;
  
  T += DeltaT;
 }
}

internal void
CalcCurve(arena *EntityArena, curve *Curve, u32 SampleCount, v2 *OutSamples)
{
 temp_arena Temp = TempArena(EntityArena);
 
 u32 PointCount = Curve->ControlPointCount;
 v2 *Controls = Curve->ControlPoints;
 f32 *Weights = Curve->ControlPointWeights;
 cubic_bezier_point *Beziers = Curve->CubicBezierPoints;
 curve_params *Params = &Curve->Params;
 f32 *B_SplineKnots = Curve->B_SplineKnots;
 
 switch (Params->Type)
 {
  case Curve_Polynomial: {CalcPolynomial(Controls, PointCount, Params->Polynomial, SampleCount, OutSamples);} break;
  case Curve_CubicSpline: {CalcCubicSpline(Controls, PointCount, Params->CubicSpline, SampleCount, OutSamples);} break;
  
  case Curve_Bezier: {
   switch (Params->Bezier)
   {
    case Bezier_Regular: {CalcRegularBezier(Controls, Weights, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Cubic: {CalcCubicBezier(Beziers, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Count: InvalidPath;
   }
  } break;
  
  case Curve_B_Spline: {
   RecomputeCurveB_SplineKnots(Curve);
   
   b_spline_knot_params KnotParams = Curve->B_SplineKnotParams;
   u32 PartitionSize = KnotParams.PartitionSize;
   u32 Degree = KnotParams.Degree;
   
   v2 *PartitionKnotPoints = PushArrayNonZero(EntityArena, PartitionSize, v2);
   
   for (u32 PartitionKnotIndex = 0;
        PartitionKnotIndex < PartitionSize;
        ++PartitionKnotIndex)
   {
    f32 T = B_SplineKnots[Degree + PartitionKnotIndex];
    v2 KnotPoint = B_SplineEvaluate(T, Controls, KnotParams, B_SplineKnots);
    PartitionKnotPoints[PartitionKnotIndex] = KnotPoint;
   }
   Curve->B_SplinePartitionKnotPoints = PartitionKnotPoints;
   
   Calc_B_Spline(Controls, PointCount, KnotParams, B_SplineKnots, SampleCount, OutSamples);
  }break;
  case Curve_Parametric: {CalcParametric(Params->Parametric, SampleCount, OutSamples);}break;
  
  case Curve_Count: InvalidPath;
 }
 
 EndTemp(Temp);
}

internal void
RecomputeCurve(curve *Curve)
{
 ProfileFunctionBegin();
 
 curve_params *Params = &Curve->Params;
 arena *ComputeArena = Curve->ComputeArena;
 u32 PointCount = Curve->ControlPointCount;
 v2 *Controls = Curve->ControlPoints;
 f32 *Weights = Curve->ControlPointWeights;
 f32 LineWidth = Params->LineWidth;
 
 ClearArena(ComputeArena);
 
 u32 SampleCount = 0;
 if (IsCurveTotalSamplesMode(Curve))
 {
  SampleCount = Curve->Params.TotalSamples;
 }
 else
 {
  if (PointCount > 0)
  {
   SampleCount = (PointCount - 1) * Params->SamplesPerControlPoint + 1;
  }
 }
 v2 *Samples = PushArrayNonZero(ComputeArena, SampleCount, v2);
 CalcCurve(ComputeArena, Curve, SampleCount, Samples);
 
 vertex_array CurveVertices = ComputeVerticesOfThickLine(ComputeArena, SampleCount, Samples, LineWidth, IsCurveLooped(Curve));
 vertex_array PolylineVertices = ComputeVerticesOfThickLine(ComputeArena, PointCount, Controls, Params->PolylineWidth, false);
 
 v2 *ConvexHullPoints = PushArrayNonZero(ComputeArena, PointCount, v2);
 u32 ConvexHullCount = CalcConvexHull(PointCount, Controls, ConvexHullPoints);
 vertex_array ConvexHullVertices = ComputeVerticesOfThickLine(ComputeArena, ConvexHullCount, ConvexHullPoints, Params->ConvexHullWidth, true);
 
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 if (IsCurveEligibleForPointTracking(Curve))
 {
  if (Tracking->Active)
  {
   f32 Fraction = Tracking->Fraction;
   
   v2 LocalSpaceTrackedPoint = BezierCurveEvaluateWeighted(Fraction, Controls, Weights, PointCount);
   Tracking->LocalSpaceTrackedPoint = LocalSpaceTrackedPoint;
   
   if (Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization)
   {
    all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(ComputeArena, Fraction, Controls, Weights, PointCount);
    vertex_array *LineVerticesPerIteration = PushArray(ComputeArena, Intermediate.IterationCount, vertex_array);
    
    u32 IterationPointsOffset = 0;
    for (u32 Iteration = 0;
         Iteration < Intermediate.IterationCount;
         ++Iteration)
    {
     u32 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
     LineVerticesPerIteration[Iteration] = ComputeVerticesOfThickLine(ComputeArena,
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
  }
 }
 else
 {
  Tracking->Active = false;
 }
 
 Curve->CurveSampleCount = SampleCount;
 Curve->CurveSamples = Samples;
 Curve->CurveVertices = CurveVertices;
 Curve->PolylineVertices = PolylineVertices;
 Curve->ConvexHullPoints = ConvexHullPoints;
 Curve->ConvexHullCount = ConvexHullCount;
 Curve->ConvexHullVertices = ConvexHullVertices;
 
 ProfileEnd();
}

internal void
MarkEntityModified(entity_with_modify_witness *Witness)
{
 Witness->Modified = true;
}

internal entity_with_modify_witness
BeginEntityModify(entity *Entity)
{
 entity_with_modify_witness Result = {};
 Result.Entity = Entity;
 return Result;
}

internal void
EndEntityModify(entity_with_modify_witness Witness)
{
 entity *Entity = Witness.Entity;
 if (Entity && Witness.Modified)
 {
  switch (Entity->Type)
  {
   case Entity_Curve: {RecomputeCurve(&Entity->Curve);}break;
   case Entity_Image: {}break;
   case Entity_Count: InvalidPath;
  }
  
  ++Entity->Version;
 }
}

internal b32
CanAddAdditionalVar(parametric_curve_resources *Resources)
{
 b32 Result = (Resources->FirstFreeAdditionalVar != 0 ||
               Resources->AllocatedAdditionalVarCount < MAX_ADDITIONAL_VAR_COUNT);
 return Result;
}

internal void
AddNewAdditionalVar(parametric_curve_resources *Resources)
{
 parametric_curve_var *Var = Resources->FirstFreeAdditionalVar;
 
 if (Var)
 {
  StackPop(Resources->FirstFreeAdditionalVar);
 }
 else if (Resources->AllocatedAdditionalVarCount < MAX_ADDITIONAL_VAR_COUNT)
 {
  Var = Resources->AdditionalVars + Resources->AllocatedAdditionalVarCount;
  Var->Id = Resources->AllocatedAdditionalVarCount;
  ++Resources->AllocatedAdditionalVarCount;
 }
 
 if (Var)
 {
  u32 Id = Var->Id;
  StructZero(Var);
  Var->Id = Id;
  
  DLLPushBack(Resources->AdditionalVarsHead,
              Resources->AdditionalVarsTail,
              Var);
 }
}

internal void
RemoveAdditionalVar(parametric_curve_resources *Resources, parametric_curve_var *Var)
{
 DLLRemove(Resources->AdditionalVarsHead,
           Resources->AdditionalVarsTail,
           Var);
 StackPush(Resources->FirstFreeAdditionalVar, Var);
}
