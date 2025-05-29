#ifndef EDITOR_DEBUG_H
#define EDITOR_DEBUG_H

struct parametric_equation_expr;

struct debug_settings
{
 b32 ParametricEquationDebugMode;
};
global debug_settings *DEBUG_Settings;

internal void UI_ParametricEquationExpr(parametric_equation_expr *Expr, string Label);

#endif //EDITOR_DEBUG_H
