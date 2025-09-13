#ifndef EDITOR_DEV_H
#define EDITOR_DEV_H

struct editor;
struct parametric_equation_expr;
struct exponential_animation;
struct entity;

enum nurbs_eval_method : u32
{
 NURBS_Eval_BSpline_Evaluate,
 NURBS_Eval_NURBS_Evaluate,
 NURBS_Eval_SimdWith1Lane,
 NURBS_Eval_SimdWith4Lanes,
 NURBS_Eval_SimdMultithreaded,
 NURBS_Eval_SimdMultithreadedUnrolled,
 NURBS_Eval_Count
};
global read_only string NURBS_Eval_Names[] = {
 StrLit("NURBS_Eval_NURBS_Evaluate"),
 StrLit("NURBS_Eval_BSpline_Evaluate"),
 StrLit("NURBS_Eval_SimdWith1Lane"),
 StrLit("NURBS_Eval_SimdWith4Lanes"),
 StrLit("NURBS_Eval_SimdMultithreaded"),
 StrLit("NURBS_Eval_SimdMultithreadedUnrolled"),
};

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

enum bezier_eval_method : u32
{
 Bezier_Eval_Scalar,
 Bezier_Eval_SSE,
 Bezier_Eval_AVX2,
 Bezier_Eval_AVX512,
 Bezier_Eval_SSE_Multithreaded,
 Bezier_Eval_AVX2_Multithreaded,
 Bezier_Eval_AVX512_Multithreaded,
 Bezier_Eval_Adaptive_Multithreaded,
 Bezier_Eval_Count
};
global read_only string Bezier_Eval_Names[] = {
 StrLit("Bezier_Eval_Scalar"),
 StrLit("Bezier_Eval_SSE"),
 StrLit("Bezier_Eval_AVX2"),
 StrLit("Bezier_Eval_AVX512"),
 StrLit("Bezier_Eval_SSE_Multithreaded"),
 StrLit("Bezier_Eval_AVX2_Multithreaded"),
 StrLit("Bezier_Eval_AVX512_Multithreaded"),
 StrLit("Bezier_Eval_Adaptive_Multithreaded"),
};
StaticAssert(ArrayCount(Bezier_Eval_Names) == Bezier_Eval_Count, Bezier_Eval_Names_AllDefined);

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
 
 u32 MultithreadedEvaluationBlockSize;
 
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
