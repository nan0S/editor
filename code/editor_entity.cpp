// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
   b32 IsLooped = (Curve->CurveParams.CurveShape.InterpolationType == Interpolation_CubicSpline &&
                   Curve->CurveParams.CurveShape.CubicSplineType   == CubicSpline_Periodic);
   return IsLooped;
}

struct points_soa
{
   f32 *Xs;
   f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2f32 *Points, u64 NumPoints)
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
CalcNewtonPolynomialCurvePoints(local_position *ControlPoints,
                                u64 ControlPointCount,
                                f32 *Ti,
                                u64 NumOutputCurvePoints,
                                local_position *OutputCurvePoints)
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
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
      
      EndTemp(Temp);
   }
}

internal void
CalcBarycentricPolynomialCurvePoints(local_position *ControlPoints,
                                     u64 ControlPointCount,
                                     points_arrangement PointsArrangement,
                                     f32 *Ti,
                                     u64 NumOutputCurvePoints,
                                     local_position *OutputCurvePoints)
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
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
      
      EndTemp(Temp);
   }
}

internal void
CalcCubicSplineCurvePoints(local_position *ControlPoints,
                           u64 ControlPointCount,
                           cubic_spline_type CubicSplineType,
                           u64 NumOutputCurvePoints,
                           local_position *OutputCurvePoints)
{
   if (ControlPointCount > 0)
   {
      temp_arena Temp = TempArena(0);
      
      if (CubicSplineType == CubicSpline_Periodic)
      {
         local_position *OriginalControlPoints = ControlPoints;
         
         ControlPoints = PushArrayNonZero(Temp.Arena, ControlPointCount + 1, v2f32);
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
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
      
      EndTemp(Temp);
   }
}

internal void
CalcNormalBezierCurvePoints(local_position *ControlPoints,
                            u64 ControlPointCount,
                            u64 NumOutputCurvePoints,
                            local_position *OutputCurvePoints)
{
   f32 T = 0.0f;
   f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
   for (u64 OutputIndex = 0;
        OutputIndex < NumOutputCurvePoints;
        ++OutputIndex)
   {
      
      OutputCurvePoints[OutputIndex] =
         BezierCurveEvaluateFast(T,
                                 ControlPoints,
                                 ControlPointCount);
      
      T += Delta;
   }
}

internal void
CalcWeightedBezierCurvePoints(local_position *ControlPoints,
                              f32 *ControlPointWeights,
                              u64 ControlPointCount,
                              u64 NumOutputCurvePoints,
                              local_position *OutputCurvePoints)
{
   f32 T = 0.0f;
   f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
   for (u64 OutputIndex = 0;
        OutputIndex < NumOutputCurvePoints;
        ++OutputIndex)
   {
      
      OutputCurvePoints[OutputIndex] =
         BezierWeightedCurveEvaluateFast(T,
                                         ControlPoints,
                                         ControlPointWeights,
                                         ControlPointCount);
      
      T += Delta;
   }
}

internal void
CalcCubicBezierCurvePoints(local_position *CubicBezierPoints,
                           u64 ControlPointCount,
                           u64 NumOutputCurvePoints,
                           local_position *OutputCurvePoints)
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
         
         local_position *Segment = CubicBezierPoints + 1 + 3 * SegmentIndex;
         u64 NumSegmentPoints = (SegmentIndex == ControlPointCount - 1 ? 1 : 4);
         OutputCurvePoints[OutputIndex] = BezierCurveEvaluateFast(Segment_T,
                                                                  Segment,
                                                                  NumSegmentPoints);
         
         T += Delta;
      }
   }
}

