#ifndef EDITOR_DEV_H
#define EDITOR_DEV_H

struct editor;
struct parametric_equation_expr;
struct exponential_animation;
struct entity;

enum nurbs_eval_method : u32
{
 NURBS_Eval_Scalar_UsingBSplineEvaluate,
 NURBS_Eval_Scalar,
 NURBS_Eval_ScalarButUsingSSE,
 NURBS_Eval_SSE,
 NURBS_Eval_SSE_MultiThreaded,
 NURBS_Eval_Count
};
global read_only string NURBS_Eval_Names[] = {
 StrLit("NURBS_Eval_Scalar_UsingBSplineEvaluate"),
 StrLit("NURBS_Eval_Scalar"),
 StrLit("NURBS_Eval_ScalarButUsingSSE"),
 StrLit("NURBS_Eval_SSE"),
 StrLit("NURBS_Eval_SSE_MultiThreaded"),
};
StaticAssert(ArrayCount(NURBS_Eval_Names) == NURBS_Eval_Count, NURBS_Eval_Names_AllDefined);

enum parametric_eval_method : u32
{
 Parametric_Eval_SingleThreaded,
 Parametric_Eval_MultiThreaded,
 Parametric_Eval_Count
};
global read_only string Parametric_Eval_Names[] = {
 StrLit("Parametric_Eval_SingleThreaded"),
 StrLit("Parametric_Eval_MultiThreaded"),
};
StaticAssert(ArrayCount(Parametric_Eval_Names) == Parametric_Eval_Count, Parametric_Eval_Names_AllDefined);

enum bezier_eval_method : u32
{
 Bezier_Eval_Scalar,
 Bezier_Eval_SSE,
 Bezier_Eval_AVX2,
 Bezier_Eval_AVX512,
 Bezier_Eval_SSE_MultiThreaded,
 Bezier_Eval_AVX2_MultiThreaded,
 Bezier_Eval_AVX512_MultiThreaded,
 Bezier_Eval_Adaptive_MultiThreaded,
 Bezier_Eval_Count
};
global read_only string Bezier_Eval_Names[] = {
 StrLit("Bezier_Eval_Scalar"),
 StrLit("Bezier_Eval_SSE"),
 StrLit("Bezier_Eval_AVX2"),
 StrLit("Bezier_Eval_AVX512"),
 StrLit("Bezier_Eval_SSE_MultiThreaded"),
 StrLit("Bezier_Eval_AVX2_MultiThreaded"),
 StrLit("Bezier_Eval_AVX512_MultiThreaded"),
 StrLit("Bezier_Eval_Adaptive_MultiThreaded"),
};
StaticAssert(ArrayCount(Bezier_Eval_Names) == Bezier_Eval_Count, Bezier_Eval_Names_AllDefined);

enum cubic_spline_eval_method : u32
{
 CubicSpline_Eval_Scalar,
 CubicSpline_Eval_Scalar_MultiThreaded,
 CubicSpline_Eval_Count
};
global read_only string CubicSpline_Eval_Names[] = {
 StrLit("CubicSpline_Eval_Scalar"),
 StrLit("CubicSpline_Eval_Scalar_MultiThreaded"),
};
StaticAssert(ArrayCount(CubicSpline_Eval_Names) == CubicSpline_Eval_Count, CubicSpline_Eval_Names_AllDefined);

struct debug_vars
{
 b32 Initialized;
 
 b32 NURBS_Benchmark;
 entity *NURBS_BenchmarkEntity;
 nurbs_eval_method NURBS_EvalMethod;
 
 b32 Parametric_Benchmark;
 entity *Parametric_BenchmarkEntity;
 parametric_eval_method Parametric_EvalMethod;
 
 b32 Bezier_Benchmark;
 entity *Bezier_BenchmarkEntity;
 bezier_eval_method Bezier_EvalMethod;
 
 b32 CubicSpline_Benchmark;
 entity *CubicSpline_BenchmarkEntity;
 cubic_spline_eval_method CubicSpline_EvalMethod;
 
 u32 MultiThreadedEvaluationBlockSize;
 
 b32 DevConsole;
 b32 ParametricEquationDebugMode;
 u32 DrawnGridLinesCount;
 b32 ShowSampleCurvePoints;
 
 u32 ParametricCurveMaxTotalSamples;
};
global debug_vars *DEBUG_Vars;

internal void DevUpdateAndRender(editor *Editor);
internal void UI_ParametricEquationExpr(parametric_equation_expr *Expr, string Label);
internal void UI_ExponentialAnimation(exponential_animation *Anim);

#endif //EDITOR_DEV_H
