#ifndef EDITOR_ENTITY2_H
#define EDITOR_ENTITY2_H

struct control_point_index
{
   u64 Index;
};

struct cubic_bezier_point_index
{
   u64 Index;
};

enum curve_point_type : u8
{
   CurvePoint_ControlPoint,
   CurvePoint_CubicBezierPoint,
};

struct curve_point_index
{
   curve_point_type Type;
   union {
      control_point_index ControlPoint;
      cubic_bezier_point_index BezierPoint;
   };
};

struct visible_cubic_bezier_points
{
   u64 Count;
   cubic_bezier_point_index Indices[4];
};

enum interpolation_type : u8
{
   Interpolation_CubicSpline,
   Interpolation_Bezier,
   Interpolation_Polynomial,
   Interpolation_Count
};
// TODO(hbr): Try to add designated array intializers
read_only global char const *InterpolationNames[] = { "Cubic Spline", "Bezier", "Polynomial" };
StaticAssert(ArrayCount(InterpolationNames) == Interpolation_Count, InterpolationNamesDefined);

enum polynomial_interpolation_type : u8
{
   PolynomialInterpolation_Barycentric,
   PolynomialInterpolation_Newton,
   PolynomialInterpolation_Count,
};
read_only global char const *PolynomialInterpolationNames[] = { "Barycentric", "Newton" };
StaticAssert(ArrayCount(PolynomialInterpolationNames) == PolynomialInterpolation_Count, PolynomialInterpolationNamesDefined);

enum points_arrangement : u8
{
   PointsArrangement_Chebychev,
   PointsArrangement_Equidistant,
   PointsArrangement_Count,
};
read_only global char const *PointsArrangementNames[] = { "Chebychev", "Equidistant" };
StaticAssert(ArrayCount(PointsArrangementNames) == PointsArrangement_Count, PointsArrangementNamesDefined);

struct polynomial_interpolation_params
{
   polynomial_interpolation_type Type;
   points_arrangement PointsArrangement;
};

enum cubic_spline_type : u8
{
   CubicSpline_Natural,
   CubicSpline_Periodic,
   CubicSpline_Count,
};
read_only global char const *CubicSplineNames[] = { "Natural", "Periodic" };
StaticAssert(ArrayCount(CubicSplineNames) == CubicSpline_Count, CubicSplineNamesDefined);

enum bezier_type : u8
{
   Bezier_Regular,
   Bezier_Cubic,
   Bezier_Count,
};
read_only global char const *BezierNames[] = { "Regular", "Cubic" };
StaticAssert(ArrayCount(BezierNames) == Bezier_Count, BezierNamesDefined);

struct curve_params
{
   interpolation_type InterpolationType;
   polynomial_interpolation_params PolynomialInterpolationParams;
   cubic_spline_type CubicSplineType;
   bezier_type BezierType;
   
   v4 CurveColor;
   f32 CurveWidth;
   
   b32 PointsDisabled;
   v4 PointColor;
   f32 PointRadius;
   
   b32 PolylineEnabled;
   v4 PolylineColor;
   f32 PolylineWidth;
   
   b32 ConvexHullEnabled;
   v4 ConvexHullColor;
   f32 ConvexHullWidth;
   
   u64 CurvePointCountPerSegment;
};

struct curve_point_tracking_state
{
   b32 Active;
   b32 NeedsRecomputationThisFrame;
   f32 Fraction;
   local_position TrackedPoint;
   
   b32 IsSplitting;
   
   // NOTE(hbr): De Casteljau visualization
   arena *Arena;
   all_de_casteljau_intermediate_results Intermediate;
   v4 *IterationColors;
   vertex_array *LineVerticesPerIteration;
};

struct curve_degree_lowering_state
{
   b32 Active;
   
   arena *Arena;
   
   // TODO(hbr): Replace Saved with Original or the other way around - I already used original somewhere
   local_position *SavedControlPoints;
   f32 *SavedControlPointWeights;
   cubic_bezier_point *SavedCubicBezierPoints;
   vertex_array SavedCurveVertices;
   
   bezier_lower_degree LowerDegree;
   f32 MixParameter;
};

typedef u64 translate_curve_point_flags;
enum
{
   TranslateCurvePoint_MatchBezierTwinDirection = (1<<0),
   TranslateCurvePoint_MatchBezierTwinLength    = (1<<1),
};

struct curve
{
   curve_params CurveParams;
   
   control_point_index SelectedIndex;
   