internal void
EvaluateCurve(curve *Curve, u64 CurvePointCount, v2f32 *CurvePoints)
{
   temp_arena Temp = TempArena(0);
   curve_shape *CurveShape = &Curve->CurveParams.CurveShape;
   switch (CurveShape->InterpolationType)
   {
      case Interpolation_Polynomial: {
         polynomial_interpolation_params *Polynomial = &CurveShape->PolynomialInterpolationParams;
         
         f32 *Ti = PushArrayNonZero(Temp.Arena, Curve->ControlPointCount, f32);
         switch (Polynomial->PointsArrangement)
         {
            case PointsArrangement_Equidistant: { EquidistantPoints(Ti, Curve->ControlPointCount); } break;
            case PointsArrangement_Chebychev:   { ChebyshevPoints(Ti, Curve->ControlPointCount);   } break;
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
            
         }
      } break;
      
      case Interpolation_CubicSpline: {
         CalcCubicSplineCurvePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                    CurveShape->CubicSplineType,
                                    CurvePointCount, CurvePoints);
      } break;
      
      case Interpolation_Bezier: {
         switch (CurveShape->BezierType)
         {
            case Bezier_Normal: {
               CalcNormalBezierCurvePoints(Curve->ControlPoints, Curve->ControlPointCount,
                                           CurvePointCount, CurvePoints);
            } break;
            
            case Bezier_Weighted: {
               CalcWeightedBezierCurvePoints(Curve->ControlPoints, Curve->ControlPointWeights,
                                             Curve->ControlPointCount, CurvePointCount, CurvePoints);
            } break;
            
            case Bezier_Cubic: {
               CalcCubicBezierCurvePoints(Curve->CubicBezierPoints, Curve->ControlPointCount,
                                          CurvePointCount, CurvePoints);
            } break;
         }
      } break;
   }
   EndTemp(Temp);
}

internal void
RecomputeCurve(entity *Entity)
{
   TimeFunction;
   
   curve *Curve = GetCurve(Entity);
   temp_arena Temp = TempArena(Entity->Arena);
   ClearArena(Entity->Arena);
   
   u64 CurvePointCount = 0;
   if (Curve->ControlPointCount > 0)
   {
      CurvePointCount = (Curve->ControlPointCount - 1) * Curve->CurveParams.CurvePointCountPerSegment + 1;
   }
   Curve->CurvePointCount = CurvePointCount;
   Curve->CurvePoints = PushArrayNonZero(Entity->Arena, CurvePointCount, local_position);
   EvaluateCurve(Curve, Curve->CurvePointCount, Curve->CurvePoints);
   
   Curve->CurveVertices = CalculateLineVertices(Curve->CurvePointCount, Curve->CurvePoints, 
                                                Curve->CurveParams.CurveWidth, Curve->CurveParams.CurveColor,
                                                IsCurveLooped(Curve), LineVerticesAllocationArena(Entity->Arena));
   Curve->PolylineVertices = CalculateLineVertices(Curve->ControlPointCount, Curve->ControlPoints,
                                                   Curve->CurveParams.PolylineWidth, Curve->CurveParams.PolylineColor,
                                                   false, LineVerticesAllocationArena(Entity->Arena));
   local_position *ConvexHullPoints = PushArrayNonZero(Temp.Arena, Curve->ControlPointCount, local_position);
   u64 NumConvexHullPoints = CalcConvexHull(Curve->ControlPointCount, Curve->ControlPoints, ConvexHullPoints);
   Curve->ConvexHullVertices = CalculateLineVertices(NumConvexHullPoints, ConvexHullPoints,
                                                     Curve->CurveParams.ConvexHullWidth, Curve->CurveParams.ConvexHullColor,
                                                     true, LineVerticesAllocationArena(Entity->Arena));
   
   Curve->CurveVersion += 1;
   
   EndTemp(Temp);
}

internal local_position
WorldToLocalEntityPosition(entity *Entity, world_position Position)
{
   local_position Result = RotateAround(Position - Entity->Position, V2F32(0.0f, 0.0f),
                                        Rotation2DInverse(Entity->Rotation));
   return Result;
}

internal world_position
LocalEntityPositionToWorld(entity *Entity, local_position Position)
{
   world_position Result = RotateAround(Position, V2F32(0.0f, 0.0f), Entity->Rotation) + Entity->Position;
   return Result;
}

internal local_position
TranslateLocalEntityPositionInWorldSpace(entity *Entity, local_position Local, v2f32 Translation)
{
   world_position World = LocalEntityPositionToWorld(Entity, Local);
   world_position Translated = World + Translation;
   local_position Result = WorldToLocalEntityPosition(Entity, Translated);
   
   return Result;
}

internal void
SelectControlPoint(entity *Entity, u64 PointIndex)
{
   curve *Curve = GetCurve(Entity);
   if (PointIndex < Curve->ControlPointCount)
   {
      Curve->SelectedControlPointIndex = PointIndex;
   }
}

