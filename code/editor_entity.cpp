//- Name String
function name_string
NameStringMake(char *Str, u64 Size)
{
   name_string Result = {};
   
   u64 FinalSize = Minimum(Size, ArrayCount(Result.Str)-1);
   MemoryCopy(Result.Str, Str, FinalSize);
   Result.Str[FinalSize] = 0;
   
   Result.Size = FinalSize;
   
   return Result;
}

function name_string
NameStringFromString(string String)
{
   u64 Size = StringSize(String);
   name_string Result = NameStringMake(String, Size);
   
   return Result;
}

function name_string
NameStringFormat(char const *Format, ...)
{
   name_string Result = {};
   
   va_list ArgList;
   va_start(ArgList, Format);
   
   int Return = vsnprintf(Result.Str, ArrayCount(Result.Str)-1, Format, ArgList);
   if (Return >= 0)
   {
      Result.Size = CStringLength(Result.Str);
   }
   
   va_end(ArgList);
   
   return Result;
}

function curve_shape
CurveShapeMake(interpolation_type InterpolationType,
               polynomial_interpolation_type PolynomialInterpolationType,
               points_arrangement PointsArrangement,
               cubic_spline_type CubicSplineType,
               bezier_type BezierType)
{
   curve_shape Result = {};
   Result.InterpolationType = InterpolationType;
   Result.PolynomialInterpolationParams.Type = PolynomialInterpolationType;
   Result.PolynomialInterpolationParams.PointsArrangement = PointsArrangement;
   Result.CubicSplineType = CubicSplineType;
   Result.BezierType = BezierType;
   
   return Result;
}

function curve_params
CurveParamsMake(curve_shape CurveShape,
                f32 CurveWidth, color CurveColor,
                b32 PointsDisabled, f32 PointSize,
                color PointColor, b32 PolylineEnabled,
                f32 PolylineWidth, color PolylineColor,
                b32 ConvexHullEnabled, f32 ConvexHullWidth,
                color ConvexHullColor, u64 NumCurvePointsPerSegment,
                color DeCasteljauVisualizationGradientA,
                color DeCasteljauVisualizationGradientB)
{
   curve_params Result = {};
   Result.CurveShape = CurveShape;
   Result.CurveWidth = CurveWidth;
   Result.CurveColor = CurveColor;
   Result.PointsDisabled = PointsDisabled;
   Result.PointSize = PointSize;
   Result.PointColor = PointColor;
   Result.PolylineEnabled = PolylineEnabled;
   Result.PolylineWidth = PolylineWidth;
   Result.PolylineColor = PolylineColor;
   Result.ConvexHullEnabled = ConvexHullEnabled;
   Result.ConvexHullWidth = ConvexHullWidth;
   Result.ConvexHullColor = ConvexHullColor;
   Result.NumCurvePointsPerSegment = NumCurvePointsPerSegment;
   Result.DeCasteljau.GradientA = DeCasteljauVisualizationGradientA;
   Result.DeCasteljau.GradientB = DeCasteljauVisualizationGradientB;
   
   return Result;
}

//- Curve
function curve
CurveMake(name_string CurveName,
          curve_params CurveParams,
          u64 SelectedControlPointIndex,
          world_position Position,
          rotation_2d Rotation)
{
   curve Result = {};
   Result.Name = CurveName;
   Result.CurveParams = CurveParams;
   Result.SelectedControlPointIndex = SelectedControlPointIndex;
   Result.Position = Position;
   Result.Rotation = Rotation;
   
   return Result;
}

function void
CurveDestroy(curve *Curve)
{
   ArrayFree(Curve->ControlPoints);
   Curve->NumControlPoints = 0;
   Curve->CapControlPoints = 0;
   
   ArrayFree(Curve->ControlPointWeights);
   Curve->CapControlPointWeights = 0;
   
   ArrayFree(Curve->CubicBezierPoints);
   Curve->CapCubicBezierPoints = 0;
   
   ArrayFree(Curve->CurvePoints);
   Curve->NumCurvePoints = 0;
   
   ArrayFree(Curve->CurveVertices.Vertices);
   Curve->CurveVertices.NumVertices = 0;
   Curve->CurveVertices.CapVertices = 0;
   
   ArrayFree(Curve->PolylineVertices.Vertices);
   Curve->PolylineVertices.NumVertices = 0;
   Curve->PolylineVertices.CapVertices = 0;
   
   ArrayFree(Curve->ConvexHullVertices.Vertices);
   Curve->ConvexHullVertices.NumVertices = 0;
   Curve->ConvexHullVertices.CapVertices = 0;
}

function curve
CurveCopy(curve Curve)
{
   curve Copy = CurveMake(Curve.Name,
                          Curve.CurveParams,
                          Curve.SelectedControlPointIndex,
                          Curve.Position, Curve.Rotation);
   
   CurveSetControlPoints(&Copy,
                         Curve.NumControlPoints,
                         Curve.ControlPoints,
                         Curve.ControlPointWeights,
                         Curve.CubicBezierPoints);
   
   return Copy;
}

