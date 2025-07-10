#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

enum curve_type : u32
{
 Curve_CubicSpline,
 Curve_Bezier,
 Curve_Polynomial,
 Curve_Parametric,
 Curve_BSpline,
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
global read_only string
PolynomialInterpolationTypeNames[] = {
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
 BSplinePartition_Natural,
 BSplinePartition_Periodic,
 BSplinePartition_Custom,
 BSplinePartition_Count,
};
global read_only string BSplinePartitionNames[] = {
 StrLitComp("Natural"),
 StrLitComp("Periodic"),
 StrLitComp("Custom"),
};
StaticAssert(ArrayCount(BSplinePartitionNames) == BSplinePartition_Count, BSplinePartitionNamesDefined);

struct draw_params
{
 b32 Enabled;
 rgba Color;
 union {
  f32 Width;
  f32 Radius;
  f32 Float;
 };
};

struct b_spline_params
{
 b_spline_partition_type Partition;
 b_spline_knot_params KnotParams;
};

union curve_draw_params
{
 struct {
  draw_params Line;
  draw_params Points;
  draw_params Polyline;
  draw_params ConvexHull;
  draw_params BSplineKnots;
  draw_params BSplinePartialConvexHull;
 };
 draw_params All[6];
};

struct curve_params
{
 curve_type Type;
 polynomial_interpolation_params Polynomial;
 cubic_spline_type CubicSpline;
 bezier_type Bezier;
 parametric_curve_params Parametric;
 b_spline_params BSpline;
 curve_draw_params DrawParams;
 u32 SamplesPerControlPoint;
 u32 TotalSamples;
};
StaticAssert(SizeOf(MemberOf(curve_params, DrawParams)) ==
             SizeOf(MemberOf(curve_params, DrawParams.All)),
             CurveParams_AllDrawParamsArrayLengthMatchesIndividuallyDefinedDrawParams);

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

enum parametric_curve_predefined_example_type : u32
{
 ParametricCurvePredefinedExample_None,
 ParametricCurvePredefinedExample_CircleEllipse,
 ParametricCurvePredefinedExample_LissajousCurve,
 ParametricCurvePredefinedExample_ButterflyCurve,
 ParametricCurvePredefinedExample_Spiral,
 ParametricCurvePredefinedExample_SpiralSkewed,
 ParametricCurvePredefinedExample_WavyCircle,
 ParametricCurvePredefinedExample_FlowerPetal,
 ParametricCurvePredefinedExample_Wave,
 ParametricCurvePredefinedExample_Epicycloid2,
 ParametricCurvePredefinedExample_Epicycloid3,
 ParametricCurvePredefinedExample_Epicycloid4,
 ParametricCurvePredefinedExample_Epicycloid5,
 ParametricCurvePredefinedExample_Hypocycloid3,
 ParametricCurvePredefinedExample_Hypocycloid4,
 ParametricCurvePredefinedExample_Hypocycloid5,
 ParametricCurvePredefinedExample_Epitrochoid3,
 ParametricCurvePredefinedExample_Epitrochoid5,
 ParametricCurvePredefinedExample_Epitrochoid7,
 ParametricCurvePredefinedExample_Epitrochoid8,
 ParametricCurvePredefinedExample_Hypotrochoid3,
 ParametricCurvePredefinedExample_Hypotrochoid5,
 ParametricCurvePredefinedExample_Hypotrochoid7_1,
 ParametricCurvePredefinedExample_Hypotrochoid7_2,
 ParametricCurvePredefinedExample_Hypotrochoid8,
 ParametricCurvePredefinedExample_Hypotrochoid15,
 ParametricCurvePredefinedExample_Count,
};
global read_only string ParametricCurvePredefinedExampleNames[] = {
 StrLit("<choose>"),
 StrLit("Circle/Ellipse"),
 StrLit("Lissajous Curve"),
 StrLit("Butterfly Curve"),
 StrLit("Spiral"),
 StrLit("Spiral Skewed"),
 StrLit("Wavy Circle"),
 StrLit("Flower Petal"),
 StrLit("Wave"),
 StrLit("Epicycloid 2"),
 StrLit("Epicycloid 3"),
 StrLit("Epicycloid 4"),
 StrLit("Epicycloid 5"),
 StrLit("Hypocycloid 3"),
 StrLit("Hypocycloid 4"),
 StrLit("Hypocycloid 5"),
 StrLit("Epitrochoid 3"),
 StrLit("Epitrochoid 5"),
 StrLit("Epitrochoid 7"),
 StrLit("Epitrochoid 8"),
 StrLit("Hypotrochoid 3"),
 StrLit("Hypotrochoid 5"),
 StrLit("Hypotrochoid 7.1"),
 StrLit("Hypotrochoid 7.2"),
 StrLit("Hypotrochoid 8"),
 StrLit("Hypotrochoid 15"),
};
StaticAssert(ArrayCount(ParametricCurvePredefinedExampleNames) == ParametricCurvePredefinedExample_Count,
             ParametricCurvePredefinedExampleNamesDefined);
struct parametric_curve_predefined_example_var
{
 string Name;
 b32 EquationMode;
 f32 Value;
 string Equation;
};
struct parametric_curve_predefined_example
{
 string X_Equation;
 string Y_Equation;
 parametric_curve_predefined_example_var Min_T;
 parametric_curve_predefined_example_var Max_T;
 parametric_curve_predefined_example_var AdditionalVars[MAX_ADDITIONAL_VAR_COUNT];
};
#define ParametricCurvePredefinedExampleVarEquation(Name, Equation) { Name, true, 0, Equation }
#define ParametricCurvePredefinedExampleVarValue(Name, Value, Equation) { Name, false, Value, Equation }
global read_only parametric_curve_predefined_example NilParametricCurvePredefinedExample;
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleCircleEllipse = 
{ 
 StrLit("a*cos(t)"),
 StrLit("b*sin(t)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32, StrLit("2pi")),
 {
  ParametricCurvePredefinedExampleVarValue(StrLit("a"), 1, StrLit("1")),
  ParametricCurvePredefinedExampleVarValue(StrLit("b"), 1, StrLit("1")),
 },
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleLissajousCurve =
{ 
 StrLit("a*cos(k_x*t)"),
 StrLit("b*sin(k_y*t)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32, StrLit("2pi")),
 {
  ParametricCurvePredefinedExampleVarValue(StrLit("a"), 1, StrLit("1")),
  ParametricCurvePredefinedExampleVarValue(StrLit("b"), 1, StrLit("1")),
  ParametricCurvePredefinedExampleVarValue(StrLit("k_x"), 3, StrLit("3")),
  ParametricCurvePredefinedExampleVarValue(StrLit("k_y"), 2, StrLit("2")),
 },
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleButterflyCurve =
{
 StrLit("sin(t) * (exp(cos(t)) - 2*cos(4*t) - sin(t/12)^5)"),
 StrLit("cos(t) * (exp(cos(t)) - 2*cos(4*t) - sin(t/12)^5)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 12*PiF32, StrLit("12pi")),
 {}
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleSpiral =
{
 StrLit("t*cos(2*pi*t)"),
 StrLit("t*sin(2*pi*t)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 4, StrLit("4")),
 {}
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleSpiralSkewed =
{
 StrLit("t*cos(2*pi*t) + t"),
 StrLit("t*sin(2*pi*t)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 4, StrLit("4")),
 {}
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleWavyCircle =
{
 StrLit("(r + h*sin(d*pi*t)) * cos(2*pi*t)"),
 StrLit("(r + h*sin(d*pi*t)) * sin(2*pi*t)"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 1, StrLit("1")),
 {
  ParametricCurvePredefinedExampleVarValue(StrLit("r"), 1, StrLit("1")),
  ParametricCurvePredefinedExampleVarValue(StrLit("h"), 0.1f, StrLit("0.1")),
  ParametricCurvePredefinedExampleVarValue(StrLit("d"), 50, StrLit("50")),
 }
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleFlowerPetal =
{
 StrLit("2.5*sin(-5*t)^2 * 2^(cos(cos(4.28*2.3*t)))"),
 StrLit("2.5*sin(sin(-5*t)) * cos(4.28*2.3*t)^2"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), -6, StrLit("-6")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 6, StrLit("6")),
 {}
};
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleWave =
{
 StrLit("10*sin(2.78*t) * round(sqrt(cos(cos(8.2*t))))"),
 StrLit("9*cos(2.78*t)^2 * sin(sin(8.2*t))"),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), -6.2f, StrLit("-6.2")),
 ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 6.2f, StrLit("6.2")),
 {}
};
#define DefineEpicycloid(R, r) \
{ \
StrLit("(R+r)*cos(t) - r*cos((R+r)/r*t)"), \
StrLit("(R+r)*sin(t) - r*sin((R+r)/r*t)"), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32, StrLit("2pi")), \
{ \
ParametricCurvePredefinedExampleVarValue(StrLit("R"), R, StrLit(#R)), \
ParametricCurvePredefinedExampleVarValue(StrLit("r"), r, StrLit(#r)), \
}, \
}
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpicycloid2 = DefineEpicycloid(2, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpicycloid3 = DefineEpicycloid(3, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpicycloid4 = DefineEpicycloid(4, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpicycloid5 = DefineEpicycloid(5, 1);
#define DefineHypocycloid(R, r) \
{ \
StrLit("(R-r)*cos(t) + r*cos((R-r)/r*t)"), \
StrLit("(R-r)*sin(t) - r*sin((R-r)/r*t)"), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32, StrLit("2pi")), \
{ \
ParametricCurvePredefinedExampleVarValue(StrLit("R"), R, StrLit(#R)), \
ParametricCurvePredefinedExampleVarValue(StrLit("r"), r, StrLit(#r)), \
}, \
}
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypocycloid3 = DefineHypocycloid(3, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypocycloid4 = DefineHypocycloid(4, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypocycloid5 = DefineHypocycloid(5, 1);
#define DefineEpitrochoid(R, r, d) \
{ \
StrLit("(R+r)*cos(t) - d*cos((R+r)/r*t)"), \
StrLit("(R+r)*sin(t) - d*sin((R+r)/r*t)"), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32 * R*r/R, StrLit("2pi * " #R "*" #r "/" #R)), \
{ \
ParametricCurvePredefinedExampleVarValue(StrLit("R"), R, StrLit(#R)), \
ParametricCurvePredefinedExampleVarValue(StrLit("r"), r, StrLit(#r)), \
ParametricCurvePredefinedExampleVarValue(StrLit("d"), d, StrLit(#d)), \
}, \
}
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpitrochoid5 = DefineEpitrochoid(5, 3, 5);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpitrochoid3 = DefineEpitrochoid(6, 4, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpitrochoid7 = DefineEpitrochoid(7, 4, 2);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleEpitrochoid8 = DefineEpitrochoid(8, 3, 2);
// TODO(hbr): There is a gcd missing in this definition
#define DefineHypotrochoid(R, r, d) \
{ \
StrLit("(R-r)*cos(t) + d*cos((R-r)/r*t)"), \
StrLit("(R-r)*sin(t) - d*sin((R-r)/r*t)"), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_min"), 0, StrLit("0")), \
ParametricCurvePredefinedExampleVarValue(StrLit("t_max"), 2*PiF32 * R*r/R, StrLit("2pi * " #R "*" #r "/" #R)), \
{ \
ParametricCurvePredefinedExampleVarValue(StrLit("R"), R, StrLit(#R)), \
ParametricCurvePredefinedExampleVarValue(StrLit("r"), r, StrLit(#r)), \
ParametricCurvePredefinedExampleVarValue(StrLit("d"), d, StrLit(#d)), \
}, \
}
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid5 = DefineHypotrochoid(5, 3, 5);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid3 = DefineHypotrochoid(6, 4, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid7_1 = DefineHypotrochoid(7, 4 ,1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid7_2 = DefineHypotrochoid(7, 4, 2);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid8 = DefineHypotrochoid(8, 3, 2);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExampleHypotrochoid15 = DefineHypotrochoid(15, 14, 1);
global read_only parametric_curve_predefined_example ParametricCurvePredefinedExamples[] = {
 NilParametricCurvePredefinedExample,
 ParametricCurvePredefinedExampleCircleEllipse,
 ParametricCurvePredefinedExampleLissajousCurve,
 ParametricCurvePredefinedExampleButterflyCurve,
 ParametricCurvePredefinedExampleSpiral,
 ParametricCurvePredefinedExampleSpiralSkewed,
 ParametricCurvePredefinedExampleWavyCircle,
 ParametricCurvePredefinedExampleFlowerPetal,
 ParametricCurvePredefinedExampleWave,
 ParametricCurvePredefinedExampleEpicycloid2,
 ParametricCurvePredefinedExampleEpicycloid3,
 ParametricCurvePredefinedExampleEpicycloid4,
 ParametricCurvePredefinedExampleEpicycloid5,
 ParametricCurvePredefinedExampleHypocycloid3,
 ParametricCurvePredefinedExampleHypocycloid4,
 ParametricCurvePredefinedExampleHypocycloid5,
 ParametricCurvePredefinedExampleEpitrochoid3,
 ParametricCurvePredefinedExampleEpitrochoid5,
 ParametricCurvePredefinedExampleEpitrochoid7,
 ParametricCurvePredefinedExampleEpitrochoid8,
 ParametricCurvePredefinedExampleHypotrochoid3,
 ParametricCurvePredefinedExampleHypotrochoid5,
 ParametricCurvePredefinedExampleHypotrochoid7_1,
 ParametricCurvePredefinedExampleHypotrochoid7_2,
 ParametricCurvePredefinedExampleHypotrochoid8,
 ParametricCurvePredefinedExampleHypotrochoid15,
};
StaticAssert(ArrayCount(ParametricCurvePredefinedExamples) == ParametricCurvePredefinedExample_Count,
             ParametricCurvePredefinedExamplesDefined);

struct b_spline_convex_hull
{
 v2 *Points;
 vertex_array Vertices;
};

struct b_spline_knot_handle
{
 u32 __Index;
};

struct curve
{
 curve_params Params; // used to compute curve shape from (might be still validated and not used "as-is")
 control_point_handle SelectedControlPoint;
 b_spline_knot_handle SelectedBSplineKnot;
 b_spline_params ComputedBSplineParams;
 point_tracking_along_curve_state PointTracking;
 curve_degree_lowering_state DegreeLowering;
 parametric_curve_resources ParametricResources;
 
 // all points here are in local space
 curve_points_static Points;
#define MAX_B_SPLINE_KNOT_COUNT (2 * CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT)
 f32 BSplineKnots[MAX_B_SPLINE_KNOT_COUNT];
 v2 BSplinePartitionKnots[MAX_B_SPLINE_KNOT_COUNT];
 
 arena *ComputeArena;
 u32 CurveSampleCount;
 v2 *CurveSamples;
 u32 ConvexHullCount;
 v2 *ConvexHullPoints;
 vertex_array CurveVertices;
 vertex_array PolylineVertices;
 vertex_array ConvexHullVertices;
 u32 BSplineConvexHullCount;
 b_spline_convex_hull *BSplineConvexHulls;
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
struct curve_points_modify_handle
{
 curve *Curve;
 u32 PointCount;
 v2 *ControlPoints;
 f32 *Weights;
 cubic_bezier_point *CubicBeziers;
 modify_curve_points_static_which_points Which;
};

struct point_draw_info
{
 f32 Radius;
 rgba Color;
 f32 OutlineThickness;
 rgba OutlineColor;
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
 
 CurvePartVisibility_BSplineKnot,
 
 CurvePartVisibility_CurveSamplePoint,
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

internal curve_points_modify_handle BeginModifyCurvePoints(entity_with_modify_witness *Curve, u32 RequestedPointCount, modify_curve_points_static_which_points Which);
internal void EndModifyCurvePoints(curve_points_modify_handle Handle);

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

internal b_spline_knot_handle BSplineKnotHandleZero(void);
internal b_spline_knot_handle BSplineKnotHandleFromKnotIndex(u32 KnotIndex);
internal b_spline_knot_handle BSplineKnotHandleFromPartitionKnotIndex(curve *Curve, u32 Index);
internal b32 BSplineKnotHandleMatch(b_spline_knot_handle A, b_spline_knot_handle B);
internal u32 KnotIndexFromBSplineKnotHandle(b_spline_knot_handle Handle);
internal u32 PartitionKnotIndexFromBSplineKnotHandle(curve *Curve, b_spline_knot_handle Handle);

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
internal void MarkEntitySelected(entity *Entity);
internal void MarkEntityDeselected(entity *Entity);
internal void SetEntityVisibility(entity *Entity, b32 Visible);
internal void SetTrackingPointFraction(entity_with_modify_witness *EntityWitness, f32 Fraction);
internal void SetBSplineKnotPoint(entity_with_modify_witness *EntityWitness, b_spline_knot_handle Knot, f32 KnotFraction);
internal void SetEntityName(entity *Entity, string Name);

//- entity query
internal b32 IsEntityVisible(entity *Entity);
internal b32 IsEntitySelected(entity *Entity);
internal b32 IsControlPointSelected(curve *Curve);
internal b32 AreCurvePointsVisible(curve *Curve);
internal b32 UsesControlPoints(curve *Curve);
internal b32 IsPolylineVisible(curve *Curve);
internal b32 IsConvexHullVisible(curve *Curve);
internal point_draw_info GetCurveControlPointDrawInfo(entity *Curve, control_point_handle Point);
internal f32 GetCurveTrackedPointRadius(curve *Curve);
internal f32 GetCurveCubicBezierPointRadius(curve *Curve);
internal visible_cubic_bezier_points GetVisibleCubicBezierPoints(entity *Entity);
internal b32 IsCurveEligibleForPointTracking(curve *Curve);
internal b32 CurveHasWeights(curve *Curve);
internal b32 IsCurveTotalSamplesMode(curve *Curve);
internal b32 IsCurveReversed(entity *Curve);
internal b32 IsRegularBezierCurve(curve *Curve);
internal b32 AreBSplineKnotsVisible(curve *Curve);
internal string GetEntityName(entity *Entity);
internal control_point GetCurveControlPointInWorldSpace(entity *Entity, control_point_handle Point);
internal void CopyCurvePointsFromCurve(curve *Curve, curve_points_dynamic *Dst);
internal rect2 EntityAABB(curve *Curve);
internal b_spline_params GetBSplineParams(curve *Curve);

//- b-spline specific
internal point_draw_info GetBSplinePartitionKnotPointDrawInfo(entity *Entity);
internal b32 IsBSplineCurve(curve *Curve);
internal b32 AreBSplineConvexHullsVisible(curve *Curve);

//- entity for merging tracker
internal entity_snapshot_for_merging MakeEntitySnapshotForMerging(entity *Entity);
internal b32 EntityModified(entity_snapshot_for_merging Versioned, entity *Entity);
internal curve_merge_compatibility AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method);

//- misc/helpers
internal curve *SafeGetCurve(entity *Entity);
internal image *SafeGetImage(entity *Entity);

internal v2 WorldToLocalEntityPosition(entity *Entity, v2 P);
internal v2 LocalToWorldEntityPosition(entity *Entity, v2 P);

internal curve_params DefaultCurveParams(void);
internal curve_params DefaultCubicSplineCurveParams(void);

#endif //EDITOR_ENTITY_H