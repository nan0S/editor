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

struct control_point_handle
{
 u32 Index;
};

struct cubic_bezier_point_handle
{
 u32 Index;
};

enum curve_point_type
{
 CurvePoint_ControlPoint,
 CurvePoint_CubicBezierPoint,
};
struct curve_point_handle
{
 curve_point_type Type;
 union {
  control_point_handle Control;
  cubic_bezier_point_handle Bezier;
 };
};

struct visible_cubic_bezier_points
{
 u32 Count;
 cubic_bezier_point_handle Indices[4];
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

struct curve_points_static
{
 u32 ControlPointCount;
#define CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT 1024
 v2 ControlPoints[CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT];
 f32 ControlPointWeights[CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT];
 cubic_bezier_point CubicBezierPoints[CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT];
};

struct curve_points_dynamic
{
 u32 *ControlPointCount;
 v2 *ControlPoints;
 f32 *ControlPointWeights;
 cubic_bezier_point *CubicBezierPoints;
 u32 Capacity;
};

struct curve_points_handle
{
 u32 ControlPointCount;
 v2 *ControlPoints;
 f32 *ControlPointWeights;
 cubic_bezier_point *CubicBezierPoints;
};

struct curve_degree_lowering_state
{
 b32 Active;
 arena *Arena;
 curve_points_static *OriginalCurvePoints;
 vertex_array OriginalCurveVertices;
 bezier_lower_degree LowerDegree;
 f32 MixParameter;
};

typedef u32 translate_curve_point_flags;
enum
{
 TranslateCurvePoint_None,
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
 control_point_handle SelectedControlPoint;
 
 // all points here are in local space
 curve_points_static Points;
 b_spline_knot_params B_SplineKnotParams;
#define MAX_B_SPLINE_KNOT_COUNT (2 * CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT)
 f32 B_SplineKnots[MAX_B_SPLINE_KNOT_COUNT];
 
 arena *ComputeArena;
 
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
 scale2d Dim;
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
};
typedef u32 entity_flags;

enum
{
 EntityInternalFlag_Tracked = (1<<0),
 EntityInternalFlag_Deactivated = (1<<1),
};
typedef u32 entity_internal_flags;

struct entity
{
 entity *Next;
 entity *Prev;
 
 xform2d XForm;
 char_buffer NameBuffer;
 i32 SortingLayer;
 entity_flags Flags;
 
 u32 Id;
 // NOTE(hbr): I don't think this needs to be bigger than u32
 u32 Generation; // increments every time entity is allocated
 u32 Version; // increments every time entity is "recomputed" (see CurveRecompute)
 entity_internal_flags InternalFlags;
 
 entity_type Type;
 curve Curve;
 image Image;
};

struct entity_with_modify_witness
{
 entity *Entity;
 b32 Modified;
};

struct entity_handle
{
 entity *Entity;
 u32 Generation;
};

struct entity_handle_node
{
 entity_handle_node *Next;
 entity_handle Value;
};

struct entity_snapshot_for_merging
{
 entity *Entity;
 xform2d XForm;
 entity_flags Flags;
 u32 Version;
};

struct entity_array
{
 u32 Count;
 entity **Entities;
};

enum modify_curve_points_static_which_points
{
 ModifyCurvePointsWhichPoints_ControlPointsOnly,
 ModifyCurvePointsWhichPoints_ControlPointsWithWeights,
 ModifyCurvePointsWhichPoints_AllPoints,
};
struct curve_points_static_modify_handle
{
 curve *Curve;
 u32 PointCount;
 v2 *ControlPoints;
 f32 *Weights;
 cubic_bezier_point *CubicBeziers;
 modify_curve_points_static_which_points Which;
};

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
StaticAssert(SizeOf(MemberOf(entity_colors, AllColors)) ==
             SizeOf(entity_colors),
             EntityColors_ArrayLengthMatchesDefinedColors);

struct point_draw_info
{
 f32 Radius;
 v4 Color;
 f32 OutlineThickness;
 v4 OutlineColor;
};

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

struct control_point
{
 v2 P;
 f32 Weight;
 cubic_bezier_point Bezier;
 b32 IsWeight;
 b32 IsBezier;
};

// defines visibility/draw order for different curve parts
enum curve_part_visibility
{
 // this is at the bottom
 CurvePartVisibility_LineShadow,
 