function added_point_index
CurveAppendControlPoint(curve *Curve,
                        world_position ControlPoint,
                        f32 ControlPointWeight)
{
   u64 NumControlPoints = Curve->NumControlPoints;
   
   local_position ControlPointLocal = WorldToLocalCurvePosition(Curve, ControlPoint);
   
   ArrayReserveCopy(Curve->ControlPoints,
                    Curve->CapControlPoints,
                    NumControlPoints + 1);
   Curve->ControlPoints[NumControlPoints] = ControlPointLocal;
   
   ArrayReserveCopy(Curve->ControlPointWeights,
                    Curve->CapControlPointWeights,
                    NumControlPoints + 1);
   Curve->ControlPointWeights[Curve->NumControlPoints] = ControlPointWeight;
   
   ArrayReserveCopy(Curve->CubicBezierPoints,
                    Curve->CapCubicBezierPoints,
                    3 * (NumControlPoints + 1));
   
   local_position *ControlPoints = Curve->ControlPoints;
   local_position *CubicBezierPoints = Curve->CubicBezierPoints;
   
   // TODO(hbr): remove
#if 0
   
   if (NumControlPoints >= 2)
   {
      local_position S =
         2.0f * (ControlPoints[NumControlPoints] - ControlPoints[NumControlPoints-1]) -
         0.5f * (ControlPoints[NumControlPoints] - ControlPoints[NumControlPoints-2]);
      
      Assert(NumCubicBezierPoints >= 2);
      CubicBezierPoints[NumCubicBezierPoints + 0] =
         CubicBezierPoints[NumCubicBezierPoints - 1] - (CubicBezierPoints[NumCubicBezierPoints - 2] -
                                                        CubicBezierPoints[NumCubicBezierPoints - 1]);
      CubicBezierPoints[NumCubicBezierPoints + 1] = ControlPointLocal - 1.0f/3.0f * S;
      CubicBezierPoints[NumCubicBezierPoints + 2] = ControlPointLocal;
      NumCubicBezierPoints += 3;
   }
   else if (NumControlPoints == 0)
   {
      Assert(NumCubicBezierPoints == 0);
      CubicBezierPoints[NumCubicBezierPoints + 0] = ControlPointLocal;
      NumCubicBezierPoints += 1;
   }
   else // if (NumControlPoints == 1)
   {
      Assert(NumCubicBezierPoints == 1);
      CubicBezierPoints[NumCubicBezierPoints + 0] = CubicBezierPoints[NumCubicBezierPoints - 1] +
         1.0f/3.0f * (ControlPointLocal - CubicBezierPoints[NumCubicBezierPoints - 1]);
      CubicBezierPoints[NumCubicBezierPoints + 1] = ControlPointLocal -
         1.0f/3.0f * (ControlPointLocal - CubicBezierPoints[NumCubicBezierPoints - 1]);
      CubicBezierPoints[NumCubicBezierPoints + 2] = ControlPointLocal;
      NumCubicBezierPoints += 3;
   }
   
#else
   
   local_position *NewCubicBezierPoints = CubicBezierPoints + 3 * NumControlPoints + 1;
   
   // TODO(hbr): Improve this, but now works
   if (NumControlPoints >= 2)
   {
      v2f32 S =
         2.0f * (ControlPoints[NumControlPoints] - ControlPoints[NumControlPoints - 1]) -
         0.5f * (ControlPoints[NumControlPoints] - ControlPoints[NumControlPoints - 2]);
      
      NewCubicBezierPoints[-1] = ControlPointLocal - 1.0f/3.0f * S;
      NewCubicBezierPoints[ 0] = ControlPointLocal;
      NewCubicBezierPoints[ 1] = ControlPointLocal + 1.0f/3.0f * S;
   }
   else if (NumControlPoints == 0)
   {
      NewCubicBezierPoints[-1] = ControlPointLocal - V2F32(0.1f, 0.0f);
      NewCubicBezierPoints[ 0] = ControlPointLocal;
      NewCubicBezierPoints[ 1] = ControlPointLocal + V2F32(0.1f, 0.0f);
   }
   else // NumControlPoints == 1
   {
      NewCubicBezierPoints[-1] = ControlPointLocal - 1.0f/3.0f * (ControlPointLocal - ControlPoints[NumControlPoints - 1]);
      NewCubicBezierPoints[ 0] = ControlPointLocal;
      NewCubicBezierPoints[ 1] = ControlPointLocal + 1.0f/3.0f * (ControlPointLocal - ControlPoints[NumControlPoints - 1]);
   }
   
#endif
   
   Curve->NumControlPoints = NumControlPoints + 1;
   
   CurveRecompute(Curve);
   
   return NumControlPoints;
}