internal void
SetCurveControlPoints(entity *Entity,
                      u64 ControlPointCount,
                      local_position *ControlPoints,
                      f32 *ControlPointWeights,
                      local_position *CubicBezierPoints)
{
   curve *Curve = GetCurve(Entity);
   Assert(ControlPointCount <= MAX_CONTROL_POINT_COUNT);
   ControlPointCount = Min(ControlPointCount, MAX_CONTROL_POINT_COUNT);
   
   MemoryCopy(Curve->ControlPoints, ControlPoints, ControlPointCount * SizeOf(Curve->ControlPoints[0]));
   MemoryCopy(Curve->ControlPointWeights, ControlPointWeights, ControlPointCount * SizeOf(Curve->ControlPointWeights[0]));
   MemoryCopy(Curve->CubicBezierPoints, CubicBezierPoints, 3*ControlPointCount * SizeOf(Curve->CubicBezierPoints[0]));
   
   Curve->ControlPointCount = ControlPointCount;
   // TODO(hbr): Make sure this is expected, maybe add to internal arguments
   Curve->SelectedControlPointIndex = U64_MAX;
   
   RecomputeCurve(Entity);
}

// TODO(hbr): Probably just implement Append in terms of insert instead of making append special case of insert
typedef u64 added_point_index_u64;
internal u64
AppendCurveControlPoint(entity *Entity, world_position Point, f32 Weight)
{
   u64 AppendIndex = U64_MAX;
   
   curve *Curve = GetCurve(Entity);
   u64 PointCount = Curve->ControlPointCount;
   if (PointCount < MAX_CONTROL_POINT_COUNT)
   {
      local_position *Points = Curve->ControlPoints;
      f32 *Weights = Curve->ControlPointWeights;
      local_position PointLocal = WorldToLocalEntityPosition(Entity, Point);
      Points[PointCount] = PointLocal;
      Weights[PointCount] = Weight;
      
      local_position *CubicBeziers = Curve->CubicBezierPoints + (3 * PointCount + 1);
      // TODO(hbr): Improve this, but now works
      if (PointCount >= 2)
      {
         v2f32 S =
            2.0f * (Points[PointCount] - Points[PointCount - 1]) -
            0.5f * (Points[PointCount] - Points[PointCount - 2]);
         
         CubicBeziers[-1] = PointLocal - 1.0f/3.0f * S;
         CubicBeziers[ 0] = PointLocal;
         CubicBeziers[ 1] = PointLocal + 1.0f/3.0f * S;
      }
      else if (PointCount == 0)
      {
         CubicBeziers[-1] = PointLocal - V2F32(0.1f, 0.0f);
         CubicBeziers[ 0] = PointLocal;
         CubicBeziers[ 1] = PointLocal + V2F32(0.1f, 0.0f);
      }
      else // PointCount == 1
      {
         CubicBeziers[-1] = PointLocal - 1.0f/3.0f * (PointLocal - Points[PointCount - 1]);
         CubicBeziers[ 0] = PointLocal;
         CubicBeziers[ 1] = PointLocal + 1.0f/3.0f * (PointLocal - Points[PointCount - 1]);
      }
      
      AppendIndex = PointCount;
      ++Curve->ControlPointCount;
      
      RecomputeCurve(Entity);
   }
   
   return AppendIndex;
}

