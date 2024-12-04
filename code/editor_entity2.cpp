internal b32
AreLinePointsVisible(curve *Curve)
{
 b32 Result = (!Curve->Params.PointsDisabled);
 return Result;
}

internal b32
IsEntityVisible(entity *Entity)
{
 b32 Result = !(Entity->Flags & EntityFlag_Hidden);
 return Result;
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
 
 curve *Curve = GetCurve(Entity);
 if (IsEntitySelected(Entity) &&
     IsControlPointSelected(Curve) &&
     Curve->Params.Interpolation == Interpolation_Bezier &&
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

internal point_info
GetCurveControlPointInfo(entity *Entity, u32 PointIndex)
{
 point_info Result = {};
 
 curve *Curve = GetCurve(Entity);
 curve_params *Params = &Curve->Params;
 
 Result.Radius = Params->PointRadius;
 if (PointIndex == Curve->ControlPointCount - 1)
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
SetCurvePoint(entity *Entity, curve_point_index Index, v2 P, translate_curve_point_flags Flags)
{
 curve *Curve = GetCurve(Entity);
 
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
 
 RecomputeCurve(Entity);
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
RemoveControlPoint(entity *Entity, control_point_index Index)
{
 curve *Curve = GetCurve(Entity);
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
  
  RecomputeCurve(Entity);
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
 MergeSortStable(Entries, EntryCount, TempMemory);
 //QuickSort(Entries, EntryCount, sort_entry, SortEntryCmp);
 EndTemp(Temp);
 
 sorted_entries Result = {};
 Result.Count = EntryCount;
 Result.Entries = Entries;
 
 return Result;
}

internal control_point_index
AppendControlPoint(entity *Entity, v2 Point)
{
 control_point_index Result = {};
 Result.Index = Entity->Curve.ControlPointCount;
 InsertControlPoint(Entity, Point, Result.Index);
 
 return Result;
}

internal void
InsertControlPoint(entity *Entity, v2 Point, u32 At)
{
 curve *Curve = GetCurve(Entity);
 if (Curve &&
     Curve->ControlPointCount < MAX_CONTROL_POINT_COUNT &&
     At <= Curve->ControlPointCount)
 {
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
 }
 
 RecomputeCurve(Entity);
}