function void
CurveInsertControlPoint(curve *Curve,
                        world_position ControlPoint,
                        u64 InsertAfterIndex,
                        f32 ControlPointWeight)
{
   u64 NumControlPoints = Curve->NumControlPoints;
   Assert(InsertAfterIndex < NumControlPoints);
   
   if (InsertAfterIndex == NumControlPoints-1)
   {
      CurveAppendControlPoint(Curve,
                              ControlPoint,
                              ControlPointWeight);
   }
   else
   {
      local_position ControlPointLocal = WorldToLocalCurvePosition(Curve, ControlPoint);
      ArrayReserveCopy(Curve->ControlPoints, Curve->CapControlPoints, NumControlPoints + 1);
      MemoryMove(Curve->ControlPoints + InsertAfterIndex + 2,
                 Curve->ControlPoints + InsertAfterIndex + 1,
                 (NumControlPoints - InsertAfterIndex - 1) * sizeof(Curve->ControlPoints[0]));
      Curve->ControlPoints[InsertAfterIndex + 1] = ControlPointLocal;
      
      ArrayReserveCopy(Curve->ControlPointWeights, Curve->CapControlPointWeights, NumControlPoints + 1);
      MemoryMove(Curve->ControlPointWeights + InsertAfterIndex + 2,
                 Curve->ControlPointWeights + InsertAfterIndex + 1,
                 (NumControlPoints - InsertAfterIndex - 1) * sizeof(Curve->ControlPointWeights[0]));
      Curve->ControlPointWeights[InsertAfterIndex + 1] = ControlPointWeight;
      
      ArrayReserveCopy(Curve->CubicBezierPoints, Curve->CapCubicBezierPoints, 3 * (NumControlPoints + 1));
      MemoryMove(Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 2),
                 Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 1),
                 3 * (NumControlPoints - InsertAfterIndex - 1) * sizeof(Curve->CubicBezierPoints[0]));
      Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 0] = ControlPointLocal - 1.0f/6.0f * (Curve->ControlPoints[InsertAfterIndex + 2] - Curve->ControlPoints[InsertAfterIndex]);
      Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 1] = ControlPointLocal;
      Curve->CubicBezierPoints[3 * (InsertAfterIndex + 1) + 2] = ControlPointLocal + 1.0f/6.0f * (Curve->ControlPoints[InsertAfterIndex + 2] - Curve->ControlPoints[InsertAfterIndex]);
      
      Curve->NumControlPoints = NumControlPoints + 1;
      
      if (Curve->SelectedControlPointIndex != U64_MAX)
      {
         if (Curve->SelectedControlPointIndex > InsertAfterIndex)
         {
            Curve->SelectedControlPointIndex += 1;
         }
      }
      
      CurveRecompute(Curve);
   }
}

function void
CurveRemoveControlPoint(curve *Curve, u64 ControlPointIndex)
{
   u64 NumControlPoints = Curve->NumControlPoints;
   
   MemoryMove(Curve->ControlPoints + ControlPointIndex,
              Curve->ControlPoints + ControlPointIndex + 1,
              (NumControlPoints - ControlPointIndex - 1) * sizeof(Curve->ControlPoints[0]));
   MemoryMove(Curve->ControlPointWeights + ControlPointIndex,
              Curve->ControlPointWeights + ControlPointIndex + 1,
              (NumControlPoints - ControlPointIndex - 1) * sizeof(Curve->ControlPointWeights[0]));
   MemoryMove(Curve->CubicBezierPoints + 3 * ControlPointIndex,
              Curve->CubicBezierPoints + 3 * (ControlPointIndex + 1),
              3 * (NumControlPoints - ControlPointIndex - 1) * sizeof(Curve->CubicBezierPoints[0]));
   
   Curve->NumControlPoints = NumControlPoints - 1;
   
   if (Curve->SelectedControlPointIndex != U64_MAX)
   {
      if (ControlPointIndex == Curve->SelectedControlPointIndex)
      {
         Curve->SelectedControlPointIndex = U64_MAX;
      }
      else if (ControlPointIndex < Curve->SelectedControlPointIndex)
      {
         Curve->SelectedControlPointIndex -= 1;
      }
   }
   
   CurveRecompute(Curve);
}

function void
CurveSetControlPoints(curve *Curve, u64 NewNumControlPoints,
                      local_position *NewControlPoints,
                      f32 *NewControlPointWeights,
                      local_position *NewCubicBezierPoints)
{
   ArrayReserve(Curve->ControlPoints, Curve->CapControlPoints, NewNumControlPoints);
   ArrayReserve(Curve->ControlPointWeights, Curve->CapControlPointWeights, NewNumControlPoints);
   ArrayReserve(Curve->CubicBezierPoints, Curve->CapCubicBezierPoints, 3 * NewNumControlPoints);
   
   MemoryCopy(Curve->ControlPoints,
              NewControlPoints,
              NewNumControlPoints * sizeof(Curve->ControlPoints[0]));
   MemoryCopy(Curve->ControlPointWeights,
              NewControlPointWeights,
              NewNumControlPoints * sizeof(Curve->ControlPointWeights[0]));
   MemoryCopy(Curve->CubicBezierPoints,
              NewCubicBezierPoints,
              3 * NewNumControlPoints * sizeof(Curve->CubicBezierPoints[0]));
   
   Curve->NumControlPoints = NewNumControlPoints;
   // TODO(hbr): Make sure this is expected, maybe add to function arguments
   Curve->SelectedControlPointIndex = U64_MAX;
   
   CurveRecompute(Curve);
}

