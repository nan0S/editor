#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

enum curve_type : u32
{
 Curve_CubicSpline,
 Curve_Bezier,
 Curve_Polynomial,
 Curve_Parametric,
 Curve_B_Spline,
 Curve_Count
};
global read_only string CurveTypeNames[] = {
 StrLitComp("Cubic Spline"),
 StrLitComp("Bezier"),
 StrLitComp("Polynomial"),
 StrLitComp("Parametric"),
 StrLitComp("B-Spline")
};
StaticAssert(ArrayCount(CurveTypeNames) == Curve_Count, CurveTypeNamesDefined);

enum polynomial_interpolation_type : u32
{
 PolynomialInterpolation_Barycentric,
 PolynomialInterpolation_Newton,
 PolynomialInterpolation_Count,
};
global read_only string PolynomialInterpolationTypeNames[] = {
 StrLitComp("Barycentric"),
 StrLitComp("Newton")
};
StaticAssert(ArrayCount(PolynomialInterpolationTypeNames) == PolynomialInterpolation_Count, PolynomialInterpolationTypeNamesDefined);

enum point_spacing : u32
{
 PointSpacing_Chebychev,
 PointSpacing_Equidistant,
 PointSpacing_Count,
};
global read_only string PointSpacingNames[] = {
 StrLitComp("Chebychev"),
 StrLitComp("Equidistant")
};
StaticAssert(ArrayCount(PointSpacingNames) == PointSpacing_Count, PointSpacingNamesDefined);

struct polynomial_interpolation_params
{
 polynomial_interpolation_type Type;
 point_spacing PointSpacing;
};

enum cubic_spline_type : u32
{
 CubicSpline_Natural,
 CubicSpline_Periodic,
 CubicSpline_Count,
};
global read_only string CubicSplineNames[] = {
 StrLitComp("Natural"),
 StrLitComp("Periodic")
};
StaticAssert(ArrayCount(CubicSplineNames) == CubicSpline_Count, CubicSplineNamesDefined);

enum bezier_type : u32
{
 Bezier_Regular,
 Bezier_Cubic,
 Bezier_Count,
};
global read_only string BezierNames[] = {
 StrLitComp("Regular"),
 StrLitComp("Cubic")
};
StaticAssert(ArrayCount(BezierNames) == Bezier_Count, BezierNamesDefined);

struct parametric_curve_params
{
 f32 MinT;
 f32 MaxT;
 
 parametric_equation_expr *X_Equation;
 parametric_equation_expr *Y_Equation;
};

enum b_spline_partition_type : u32
{
 B_SplinePartition_Natural,
 B_SplinePartition_Periodic,
 B_SplinePartition_Count,
};
global read_only string B_SplinePartitionNames[] = {
 StrLitComp("Natural"),
 StrLitComp("Periodic")
};
StaticAssert(ArrayCount(B_SplinePartitionNames) == B_SplinePartition_Count, B_SplinePartitionNamesDefined);

struct b_spline_params
{
 b_spline_partition_type Partition;
 b32 ShowPartitionKnotPoints;
 f32 KnotPointRadius;
 v4 KnotPointColor;
};

struct curve_params
{
 curve_type Type;
 polynomial_interpolation_params Polynomial;
 cubic_spline_type CubicSpline;
 bezier_type Bezier;
 parametric_curve_params Parametric;
 b_spline_params B_Spline;
 
 b32 LineDisabled;
 v4 LineColor;
 f32 LineWidth;
 
 b32 PointsDisabled;
 v4 PointColor;
 f32 PointRadius;
 
 b32 PolylineEnabled;
 v4 PolylineColor;
 f32 PolylineWidth;
 
 b32 ConvexHullEnabled;
 v4 ConvexHullColor;
 f32 ConvexHullWidth;
 
 u32 SamplesPerControlPoint;
 u32 TotalSamples;
};

struct control_point_index
{
 u32 Index;
};
struct cubic_bezier_point_index
{
 u32 Index;
};
enum curve_point_type
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
 u32 Count;
 cubic_bezier_point_index Indices[4];
};

enum point_tracking_along_curve_type
{
 PointTrackingAlongCurve_DeCasteljauVisualization,
 PointTrackingAlongCurve_BezierCurveSplit,
};

struct point_tracking_along_curve_state
{
 b32 Active;
 f32 Fraction;
 v2 LocalSpaceTrackedPoint;
 
 point_tracking_along_curve_type Type;
 
 all_de_casteljau_intermediate_results Intermediate;
 v4 *IterationColors;
 vertex_array *LineVerticesPerIteration;
};

struct curve_degree_lowering_state
{
 b32 Active;
 
 arena *Arena;
 
 // TODO(hbr): Replace Saved with Original or the other way around - I already used original somewhere
 v2 *SavedControlPoints;
 f32 *SavedControlPointWeights;
 cubic_bezier_point *SavedCubicBezierPoints;
 vertex_array SavedLineVertices;
 
