inline internal entity_snapshot_for_merging
MakeEntitySnapshotForMerging(entity *Entity)
{
 entity_snapshot_for_merging Result = {};
 Result.Entity = Entity;
 if (Entity)
 {
  Result.XForm = Entity->XForm;
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
     (Entity && (!StructsEqual(&Entity->XForm, &Versioned.XForm) ||
                 !StructsEqual(&Entity->Flags, &Versioned.Flags) ||
                 !StructsEqual(&Entity->Version, &Versioned.Version))))
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
EntityFromHandle(entity_handle Handle, b32 AllowDeactived)
{
 entity *Entity = 0;
 if (Handle.Entity &&
     (Handle.Generation == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 if (AllowDeactived &&
     (Entity == 0) &&
     (Handle.Entity->InternalFlags & EntityInternalFlag_Deactivated) &&
     (Handle.Generation + 1 == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 return Entity;
}

internal inline f32
GetCurvePartVisibilityZOffset(curve_part_visibility Part)
{
 Assert(Part < CurvePartVisibility_Count);
 f32 Result = Cast(f32)(CurvePartVisibility_Count-1 - Part) / CurvePartVisibility_Count;
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

inline internal control_point_handle
ControlPointHandleFromIndex(u32 Index)
{
 control_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal u32
IndexFromControlPointHandle(control_point_handle Handle)
{
 Assert(!ControlPointHandleMatch(Handle, ControlPointHandleZero()));
 u32 Result = Handle.Index - 1;
 return Result;
}

inline internal b32
ControlPointHandleMatch(control_point_handle A, control_point_handle B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}

inline internal control_point_handle
ControlPointHandleZero(void)
{
 control_point_handle Index = {};
 return Index;
}

inline internal control_point_handle
ControlPointFromCubicBezierPoint(cubic_bezier_point_handle Bezier)
{
 control_point_handle Result = {};
 if (!CubicBezierPointHandleMatch(Bezier, CubicBezierPointHandleZero()))
 {
  u32 BezierIndex = IndexFromCubicBezierPointHandle(Bezier);
  Result = ControlPointHandleFromIndex(BezierIndex / 3);
 }
 return Result;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleFromIndex(u32 Index)
{
 cubic_bezier_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal cubic_bezier_point_handle
CubicBezierPointFromControlPoint(control_point_handle Handle)
{
 cubic_bezier_point_handle Bezier = {};
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  u32 I = IndexFromControlPointHandle(Handle);
  Bezier = CubicBezierPointHandleFromIndex(3 * I + 1);
 }
 return Bezier;
}

inline internal u32
IndexFromCubicBezierPointHandle(cubic_bezier_point_handle Handle)
{
 Assert(!CubicBezierPointHandleMatch(Handle, CubicBezierPointHandleZero()));
 u32 Index = Handle.Index - 1;
 return Index;
}

inline internal b32
CubicBezierPointHandleMatch(cubic_bezier_point_handle A, cubic_bezier_point_handle B)
{
 b32 Equal = (A.Index == B.Index);
 return Equal;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleZero(void)
{
 cubic_bezier_point_handle Handle = {};
 return Handle;
}

inline internal curve_point_handle
CurvePointFromControlPoint(control_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_ControlPoint;
 Result.Control = Handle;
 return Result;
}

inline internal curve_point_handle
CurvePointFromCubicBezierPoint(cubic_bezier_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_CubicBezierPoint;
 Result.Bezier = Handle;
 return Result;
}

inline internal control_point_handle
ControlPointFromCurvePoint(curve_point_handle Handle)
{
 control_point_handle Result = {};
 switch (Handle.Type)
 {
  case CurvePoint_ControlPoint: {Result = Handle.Control;}break;
  case CurvePoint_CubicBezierPoint: {Result = ControlPointFromCubicBezierPoint(Handle.Bezier);}break;
 }
 return Result;
}

inline internal v2
GetCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromCubicBezierPointHandle(Point);
 if (Index < 3 * Curve->Points.ControlPointCount)
 {
  v2 *Beziers = Cast(v2 *)Curve->Points.CubicBezierPoints;
  Result = Beziers[Index];
 }
 return Result;
}

inline internal v2
GetControlPointP(curve *Curve, control_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromControlPointHandle(Point);
 if (Index < Curve->Points.ControlPointCount)
 {
  Result = Curve->Points.ControlPoints[Index];
 }
 return Result;
}

inline internal v2
GetCenterPointFromCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Bezier)
{
 control_point_handle Control = ControlPointFromCubicBezierPoint(Bezier);
 v2 Point = GetControlPointP(Curve, Control);
 return Point;
}

inline internal control_point_handle
ControlPointIndexFromCurveSampleIndex(curve *Curve, u32 CurveSampleIndex)
{
 Assert(!IsCurveTotalSamplesMode(Curve));
 u32 Index = SafeDiv0(CurveSampleIndex, Curve->Params.SamplesPerControlPoint);
 Assert(Index < Curve->Points.ControlPointCount);
 Index = ClampTop(Index, Curve->Points.ControlPointCount - 1);
 control_point_handle Control = ControlPointHandleFromIndex(Index);
 return Control;
}

internal b32
AreCurvePointsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.DrawParams.Points.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsPolylineVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.Polyline.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsConvexHullVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.ConvexHull.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsBSplineCurve(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->Type == Curve_BSpline);
 return Result;
}

internal b32
AreBSplineConvexHullsVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (IsBSplineCurve(Curve) && CurveParams->DrawParams.BSplinePartialConvexHull.Enabled);
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
  case Curve_BSpline: {Result = true;}break;
  
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
 f32 Result = 1.5f * Curve->Params.DrawParams.Line.Width;
 return Result;
}

internal void
AddVisibleCubicBezierPoint(visible_cubic_bezier_points *Visible, cubic_bezier_point_handle Point)
{
 Assert(Visible->Count < ArrayCount(Visible->Indices));
 Visible->Indices[Visible->Count] = Point;
 ++Visible->Count;
}

inline internal b32
PrevCubicBezierPoint(cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 if (Index > 0)
 {
  *Point = CubicBezierPointHandleFromIndex(Index - 1);
  Result = true;
 }
 return Result;
}

inline internal b32
NextCubicBezierPoint(curve *Curve, cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 if (Index + 1 < 3 * Curve->Points.ControlPointCount)
 {
  *Point = CubicBezierPointHandleFromIndex(Index + 1);
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
  cubic_bezier_point_handle StartPoint = CubicBezierPointFromControlPoint(Curve->SelectedControlPoint);
  
  cubic_bezier_point_handle PrevPoint = StartPoint;
  if (PrevCubicBezierPoint(&PrevPoint))
  {
   AddVisibleCubicBezierPoint(&Result, PrevPoint);
   if (PrevCubicBezierPoint(&PrevPoint))
   {
    AddVisibleCubicBezierPoint(&Result, PrevPoint);
   }
  }
  
  cubic_bezier_point_handle NextPoint = StartPoint;
  if (NextCubicBezierPoint(Curve, &NextPoint))
  {
   AddVisibleCubicBezierPoint(&Result, NextPoint);
   if (NextCubicBezierPoint(Curve, &NextPoint))
   {
    AddVisibleCubicBezierPoint(&Result, NextPoint);
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
  case Curve_BSpline: {}break;
  
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
AreBSplineKnotsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_BSpline && Curve->Params.DrawParams.BSplineKnots.Enabled);
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
   
   case Curve_BSpline: {
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

internal void
MarkEntitySelected(entity *Entity)
{
 Entity->Flags |= EntityFlag_Selected;
}

internal void
MarkEntityDeselected(entity *Entity)
{
 Entity->Flags &= ~EntityFlag_Selected;
}

internal point_draw_info
GetEntityPointDrawInfo(entity *Entity, f32 BaseRadius, rgba BaseColor)
{
 point_draw_info Result = {};
 
 Result.Radius = BaseRadius;
 
 f32 OutlineScale = 0.0f;
 if (IsEntitySelected(Entity))
 {
  OutlineScale = 0.55f;
 }
 Result.OutlineThickness = OutlineScale * Result.Radius;
 
 Result.Color = BaseColor;
 Result.OutlineColor = RGBA_Brighten(Result.Color, 0.3f);
 
 return Result;
}

internal point_draw_info
GetCurveControlPointDrawInfo(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 u32 Index = IndexFromControlPointHandle(Point);
 
 point_draw_info Result = GetEntityPointDrawInfo(Entity, Params->DrawParams.Points.Radius, Params->DrawParams.Points.Color);
 
 if (( (Entity->Flags & EntityFlag_CurveAppendFront) && Index == 0) ||
     (!(Entity->Flags & EntityFlag_CurveAppendFront) && Index == Curve->Points.ControlPointCount-1))
 {
  Result.Radius *= 2.0f;
 }
 
 if (ControlPointHandleMatch(Point, Curve->SelectedControlPoint))
 {
  //Result.OutlineColor = RGBA_Brighten(Result.OutlineColor, 0.5f);
  
  Result.Color = RGBA_Opposite(Result.Color);
  Result.OutlineColor = RGBA_Brighten(Result.Color, 0.5f);
 }
 
 return Result;
}

internal point_draw_info
GetBSplinePartitionKnotPointDrawInfo(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 draw_params *BSplineKnots = &Params->DrawParams.BSplineKnots;
 point_draw_info Result = GetEntityPointDrawInfo(Entity, BSplineKnots->Radius, BSplineKnots->Color);
 return Result;
}

internal f32
GetCurveCubicBezierPointRadius(curve *Curve)
{
 // TODO(hbr): Make bezier points smaller
 f32 Result = Curve->Params.DrawParams.Points.Radius;
 return Result;
}

internal string
GetEntityName(entity *Entity)
{
 string Name = StrFromCharBuffer(Entity->NameBuffer);
 return Name;
}

internal control_point
MakeControlPoint(v2 Point)
{
 control_point Result = {};
 Result.P = Point;
 return Result;
}

internal control_point
MakeControlPoint(v2 Point, f32 Weight, cubic_bezier_point Bezier)
{
 control_point Result = {};
 Result.P = Point;
 Result.Weight = Weight;
 Result.Bezier = Bezier;
 Result.IsWeight = Result.IsBezier = true;
 return Result;
}

internal curve_points_handle
MakeCurvePointsHandle(u32 ControlPointCount,
                      v2 *ControlPoints,
                      f32 *ControlPointWeights,
                      cubic_bezier_point *CubicBezierPoints)
{
 curve_points_handle Points = {};
 Points.ControlPointCount = ControlPointCount;
 Points.ControlPoints = ControlPoints;
 Points.ControlPointWeights = ControlPointWeights;
 Points.CubicBezierPoints = CubicBezierPoints;
 return Points;
}

internal curve_points_handle
CurvePointsHandleZero(void)
{
 curve_points_handle Handle = {};
 return Handle;
}

internal curve_points_handle
CurvePointsHandleFromCurvePointsStatic(curve_points_static *Static)
{
 curve_points_handle Handle = MakeCurvePointsHandle(Static->ControlPointCount,
                                                    Static->ControlPoints,
                                                    Static->ControlPointWeights,
                                                    Static->CubicBezierPoints);
 return Handle;
}

internal curve_points_dynamic
MakeCurvePointsDynamic(u32 *ControlPointCount,
                       v2 *ControlPoints,
                       f32 *ControlPointWeights,
                       cubic_bezier_point *CubicBezierPoints,
                       u32 Capacity)
{
 curve_points_dynamic Result = {};
 Result.ControlPointCount = ControlPointCount;
 Result.ControlPoints = ControlPoints;
 Result.ControlPointWeights = ControlPointWeights;
 Result.CubicBezierPoints = CubicBezierPoints;
 Result.Capacity = Capacity;
 return Result;
}

internal curve_points_dynamic
CurvePointsDynamicFromStatic(curve_points_static *Static)
{
 curve_points_dynamic Dynamic = MakeCurvePointsDynamic(&Static->ControlPointCount,
                                                       Static->ControlPoints,
                                                       Static->ControlPointWeights,
                                                       Static->CubicBezierPoints,
                                                       CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT);
 return Dynamic;
}

internal control_point
GetCurveControlPointInWorldSpace(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 
 v2 P = LocalToWorldEntityPosition(Entity, Curve->Points.ControlPoints[Index]);
 f32 Weight = Curve->Points.ControlPointWeights[Index];
 cubic_bezier_point B = Curve->Points.CubicBezierPoints[Index];
 cubic_bezier_point Bezier = {
  LocalToWorldEntityPosition(Entity, B.Ps[0]),
  LocalToWorldEntityPosition(Entity, B.Ps[1]),
  LocalToWorldEntityPosition(Entity, B.Ps[2]),
 };
 
 control_point Result = MakeControlPoint(P, Weight, Bezier);
 return Result;
}

internal void
CopyCurvePointsFromCurve(curve *Curve, curve_points_dynamic Dst)
{
 CopyCurvePoints(Dst, CurvePointsHandleFromCurvePointsStatic(&Curve->Points));
}

internal void
CopyCurvePoints(curve_points_dynamic Dst, curve_points_handle Src)
{
 u32 Copy = Min(Src.ControlPointCount, Dst.Capacity);
 *Dst.ControlPointCount = Copy;
 ArrayCopy(Dst.ControlPoints, Src.ControlPoints, Copy);
 ArrayCopy(Dst.ControlPointWeights, Src.ControlPointWeights, Copy);
 ArrayCopy(Dst.CubicBezierPoints, Src.CubicBezierPoints, Copy);
}

internal rect2
EntityAABB(entity *Entity)
{
 rect2 AABB = EmptyAABB();
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   u32 SampleCount = Curve->CurveSampleCount;
   v2 *Samples = Curve->CurveSamples;
   u32 SampleIndex = 0;
   while (SampleIndex + 8 < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex + 0]);
    AddPointAABB(&AABB, Samples[SampleIndex + 1]);
    AddPointAABB(&AABB, Samples[SampleIndex + 2]);
    AddPointAABB(&AABB, Samples[SampleIndex + 3]);
    AddPointAABB(&AABB, Samples[SampleIndex + 4]);
    AddPointAABB(&AABB, Samples[SampleIndex + 5]);
    AddPointAABB(&AABB, Samples[SampleIndex + 6]);
    AddPointAABB(&AABB, Samples[SampleIndex + 7]);
    SampleIndex += 8;
   }
   while (SampleIndex < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex]);
    ++SampleIndex;
   }
  } break;
  
  case Entity_Image: {
   image *Image = SafeGetImage(Entity);
   scale2d Dim = Image->Dim;
   AddPointAABB(&AABB, V2( Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2( Dim.V.X, -Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X, -Dim.V.Y));
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
 
 rect2_corners AABB_Corners = AABBCorners(AABB);
 rect2 AABB_Transformed = EmptyAABB();
 ForEachEnumVal(Corner, Corner_Count, corner)
 {
  v2 P = LocalToWorldEntityPosition(Entity, AABB_Corners.Corners[Corner]);
  AddPointAABB(&AABB_Transformed, P);
 }
 
 return AABB_Transformed;
}

internal b_spline_params
GetBSplineParams(curve *Curve)
{
 return Curve->ComputedBSplineParams;
}

internal b32
BSplineKnotHandleMatch(b_spline_knot_handle A, b_spline_knot_handle B)
{
 b32 Result = (A.__Index == B.__Index);
 return Result;
}

internal b_spline_knot_handle
BSplineKnotHandleZero(void)
{
 b_spline_knot_handle Handle = {};
 return Handle;
}

internal u32
KnotIndexFromBSplineKnotHandle(b_spline_knot_handle Handle)
{
 Assert(!BSplineKnotHandleMatch(Handle, BSplineKnotHandleZero()));
 u32 Index = Handle.__Index - 1;
 return Index;
}

internal u32
PartitionKnotIndexFromBSplineKnotHandle(curve *Curve, b_spline_knot_handle Handle)
{
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Handle);
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 Assert(KnotIndex >= KnotParams.Degree);
 u32 PartitionKnotIndex = KnotIndex - KnotParams.Degree;
 return PartitionKnotIndex;
}

internal b_spline_knot_handle
BSplineKnotHandleFromKnotIndex(u32 KnotIndex)
{
 b_spline_knot_handle Handle = {};
 Handle.__Index = KnotIndex + 1;
 return Handle;
}

internal b_spline_knot_handle
BSplineKnotHandleFromPartitionKnotIndex(curve *Curve, u32 PartitionKnotIndex)
{
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 u32 KnotIndex = KnotParams.Degree + PartitionKnotIndex;
 b_spline_knot_handle Handle = BSplineKnotHandleFromKnotIndex(KnotIndex);
 return Handle;
}

internal void
ControlPointAndBezierOffsetFromCurvePoint(curve_point_handle CurvePoint,
                                          control_point_handle *ControlPoint,
                                          u32 *BezierOffset)
{
 switch (CurvePoint.Type)
 {
  case CurvePoint_ControlPoint: {
   *ControlPoint = CurvePoint.Control;
   *BezierOffset = 1;
  }break;
  case CurvePoint_CubicBezierPoint: {
   u32 BezierIndex = IndexFromCubicBezierPointHandle(CurvePoint.Bezier);
   *ControlPoint = ControlPointFromCubicBezierPoint(CurvePoint.Bezier);
   *BezierOffset = BezierIndex % 3;
  }break;
 }
 Assert(*BezierOffset < 3);
}

internal void
TranslateCurvePointTo(entity_with_modify_witness *EntityWitness,
                      curve_point_handle CurvePoint,
                      v2 P,
                      translate_curve_point_flags Flags)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 control_point_handle ControlPoint = {};
 u32 BezierOffset = 0;
 ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
 
 u32 ControlPointIndex = IndexFromControlPointHandle(ControlPoint);
 
 if (ControlPointIndex < Curve->Points.ControlPointCount)
 {
  cubic_bezier_point *B = Curve->Points.CubicBezierPoints + ControlPointIndex;
  v2 *TranslatePoint = B->Ps + BezierOffset;
  v2 LocalP = WorldToLocalEntityPosition(Entity, P);
  
  if (BezierOffset == 1)
  {
   v2 LocalTranslation = LocalP - B->P1;
   
   B->P0 += LocalTranslation;
   B->P2 += LocalTranslation;
   
   Curve->Points.ControlPoints[ControlPointIndex] += LocalTranslation;
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
SelectControlPoint(curve *Curve, control_point_handle ControlPoint)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
SelectControlPointFromCurvePoint(curve *Curve, curve_point_handle CurvePoint)
{
 if (Curve)
 {
  control_point_handle ControlPoint = {};
  u32 BezierOffset = 0;
  ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
MaybeRecomputeCurveBSplineKnots(curve *Curve)
{
 u32 ControlPointCount = Curve->Points.ControlPointCount;
 curve_params *CurveParams = &Curve->Params;
 b_spline_params *RequestedBSplineParams = &CurveParams->BSpline;
 
 b_spline_knot_params FixedBSplineKnotParams = BSplineKnotParamsFromDegree(RequestedBSplineParams->KnotParams.Degree, ControlPointCount);
 b_spline_params FixedBSplineParams = {};
 FixedBSplineParams.Partition = RequestedBSplineParams->Partition;
 FixedBSplineParams.KnotParams = FixedBSplineKnotParams;
 CurveParams->BSpline = FixedBSplineParams;
 
 b_spline_params ComputedBSplineParams = Curve->ComputedBSplineParams;
 
 if (!StructsEqual(&ComputedBSplineParams, &FixedBSplineParams))
 {
  f32 *Knots = Curve->BSplineKnots;
  Assert(FixedBSplineKnotParams.KnotCount <= MAX_B_SPLINE_KNOT_COUNT);
  BSplineBaseKnots(FixedBSplineKnotParams, Knots);
  switch (FixedBSplineParams.Partition)
  {
   case BSplinePartition_Natural: {BSplineKnotsNaturalExtension(FixedBSplineParams.KnotParams, Knots);}break;
   case BSplinePartition_Periodic: {BSplineKnotsPeriodicExtension(FixedBSplineParams.KnotParams, Knots);}break;
   case BSplinePartition_Count: InvalidPath;
  }
  
  Curve->ComputedBSplineParams = FixedBSplineParams;
 }
}

internal void
RemoveControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Point)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 
 if (Index < Curve->Points.ControlPointCount)
 {
#define ArrayRemoveOrdered(Array, At, Count) \
MemoryMove((Array) + (At), \
(Array) + (At) + 1, \
((Count) - (At) - 1) * SizeOf((Array)[0]))
  ArrayRemoveOrdered(Curve->Points.ControlPoints,       Index, Curve->Points.ControlPointCount);
  ArrayRemoveOrdered(Curve->Points.ControlPointWeights, Index, Curve->Points.ControlPointCount);
  ArrayRemoveOrdered(Curve->Points.CubicBezierPoints,   Index, Curve->Points.ControlPointCount);
#undef ArrayRemoveOrdered
  
  --Curve->Points.ControlPointCount;
  
  // NOTE(hbr): Maybe fix selected control point
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (Index == SelectedIndex)
   {
    DeselectControlPoint(Curve);
   }
   else if (Index < SelectedIndex)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex - 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
DeselectControlPoint(curve *Curve)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPointHandleZero();
 }
}

internal b32
IsControlPointSelected(curve *Curve)
{
 b32 Result = false;
 if (UsesControlPoints(Curve) &&
     !ControlPointHandleMatch(Curve->SelectedControlPoint, ControlPointHandleZero()))
 {
  u32 Index = IndexFromControlPointHandle(Curve->SelectedControlPoint);
  Result = (Index < Curve->Points.ControlPointCount);
 }
 return Result;
}

internal control_point_handle
AppendControlPoint(entity_with_modify_witness *EntityWitness, v2 P)
{
 entity *Entity = EntityWitness->Entity;
 u32 InsertAt = 0;
 if (Entity->Flags & EntityFlag_CurveAppendFront)
 {
  InsertAt = 0;
 }
 else
 {
  InsertAt = Entity->Curve.Points.ControlPointCount;
 }
 control_point_handle Result = InsertControlPoint(EntityWitness, MakeControlPoint(P), InsertAt);
 return Result;
}

internal void
SetControlPoint(entity_with_modify_witness *Witness, control_point_handle Handle, control_point Point)
{
 entity *Entity = Witness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 u32 I = IndexFromControlPointHandle(Handle);
 u32 N = Curve->Points.ControlPointCount;
 v2 *P = Curve->Points.ControlPoints;
 f32 *W = Curve->Points.ControlPointWeights;
 cubic_bezier_point *B = Curve->Points.CubicBezierPoints;
 
 P[I] = WorldToLocalEntityPosition(Entity, Point.P);
 W[I] = (Point.IsWeight ? Point.Weight : 1.0f);
 if (Point.IsBezier)
 {
  cubic_bezier_point Bezier = {
   WorldToLocalEntityPosition(Entity, Point.Bezier.P0),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P1),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P2),
  };
  B[I] = Bezier;
 }
 else
 {
  // NOTE(hbr): Cubic bezier point calculation is not really defined
  // for N<=2. At least I tried to "define" it but failed to do something
  // that was super useful. In that case, when the third point is added,
  // just recalculate all bezier points.
  if (Curve->Points.ControlPointCount == 2)
  {
   BezierCubicCalculateAllControlPoints(N+1, P, B);
  }
  else
  {
   CalculateBezierCubicPointAt(N+1, P, B, I);
  }
 }
 
 MarkEntityModified(Witness);
}

internal control_point_handle
InsertControlPoint(entity_with_modify_witness *EntityWitness, control_point Point, u32 At)
{
 control_point_handle Result = {};
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 if (Curve &&
     Curve->Points.ControlPointCount < CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT &&
     At <= Curve->Points.ControlPointCount)
 {
  Assert(UsesControlPoints(Curve));
  
  control_point_handle Handle = ControlPointHandleFromIndex(At);
  u32 N = Curve->Points.ControlPointCount;
  v2 *P = Curve->Points.ControlPoints;
  f32 *W = Curve->Points.ControlPointWeights;
  cubic_bezier_point *B = Curve->Points.CubicBezierPoints;
  
#define ShiftRightArray(Array, ArrayLength, At, ShiftCount) \
MemoryMove((Array) + ((At)+(ShiftCount)), \
(Array) + (At), \
((ArrayLength) - (At)) * SizeOf((Array)[0]))
  ShiftRightArray(P, N, At, 1);
  ShiftRightArray(W, N, At, 1);
  ShiftRightArray(B, N, At, 1);
#undef ShiftRightArray
  
  SetControlPoint(EntityWitness, Handle, Point);
  
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (SelectedIndex >= At)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex + 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  ++Curve->Points.ControlPointCount;
  Result = Handle;
 }
 
 return Result;
}

internal void
SetCurvePoints(entity_with_modify_witness *EntityWitness, curve_points_handle Points)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 CopyCurvePoints(CurvePointsDynamicFromStatic(&Curve->Points), Points);
 DeselectControlPoint(Curve);
 MarkEntityModified(EntityWitness);
}

inline internal void
SetEntityName(entity *Entity, string Name)
{
 FillCharBuffer(&Entity->NameBuffer, Name);
}

internal void
InitEntityPart(entity *Entity,
               entity_type Type,
               xform2d XForm,
               string Name,
               i32 SortingLayer,
               entity_flags Flags)
{
 Entity->Type = Type;
 Entity->XForm = XForm;
 SetEntityName(Entity, Name);
 Entity->SortingLayer = SortingLayer;
 Entity->Flags = Flags;
}

internal void
InitEntityAsCurve(entity *Entity, string Name, curve_params CurveParams)
{
 InitEntityPart(Entity,
                Entity_Curve,
                XForm2DZero(),
                Name, 0, 0);
 curve *Curve = &Entity->Curve;
 Curve->Params = CurveParams;
}

internal void
InitEntityAsImage(entity *Entity,
                  v2 P,
                  string Name,
                  u32 Width, u32 Height,
                  render_texture_handle TextureHandle)
{
 InitEntityPart(Entity,
                Entity_Image,
                XForm2DFromP(P),
                Name, 0, 0);
 image *Image = &Entity->Image;
 Image->Dim = Scale2D(Cast(f32)Width / Height, 1.0f);
 Image->TextureHandle = TextureHandle;
}

internal void
SetControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Handle, v2 P, f32 Weight)
{
 curve_point_handle CurvePoint = CurvePointFromControlPoint(Handle);
 entity *Entity = EntityWitness->Entity;
 TranslateCurvePointTo(EntityWitness, CurvePoint, P, TranslateCurvePoint_None);
 
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  curve *Curve = SafeGetCurve(Entity);
  u32 Index = IndexFromControlPointHandle(Handle);
  Curve->Points.ControlPointWeights[Index] = Weight;
  MarkEntityModified(EntityWitness);
 }
}

internal curve_points_modify_handle
BeginModifyCurvePoints(entity_with_modify_witness *EntityWitness, u32 RequestedPointCount, modify_curve_points_static_which_points Which)
{
 curve_points_modify_handle Result = {};
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *CurvePoints = &Curve->Points;
 Result.Curve = Curve;
 u32 ActualPointCount = Min(RequestedPointCount, CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT);
 Result.PointCount = ActualPointCount;
 Result.ControlPoints = CurvePoints->ControlPoints;
 Result.Weights = CurvePoints->ControlPointWeights;
 Result.CubicBeziers = CurvePoints->CubicBezierPoints;
 Result.Which = Which;
 // NOTE(hbr): Assume (which is very reasonable) that the curve is always changing here
 MarkEntityModified(EntityWitness);
 
 return Result;
}

internal void
EndModifyCurvePoints(curve_points_modify_handle Handle)
{
 curve *Curve = Handle.Curve;
 curve_points_static *Points = &Curve->Points;
 Points->ControlPointCount = Handle.PointCount;
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsOnly)
 {
  for (u32 PointIndex = 0;
       PointIndex < Points->ControlPointCount;
       ++PointIndex)
  {
   Points->ControlPointWeights[PointIndex] = 1.0f;
  }
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsWithWeights)
 {
  BezierCubicCalculateAllControlPoints(Points->ControlPointCount,
                                       Points->ControlPoints,
                                       Points->CubicBezierPoints);
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_AllPoints)
 {
  // NOTE(hbr): Nothing to do
 }
}

inline internal v2
ProjectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Hadamard(Q, XForm.Scale.V);
 Q = RotateAround(Q, V2(0.0f, 0.0f), XForm.Rotation);
 Q = Q + XForm.P;
 return Q;
}