internal local_position
TranslateLocalCurvePositionInWorldSpace(curve *Curve, local_position Local, v2f32 Translation)
{
   world_position World = LocalCurvePositionToWorld(Curve, Local);
   world_position Translated = World + Translation;
   local_position Result = WorldToLocalCurvePosition(Curve, Translated);
   
   return Result;
}

function void
CurveTranslatePoint(curve *Curve, u64 PointIndex, b32 IsCubicBezierPoint,
                    b32 MatchCubicBezierTwinDirection, b32 MatchCubicBezierTwinLength,
                    v2f32 TranslationWorld)
{
   if (IsCubicBezierPoint)
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
         Assert(false);
      }
      
      local_position TranslatedPoint =
         TranslateLocalCurvePositionInWorldSpace(Curve,
                                                 *Point,
                                                 TranslationWorld);
      *Point = TranslatedPoint;
      
      v2f32 DesiredTwinDirection = (MatchCubicBezierTwinDirection ?
                                    CenterPoint - TranslatedPoint:
                                    *TwinPoint - CenterPoint);
      Normalize(&DesiredTwinDirection);
      f32 DesiredTwinLength = (MatchCubicBezierTwinLength ?
                               Norm(CenterPoint - TranslatedPoint) :
                               Norm(*TwinPoint - CenterPoint));
      *TwinPoint = CenterPoint + DesiredTwinLength * DesiredTwinDirection;
      
      CurveRecompute(Curve);
   }
   else
   {
      Assert(PointIndex < Curve->NumControlPoints);
      
      local_position TranslatedPoint =
         TranslateLocalCurvePositionInWorldSpace(Curve,
                                                 Curve->ControlPoints[PointIndex],
                                                 TranslationWorld);
      CurveSetControlPoint(Curve, PointIndex, TranslatedPoint,
                           Curve->ControlPointWeights[PointIndex]);
   }
}

function void
CurveSetControlPoint(curve *Curve, u64 ControlPointIndex, local_position ControlPoint,
                     f32 ControlPointWeight)
{
   Assert(ControlPointIndex < Curve->NumControlPoints);
   
   v2f32 Translation = ControlPoint - Curve->ControlPoints[ControlPointIndex];
   
   Curve->ControlPoints[ControlPointIndex] = ControlPoint;
   Curve->ControlPointWeights[ControlPointIndex] = ControlPointWeight;
   
   local_position *CubicBezierPoint = Curve->CubicBezierPoints + 3 * ControlPointIndex + 1;
   CubicBezierPoint[-1] += Translation;
   CubicBezierPoint[0] = ControlPoint;
   CubicBezierPoint[1] += Translation;
   
   CurveRecompute(Curve);
}

function void
CurveRotateAround(curve *Curve, world_position Center, rotation_2d Rotation)
{
   Curve->Position = RotateAround(Curve->Position, Center, Rotation);
   Curve->Rotation = CombineRotations2D(Curve->Rotation, Rotation);
}

// NOTE(hbr): Trinale-based, no mitter, no-spiky line version.
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   u64 NumVertices = 10 * NumLinePoints + 100;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   u64 VertexIndex = 0;
   for (u64 PointIndex = 1;
        PointIndex + 1 < NumLinePoints;
        ++PointIndex)
   {
      v2f32 A = LinePoints[PointIndex - 1];
      v2f32 B = LinePoints[PointIndex + 0];
      v2f32 C = LinePoints[PointIndex + 1];
      
      v2f32 V_Line = B - A;
      Normalize(&V_Line);
      
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2f32 V_Succ = C - B;
      Normalize(&V_Succ);
      v2f32 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      v2f32 V_Miter = NV_Line + NV_Succ;
      Normalize(&V_Miter);
      
      if (Cross(B - A, C - B) >= 0.0f) // left-turn
      {
         line_intersection Intersection = LineIntersection(A + 0.5f * LineWidth * NV_Line,
                                                           B + 0.5f * LineWidth * NV_Line,
                                                           B + 0.5f * LineWidth * NV_Succ,
                                                           C + 0.5f * LineWidth * NV_Succ);
         
         v2f32 Position1 = {};
         if (Intersection.IsOneIntersection)
         {
            Position1 = Intersection.IntersectionPoint;
         }
         else
         {
            Position1 = B + 0.5f * LineWidth * NV_Line;
         }
         v2f32 Position2 = B - 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B - 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position3);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
      else // right-turn
      {
         line_intersection Intersection = LineIntersection(A - 0.5f * LineWidth * NV_Line,
                                                           B - 0.5f * LineWidth * NV_Line,
                                                           B - 0.5f * LineWidth * NV_Succ,
                                                           C - 0.5f * LineWidth * NV_Succ);
         
         v2f32 Position1 = {};
         if (Intersection.IsOneIntersection)
         {
            Position1 = Intersection.IntersectionPoint;
         }
         else
         {
            Position1 = B - 0.5f * LineWidth * NV_Line;
         }
         v2f32 Position2 = B + 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B + 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position3);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
   }
   
   NumVertices = VertexIndex;
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