 bezier_lower_degree LowerDegree;
 f32 MixParameter;
};

typedef u32 translate_curve_point_flags;
enum
{
 TranslateCurvePoint_MatchBezierTwinDirection = (1<<0),
 TranslateCurvePoint_MatchBezierTwinLength    = (1<<1),
};

#define MAX_EQUATION_BUFFER_LENGTH 64
struct parametric_curve_var
{
 parametric_curve_var *Next;
 parametric_curve_var *Prev;
 
 u32 Id;
 
 b32 EquationMode;
 f32 DragValue;
 f32 EquationValue;
 
#define MAX_VAR_NAME_BUFFER_LENGTH 16
 char VarNameBuffer[MAX_VAR_NAME_BUFFER_LENGTH];
 char VarEquationBuffer[MAX_EQUATION_BUFFER_LENGTH];
 
 b32 EquationFail;
 string EquationErrorMessage;
};
struct parametric_curve_resources
{
 arena *Arena;
 b32 ShouldClearArena;
 
 parametric_curve_var MinT_Var;
 parametric_curve_var MaxT_Var;
 
 char X_EquationBuffer[MAX_EQUATION_BUFFER_LENGTH];
 char Y_EquationBuffer[MAX_EQUATION_BUFFER_LENGTH];
 
 b32 X_Fail;
 string X_ErrorMessage;
 
 b32 Y_Fail;
 string Y_ErrorMessage;
 
#define MAX_ADDITIONAL_VAR_COUNT 16
 u32 AllocatedAdditionalVarCount;
 parametric_curve_var AdditionalVars[MAX_ADDITIONAL_VAR_COUNT];
 parametric_curve_var *FirstFreeAdditionalVar;
 parametric_curve_var *AdditionalVarsHead;
 parametric_curve_var *AdditionalVarsTail;
};

struct curve
{
 curve_params Params;
 
 // all points here are in local space
 u32 ControlPointCount;
#define MAX_CONTROL_POINT_COUNT 1024
 v2 ControlPoints[MAX_CONTROL_POINT_COUNT];
 f32 ControlPointWeights[MAX_CONTROL_POINT_COUNT];
 cubic_bezier_point CubicBezierPoints[MAX_CONTROL_POINT_COUNT];
 
 b_spline_knot_params B_SplineKnotParams;
#define MAX_B_SPLINE_KNOT_COUNT (2 * MAX_CONTROL_POINT_COUNT)
 f32 B_SplineKnots[MAX_B_SPLINE_KNOT_COUNT];
 
 control_point_index SelectedIndex;
 
 u32 CurveSampleCount;
 v2 *CurveSamples;
 
 u32 ConvexHullCount;
 v2 *ConvexHullPoints;
 
 v2 *B_SplinePartitionKnotPoints;
 
 vertex_array CurveVertices;
 vertex_array PolylineVertices;
 vertex_array ConvexHullVertices;
 
 point_tracking_along_curve_state PointTracking;
 curve_degree_lowering_state DegreeLowering;
 parametric_curve_resources ParametricResources;
};

struct image
{
 v2 Dim;
 render_texture_handle TextureHandle;
};

enum entity_type
{
 Entity_Curve,
 Entity_Image,
 Entity_Count,
};

enum
{
 EntityFlag_Hidden           = (1<<0),
 EntityFlag_Selected         = (1<<1),
 EntityFlag_CurveAppendFront = (1<<2),
 EntityFlag_Tracked          = (1<<3),
};
typedef u32 entity_flags;

struct entity
{
 entity *Next;
 entity *Prev;
 
 arena *Arena;
 
 v2 P;
 v2 Scale;
 v2 Rotation;
 
 char NameBuffer[64];
 string Name;
 
 i32 SortingLayer;
 entity_flags Flags;
 
 // NOTE(hbr): I don't think this needs to be bigger than u32
 u32 Generation; // increments every time entity is allocated
 u32 Version; // increments every time entity is "recomputed" (see CurveRecompute)
 
 entity_type Type;
 curve Curve;
 image Image;
};

struct entity_with_modify_witness
{
 entity *Entity;
 b32 EntityModified;
};
internal entity_with_modify_witness BeginEntityModify(entity *Entity);
internal void                       EndEntityModify(entity_with_modify_witness Witness);
internal void                       MarkEntityModified(entity_with_modify_witness *Witness);

// TODO(hbr): rename to entity_handle
struct entity_handle
{
 entity *Entity;
 u32 Generation;
};

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

struct entity_snapshot_for_merging
{
 entity *Entity;
 
 v2 P;
 v2 Scale;
 v2 Rotation;
 
 entity_flags Flags;
 
 u32 Version;
};

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

struct entity_array
{
 u32 Count;
 entity **Entities;
};

internal void InitEntity(entity *Entity, v2 P, v2 Scale, v2 Rotation, string Name, i32 SortingLayer);
internal void InitCurve(entity_with_modify_witness *Entity, curve_params Params);
internal void InitImage(entity *Entity);
internal void InitEntityFromEntity(entity_with_modify_witness *Dst, entity *Src);

