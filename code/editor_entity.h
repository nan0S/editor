#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

// TODO(hbr): This is not interpolation_type anymore - with parametric variant
enum interpolation_type : u32
{
 Interpolation_CubicSpline,
 Interpolation_Bezier,
 Interpolation_Polynomial,
 Interpolation_Parametric,
 Interpolation_Count
};
// TODO(hbr): Try to add designated array intializers
global char const *InterpolationNames[] = { "Cubic Spline", "Bezier", "Polynomial", "Parametric" };
StaticAssert(ArrayCount(InterpolationNames) == Interpolation_Count, InterpolationNamesDefined);

enum polynomial_interpolation_type : u32
{
 PolynomialInterpolation_Barycentric,
 PolynomialInterpolation_Newton,
 PolynomialInterpolation_Count,
};
global char const *PolynomialInterpolationNames[] = { "Barycentric", "Newton" };
StaticAssert(ArrayCount(PolynomialInterpolationNames) == PolynomialInterpolation_Count, PolynomialInterpolationNamesDefined);

enum point_spacing : u32
{
 PointSpacing_Chebychev,
 PointSpacing_Equidistant,
 PointSpacing_Count,
};
global char const *PointSpacingNames[] = { "Chebychev", "Equidistant" };
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
global char const *CubicSplineNames[] = { "Natural", "Periodic" };
StaticAssert(ArrayCount(CubicSplineNames) == CubicSpline_Count, CubicSplineNamesDefined);

enum bezier_type : u32
{
 Bezier_Regular,
 Bezier_Cubic,
 Bezier_Count,
};
global char const *BezierNames[] = { "Regular", "Cubic" };
StaticAssert(ArrayCount(BezierNames) == Bezier_Count, BezierNamesDefined);

struct parametric_curve_params
{
 f32 MinT;
 f32 MaxT;
 
 parametric_equation_expr *X_Equation;
 parametric_equation_expr *Y_Equation;
};

struct curve_params
{
 interpolation_type Interpolation;
 polynomial_interpolation_params Polynomial;
 cubic_spline_type CubicSpline;
 bezier_type Bezier;
 parametric_curve_params Parametric;
 
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

struct curve_point_tracking_state
{
 b32 Active;
 f32 Fraction;
 v2 LocalSpaceTrackedPoint;
 
 b32 IsSplitting;
 
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
 
#define MAX_ADDITIONAL_VARS_COUNT 16
 u32 AdditionalVarCount;
 parametric_curve_var AdditionalVars[MAX_ADDITIONAL_VARS_COUNT];
};

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

struct curve
{
 curve_params Params;
 
 // all points here are in local space
 u32 ControlPointCount;
#define MAX_CONTROL_POINT_COUNT 1024
 v2 ControlPoints[MAX_CONTROL_POINT_COUNT];
 f32 ControlPointWeights[MAX_CONTROL_POINT_COUNT];
 cubic_bezier_point CubicBezierPoints[MAX_CONTROL_POINT_COUNT];
 
 control_point_index SelectedIndex;
 
 u32 LinePointCount;
 v2 *LinePoints;
 
 u32 ConvexHullCount;
 v2 *ConvexHullPoints;
 
 vertex_array LineVertices;
 vertex_array PolylineVertices;
 vertex_array ConvexHullVertices;
 
 curve_point_tracking_state PointTracking;
 curve_degree_lowering_state DegreeLowering;
 parametric_curve_resources ParametricResources;
};

struct renderer_index;
struct image
{
 v2 Dim;
 renderer_index *TextureIndex;
};

enum entity_type
{
 Entity_Curve,
 Entity_Image,
 Entity_Count,
};

enum
{
 EntityFlag_Active   = (1<<0),
 EntityFlag_Hidden   = (1<<1),
 EntityFlag_Selected = (1<<2),
 EntityFlag_CurveAppendFront = (1<<3),
};
typedef u32 entity_flags;

struct entity
{
 arena *Arena;
 
 v2 P;
 v2 Scale;
 v2 Rotation;
 
 char NameBuffer[64];
 string Name;
 
 i32 SortingLayer;
 entity_flags Flags;
 
 entity_type Type;
 curve Curve;
 image Image;
};

struct entity_with_modify_witness
{
 entity *Entity;
 b32 EntityModified;
};
global entity_with_modify_witness _NilEntityWitness;
global entity_with_modify_witness *NilEntityWitness = &_NilEntityWitness;

internal void MarkEntityModified(entity_with_modify_witness *Witness);
internal entity_with_modify_witness BeginEntityModify(entity *Entity);
internal void EndEntityModify(entity_with_modify_witness Witness);

struct entity_array
{
 u32 Count;
 entity *Entities;
};

struct point_info
{
 f32 Radius;
 v4 Color;
 f32 OutlineThickness;
 v4 OutlineColor;
};

internal void InitAllocEntity(entity *Entity);
internal void InitEntity(entity *Entity, v2 P, v2 Scale, v2 Rotation, string Name, i32 SortingLayer);
internal void InitCurve(entity_with_modify_witness *Entity, curve_params Params);
internal void InitImage(entity *Entity);
internal void InitEntityFromEntity(entity_with_modify_witness *Dst, entity *Src);

internal v2 WorldToLocalEntityPosition(entity *Entity, v2 P);
internal v2 LocalEntityPositionToWorld(entity *Entity, v2 P);

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

internal void SetEntityName(entity *Entity, string Name);
internal sorted_entries SortEntities(arena *Arena, entity_array Entities);
internal void RotateEntityAround(entity *Entity, v2 Rotate, v2 Around);

internal b32 IsEntityVisible(entity *Entity);
internal void SwitchEntityVisibility(entity *Entity);
internal b32 IsEntitySelected(entity *Entity);
internal b32 IsControlPointSelected(curve *Curve);
internal b32 AreLinePointsVisible(curve *Curve);
internal b32 DoesCurveUseControlPoints(curve *Curve);
internal point_info GetCurveControlPointInfo(entity *Curve, u32 PointIndex);
internal f32 GetCurveTrackedPointRadius(curve *Curve);
internal f32 GetCurveCubicBezierPointRadius(curve *Curve);
internal visible_cubic_bezier_points GetVisibleCubicBezierPoints(entity *Entity);
internal b32 IsCurveEligibleForPointTracking(curve *Curve);
internal b32 CurveHasWeights(curve *Curve);
internal b32 IsCurveTotalSamplesMode(curve *Curve);
internal b32 CanAddControlPoints(curve *Curve);

enum curve_merge_method : u32
{
 CurveMerge_Concat,
 
 CurveMerge_C0,
 CurveMerge_C1,
 CurveMerge_G1,
 CurveMerge_C2,
 
 CurveMerge_Count,
};
global char const *CurveMergeNames[] = { "Concat", "C0", "C1", "C2", "G1" };
StaticAssert(ArrayCount(CurveMergeNames) == CurveMerge_Count, CurveMergeNamesDefined);

struct curve_merge_compatibility
{
 b32 Compatible;
 string WhyIncompatible;
};
internal curve_merge_compatibility AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method);

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
 ModifyCurvePointsWhichPoints_ControlPointsWithCubicBeziers,
};
struct curve_points
{
 u32 PointCount;
 v2 *ControlPoints;
 f32 *Weights;
 cubic_bezier_point *CubicBeziers;
 modify_curve_points_which_points Which;
};

internal curve_points BeginModifyCurvePoints(entity_with_modify_witness *Curve, u32 RequestedPointCount, modify_curve_points_which_points Which);
internal void EndModifyCurvePoints(curve *Curve, curve_points *Handle);

#endif //EDITOR_ENTITY_H