// NOTE(hbr): Triangle-based, mitter, non-spiky line version.
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   u64 NumVertices = 10 * NumLinePoints + 100;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   u64 VertexIndex = 0;
   for (u64 PointIndex = 1;
        PointIndex + 1 < NumLinePoints;
        ++PointIndex)
   {
      v2f32 A = LinePoints[PointIndex - 1];
      v2f32 B = LinePoints[PointIndex + 0];
      v2f32 C = LinePoints[PointIndex + 1];
      
      v2f32 V_Line = B - A;
      Normalize(&V_Line);
      
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2f32 V_Succ = C - B;
      Normalize(&V_Succ);
      v2f32 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      v2f32 V_Miter = NV_Line + NV_Succ;
      Normalize(&V_Miter);
      
      if (Cross(B - A, C - B) >= 0.0f) // left-turn
      {
         v2f32 Position1 = B + 0.5f * LineWidth / Dot(V_Miter, NV_Line) * V_Miter;
         v2f32 Position2 = B - 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B - 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position3);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
      else // right-turn
      {
         v2f32 Position1 = B - 0.5f * LineWidth / Dot(V_Miter, NV_Line) * V_Miter;
         v2f32 Position2 = B + 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B + 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position3);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
   }
   
   NumVertices = VertexIndex;
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

// NOTE(hbr): Triangle-based, mitter, spike line version
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   if (NumLinePoints == 0)
   {
      return OldLineVertices;
   }
   
   u64 N = NumLinePoints;
   u64 NumVertices = 6 * (N-1);
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   if (NumLinePoints >= 2)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      v2f32 *LinePointsLooped = PushArray(Scratch.Arena, NumLinePoints + 3, v2f32);
      for (u64 PointIndex = 0;
           PointIndex < NumLinePoints;
           ++PointIndex)
      {
         LinePointsLooped[PointIndex + 1] = LinePoints[PointIndex];
      }
      
#if 0
      LinePointsLooped[0] = LinePoints[NumLinePoints - 1];
      LinePointsLooped[NumLinePoints + 1] = LinePoints[0];
#else
      LinePointsLooped[0] = LinePoints[0];
      LinePointsLooped[NumLinePoints + 1] = LinePoints[NumLinePoints - 1];
#endif
      //LinePointsLooped[NumLinePoints + 2] = LinePoints[1];
      
      u64 VertexIndex = 0;
      for (u64 LineIndex = 0;
           LineIndex < N-1;
           ++LineIndex)
      {
         v2f32 Points[4] = {};
         for (u64 I = 0; I < 4; ++I) Points[I] = LinePointsLooped[LineIndex + I];
         
         for (u64 TriIndex = 0;
              TriIndex < 6;
              ++TriIndex)
         {
            v2f32 V_Line = Points[2] - Points[1];
            Normalize(&V_Line);
            
            v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
            
            v2f32 Position = {};
            if (TriIndex == 0 || TriIndex == 1 || TriIndex == 3)
            {
               v2f32 V_Pred = Points[1] - Points[0];
               Normalize(&V_Pred);
               
               v2f32 V_Miter = NV_Line + Rotate90DegreesAntiClockwise(V_Pred);
               Normalize(&V_Miter);
               
               Position = Points[1] + LineWidth * (TriIndex == 1 ? -0.5f : 0.5f) / Dot(V_Miter, NV_Line) * V_Miter;
            }
            else
            {
               v2f32 V_Succ = Points[3]  - Points[2];
               Normalize(&V_Succ);
               
               v2f32 V_Miter = NV_Line + Rotate90DegreesAntiClockwise(V_Succ);
               Normalize(&V_Miter);
               
               Position = Points[2] + LineWidth * (TriIndex == 5 ? 0.5f : -0.5f) / Dot(V_Miter, NV_Line) * V_Miter;
            }
            
            Vertices[VertexIndex].position = V2F32ToVector2f(Position);
            Vertices[VertexIndex].color = ColorToSFMLColor(LineColor);
            VertexIndex += 1;
         }
      }
      Assert(VertexIndex == NumVertices);
   }
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

struct points_soa
{
   f32 *Xs;
   f32 *Ys;
};

internal points_soa
SplitPointsIntoComponents(arena *Arena, v2f32 *Points, u64 NumPoints)
{
   points_soa Result = {};
   Result.Xs = PushArray(Arena, NumPoints, f32);
   Result.Ys = PushArray(Arena, NumPoints, f32);
   
   for (u64 I = 0; I < NumPoints; ++I)
   {
      Result.Xs[I] = Points[I].X;
      Result.Ys[I] = Points[I].Y;
   }
   
   return Result;
}

internal void
PolynomialInterpolationNewtonCalculateCurvePoints(local_position *ControlPoints,
                                                  u64 NumControlPoints, f32 *Ti,
                                                  u64 NumOutputCurvePoints,
                                                  local_position *OutputCurvePoints)
{
   if (NumControlPoints > 0)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      points_soa SOA = SplitPointsIntoComponents(Scratch.Arena,
                                                 ControlPoints,
                                                 NumControlPoints);
      
      f32 *Beta_X = PushArray(Scratch.Arena, NumControlPoints, f32);
      f32 *Beta_Y = PushArray(Scratch.Arena, NumControlPoints, f32);
      NewtonBetaFast(Beta_X, Ti, SOA.Xs, NumControlPoints);
      NewtonBetaFast(Beta_Y, Ti, SOA.Ys, NumControlPoints);
      
      f32 Begin = Ti[0];
      f32 End = Ti[NumControlPoints - 1];
      f32 T = Begin;
      f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
      
      for (u64 OutputIndex = 0;
           OutputIndex < NumOutputCurvePoints;
           ++OutputIndex)
      {
         f32 X = NewtonEvaluate(T, Beta_X, Ti, NumControlPoints);
         f32 Y = NewtonEvaluate(T, Beta_Y, Ti, NumControlPoints);
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
   }
}