internal u64
CurveInsertControlPoint(entity *Entity, world_position Point, u64 InsertAfterIndex, f32 Weight)
{
   u64 InsertIndex = U64_MAX;
   
   curve *Curve = GetCurve(Entity);
   Assert(InsertAfterIndex < Curve->ControlPointCount);
   if (Curve->ControlPointCount < MAX_CONTROL_POINT_COUNT)
   {
      if (InsertAfterIndex == Curve->ControlPointCount - 1)
      {
         InsertIndex = AppendCurveControlPoint(Entity, Point, Weight);
      }
      else
      {
         local_position PointLocal = WorldToLocalEntityPosition(Entity, Point);
         
         u64 PointsAfter = (Curve->ControlPointCount - InsertAfterIndex - 1);
         MemoryMove(Curve->ControlPoints + InsertAfterIndex + 2,
                    Curve->ControlPoints + InsertAfterIndex + 1,
                    PointsAfter * SizeOf(Curve->ControlPoints[0]));
         Curve->ControlPoints[InsertAfterIndex + 1] = PointLocal;
         
         MemoryMove(Curve->ControlPointWeights + InsertAfterIndex + 2,
                    Curve->ControlPointWeights + InsertAfterIndex + 1,
                    PointsAfter * SizeOf(Curve->ControlPointWeights[0]));
         Curve->ControlPointWeights[InsertAfterIndex + 1] = Weight;
         
         MemoryMove(Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 2),
                    Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 1),
                    3 * PointsAfter * SizeOf(Curve->CubicBezierPoints[0]));
         Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 0] = PointLocal - 1.0f/6.0f * (Curve->ControlPoints[InsertAfterIndex + 2] - Curve->ControlPoints[InsertAfterIndex]);
         Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 1] = PointLocal;
         Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 2] = PointLocal + 1.0f/6.0f * (Curve->ControlPoints[InsertAfterIndex + 2] - Curve->ControlPoints[InsertAfterIndex]);
         
         InsertIndex = InsertAfterIndex + 1;
         ++Curve->ControlPointCount;
         
         if (Curve->SelectedControlPointIndex != U64_MAX)
         {
            if (Curve->SelectedControlPointIndex > InsertAfterIndex)
            {
               SelectControlPoint(Entity, Curve->SelectedControlPointIndex + 1);
            }
         }
         
         RecomputeCurve(Entity);
      }
   }
   
   return InsertIndex;
}

internal void
RemoveCurveControlPoint(entity *Entity, u64 PointIndex)
{
   curve *Curve = GetCurve(Entity);
   if (PointIndex < Curve->ControlPointCount)
   {
      u64 PointsAfter = (Curve->ControlPointCount - PointIndex - 1);
      MemoryMove(Curve->ControlPoints + PointIndex,
                 Curve->ControlPoints + PointIndex + 1,
                 PointsAfter * SizeOf(Curve->ControlPoints[0]));
      MemoryMove(Curve->ControlPointWeights + PointIndex,
                 Curve->ControlPointWeights + PointIndex + 1,
                 PointsAfter * SizeOf(Curve->ControlPointWeights[0]));
      MemoryMove(Curve->CubicBezierPoints + 3 * PointIndex,
                 Curve->CubicBezierPoints + 3 * (PointIndex + 1),
                 3 * PointsAfter * SizeOf(Curve->CubicBezierPoints[0]));
      
      --Curve->ControlPointCount;
      
      if (PointIndex == Curve->SelectedControlPointIndex)
      {
         Curve->SelectedControlPointIndex = U64_MAX;
      }
      else if (PointIndex < Curve->SelectedControlPointIndex)
      {
         SelectControlPoint(Entity, Curve->SelectedControlPointIndex - 1);
      }
      
      RecomputeCurve(Entity);
   }
}

internal void
SetCurveControlPoint(entity *Entity, u64 PointIndex, local_position Point, f32 Weight)
{
   curve *Curve = GetCurve(Entity);
   if (PointIndex < Curve->ControlPointCount)
   {
      v2f32 Translation = Point - Curve->ControlPoints[PointIndex];
      
      Curve->ControlPoints[PointIndex] = Point;
      Curve->ControlPointWeights[PointIndex] = Weight;
      
      local_position *CubicBeziers = Curve->CubicBezierPoints + (3 * PointIndex + 1);
      CubicBeziers[-1] += Translation;
      CubicBeziers[0] = Point;
      CubicBeziers[1] += Translation;
      
      RecomputeCurve(Entity);
   }
}

