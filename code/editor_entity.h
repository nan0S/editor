#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

struct name_string
{
   char Data[100];
   u64 Count;
};

function name_string NameStr(char *Data, u64 Count);
function name_string StrToNameStr(string String);
#define NameStrLit(Literal) NameStr(Cast(char *)Literal, ArrayCount(Literal) - 1)
function name_string NameStrF(char const *Fmt, ...);

enum interpolation_type
{
   Interpolation_Polynomial,
   Interpolation_CubicSpline,
   Interpolation_Bezier,
};

enum polynomial_interpolation_type
{
   PolynomialInterpolation_Barycentric,
   PolynomialInterpolation_Newton,
};

enum points_arrangement
{
   PointsArrangement_Equidistant,
   PointsArrangement_Chebychev,
};

struct polynomial_interpolation_params
{
   polynomial_interpolation_type Type;
   points_arrangement PointsArrangement;
};

enum cubic_spline_type
{
   CubicSpline_Natural,
   CubicSpline_Periodic,
};

enum bezier_type
{
   Bezier_Normal,
   Bezier_Weighted,
   Bezier_Cubic,
};

struct curve_shape
{
   interpolation_type InterpolationType;
   polynomial_interpolation_params PolynomialInterpolationParams;
   cubic_spline_type CubicSplineType;
   bezier_type BezierType;
};

struct curve_params
{
   curve_shape CurveShape;
   color CurveColor;
   f32 CurveWidth;
   
   b32 PointsDisabled;
   color PointColor;
   f32 PointSize;
   
   b32 PolylineEnabled;
   color PolylineColor;
   f32 PolylineWidth;
   
   b32 ConvexHullEnabled;
   color ConvexHullColor;
   f32 ConvexHullWidth;
   
   u64 NumCurvePointsPerSegment;
   
   struct {
      color GradientA;
      color GradientB;
   } DeCasteljau;
   
   b32 CubicBezierHelpersDisabled;
};

struct curve
{
   curve_params CurveParams;
   u64 SelectedControlPointIndex;
   
   u64 NumControlPoints;
   u64 CapControlPoints;
   local_position *ControlPoints;
   
   // NOTE(hbr): Weighted Bezier case
   // TODO(hbr): Remove capacity
   u64 CapControlPointWeights;
   f32 *ControlPointWeights;
   
   u64 CapCubicBezierPoints;
   local_position *CubicBezierPoints;
   
   u64 NumCurvePoints;
   local_position *CurvePoints;
   
   line_vertices CurveVertices;
   line_vertices PolylineVertices;
   line_vertices ConvexHullVertices;
   
   u64 CurveVersion; // NOTE(hbr): Changes every recomputation
};

struct image
{
   string FilePath;
   sf::Texture Texture;
};

enum entity_type
{
   Entity_Curve,
   Entity_Image,
};

typedef u64 entity_flags;
enum
{
   EntityFlag_Hidden = (1<<0),
   EntityFlag_Selected = (1<<1),
};

struct entity
{
   entity *Prev;
   entity *Next;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   name_string Name;
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   entity_type Type;
   curve Curve;
   image Image;
};

function void
ImageInit(image *Image, string ImagePath, sf::Texture *Texture)
{
   // NOTE(hbr): This is only because of C++ ugliness
   new (&Image->Texture) sf::Texture(*Texture);
   Image->FilePath = DuplicateStr(ImagePath);
}

