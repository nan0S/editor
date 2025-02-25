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
AreLinePointsVisible(curve *Curve)
{
 b32 Result = (!Curve->Params.PointsDisabled && DoesCurveUseControlPoints(Curve));
 return Result;
}

internal b32
DoesCurveUseControlPoints(curve *Curve)
{
 b32 Result = (Curve->Params.Type != Curve_Parametric);
 return Result;
}

internal b32
IsEntityVisible(entity *Entity)
{
 b32 Result = !(Entity->Flags & EntityFlag_Hidden);
 return Result;
}

internal void
SetEntityVisibility(entity *Entity, b32 Hidden)
{
 if (Hidden)
 {
  Entity->Flags |= EntityFlag_Hidden;
 }
 else
 {
  Entity->Flags &= ~EntityFlag_Hidden;
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
  case Curve_Polynomial: {}break;
  
  case Curve_Parametric: {
   Result = true;
  }break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

internal b32
CanAddControlPoints(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial: {Result = true;}break;
  
  case Curve_Parametric: {}break;
  
  case Curve_Count: InvalidPath;
 }
 
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
 
 if (Params0->Type == Params1->Type)
 {
  switch (Params0->Type)
  {
   case Curve_Polynomial: {
    if (Method == CurveMerge_Concat)
    {
     Compatible = true;
    }
    else
    {
     WhyIncompatible = StrLit("polynomial curves can only be concatenated");
    }
   }break;
   
   case Curve_CubicSpline: {
    if (Params0->CubicSpline == Params1->CubicSpline)
    {
     if (Method == CurveMerge_Concat)
     {
      Compatible = true;
     }
     else
     {
      WhyIncompatible = StrLit("cubic spline curves can only be concatenated");
     }
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
      case Bezier_Regular: {
       Compatible = true;
      }break;
      
      case Bezier_Cubic: {
       if (Method == CurveMerge_Concat)
       {
        Compatible = true;
       }
       else
       {
        WhyIncompatible = StrLit("cubic bezier curves can only be concatenated");
       }
      }break;
      
      case Bezier_Count: InvalidPath;
     }
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_Parametric: {
    WhyIncompatible = IncompatibleTypes;
   }break;
   
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
GetCurveControlPointInfo(entity *Entity, u32 PointIndex)
{
 point_info Result = {};
 
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 
 Result.Radius = Params->PointRadius;
 
 if (( (Entity->Flags & EntityFlag_CurveAppendFront) && PointIndex == 0) ||
     (!(Entity->Flags & EntityFlag_CurveAppendFront) && PointIndex == Curve->ControlPointCount-1))
 {
  Result.Radius *= 1.5f;
 }
 
 f32 OutlineScale = 0.0f;
 if (IsEntitySelected(Entity))
 {
  OutlineScale = 0.55f;
 }
 Result.OutlineThickness = OutlineScale * Result.Radius;
 
 Result.Color = Params->PointColor;
 Result.OutlineColor = BrightenColor(Result.Color, 0.3f);
 if (PointIndex == Curve->SelectedIndex.Index)
 {
  Result.OutlineColor = BrightenColor(Result.OutlineColor, 0.5f);
 }
 
 return Result;
}

internal f32
GetCurveCubicBezierPointRadius(curve *Curve)
{
 // TODO(hbr): Make bezier points smaller
 f32 Result = Curve->Params.PointRadius;
 return Result;
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

internal sorted_entries
SortEntities(arena *Arena, entity_array Entities)
{
 sort_entry *Entries = PushArrayNonZero(Arena, Entities.Count, sort_entry);
 u32 EntryCount = 0;
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities + EntityIndex;
  if (Entity->Flags & EntityFlag_Active)
  {
   sort_entry *Entry = Entries + EntryCount;
   
   // NOTE(hbr): Equal sorting layer images should be below curves
   f32 Offset = 0.0f;
   switch (Entity->Type)
   {
    case Entity_Image: {Offset = 0.5f;} break;
    case Entity_Curve: {Offset = 0.0f;} break;
    case Entity_Count: InvalidPath; break;
   }
   
   Entry->SortKey = -Entity->SortingLayer - Offset;
   Entry->Index = EntityIndex;
   ++EntryCount;
  }
 }
 
 temp_arena Temp = TempArena(Arena);
 sort_entry *TempMemory = PushArrayNonZero(Temp.Arena, EntryCount, sort_entry);
 sort_entry *Sorted = MergeSortStable(Entries, EntryCount, TempMemory);
 if (Sorted != Entries)
 {
  ArrayCopy(Entries, Sorted, EntryCount);
 }
 EndTemp(Temp);
 
 sorted_entries Result = {};
 Result.Count = EntryCount;
 Result.Entries = Entries;
 
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
  Assert(CanAddControlPoints(Curve));
  
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

internal void
InitCurve(entity_with_modify_witness *EntityWitness, curve_params Params)
{
 entity *Entity = EntityWitness->Entity;
 Entity->Type = Entity_Curve;
 Entity->Curve.Params = Params;
 SetCurveControlPoints(EntityWitness, 0, 0, 0, 0);
}

internal void
InitImage(entity *Entity)
{
 Entity->Type = Entity_Image;
 ClearArena(Entity->Arena);
}

internal void
InitAllocEntity(entity *Entity)
{
 Entity->Arena = AllocArena();
 Entity->Curve.DegreeLowering.Arena = AllocArena();
 
 curve *Curve = &Entity->Curve;
 Curve->ParametricResources.Arena = AllocArena();
}

internal void
InitEntity(entity *Entity,
           v2 P,
           v2 Scale,
           v2 Rotation,
           string Name,
           i32 SortingLayer)
{
 Entity->P = P;
 Entity->Scale = Scale;
 Entity->Rotation = Rotation;
 SetEntityName(Entity, Name);
 Entity->SortingLayer = SortingLayer;
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

internal void
SetEntityName(entity *Entity, string Name)
{
 u64 ToCopy = Min(Name.Count, ArrayCount(Entity->NameBuffer) - 1);
 MemoryCopy(Entity->NameBuffer, Name.Data, ToCopy);
 Entity->NameBuffer[ToCopy] = 0;
 Entity->Name = MakeStr(Entity->NameBuffer, ToCopy);
}

internal void
InitEntityFromEntity(entity_with_modify_witness *DstWitness, entity *Src)
{
 entity *Dst = DstWitness->Entity;
 
 InitEntity(Dst, Src->P, Src->Scale, Src->Rotation, Src->Name, Src->SortingLayer);
 switch (Src->Type)
 {
  case Entity_Curve: {
   curve *SrcCurve = SafeGetCurve(Src);
   curve *DstCurve = SafeGetCurve(Dst);
   
   InitCurve(DstWitness, SrcCurve->Params);
   SetCurveControlPoints(DstWitness, SrcCurve->ControlPointCount, SrcCurve->ControlPoints, SrcCurve->ControlPointWeights, SrcCurve->CubicBezierPoints);
   SelectControlPoint(DstCurve, SrcCurve->SelectedIndex);
  } break;
  
  case Entity_Image: {
   image *Image = SafeGetImage(Src);
   InitImage(Dst);
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
}

internal curve_points
BeginModifyCurvePoints(entity_with_modify_witness *EntityWitness, u32 RequestedPointCount, modify_curve_points_which_points Which)
{
 curve_points Result = {};
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 u32 ActualPointCount = Min(RequestedPointCount, MAX_CONTROL_POINT_COUNT);
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
EndModifyCurvePoints(curve *Curve, curve_points *Handle)
{
 Curve->ControlPointCount = Handle->PointCount;
 switch (Handle->Which)
 {
  case ModifyCurvePointsWhichPoints_JustControlPoints: {
   // NOTE(hbr): Recalculate cubic bezier points from scratch in case
   // we changed only control points
   BezierCubicCalculateAllControlPoints(Handle->PointCount,
                                        Handle->ControlPoints,
                                        Handle->CubicBeziers);
  }break;
  
  case ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers: {
   // NOTE(hbr): Nothing to do
  }break;
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
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 
 Tracking->Fraction = Clamp01(Fraction);
 MarkEntityModified(EntityWitness);
}

// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->Params.Type == Curve_CubicSpline &&
                 Curve->Params.CubicSpline   == CubicSpline_Periodic);
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
CalcNewtonPolynomialLinePoints(v2 *ControlPoints,
                               u32 ControlPointCount,
                               f32 *Ti,
                               u32 OutputLinePointCount,
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
  f32 Delta = (End - Begin) / (OutputLinePointCount - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < OutputLinePointCount;
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
                                    u32 OutputLinePointCount,
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
  f32 Delta = (End - Begin) / (OutputLinePointCount - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < OutputLinePointCount;
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
                          u32 OutputLinePointCount,
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
  f32 Delta = (End - Begin) / (OutputLinePointCount - 1);
  
  for (u32 OutputIndex = 0;
       OutputIndex < OutputLinePointCount;
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

internal void
CalcRegularBezierLinePoints(v2 *ControlPoints,
                            f32 *ControlPointWeights,
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
                          u32 OutputLinePointCount,
                          v2 *OutputLinePoints)
{
 if (ControlPointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta = 1.0f / (OutputLinePointCount - 1);
  for (u32 OutputIndex = 0;
       OutputIndex < OutputLinePointCount;
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

internal v2
ParametricCurveEvaluate(f32 T, parametric_equation_expr *Equation)
{
 v2 P = {};
 return P;
}

internal void
CalcParametricCurveLinePoints(f32 MinT, f32 MaxT,
                              parametric_equation_expr *X_Equation,
                              parametric_equation_expr *Y_Equation,
                              u32 LinePointCount, v2 *LinePoints)
{
 f32 T = MinT;
 MaxT = Max(MinT, MaxT);
 Assert(MinT <= MaxT);
 f32 DeltaT = (MaxT - MinT) / (LinePointCount - 1);
 
 for (u32 LinePointIndex = 0;
      LinePointIndex	< LinePointCount;
      ++LinePointIndex)
 {
  f32 X = ParametricEquationEvalWithT(X_Equation, T);
  f32 Y = ParametricEquationEvalWithT(Y_Equation, T);
  
  v2 Point = V2(X, Y);
  LinePoints[LinePointIndex] = Point;
  
  T += DeltaT;
 }
}

internal void
EvaluateCurve(curve *Curve, u32 LinePointCount, v2 *LinePoints)
{
 temp_arena Temp = TempArena(0);
 curve_params *CurveParams = &Curve->Params;
 switch (CurveParams->Type)
 {
  case Curve_Polynomial: {
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
  
  case Curve_CubicSpline: {
   CalcCubicSplineLinePoints(Curve->ControlPoints, Curve->ControlPointCount,
                             CurveParams->CubicSpline,
                             LinePointCount, LinePoints);
  } break;
  
  case Curve_Bezier: {
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
  
  case Curve_Parametric: {
   parametric_curve_params *Parametric = &CurveParams->Parametric;
   CalcParametricCurveLinePoints(Parametric->MinT, Parametric->MaxT,
                                 Parametric->X_Equation, Parametric->Y_Equation,
                                 LinePointCount, LinePoints);
  }break;
  
  case Curve_Count: InvalidPath;
 }
 EndTemp(Temp);
}

internal void
RecomputeCurve(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 arena *EntityArena = Entity->Arena;
 
 ClearArena(EntityArena);
 
 u32 LinePointCount = 0;
 if (IsCurveTotalSamplesMode(Curve))
 {
  LinePointCount = Curve->Params.TotalSamples;
 }
 else
 {
  if (Curve->ControlPointCount > 0)
  {
   LinePointCount = (Curve->ControlPointCount - 1) * Curve->Params.SamplesPerControlPoint + 1;
  }
 }
 
 Curve->LinePointCount = LinePointCount;
 Curve->LinePoints = PushArrayNonZero(Entity->Arena, LinePointCount, v2);
 EvaluateCurve(Curve, Curve->LinePointCount, Curve->LinePoints);
 
 Curve->LineVertices = ComputeVerticesOfThickLine(EntityArena,
                                                  Curve->LinePointCount,
                                                  Curve->LinePoints,
                                                  Curve->Params.LineWidth,
                                                  IsCurveLooped(Curve));
 Curve->PolylineVertices = ComputeVerticesOfThickLine(EntityArena,
                                                      Curve->ControlPointCount,
                                                      Curve->ControlPoints,
                                                      Curve->Params.PolylineWidth,
                                                      false);
 
 Curve->ConvexHullPoints = PushArrayNonZero(EntityArena, Curve->ControlPointCount, v2);
 Curve->ConvexHullCount = CalcConvexHull(Curve->ControlPointCount, Curve->ControlPoints, Curve->ConvexHullPoints);
 Curve->ConvexHullVertices = ComputeVerticesOfThickLine(EntityArena,
                                                        Curve->ConvexHullCount,
                                                        Curve->ConvexHullPoints,
                                                        Curve->Params.ConvexHullWidth,
                                                        true);
 
 curve_point_tracking_state *Tracking = &Curve->PointTracking;
 if (IsCurveEligibleForPointTracking(Curve))
 {
  Tracking->LocalSpaceTrackedPoint = BezierCurveEvaluateWeighted(Tracking->Fraction,
                                                                 Curve->ControlPoints,
                                                                 Curve->ControlPointWeights,
                                                                 Curve->ControlPointCount);
  
  if (!Tracking->IsSplitting)
  {
   all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(EntityArena,
                                                                             Tracking->Fraction,
                                                                             Curve->ControlPoints,
                                                                             Curve->ControlPointWeights,
                                                                             Curve->ControlPointCount);
   vertex_array *LineVerticesPerIteration = PushArray(EntityArena,
                                                      Intermediate.IterationCount,
                                                      vertex_array);
   f32 LineWidth = Curve->Params.LineWidth;
   
   u32 IterationPointsOffset = 0;
   for (u32 Iteration = 0;
        Iteration < Intermediate.IterationCount;
        ++Iteration)
   {
    u32 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
    LineVerticesPerIteration[Iteration] =
     ComputeVerticesOfThickLine(EntityArena,
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
 else
 {
  Tracking->Active = false;
 }
}

internal void
MarkEntityModified(entity_with_modify_witness *Witness)
{
 Witness->EntityModified = true;
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
 if (Entity && Witness.EntityModified)
 {
  switch (Entity->Type)
  {
   case Entity_Curve: {RecomputeCurve(Entity);}break;
   case Entity_Image: {}break;
   case Entity_Count: InvalidPath;
  }
 }
}

internal b32
HasFreeAdditionalVar(parametric_curve_resources *Resources)
{
 b32 Result = (Resources->AdditionalVarCount < MAX_ADDITIONAL_VARS_COUNT);
 return Result;
}

internal void
ActivateNewAdditionalVar(parametric_curve_resources *Resources)
{
 Assert(HasFreeAdditionalVar(Resources));
 
 parametric_curve_var *Var = Resources->AdditionalVars + Resources->AdditionalVarCount;
 ++Resources->AdditionalVarCount;
 
 u32 Id = Var->Id;
 if (Id == 0)
 {
  Id = Resources->AdditionalVarCount;
 }
 
 StructZero(Var);
 Var->Id = Id;
}

internal void
DeactiveAdditionalVar(parametric_curve_resources *Resources, u32 VarIndex)
{
 Assert(VarIndex < Resources->AdditionalVarCount);
 
 parametric_curve_var *Remove = Resources->AdditionalVars + VarIndex;
 parametric_curve_var *From = Resources->AdditionalVars + (Resources->AdditionalVarCount - 1);
 
 ArrayMove(Remove, Remove + 1, (Resources->AdditionalVarCount - 1) - VarIndex);
 
 --Resources->AdditionalVarCount;
}
