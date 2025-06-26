#ifndef EDITOR_DEBUG_H
#define EDITOR_DEBUG_H

struct parametric_equation_expr;

struct debug_vars
{
 b32 ParametricEquationDebugMode;
 u32 DrawnGridLinesCount;
};
global debug_vars *DEBUG_Vars;

internal void UI_ParametricEquationExpr(parametric_equation_expr *Expr, string Label);

#endif //EDITOR_DEBUG_H