// TODO(hbr): I'm not sure if this function belongs here, in this file
function sf::Texture
LoadTextureFromFile(arena *Arena, string FilePath, error_string *OutError)
{
   sf::Texture Texture;
   
   temp_arena Temp = TempArena(Arena);
   
   error_string FileReadError = {};
   file_contents TextureFileContents = ReadEntireFile(Temp.Arena, FilePath, &FileReadError);
   if (IsError(FileReadError))
   {
      *OutError = StrF(Arena,
                       "failed to load texture from file: %s",
                       FileReadError);
   }
   else
   {
      if (!Texture.loadFromMemory(TextureFileContents.Contents, TextureFileContents.Size))
      {
         *OutError = StrF(Arena,
                          "failed to load texture from file %s",
                          FilePath);
      }
   }
   
   EndTemp(Temp);
   
   return Texture;
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
PolynomialInterpolationNewtonCalculateCurvePoints(local_position *ControlPoints,
                                                  u64 NumControlPoints, f32 *Ti,
                                                  u64 NumOutputCurvePoints,
                                                  local_position *OutputCurvePoints)
{
   if (NumControlPoints > 0)
   {
      temp_arena Temp = TempArena(0);
      
      points_soa SOA = SplitPointsIntoComponents(Temp.Arena,
                                                 ControlPoints,
                                                 NumControlPoints);
      
      f32 *Beta_X = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
      f32 *Beta_Y = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
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
      
      EndTemp(Temp);
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
      temp_arena Temp = TempArena(0);
      
      f32 *Omega = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
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
      
      points_soa SOA = SplitPointsIntoComponents(Temp.Arena,
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
      
      EndTemp(Temp);
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
      temp_arena Temp = TempArena(0);
      
      if (CubicSplineType == CubicSpline_Periodic)
      {
         local_position *OriginalControlPoints = ControlPoints;
         
         ControlPoints = PushArrayNonZero(Temp.Arena, NumControlPoints + 1, v2f32);
         MemoryCopy(ControlPoints,
                    OriginalControlPoints,
                    NumControlPoints * SizeOf(ControlPoints[0]));
         ControlPoints[NumControlPoints] = OriginalControlPoints[0];
         
         NumControlPoints += 1;
      }
      
      f32 *Ti = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
      EquidistantPoints(Ti, NumControlPoints);
      
      points_soa SOA = SplitPointsIntoComponents(Temp.Arena, ControlPoints, NumControlPoints);
      
      f32 *Mx = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
      f32 *My = PushArrayNonZero(Temp.Arena, NumControlPoints, f32);
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
      
      EndTemp(Temp);
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
         u64 SegmentIndex = Cast(u64)Expanded_T;
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

internal b32
CurveIsLooped(curve_shape CurveShape)
{
   b32 Loop = (CurveShape.InterpolationType == Interpolation_CubicSpline &&
               CurveShape.CubicSplineType == CubicSpline_Periodic);
   
   return Loop;
}

// TODO(hbr): Remove this, why the heck this is necessary?!!!
function void ChebyshevPoints(f32 *Ti, u64 N);

function void
CurveEvaluate(curve *Curve, u64 NumOutputCurvePoints, v2f32 *OutputCurvePoints)
{
   temp_arena Temp = TempArena(0);
   
   curve_shape CurveShape = Curve->CurveParams.CurveShape;
   switch (CurveShape.InterpolationType)
   {
      case Interpolation_Polynomial: {
         f32 *Ti = PushArrayNonZero(Temp.Arena, Curve->NumControlPoints, f32);
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
   
   EndTemp(Temp);
}

function void
CurveRecompute(curve *Curve)
{
   TimeFunction;
   
   temp_arena Temp = TempArena(0);
   
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
      local_position *ConvexHullPoints = PushArrayNonZero(Temp.Arena, Curve->NumControlPoints, v2f32);
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
   
   EndTemp(Temp);
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
              NewNumControlPoints * SizeOf(Curve->ControlPoints[0]));
   MemoryCopy(Curve->ControlPointWeights,
              NewControlPointWeights,
              NewNumControlPoints * SizeOf(Curve->ControlPointWeights[0]));
   MemoryCopy(Curve->CubicBezierPoints,
              NewCubicBezierPoints,
              3 * NewNumControlPoints * SizeOf(Curve->CubicBezierPoints[0]));
   
   Curve->NumControlPoints = NewNumControlPoints;
   // TODO(hbr): Make sure this is expected, maybe add to function arguments
   Curve->SelectedControlPointIndex = U64_MAX;
   
   CurveRecompute(Curve);
}

function local_position
WorldToLocalEntityPosition(entity *Entity, world_position Position)
{
   local_position Result = RotateAround(Position - Entity->Position, V2F32(0.0f, 0.0f), Rotation2DInverse(Entity->Rotation));
   return Result;
}

function world_position
LocalEntityPositionToWorld(entity *Entity, local_position Position)
{
   world_position Result = RotateAround(Position, V2F32(0.0f, 0.0f), Entity->Rotation) + Entity->Position;
   return Result;
}

// TODO(hbr): Probably just implement Append in terms of insert instead of making append special case of insert
typedef u64 added_point_index_u64;
function added_point_index_u64
CurveAppendControlPoint(entity *CurveEntity,
                        world_position ControlPoint,
                        f32 ControlPointWeight)
{
   curve *Curve = &CurveEntity->Curve;
   u64 NumControlPoints = Curve->NumControlPoints;
   local_position ControlPointLocal = WorldToLocalEntityPosition(CurveEntity, ControlPoint);
   
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
CurveInsertControlPoint(entity *CurveEntity,
                        world_position ControlPoint,
                        u64 InsertAfterIndex,
                        f32 ControlPointWeight)
{
   curve *Curve = &CurveEntity->Curve;
   u64 NumControlPoints = Curve->NumControlPoints;
   Assert(InsertAfterIndex < NumControlPoints);
   
   if (InsertAfterIndex == NumControlPoints-1)
   {
      CurveAppendControlPoint(CurveEntity, ControlPoint, ControlPointWeight);
   }
   else
   {
      local_position ControlPointLocal = WorldToLocalEntityPosition(CurveEntity, ControlPoint);
      
      ArrayReserveCopy(Curve->ControlPoints, Curve->CapControlPoints, NumControlPoints + 1);
      MemoryMove(Curve->ControlPoints + InsertAfterIndex + 2,
                 Curve->ControlPoints + InsertAfterIndex + 1,
                 (NumControlPoints - InsertAfterIndex - 1) * SizeOf(Curve->ControlPoints[0]));
      Curve->ControlPoints[InsertAfterIndex + 1] = ControlPointLocal;
      
      ArrayReserveCopy(Curve->ControlPointWeights, Curve->CapControlPointWeights, NumControlPoints + 1);
      MemoryMove(Curve->ControlPointWeights + InsertAfterIndex + 2,
                 Curve->ControlPointWeights + InsertAfterIndex + 1,
                 (NumControlPoints - InsertAfterIndex - 1) * SizeOf(Curve->ControlPointWeights[0]));
      Curve->ControlPointWeights[InsertAfterIndex + 1] = ControlPointWeight;
      
      ArrayReserveCopy(Curve->CubicBezierPoints, Curve->CapCubicBezierPoints, 3 * (NumControlPoints + 1));
      MemoryMove(Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 2),
                 Curve->CubicBezierPoints + 3 * (InsertAfterIndex + 1),
                 3 * (NumControlPoints - InsertAfterIndex - 1) * SizeOf(Curve->CubicBezierPoints[0]));
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
              (NumControlPoints - ControlPointIndex - 1) * SizeOf(Curve->ControlPoints[0]));
   MemoryMove(Curve->ControlPointWeights + ControlPointIndex,
              Curve->ControlPointWeights + ControlPointIndex + 1,
              (NumControlPoints - ControlPointIndex - 1) * SizeOf(Curve->ControlPointWeights[0]));
   MemoryMove(Curve->CubicBezierPoints + 3 * ControlPointIndex,
              Curve->CubicBezierPoints + 3 * (ControlPointIndex + 1),
              3 * (NumControlPoints - ControlPointIndex - 1) * SizeOf(Curve->CubicBezierPoints[0]));
   
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

internal local_position
TranslateLocalCurvePositionInWorldSpace(entity *Entity, local_position Local, v2f32 Translation)
{
   world_position World = LocalEntityPositionToWorld(Entity, Local);
   world_position Translated = World + Translation;
   local_position Result = WorldToLocalEntityPosition(Entity, Translated);
   
   return Result;
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
CurveTranslatePoint(entity *CurveEntity, u64 PointIndex, b32 IsCubicBezierPoint,
                    b32 MatchCubicBezierTwinDirection, b32 MatchCubicBezierTwinLength,
                    v2f32 TranslationWorld)
{
   curve *Curve = &CurveEntity->Curve;
   
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
         Assert(!"Unexpected point index");
      }
      
      local_position TranslatedPoint =
         TranslateLocalCurvePositionInWorldSpace(CurveEntity, *Point, TranslationWorld);
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
         TranslateLocalCurvePositionInWorldSpace(CurveEntity,
                                                 Curve->ControlPoints[PointIndex],
                                                 TranslationWorld);
      CurveSetControlPoint(Curve, PointIndex, TranslatedPoint,
                           Curve->ControlPointWeights[PointIndex]);
   }
}

// TODO(hbr): Rename this function and also maybe others to [EntityRotateAround] because it's not [curve] specific.
function void
CurveRotateAround(entity *CurveEntity, world_position Center, rotation_2d Rotation)
{
   CurveEntity->Position = RotateAround(CurveEntity->Position, Center, Rotation);
   CurveEntity->Rotation = CombineRotations2D(CurveEntity->Rotation, Rotation);
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

function void
InitCurve(curve *Curve,
          curve_params CurveParams, u64 SelectedControlPointIndex,
          u64 NumControlPoints,
          world_position *ControlPoints, f32 *ControlPointWeights, world_position *CubicBezierPoints)
{
   MemoryZero(Curve, SizeOf(*Curve));
   Curve->CurveParams = CurveParams;
   CurveSetControlPoints(Curve, NumControlPoints, ControlPoints, ControlPointWeights, CubicBezierPoints);
   Curve->SelectedControlPointIndex = SelectedControlPointIndex;
}

function void
InitImage(image *Image, string ImageFilePath, sf::Texture *Texture)
{
   Image->FilePath = DuplicateStr(ImageFilePath);
   new (&Image->Texture) sf::Texture(*Texture);
}

function void
InitCurveFromCurve(curve *Dst, curve *Src)
{
   InitCurve(Dst,
             Src->CurveParams, U64_MAX,
             Src->NumControlPoints,
             Src->ControlPoints, Src->ControlPointWeights, Src->CubicBezierPoints);
}

function void
InitImageFromImage(image *Dst, image *Src)
{
   MemoryZero(Dst, SizeOf(*Dst));
   new (&Dst->Texture) sf::Texture(Src->Texture);
   Dst->FilePath = DuplicateStr(Src->FilePath);
}

function void
InitEntity(entity *Entity,
           world_position Position, v2f32 Scale, rotation_2d Rotation,
           name_string Name,
           s64 SortingLayer,
           u64 RenamingFrame,
           entity_flags Flags,
           entity_type Type)
{
   MemoryZero(Entity, SizeOf(*Entity));
   Entity->Position = Position;
   Entity->Scale = Scale;
   Entity->Rotation = Rotation;
   Entity->Name = Name;
   Entity->SortingLayer = SortingLayer;
   Entity->Flags = Flags;
   Entity->Type = Type;
}

function void
InitEntityFromEntity(entity *Dest, entity *Src)
{
   InitEntity(Dest,
              Src->Position, Src->Scale, Src->Rotation,
              Src->Name,
              Src->SortingLayer,
              Src->RenamingFrame,
              Src->Flags,
              Src->Type);
   switch (Src->Type)
   {
      case Entity_Curve: { InitCurveFromCurve(&Dest->Curve, &Src->Curve); } break;
      case Entity_Image: { InitImageFromImage(&Dest->Image, &Src->Image); } break;
   }
}

function void
InitCurveEntity(entity *Entity,
                world_position Position, v2f32 Scale, rotation_2d Rotation,
                name_string Name,
                curve_params CurveParams)
{
   InitEntity(Entity, Position, Scale, Rotation, Name, 0, 0, 0, Entity_Curve);
   InitCurve(&Entity->Curve, CurveParams, U64_MAX, 0, 0, 0, 0);
}

function void
InitImageEntity(entity *Entity,
                world_position Position, v2f32 Scale, rotation_2d Rotation,
                name_string Name,
                string ImageFilePath, sf::Texture *Texture)
{
   MemoryZero(Entity, SizeOf(*Entity));
   
   InitEntity(Entity, Position, Scale, Rotation, Name, 0, 0, 0, Entity_Image);
   InitImage(&Entity->Image, ImageFilePath, Texture);
}

#if 0
function curve_shape CurveShapeMake(interpolation_type InterpolationType,
                                    polynomial_interpolation_type PolynomialInterpolationType,
                                    points_arrangement PointsArrangement,
                                    cubic_spline_type CubicSplineType,
                                    bezier_type BezierType);

function curve_params CurveParamsMake(curve_shape CurveShape,
                                      f32 CurveWidth, color CurveColor,
                                      b32 PointsDisabled, f32 PointSize, color PointColor,
                                      b32 PolylineEnabled, f32 PolylineWidth, color PolylineColor,
                                      b32 ConvexHullEnabled, f32 ConvexHullWidth, color ConvexHullColor,
                                      u64 NumCurvePointsPerSegment, color DeCasteljauVisualizationGradientA,
                                      color DeCasteljauVisualizationGradientB);

function curve CurveMake(name_string CurveName,
                         curve_params CurveParams,
                         u64 SelectedControlPointIndex,
                         world_position Position,
                         rotation_2d Rotation);
function void CurveDestroy(curve *Curve);
function curve CurveCopy(curve Curve);

typedef u64 added_point_index;
function added_point_index CurveAppendControlPoint(curve *Curve, world_position ControlPoint,
                                                   f32 ControlPointWeight);

function void CurveInsertControlPoint(curve *Curve, world_position ControlPoint, u64 InsertAfterIndex,
                                      f32 ControlPointWeight);
function void CurveRemoveControlPoint(curve *Curve, u64 ControlPointIndex);
function void CurveSetControlPoints(curve *Curve, u64 NewNumControlPoints, local_position *NewControlPoints,
                                    f32 *NewControlPointWeights, local_position *NewCubicBezierPoints);
function void CurveTranslatePoint(curve *Curve, u64 PointIndex, b32 IsCubicBezierPoint,
                                  b32 MatchCubicBezierTwinDirection, b32 MatchCubicBezierTwinLength,
                                  v2f32 TranslationWorld);
function void CurveSetControlPoint(curve *Curve, u64 ControlPointIndex, local_position ControlPoint,
                                   f32 ControlPointWeight);
function void CurveRotateAround(curve *Curve, world_position Center, rotation_2d Rotation);
function void CurveRecompute(curve *Curve);
function void CurveEvaluate(curve *Curve, u64 NumOutputCurvePoints, v2f32 *OutputCurvePoints);

function sf::Transform CurveGetAnimate(curve *Curve);
function b32 CurveIsLooped(curve_shape CurveShape);

function local_position WorldToLocalCurvePosition(curve *Curve, world_position Position);
function world_position LocalCurvePositionToWorld(curve *Curve, local_position Position);

function image ImageMake(name_string Name, world_position Position,
                         v2f32 Scale, rotation_2d Rotation,
                         s64 SoritingLayer, b32 Hidden,
                         string FilePath, sf::Texture Texture);
function image ImageCopy(image Image);
function void ImageDestroy(image *Image);

function sf::Texture LoadTextureFromFile(arena *Arena, string FilePath,
                                         error_string *OutError);

struct image_sort_entry
{
   image *Image;
   u64 OriginalOrder;
};
struct sorting_layer_sorted_images
{
   u64 NumImages;
   image_sort_entry *SortedImages;
};
function sorting_layer_sorted_images SortingLayerSortedImages(arena *Arena,
                                                              u64 NumEntities,
                                                              image *EntityList);

// TODO(hbr): This should be temporary
#define EntityFromCurve(CurvePtr) EnclosingTypeAddr(entity, Curve, CurvePtr)
#define EntityFromImage(ImagePtr) EnclosingTypeAddr(entity, Image, ImagePtr)

function entity CurveEntity(curve Curve);
function entity ImageEntity(image Image);
function void EntityDestroy(entity *Entity);

#endif

struct entity_sort_entry
{
   entity *Entity;
   u64 OriginalOrder;
};
struct sorted_entity_array
{
   u64 EntityCount;
   entity_sort_entry *Entries;
};
function sorted_entity_array SortEntitiesBySortingLayer(arena *Arena, entity *Entities);

internal int
EntitySortEntryCmp(entity_sort_entry *A, entity_sort_entry *B)
{
   int Result = 0;
   
   int Cmp1 = IntCmp(A->Entity->SortingLayer, B->Entity->SortingLayer);
   if (Cmp1 == 0)
   {
      int Cmp2 = IntCmp(A->OriginalOrder, B->OriginalOrder);
      Result = Cmp2;
   }
   else
   {
      Result = Cmp1;
   }
   
   return Result;
}

function sorted_entity_array
SortEntitiesBySortingLayer(arena *Arena, entity *Entities)
{
   sorted_entity_array Result = {};
   
   ListIter(Entity, Entities, entity) { ++Result.EntityCount; }
   Result.Entries = PushArrayNonZero(Arena, Result.EntityCount, entity_sort_entry);
   
   {
      entity_sort_entry *Entry = Result.Entries;
      u64 Index = 0;
      ListIter(Entity, Entities, entity)
      {
         Entry->Entity = Entity;
         Entry->OriginalOrder = Index;
         ++Entry;
         ++Index;
      }
   }
   
   // NOTE(hbr): Compare function makes this sort stable, and we need stable property
   QuickSort(Result.Entries, Result.EntityCount, entity_sort_entry, EntitySortEntryCmp);
   
   return Result;
}

// TODO(hbr): Why the fuck this is the name of this function
// TODO(hbr): Rewrite this function
function sf::Transform
CurveGetAnimate(entity *Curve)
{
   sf::Transform Result =
      sf::Transform()
      .translate(V2F32ToVector2f(Curve->Position))
      .rotate(Rotation2DToDegrees(Curve->Rotation));
   
   return Result;
}

function name_string
NameStr(char *Data, u64 Count)
{
   name_string Result = {};
   
   u64 FinalCount = Minimum(Count, ArrayCount(Result.Data) - 1);
   MemoryCopy(Result.Data, Data, FinalCount);
   Result.Data[FinalCount] = 0;
   Result.Count = FinalCount;
   
   return Result;
}

function name_string
StrToNameStr(string String)
{
   name_string Result = NameStr(String.Data, String.Count);
   return Result;
}

function name_string
NameStrF(char const *Fmt, ...)
{
   name_string Result = {};
   
   va_list Args;
   DeferBlock(va_start(Args, Fmt), va_end(Args))
   {
      int Return = vsnprintf(Result.Data,
                             ArrayCount(Result.Data) - 1,
                             Fmt, Args);
      if (Return >= 0)
      {
         // TODO(hbr): What the fuck is wrong with this function
         Result.Count = CStrLength(Result.Data);
      }
   }
   
   return Result;
}

inline curve *
CurveFromEntity(entity *Entity)
{
   Assert(Entity->Type == Entity_Curve);
   return &Entity->Curve;
}

inline image *
ImageFromEntity(entity *Entity)
{
   Assert(Entity->Type == Entity_Image);
   return &Entity->Image;
}

function void
EntityDestroy(entity *Entity)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         ArrayFree(Curve->ControlPoints);
         ArrayFree(Curve->ControlPointWeights);
         ArrayFree(Curve->CubicBezierPoints);
         ArrayFree(Curve->CurvePoints);
         ArrayFree(Curve->CurveVertices.Vertices);
         ArrayFree(Curve->PolylineVertices.Vertices);
         ArrayFree(Curve->ConvexHullVertices.Vertices);
      } break;
      
      case Entity_Image: {
         image *Image = &Entity->Image;
         Image->Texture.~Texture();
         FreeStr(&Image->FilePath);
      } break;
   }
   
   MemoryZero(Entity, SizeOf(*Entity));
}

#endif //EDITOR_ENTITY_H