internal void
PolynomialInterpolationBarycentricCalculateCurvePoints(local_position *ControlPoints,
                                                       u64 NumControlPoints,
                                                       points_arrangement PointsArrangement,
                                                       f32 *Ti, u64 NumOutputCurvePoints,
                                                       local_position *OutputCurvePoints)
{
   if (NumControlPoints > 0)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      f32 *Omega = PushArray(Scratch.Arena, NumControlPoints, f32);
      switch (PointsArrangement)
      {
         case PointsArrangement_Equidistant: {
            // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
            BarycentricOmega(Omega, Ti, NumControlPoints);
         } break;
         case PointsArrangement_Chebychev: {
            BarycentricOmegaChebychev(Omega, NumControlPoints);
         } break;
      }
      
      points_soa SOA = SplitPointsIntoComponents(Scratch.Arena,
                                                 ControlPoints,
                                                 NumControlPoints);
      
      f32 Begin = Ti[0];
      f32 End = Ti[NumControlPoints - 1];
      f32 T = Begin;
      f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
      
      for (u64 OutputIndex = 0;
           OutputIndex < NumOutputCurvePoints;
           ++OutputIndex)
      {
         f32 X = BarycentricEvaluate(T, Omega, Ti, SOA.Xs, NumControlPoints);
         f32 Y = BarycentricEvaluate(T, Omega, Ti, SOA.Ys, NumControlPoints);
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
   }
}

internal void
CubicSplineCalculateCurvePoints(cubic_spline_type CubicSplineType,
                                local_position *ControlPoints,
                                u64 NumControlPoints,
                                u64 NumOutputCurvePoints,
                                local_position *OutputCurvePoints)
{
   if (NumControlPoints > 0)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      if (CubicSplineType == CubicSpline_Periodic)
      {
         local_position *OriginalControlPoints = ControlPoints;
         
         ControlPoints = PushArray(Scratch.Arena, NumControlPoints + 1, v2f32);
         MemoryCopy(ControlPoints,
                    OriginalControlPoints,
                    NumControlPoints * sizeof(ControlPoints[0]));
         ControlPoints[NumControlPoints] = OriginalControlPoints[0];
         
         NumControlPoints += 1;
      }
      
      f32 *Ti = PushArray(Scratch.Arena, NumControlPoints, f32);
      EquidistantPoints(Ti, NumControlPoints);
      
      points_soa SOA = SplitPointsIntoComponents(Scratch.Arena, ControlPoints, NumControlPoints);
      
      f32 *Mx = PushArray(Scratch.Arena, NumControlPoints, f32);
      f32 *My = PushArray(Scratch.Arena, NumControlPoints, f32);
      switch (CubicSplineType)
      {
         case CubicSpline_Natural: {
            CubicSplineNaturalM(Mx, Ti, SOA.Xs, NumControlPoints);
            CubicSplineNaturalM(My, Ti, SOA.Ys, NumControlPoints);
         } break;
         
         case CubicSpline_Periodic: {
            CubicSplinePeriodicM(Mx, Ti, SOA.Xs, NumControlPoints);
            CubicSplinePeriodicM(My, Ti, SOA.Ys, NumControlPoints);
         } break;
      }
      
      f32 Begin = Ti[0];
      f32 End = Ti[NumControlPoints - 1];
      f32 T = Begin;
      f32 Delta = (End - Begin) / (NumOutputCurvePoints - 1);
      
      for (u64 OutputIndex = 0;
           OutputIndex < NumOutputCurvePoints;
           ++OutputIndex)
      {
         
         f32 X = CubicSplineEvaluate(T, Mx, Ti, SOA.Xs, NumControlPoints);
         f32 Y = CubicSplineEvaluate(T, My, Ti, SOA.Ys, NumControlPoints);
         OutputCurvePoints[OutputIndex] = V2F32(X, Y);
         
         T += Delta;
      }
   }
}

internal void
BezierNormalCalculateCurvePoints(local_position *ControlPoints,
                                 u64 NumControlPoints,
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
                                 NumControlPoints);
      
      T += Delta;
   }
}

internal void
BezierWeightedCalculateCurvePoints(local_position *ControlPoints,
                                   f32 *ControlPointWeights,
                                   u64 NumControlPoints,
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
                                         NumControlPoints);
      
      T += Delta;
   }
}