inline internal v2
UnprojectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Q - XForm.P;
 Q = RotateAround(Q, V2(0, 0), Rotation2DInverse(XForm.Rotation));
 Q = Hadamard(Q, V2(1.0f / XForm.Scale.V.X, 1.0f / XForm.Scale.V.Y));
 return Q;
}

// TODO(hbr): Consider using ModelTransform instead (might be a little slower but probably doesn't matter)
internal v2
WorldToLocalEntityPosition(entity *Entity, v2 P)
{
 v2 Result = UnprojectThroughXForm(Entity->XForm, P);
 return Result;
}

internal v2
LocalToWorldEntityPosition(entity *Entity, v2 P)
{
 v2 Result = ProjectThroughXForm(Entity->XForm, P);
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

internal void
SetBSplineKnotPoint(entity_with_modify_witness *EntityWitness,
                    b_spline_knot_handle Knot,
                    f32 KnotFraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
 f32 *Knots = Curve->BSplineKnots;
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Knot);
 b32 Moveable = ((KnotIndex >= KnotParams.Degree + 1) &&
                 (KnotIndex + 1 < KnotParams.Degree + KnotParams.PartitionSize));
 Assert(Moveable);
 if (Moveable)
 {
  for (u32 Index = KnotIndex;
       Index > 0;
       --Index)
  {
   Knots[Index - 1] = Min(KnotFraction, Knots[Index - 1]);
  }
  for (u32 Index = KnotIndex + 1;
       Index < KnotParams.KnotCount;
       ++Index)
  {
   Knots[Index] = Max(KnotFraction, Knots[Index]);
  }
  Knots[KnotIndex] = KnotFraction;
  MarkEntityModified(EntityWitness);
 }
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
                          f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
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
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = BarycentricEvaluate(TT, Omega, Ti, SOA.Xs, PointCount);
    f32 Y = BarycentricEvaluate(TT, Omega, Ti, SOA.Ys, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcNewtonPolynomial(v2 *Controls, u32 PointCount, f32 *Ti,
                     u32 SampleCount, v2 *OutSamples,
                     f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Beta_X = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *Beta_Y = PushArrayNonZero(Temp.Arena, PointCount, f32);
  NewtonBetaFast(Beta_X, Ti, SOA.Xs, PointCount);
  NewtonBetaFast(Beta_Y, Ti, SOA.Ys, PointCount);
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = NewtonEvaluate(TT, Beta_X, Ti, PointCount);
    f32 Y = NewtonEvaluate(TT, Beta_Y, Ti, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcPolynomial(v2 *Controls, u32 PointCount,
               polynomial_interpolation_params Polynomial,
               u32 SampleCount, v2 *OutSamples,
               u32 SamplesPerControlPoint)
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
  case PolynomialInterpolation_Barycentric: {CalcBarycentricPolynomial(Controls, PointCount, PointSpacing, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
  case PolynomialInterpolation_Newton:      {CalcNewtonPolynomial(Controls, PointCount, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
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

inline internal u32
PartitionKnotIndexToKnotIndex(u32 PartitionKnotIndex, b_spline_knot_params KnotParams)
{
 Assert(PartitionKnotIndex < KnotParams.PartitionSize);
 u32 Result = PartitionKnotIndex + KnotParams.Degree;
 return Result;
}

internal void
CalcBSpline(v2 *Controls,
            b_spline_knot_params KnotParams, f32 *Knots,
            u32 SampleCount, v2 *OutSamples)
{
 // TODO(hbr): This slightly breakes moving BSpline along the curve.
 // When we calculate how much to move point forward or backwards, we
 // convert from [0,1] fraction into [SampleIndex], assuming that
 // sample points are sampled uniformly. This isn't the case with the code below.
#if 0
 v2 *Out = OutSamples;
 u32 SamplesPerPartitionKnot = (SampleCount - 1) / (KnotParams.PartitionSize - 1);
 u32 SamplesLeft = SampleCount - 1 - SamplesPerPartitionKnot * (KnotParams.PartitionSize - 1);
 
 for (u32 PartitionKnotIndex = 1;
      PartitionKnotIndex < KnotParams.PartitionSize;
      ++PartitionKnotIndex)
 {
  u32 PrevKnot = PartitionKnotIndexToKnotIndex(PartitionKnotIndex - 1, KnotParams);
  u32 NextKnot = PartitionKnotIndexToKnotIndex(PartitionKnotIndex - 0, KnotParams);
  
  f32 PrevT = Knots[PrevKnot];
  f32 NextT = Knots[NextKnot];
  
  u32 CurrentSampleCount = SamplesPerPartitionKnot;
  if (PartitionKnotIndex == 1) CurrentSampleCount += SamplesLeft;
  
  f32 T = PrevT;
  Assert(NextT >= PrevT);
  f32 DeltaT = (NextT - PrevT) / CurrentSampleCount;
  
  ForEachIndex(Index, CurrentSampleCount)
  {
   *Out++ = BSplineEvaluate(T, Controls, KnotParams, Knots);
   T += DeltaT;
  }
 }
 *Out++ = BSplineEvaluate(KnotParams.B, Controls, KnotParams, Knots);
 Assert(Cast(u64)(Out - OutSamples) == SampleCount);
 
#else
 
 f32 T = KnotParams.A;
 f32 Delta_T = (KnotParams.B - KnotParams.A) / (SampleCount - 1);
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  OutSamples[SampleIndex] = BSplineEvaluate(T, Controls, KnotParams, Knots);
  T += Delta_T;
 }
 
#endif
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
CalcCurve(curve *Curve, u32 SampleCount, v2 *OutSamples)
{
 u32 PointCount = Curve->Points.ControlPointCount;
 v2 *Controls = Curve->Points.ControlPoints;
 f32 *Weights = Curve->Points.ControlPointWeights;
 cubic_bezier_point *Beziers = Curve->Points.CubicBezierPoints;
 curve_params *Params = &Curve->Params;
 f32 *BSplineKnots = Curve->BSplineKnots;
 u32 SamplesPerControlPoint = Params->SamplesPerControlPoint;
 
 switch (Params->Type)
 {
  case Curve_Polynomial: {CalcPolynomial(Controls, PointCount, Params->Polynomial, SampleCount, OutSamples, SamplesPerControlPoint);} break;
  case Curve_CubicSpline: {CalcCubicSpline(Controls, PointCount, Params->CubicSpline, SampleCount, OutSamples);} break;
  
  case Curve_Bezier: {
   switch (Params->Bezier)
   {
    case Bezier_Regular: {CalcRegularBezier(Controls, Weights, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Cubic: {CalcCubicBezier(Beziers, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Count: InvalidPath;
   }
  } break;
  
  case Curve_BSpline: {
   MaybeRecomputeCurveBSplineKnots(Curve);
   b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
   u32 PartitionSize = KnotParams.PartitionSize;
   u32 Degree = KnotParams.Degree;
   v2 *PartitionKnots = Curve->BSplinePartitionKnots;
   for (u32 PartitionKnotIndex = 0;
        PartitionKnotIndex < PartitionSize;
        ++PartitionKnotIndex)
   {
    f32 T = BSplineKnots[Degree + PartitionKnotIndex];
    v2 KnotPoint = BSplineEvaluate(T, Controls, KnotParams, BSplineKnots);
    PartitionKnots[PartitionKnotIndex] = KnotPoint;
   }
   CalcBSpline(Controls, KnotParams, BSplineKnots, SampleCount, OutSamples);
  }break;
  case Curve_Parametric: {CalcParametric(Params->Parametric, SampleCount, OutSamples);}break;
  
  case Curve_Count: InvalidPath;
 }
}

// TODO(hbr): This function needs some serious refactoring.
// First of all, don't break down all the "Compute" functions into hierarchical structure.
// It's very fragile. It's just better (I think) to duplicate some code but have it more independent.
// Second of all just refactor stupid code duplication here.
// Basically inline everything and work your way up from there.
// Then parallelize this heavily. I mean, we can unroll, we can simd, we can multithread this.
internal void
RecomputeCurve(curve *Curve)
{
 ProfileFunctionBegin();
 
 curve_params *Params = &Curve->Params;
 arena *ComputeArena = Curve->ComputeArena;
 u32 ControlCount = Curve->Points.ControlPointCount;
 v2 *Controls = Curve->Points.ControlPoints;
 f32 *Weights = Curve->Points.ControlPointWeights;
 f32 LineWidth = Params->DrawParams.Line.Width;
 
 ClearArena(ComputeArena);
 
 u32 SampleCount = 0;
 if (IsCurveTotalSamplesMode(Curve))
 {
  SampleCount = Curve->Params.TotalSamples;
 }
 else
 {
  u32 ControlCountWithLooped = (IsCurveLooped(Curve) ? ControlCount + 1 : ControlCount);
  if (ControlCountWithLooped > 0)
  {
   SampleCount = (ControlCountWithLooped - 1) * Params->SamplesPerControlPoint + 1;
  }
 }
 v2 *Samples = PushArrayNonZero(ComputeArena, SampleCount, v2);
 CalcCurve(Curve, SampleCount, Samples);
 
 vertex_array CurveVertices = ComputeVerticesOfThickLine(ComputeArena, SampleCount, Samples, LineWidth, IsCurveLooped(Curve));
 vertex_array PolylineVertices = ComputeVerticesOfThickLine(ComputeArena, ControlCount, Controls, Params->DrawParams.Polyline.Width, false);
 
 v2 *ConvexHullPoints = PushArrayNonZero(ComputeArena, ControlCount, v2);
 u32 ConvexHullCount = CalcConvexHull(ControlCount, Controls, ConvexHullPoints);
 vertex_array ConvexHullVertices = ComputeVerticesOfThickLine(ComputeArena, ConvexHullCount, ConvexHullPoints, Params->DrawParams.ConvexHull.Width, true);
 
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 if (IsCurveEligibleForPointTracking(Curve))
 {
  if (Tracking->Active)
  {
   f32 Fraction = Tracking->Fraction;
   
   v2 LocalSpaceTrackedPoint = BezierCurveEvaluateWeighted(Fraction, Controls, Weights, ControlCount);
   Tracking->LocalSpaceTrackedPoint = LocalSpaceTrackedPoint;
   
   if (Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization)
   {
    all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(ComputeArena, Fraction, Controls, Weights, ControlCount);
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
 
 u32 BSplineConvexHullCount = 0;
 b_spline_convex_hull *BSplineConvexHulls = 0;
 if (IsBSplineCurve(Curve))
 {
  b_spline_knot_params BSpline = GetBSplineParams(Curve).KnotParams;
  u32 Degree = BSpline.Degree;
  Assert(Degree <= ControlCount);
  u32 ConvexHullCount = ControlCount - Degree;
  b_spline_convex_hull *Hulls = PushArray(ComputeArena, ConvexHullCount, b_spline_convex_hull);
  ForEachIndex(ConvexHullIndex, ConvexHullCount)
  {
   u32 PointCount = Degree + 1;
   v2 *Points = PushArray(ComputeArena, PointCount, v2);
   PointCount = CalcConvexHull(PointCount, Controls + ConvexHullIndex, Points);
   vertex_array Vertices = ComputeVerticesOfThickLine(ComputeArena, PointCount, Points, Params->DrawParams.BSplinePartialConvexHull.Width, true);
   b_spline_convex_hull *Hull = Hulls + ConvexHullIndex;
   Hull->Points = Points;
   Hull->Vertices = Vertices;
  }
  BSplineConvexHullCount = ConvexHullCount;
  BSplineConvexHulls = Hulls;
 }
 
 Curve->CurveSampleCount = SampleCount;
 Curve->CurveSamples = Samples;
 Curve->CurveVertices = CurveVertices;
 Curve->PolylineVertices = PolylineVertices;
 Curve->ConvexHullPoints = ConvexHullPoints;
 Curve->ConvexHullCount = ConvexHullCount;
 Curve->ConvexHullVertices = ConvexHullVertices;
 Curve->BSplineConvexHullCount = BSplineConvexHullCount;
 Curve->BSplineConvexHulls = BSplineConvexHulls;
 
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

internal parametric_curve_var *
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
 
 return Var;
}

internal void
RemoveAdditionalVar(parametric_curve_resources *Resources, parametric_curve_var *Var)
{
 DLLRemove(Resources->AdditionalVarsHead,
           Resources->AdditionalVarsTail,
           Var);
 StackPush(Resources->FirstFreeAdditionalVar, Var);
}

internal curve_params
DefaultCurveParams(void)
{
 curve_params Params = {};
 
 f32 LineWidth = 0.009f;
 rgba PolylineColor = RGBA_U8(16, 31, 31, 200);
 
 Params.DrawParams.Line.Enabled = true;
 Params.DrawParams.Line.Color = RGBA_U8(21, 69, 98);
 Params.DrawParams.Line.Width = LineWidth;
 Params.DrawParams.Points.Enabled = true;
 Params.DrawParams.Points.Color = RGBA_U8(0, 138, 138, 148);
 Params.DrawParams.Points.Radius = 0.014f;
 Params.DrawParams.Polyline.Color = PolylineColor;
 Params.DrawParams.Polyline.Width = LineWidth;
 Params.DrawParams.ConvexHull.Color = PolylineColor;
 Params.DrawParams.ConvexHull.Width = LineWidth;
 Params.DrawParams.BSplineKnots.Radius = 0.010f;
 Params.DrawParams.BSplineKnots.Color = RGBA_U8(138, 0, 0, 148);
 Params.DrawParams.BSplinePartialConvexHull.Color = PolylineColor;
 Params.DrawParams.BSplinePartialConvexHull.Width = LineWidth;
 
 Params.SamplesPerControlPoint = 50;
 Params.TotalSamples = 1000;
 Params.Parametric.MaxT = 1.0f;
 Params.Parametric.X_Equation = NilExpr;
 Params.Parametric.Y_Equation = NilExpr;
 
 return Params;
}

internal curve_params
DefaultCubicSplineCurveParams(void)
{
 curve_params Params = DefaultCurveParams();
 Params.Type = Curve_CubicSpline;
 Params.CubicSpline = CubicSpline_Natural;
 return Params;
}