 CurvePartVisibility_CurveLine,
 CurvePartVisibility_CurveControlPoint,
 
 CurvePartVisibility_CurvePolyline,
 CurvePartVisibility_CurveConvexHull,
 
 CurvePartVisibility_CubicBezierHelperLines,
 CurvePartVisibility_CubicBezierHelperPoints,
 
 CurvePartVisibility_DeCasteljauAlgorithmLines,
 CurvePartVisibility_DeCasteljauAlgorithmPoints,
 
 CurvePartVisibility_BezierSplitPoint,
 
 CurvePartVisibility_B_SplineKnot,
 // this is at the very top
 
 CurvePartVisibility_Count,
};
internal f32 GetCurvePartVisibilityZOffset(curve_part_visibility Part);

//- entity handles/indices
internal entity_handle MakeEntityHandle(entity *Entity);
internal entity *EntityFromHandle(entity_handle Handle, b32 AllowDeactived = false);

internal entity_with_modify_witness BeginEntityModify(entity *Entity);
internal void EndEntityModify(entity_with_modify_witness Witness);
internal void MarkEntityModified(entity_with_modify_witness *Witness);

internal curve_points_static_modify_handle BeginModifyCurvePoints(entity_with_modify_witness *Curve, u32 RequestedPointCount, modify_curve_points_static_which_points Which);
internal void EndModifyCurvePoints(curve_points_static_modify_handle Handle);

internal control_point_handle ControlPointHandleZero(void);
internal control_point_handle ControlPointHandleFromIndex(u32 Index);
internal u32 IndexFromControlPointHandle(control_point_handle Handle);
internal b32 ControlPointHandleMatch(control_point_handle A, control_point_handle B);
internal control_point_handle ControlPointFromCubicBezierPoint(cubic_bezier_point_handle Bezier);
internal cubic_bezier_point_handle CubicBezierPointHandleFromIndex(u32 Index);
internal cubic_bezier_point_handle CubicBezierPointFromControlPoint(control_point_handle Handle);
internal u32 IndexFromCubicBezierPointHandle(cubic_bezier_point_handle Handle);
internal b32 CubicBezierPointHandleMatch(cubic_bezier_point_handle A, cubic_bezier_point_handle B);
internal cubic_bezier_point_handle CubicBezierPointHandleZero(void);
internal curve_point_handle CurvePointFromControlPoint(control_point_handle Handle);
internal curve_point_handle CurvePointFromCubicBezierPoint(cubic_bezier_point_handle Handle);
internal control_point_handle ControlPointFromCurvePoint(curve_point_handle Handle);

internal control_point MakeControlPoint(v2 Point);
internal control_point MakeControlPoint(v2 Point, f32 Weight, cubic_bezier_point Bezier);

internal curve_points_handle MakeCurvePointsHandle(u32 ControlPointCount, v2 *ControlPoints, f32 *ControlPointWeights, cubic_bezier_point *CubicBezierPoints);
internal curve_points_handle CurvePointsHandleZero(void);
internal curve_points_handle CurvePointsHandleFromCurvePointsStatic(curve_points_static *Static);
internal curve_points_dynamic MakeCurvePointsDynamic(u32 *ControlPointCount, v2 *ControlPoints, f32 *ControlPointWeights, cubic_bezier_point *CubicBezierPoints, u32 Capacity);
internal curve_points_dynamic CurvePointsDynamicFromStatic(curve_points_static *Static);
internal void CopyCurvePoints(curve_points_dynamic Dst, curve_points_handle Src);

//- entity initialization
internal void InitEntityPart(entity *Entity, entity_type Type, xform2d XForm, string Name, i32 SortingLayer, entity_flags Flags);
internal void InitEntityAsImage(entity *Entity, v2 P, u32 Width, u32 Height, string FilePath);
internal void InitEntityAsCurve(entity *Entity, string Name, curve_params CurveParams);

//- entity modify
internal void TranslateCurvePointTo(entity_with_modify_witness *Entity, curve_point_handle Handle, v2 P, translate_curve_point_flags Flags); // this can be any point - either control or bezier
internal void SetControlPoint(entity_with_modify_witness *Entity, control_point_handle Handle, v2 P, f32 Weight); // this can be only control point thus we accept weight as well
internal void SetControlPoint(entity_with_modify_witness *Witness, control_point_handle Handle, control_point Point);
internal void RemoveControlPoint(entity_with_modify_witness *Entity, control_point_handle Point);
internal control_point_handle AppendControlPoint(entity_with_modify_witness *Entity, v2 P);
internal control_point_handle InsertControlPoint(entity_with_modify_witness *Entity, control_point Point, u32 At);
internal void SetCurvePoints(entity_with_modify_witness *EntityWitness, curve_points_handle Points);
internal void SelectControlPoint(curve *Curve, control_point_handle ControlPoint);
internal void SelectControlPointFromCurvePoint(curve *Curve, curve_point_handle CurvePoint);
internal void DeselectControlPoint(curve *Curve);
internal void ApplyColorsToEntity(entity *Entity, entity_colors Colors);
internal void MarkEntitySelected(entity *Entity);
internal void MarkEntityDeselected(entity *Entity);
internal void SetEntityVisibility(entity *Entity, b32 Visible);

internal v2 WorldToLocalEntityPosition(entity *Entity, v2 P);
internal v2 LocalToWorldEntityPosition(entity *Entity, v2 P);

//- entity query
internal b32 IsEntityVisible(entity *Entity);
internal b32 IsEntitySelected(entity *Entity);
internal b32 IsControlPointSelected(curve *Curve);
internal b32 AreCurvePointsVisible(curve *Curve);
internal b32 UsesControlPoints(curve *Curve);
internal b32 IsPolylineVisible(curve *Curve);
internal b32 IsConvexHullVisible(curve *Curve);
internal point_draw_info GetCurveControlPointDrawInfo(entity *Curve, control_point_handle Point);
internal point_draw_info Get_B_SplineKnotPointDrawInfo(entity *Entity);
internal f32 GetCurveTrackedPointRadius(curve *Curve);
internal f32 GetCurveCubicBezierPointRadius(curve *Curve);
internal visible_cubic_bezier_points GetVisibleCubicBezierPoints(entity *Entity);
internal b32 IsCurveEligibleForPointTracking(curve *Curve);
internal b32 CurveHasWeights(curve *Curve);
internal b32 IsCurveTotalSamplesMode(curve *Curve);
internal b32 IsCurveReversed(entity *Curve);
internal b32 IsRegularBezierCurve(curve *Curve);
internal b32 Are_B_SplineKnotsVisible(curve *Curve);
internal entity_colors ExtractEntityColors(entity *Entity);
internal string GetEntityName(entity *Entity);
internal control_point GetCurveControlPointInWorldSpace(entity *Entity, control_point_handle Point);
internal void CopyCurvePointsFromCurve(curve *Curve, curve_points_dynamic *Dst);

//- entity for merging tracker
internal entity_snapshot_for_merging MakeEntitySnapshotForMerging(entity *Entity);
internal b32 EntityModified(entity_snapshot_for_merging Versioned, entity *Entity);
internal curve_merge_compatibility AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method);

//- misc/helpers
internal curve *SafeGetCurve(entity *Entity);
internal image *SafeGetImage(entity *Entity);

#endif //EDITOR_ENTITY_H