   u64 ControlPointCount;
#define MAX_CONTROL_POINT_COUNT 1024
   local_position ControlPoints[MAX_CONTROL_POINT_COUNT];
   f32 ControlPointWeights[MAX_CONTROL_POINT_COUNT];
   cubic_bezier_point CubicBezierPoints[MAX_CONTROL_POINT_COUNT];
   
   translate_curve_point_flags CubicBezierTranslateFlags;
   
   u64 CurvePointCount;
   local_position *CurvePoints;
   vertex_array CurveVertices;
   vertex_array PolylineVertices;
   vertex_array ConvexHullVertices;
   
   b32 RecomputeRequested;
   
   curve_point_tracking_state PointTracking;
   curve_degree_lowering_state DegreeLowering;
};

struct image
{
   string ImagePath;
   sf::Texture Texture;
};

enum entity_type
{
   Entity_Curve,
   Entity_Image,
};

enum
{
   EntityFlag_Active   = (1<<0),
   EntityFlag_Hidden   = (1<<1),
   EntityFlag_Selected = (1<<2),
};
typedef u64 entity_flags;

struct entity
{
   arena *Arena;
   
   world_position Position;
   v2 Scale;
   rotation_2d Rotation;
   
   char NameBuffer[64];
   string Name;
   
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   entity_type Type;
   curve Curve;
   image Image;
};

struct entity_array
{
   u64 Count;
   entity *Entities;
};

struct point_info
{
   f32 Radius;
   v4 Color;
   f32 OutlineThickness;
   v4 OutlineColor;
};

internal b32 IsEntityVisible(entity *Entity);
internal b32 IsEntitySelected(entity *Entity);
internal b32 AreCurvePointsVisible(curve *Curve);
internal point_info GetCurveControlPointInfo(entity *Curve, u64 PointIndex);
internal f32 GetCurveTrackedPointRadius(curve *Curve);
internal f32 GetCurveCubicBezierPointRadius(curve *Curve);
internal visible_cubic_bezier_points GetVisibleCubicBezierPoints(entity *Entity);

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
MakeControlPointIndex(u64 Index)
{
   control_point_index Result = {};
   Result.Index = Index;
   return Result;
}

inline internal local_position
GetCubicBezierPoint(curve *Curve, cubic_bezier_point_index Index)
{
   local_position Result = {};
   if (Index.Index < 3 * Curve->ControlPointCount)
   {
      local_position *Beziers = Cast(local_position *)Curve->CubicBezierPoints;
      Result = Beziers[Index.Index];
   }
   
   return Result;
}

inline internal local_position
GetCenterPointFromCubicBezierPointIndex(curve *Curve, cubic_bezier_point_index Index)
{
   local_position Result = {};
   u64 ControlPointIndex = Index.Index / 3;
   if (ControlPointIndex < Curve->ControlPointCount)
   {
      Result = Curve->ControlPoints[ControlPointIndex];
   }
   
   return Result;
}

inline internal control_point_index
CurveLinePointIndexToControlPointIndex(curve *Curve, u64 CurveLinePointIndex)
{
   u64 Index = SafeDiv0(CurveLinePointIndex, Curve->CurveParams.CurvePointCountPerSegment);
   Assert(Index < Curve->ControlPointCount);
   control_point_index Result = {};
   Result.Index = ClampTop(Index, Curve->ControlPointCount - 1);
   
   return Result;
}

internal void SetCurvePoint(entity *Entity, curve_point_index Index, v2 P, translate_curve_point_flags Flags);
internal void RemoveControlPoint(entity *Entity, u64 Index);
internal control_point_index AppendControlPoint(entity *Entity, world_position Point);
internal void InsertControlPoint(entity *Entity, world_position PointWorld, u64 At);
internal void SelectControlPoint(curve *Curve, control_point_index Index);
internal void SelectControlPointFromCurvePointIndex(curve *Curve, curve_point_index Index);
internal b32 IsControlPointSelected(curve *Curve);
internal void UnselectControlPoint(curve *Curve);

internal sorted_entries SortEntities(arena *Arena, entity_array Entities);

internal void RotateEntityAround(entity *Entity, rotation_2d Rotate, world_position Around);

// TODO(hbr): Temporary solution
internal transform
GetEntityModelTransform(entity *Entity)
{
   transform Result = {};
   Result.Scale = Entity->Scale;
   Result.Offset = Entity->Position;
   Result.Rotation = Entity->Rotation;
   
   return Result;
}

internal curve *
SafeGetCurve(entity *Entity)
{
   Assert(Entity->Type == Entity_Curve);
   return &Entity->Curve;
}

#endif //EDITOR_ENTITY2_H