internal void
BezierCubicCalculateCurvePoints(local_position *CubicBezierPoints,
                                u64 NumControlPoints,
                                u64 NumOutputCurvePoints,
                                local_position *OutputCurvePoints)
{
   if (NumControlPoints > 0)
   {
      f32 T = 0.0f;
      f32 Delta = 1.0f / (NumOutputCurvePoints - 1);
      for (u64 OutputIndex = 0;
           OutputIndex < NumOutputCurvePoints;
           ++OutputIndex)
      {
         f32 Expanded_T = (NumControlPoints - 1) * T;
         u64 SegmentIndex = cast(u64)Expanded_T;
         f32 Segment_T = Expanded_T - SegmentIndex;
         
         Assert(SegmentIndex < NumControlPoints);
         Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
         
         local_position *Segment = CubicBezierPoints + 1 + 3 * SegmentIndex;
         u64 NumSegmentPoints = (SegmentIndex == NumControlPoints - 1 ? 1 : 4);
         OutputCurvePoints[OutputIndex] = BezierCurveEvaluateFast(Segment_T,
                                                                  Segment,
                                                                  NumSegmentPoints);
         
         T += Delta;
      }
   }
}

function b32
CurveIsLooped(curve_shape CurveShape)
{
   b32 Loop = (CurveShape.InterpolationType == Interpolation_CubicSpline &&
               CurveShape.CubicSplineType == CubicSpline_Periodic);
   
   return Loop;
}

function void
CurveRecompute(curve *Curve)
{
   TimeFunction;
   
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   curve_params CurveParams = Curve->CurveParams;
   curve_shape CurveShape = CurveParams.CurveShape;
   
   // NOTE(hbr): Calculate curve points
   {
      u64 NumControlPoints = Curve->NumControlPoints;
      
      u64 NumCurvePoints = 0;
      local_position *CurvePoints = 0;
      if (NumControlPoints > 0)
      {
         NumCurvePoints = (NumControlPoints - 1) * CurveParams.NumCurvePointsPerSegment + 1;
      }
      CurvePoints = Curve->CurvePoints;
      ArrayReserve(CurvePoints, Curve->NumCurvePoints, NumCurvePoints);
      CurveEvaluate(Curve, NumCurvePoints, CurvePoints);
      Curve->NumCurvePoints = NumCurvePoints;
      Curve->CurvePoints = CurvePoints;
      
      b32 Loop = CurveIsLooped(CurveShape);
      Curve->CurveVertices = CalculateLineVertices(Curve->NumCurvePoints,
                                                   Curve->CurvePoints, 
                                                   CurveParams.CurveWidth,
                                                   CurveParams.CurveColor,
                                                   Loop,
                                                   LineVerticesAllocationHeap(Curve->CurveVertices));
   }
   
   Curve->PolylineVertices = CalculateLineVertices(Curve->NumControlPoints,
                                                   // NOTE(hbr): Calculate polyline and convex hull vertices
                                                   Curve->ControlPoints,
                                                   CurveParams.PolylineWidth,
                                                   CurveParams.PolylineColor,
                                                   false,
                                                   LineVerticesAllocationHeap(Curve->PolylineVertices));
   
   {
      local_position *ConvexHullPoints = PushArray(Scratch.Arena, Curve->NumControlPoints, v2f32);
      u64 NumConvexHullPoints = CalculateConvexHull(Curve->NumControlPoints,
                                                    Curve->ControlPoints,
                                                    ConvexHullPoints);
      
      Curve->ConvexHullVertices = CalculateLineVertices(NumConvexHullPoints,
                                                        ConvexHullPoints,
                                                        CurveParams.ConvexHullWidth,
                                                        CurveParams.ConvexHullColor,
                                                        true,
                                                        LineVerticesAllocationHeap(Curve->ConvexHullVertices));
   }
   
   Curve->CurveVersion += 1;
}

function sf::Transform
CurveGetAnimate(curve *Curve)
{
   sf::Transform Result =
      sf::Transform()
      .translate(V2F32ToVector2f(Curve->Position))
      .rotate(Rotation2DToDegrees(Curve->Rotation));
   
   return Result;
}