internal v2 WorldToLocalEntityPosition(entity *Entity, v2 P);
internal v2 LocalEntityPositionToWorld(entity *Entity, v2 P);

internal void SetEntityName(entity *Entity, string Name);
internal sort_entry_array SortEntities(arena *Arena, entity_array Entities);
internal void RotateEntityAround(entity *Entity, v2 Rotate, v2 Around);

internal void SetCurvePoint(entity_with_modify_witness *Entity, curve_point_index Index, v2 P, translate_curve_point_flags Flags); // this can be any point - either control or bezier
internal void SetCurveControlPoint(entity_with_modify_witness *Entity, control_point_index Index, v2 P, f32 Weight); // this can be only control point thus we accept weight as well
internal void RemoveControlPoint(entity_with_modify_witness *Entity, u32 Index);
internal control_point_index AppendControlPoint(entity_with_modify_witness *Entity, v2 Point);
internal void InsertControlPoint(entity_with_modify_witness *Entity, v2 Point, u32 At);
internal void SetCurveControlPoints(entity_with_modify_witness *Entity, u32 PointCount, v2 *Points, f32 *Weights, cubic_bezier_point *CubicBeziers);

internal void SelectControlPoint(curve *Curve, control_point_index Index);
internal void SelectControlPointFromCurvePointIndex(curve *Curve, curve_point_index Index);
internal void UnselectControlPoint(curve *Curve);

enum modify_curve_points_which_points
{
 ModifyCurvePointsWhichPoints_JustControlPoints,
 ModifyCurvePointsWhichPoints_ControlPointsWithWeights,
 ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers,
};
struct curve_points_modify_handle
{
 curve *Curve;
 
 u32 PointCount;
 v2 *ControlPoints;
 f32 *Weights;
 cubic_bezier_point *CubicBeziers;
 
 modify_curve_points_which_points Which;
};
internal curve_points_modify_handle BeginModifyCurvePoints(entity_with_modify_witness *Curve, u32 RequestedPointCount, modify_curve_points_which_points Which);
internal void EndModifyCurvePoints(curve_points_modify_handle Handle);

union entity_colors
{
 struct {
  v4 CurveLineColor;
  v4 CurvePointColor;
  v4 CurvePolylineColor;
  v4 CurveConvexHullColor;
 };
 v4 AllColors[4];
};
StaticAssert(SizeOf(MemberOf(entity_colors, AllColors)) == SizeOf(entity_colors), EntityColors_ArrayLengthMatchesDefinedColors);
internal entity_colors ExtractEntityColors(entity *Entity);
internal void ApplyColorsToEntity(entity *Entity, entity_colors Colors);

struct point_info
{
 f32 Radius;
 v4 Color;
 f32 OutlineThickness;
 v4 OutlineColor;
};

internal b32 IsEntityVisible(entity *Entity);
internal void SwitchEntityVisibility(entity *Entity);
internal b32 IsEntitySelected(entity *Entity);
internal b32 IsControlPointSelected(curve *Curve);
internal b32 AreCurvePointsVisible(curve *Curve);
internal b32 UsesControlPoints(curve *Curve);
internal b32 IsPolylineVisible(curve *Curve);
internal b32 IsConvexHullVisible(curve *Curve);
internal point_info GetCurveControlPointInfo(entity *Curve, u32 PointIndex);
internal point_info Get_B_SplineKnotPointInfo(entity *Entity);
internal f32 GetCurveTrackedPointRadius(curve *Curve);
internal f32 GetCurveCubicBezierPointRadius(curve *Curve);
internal visible_cubic_bezier_points GetVisibleCubicBezierPoints(entity *Entity);
internal b32 IsCurveEligibleForPointTracking(curve *Curve);
internal b32 CurveHasWeights(curve *Curve);
internal b32 IsCurveTotalSamplesMode(curve *Curve);
internal b32 IsCurveReversed(entity *Curve);
internal b32 IsRegularBezierCurve(curve *Curve);
internal b32 Are_B_SplineKnotsVisible(curve *Curve);

enum curve_merge_method : u32
{
 CurveMerge_Concat,
 CurveMerge_C0,
 CurveMerge_C1,
 CurveMerge_C2,
 CurveMerge_G1,
 
 CurveMerge_Count,
};
global read_only string CurveMergeNames[] = {
 StrLitComp("Concat"),
 StrLitComp("C0"),
 StrLitComp("C1"),
 StrLitComp("C2"),
 StrLitComp("G1")
};
StaticAssert(ArrayCount(CurveMergeNames) == CurveMerge_Count, CurveMergeNamesDefined);

struct curve_merge_compatibility
{
 b32 Compatible;
 string WhyIncompatible;
};
internal curve_merge_compatibility AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method);

#endif //EDITOR_ENTITY_H