internal void
TranslateCurveControlPoint(entity *Entity, u64 PointIndex,
                           translate_control_point_flags Flags,
                           v2f32 TranslationWorld)
{
   curve *Curve = GetCurve(Entity);
   if (Flags & TranslateControlPoint_BezierPoint)
   {
      if (PointIndex < 3 * Curve->ControlPointCount)
      {
         local_position *Points = Curve->CubicBezierPoints;
         local_position *Point = Points + PointIndex;
         
         local_position CenterPoint = {};
         local_position *TwinPoint = 0;
         u64 PointOffset = PointIndex % 3;
         if (PointOffset == 2)
         {
            CenterPoint = Points[PointIndex - 1];
            TwinPoint = Points + (PointIndex - 2);
         }
         else if (PointOffset == 0)
         {
            CenterPoint = Points[PointIndex + 1];
            TwinPoint = Points + (PointIndex + 2);
         }
         else
         {
            Assert(!"Expected cubic bezier point index");
         }
         
         local_position TranslatedPoint = TranslateLocalEntityPositionInWorldSpace(Entity, *Point, TranslationWorld);
         *Point = TranslatedPoint;
         
         v2f32 DesiredTwinDirection = ((Flags & TranslateControlPoint_MatchBezierTwinDirection) ?
                                       (CenterPoint - TranslatedPoint) :
                                       (*TwinPoint - CenterPoint));
         Normalize(&DesiredTwinDirection);
         f32 DesiredTwinLength = ((Flags & TranslateControlPoint_MatchBezierTwinLength) ?
                                  Norm(CenterPoint - TranslatedPoint) :
                                  Norm(*TwinPoint - CenterPoint));
         *TwinPoint = CenterPoint + DesiredTwinLength * DesiredTwinDirection;
         
         RecomputeCurve(Entity);
      }
   }
   else
   {
      if (PointIndex < Curve->ControlPointCount)
      {
         local_position TranslatedPoint =
            TranslateLocalEntityPositionInWorldSpace(Entity,
                                                     Curve->ControlPoints[PointIndex],
                                                     TranslationWorld);
         SetCurveControlPoint(Entity, PointIndex, TranslatedPoint,
                              Curve->ControlPointWeights[PointIndex]);
      }
   }
}

internal void
InitCurve(entity *Entity,
          curve_params CurveParams, u64 SelectedControlPointIndex,
          u64 ControlPointCount,
          world_position *ControlPoints, f32 *ControlPointWeights, world_position *CubicBezierPoints)
{
   Entity->Type = Entity_Curve;
   Entity->Curve.CurveParams = CurveParams;
   SetCurveControlPoints(Entity, ControlPointCount, ControlPoints, ControlPointWeights, CubicBezierPoints);
   Entity->Curve.SelectedControlPointIndex = SelectedControlPointIndex;
}

internal void
InitCurveFromCurve(entity *Dst, curve *Src)
{
   InitCurve(Dst,
             Src->CurveParams, U64_MAX,
             Src->ControlPointCount,
             Src->ControlPoints, Src->ControlPointWeights, Src->CubicBezierPoints);
}

internal void
InitImage(image *Image, string ImageFilePath, sf::Texture *Texture)
{
   Image->FilePath = DuplicateStr(ImageFilePath);
   new (&Image->Texture) sf::Texture(*Texture);
}

internal void
InitImageFromImage(entity *Dst, image *Src)
{
   new (&Dst->Image.Texture) sf::Texture(Src->Texture);
   Dst->Image.FilePath = DuplicateStr(Src->FilePath);
}

internal void
InitEntity(entity *Entity, world_position Position, v2f32 Scale,
           rotation_2d Rotation, name_string Name, s64 SortingLayer,
           u64 RenamingFrame, entity_flags Flags)
{
   Entity->Position = Position;
   Entity->Scale = Scale;
   Entity->Rotation = Rotation;
   Entity->Name = Name;
   Entity->SortingLayer = SortingLayer;
   Entity->Flags = Flags;
}

internal void
InitEntityFromEntity(entity *Dest, entity *Src)
{
   InitEntity(Dest, Src->Position, Src->Scale, Src->Rotation,
              Src->Name, Src->SortingLayer, Src->RenamingFrame,
              Src->Flags);
   switch (Src->Type)
   {
      case Entity_Curve: { InitCurveFromCurve(Dest, &Src->Curve); } break;
      case Entity_Image: { InitImageFromImage(Dest, &Src->Image); } break;
   }
}

internal void
InitCurveEntity(entity *Entity,
                world_position Position, v2f32 Scale, rotation_2d Rotation,
                name_string Name,
                curve_params CurveParams)
{
   InitEntity(Entity, Position, Scale, Rotation, Name, 0, 0, 0);
   InitCurve(Entity, CurveParams, U64_MAX, 0, 0, 0, 0);
}

internal void
InitImageEntity(entity *Entity,
                world_position Position, v2f32 Scale, rotation_2d Rotation,
                name_string Name,
                string ImageFilePath, sf::Texture *Texture)
{
   InitEntity(Entity, Position, Scale, Rotation, Name, 0, 0, 0);
   InitImage(&Entity->Image, ImageFilePath, Texture);
}
