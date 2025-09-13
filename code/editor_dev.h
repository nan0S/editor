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

struct debug_vars
{
 b32 Initialized;
 entity *BenchmarkEntity;
 b32 DevConsole;
 b32 ParametricEquationDebugMode;
 u32 DrawnGridLinesCount;
 b32 ShowSampleCurvePoints;
 nurbs_eval_method NURBS_EvalMethod;
 u32 NURBS_MultithreadedEvaluationBlockSize;
};
global debug_vars *DEBUG_Vars;

internal void DevUpdateAndRender(editor *Editor);
internal void UI_ParametricEquationExpr(parametric_equation_expr *Expr, string Label);
internal void UI_ExponentialAnimation(exponential_animation *Anim);

#endif //EDITOR_DEV_H