function void
CurveEvaluate(curve *Curve, u64 NumOutputCurvePoints, v2f32 *OutputCurvePoints)
{
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   curve_shape CurveShape = Curve->CurveParams.CurveShape;
   switch (CurveShape.InterpolationType)
   {
      case Interpolation_Polynomial: {
         f32 *Ti = PushArray(Scratch.Arena, Curve->NumControlPoints, f32);
         switch (CurveShape.PolynomialInterpolationParams.PointsArrangement)
         {
            case PointsArrangement_Equidistant: { EquidistantPoints(Ti, Curve->NumControlPoints); } break;
            case PointsArrangement_Chebychev:   { ChebyshevPoints(Ti, Curve->NumControlPoints);   } break;
         }
         
         switch (CurveShape.PolynomialInterpolationParams.Type)
         {
            case PolynomialInterpolation_Barycentric: {
               PolynomialInterpolationBarycentricCalculateCurvePoints(Curve->ControlPoints,
                                                                      Curve->NumControlPoints,
                                                                      CurveShape.PolynomialInterpolationParams.PointsArrangement,
                                                                      Ti, NumOutputCurvePoints,
                                                                      OutputCurvePoints);
            } break;
            
            case PolynomialInterpolation_Newton: {
               PolynomialInterpolationNewtonCalculateCurvePoints(Curve->ControlPoints,
                                                                 Curve->NumControlPoints,
                                                                 Ti, NumOutputCurvePoints,
                                                                 OutputCurvePoints);
            } break;
            
         }
      } break;
      
      case Interpolation_CubicSpline: {
         CubicSplineCalculateCurvePoints(CurveShape.CubicSplineType,
                                         Curve->ControlPoints,
                                         Curve->NumControlPoints,
                                         NumOutputCurvePoints,
                                         OutputCurvePoints);
      } break;
      
      case Interpolation_Bezier: {
         switch (CurveShape.BezierType)
         {
            case Bezier_Normal: {
               BezierNormalCalculateCurvePoints(Curve->ControlPoints,
                                                Curve->NumControlPoints,
                                                NumOutputCurvePoints,
                                                OutputCurvePoints);
            } break;
            
            case Bezier_Weighted: {
               BezierWeightedCalculateCurvePoints(Curve->ControlPoints,
                                                  Curve->ControlPointWeights,
                                                  Curve->NumControlPoints,
                                                  NumOutputCurvePoints,
                                                  OutputCurvePoints);
            } break;
            
            case Bezier_Cubic: {
               BezierCubicCalculateCurvePoints(Curve->CubicBezierPoints,
                                               Curve->NumControlPoints,
                                               NumOutputCurvePoints,
                                               OutputCurvePoints);
            } break;
         }
      } break;
   }
}

function local_position
WorldToLocalCurvePosition(curve *Curve, world_position Position)
{
   local_position Result = RotateAround(Position - Curve->Position,
                                        V2F32(0.0f, 0.0f),
                                        Rotation2DInverse(Curve->Rotation));
   
   return Result;
}

function world_position
LocalCurvePositionToWorld(curve *Curve, local_position Position)
{
   world_position Result = RotateAround(Position,
                                        V2F32(0.0f, 0.0f),
                                        Curve->Rotation) + Curve->Position;
   
   return Result;
}

//- Image
function image
ImageMake(name_string Name, world_position Position,
          v2f32 Scale, rotation_2d Rotation,
          s64 SortingLayer, b32 Hidden,
          string FilePath, sf::Texture Texture)
{
   image Result = {};
   Result.Name = Name;
   Result.Position = Position;
   Result.Scale = Scale;
   Result.Rotation = Rotation;
   Result.SortingLayer = SortingLayer;
   Result.Hidden = Hidden;
   Result.FilePath = StringDuplicate(FilePath);
   Result.Texture = Texture;
   
   return Result;
}

function sf::Texture
LoadTextureFromFile(arena *Arena, string FilePath, error_string *OutError)
{
   sf::Texture Texture;
   
   auto Scratch = ScratchArena(Arena);
   defer { ReleaseScratch(Scratch); };
   
   error_string FileReadError = 0;
   file_contents TextureFileContents = ReadEntireFile(Scratch.Arena, FilePath, &FileReadError);
   
   if (FileReadError)
   {
      *OutError = StringMakeFormat(Arena,
                                   "failed to load texture from file: %s",
                                   FileReadError);
   }
   else
   {
      if (!Texture.loadFromMemory(TextureFileContents.Contents, TextureFileContents.Size))
      {
         *OutError = StringMakeFormat(Arena,
                                      "failed to load texture from file %s",
                                      FilePath);
      }
   }
   
   return Texture;
}

function image
ImageCopy(image Image)
{
   image Result = ImageMake(Image.Name, Image.Position,
                            Image.Scale, Image.Rotation,
                            Image.SortingLayer, Image.Hidden,
                            Image.FilePath, Image.Texture);
   
   return Result;
}

function void
ImageDestroy(image *Image)
{
   StringFree(Image->FilePath);
   Image->Texture.~Texture();
   MemoryZero(cast(void *)Image, sizeof(Image));
}

internal int
ImageSortEntryCompare(image_sort_entry *A, image_sort_entry *B)
{
   s64 SortingLayerA = A->Image->SortingLayer;
   s64 SortingLayerB = B->Image->SortingLayer;
   
   int Result = 0;
   if      (SortingLayerA < SortingLayerB)       Result = -1;
   else if (SortingLayerA > SortingLayerB)       Result = 1;
   else if (A->OriginalOrder < B->OriginalOrder) Result = -1;
   else if (A->OriginalOrder > B->OriginalOrder) Result = 1;
   
   return Result;
}

function sorting_layer_sorted_images
SortingLayerSortedImages(arena *Arena, u64 NumImages, image *ImageList)
{
   image_sort_entry *Entries = PushArray(Arena, NumImages, image_sort_entry);
   
   image_sort_entry *Entry = Entries;
   u64 Index = 0;
   ListIter(Image, ImageList, image)
   {
      Entry->Image = Image;
      Entry->OriginalOrder = Index;
      ++Entry;
      ++Index;
   }
   
   QuickSort(Entries, NumImages, image_sort_entry, ImageSortEntryCompare);
   
   sorting_layer_sorted_images Result = {};
   Result.NumImages = NumImages;
   Result.SortedImages = Entries;
   
   return Result;
}