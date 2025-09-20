#ifndef EDITOR_PARAMETRIC_EQUATION_H
#define EDITOR_PARAMETRIC_EQUATION_H

#define ParametricEquationIdentifierClasses \
X(Trigonometry) \
X(Arithmetic) \
X(Exponential) \
X(Constant) \
X(Variable)

enum parametric_equation_identifier_class
{
#define X(_ClassName) ParametricEquationIdentifierClass_##_ClassName,
 ParametricEquationIdentifierClasses
#undef X
 ParametricEquationIdentifierClass_Count,
};
global read_only string ParametricEquationIdentifierClassNames[] = {
#define X(_ClassName) StrLitComp(#_ClassName),
 ParametricEquationIdentifierClasses
#undef X
};

#define ParametricEquationIdentifiers \
X(sin, 1, Trigonometry) \
X(cos, 1, Trigonometry) \
X(tan, 1, Trigonometry) \
X(asin, 1, Trigonometry) \
X(acos, 1, Trigonometry) \
X(atan, 1, Trigonometry) \
X(atan2, 1, Trigonometry) \
X(floor, 1, Arithmetic) \
X(ceil, 1, Arithmetic) \
X(round, 1, Arithmetic) \
X(mod, 2, Arithmetic) \
X(pow, 2, Exponential) \
X(sqrt, 1, Exponential) \
X(exp, 1, Exponential) \
X(log, 1, Exponential) \
X(log10, 1, Exponential) \
X(sinh, 1, Trigonometry) \
X(cosh, 1, Trigonometry) \
X(tanh, 1, Trigonometry) \
X(pi, 0, Constant) \
X(tau, 0, Constant) \
X(euler, 0, Constant) \
X(t, 0, Variable) \
X(var, 0, Variable) // this is just regular user-defined variable but we have to define all the types

enum parametric_equation_token_type
{
 ParametricEquationToken_None,
 ParametricEquationToken_Identifier,
 ParametricEquationToken_Number,
 ParametricEquationToken_Equal,
 ParametricEquationToken_Slash,
 ParametricEquationToken_Percent,
 ParametricEquationToken_Asterisk,
 ParametricEquationToken_Plus,
 ParametricEquationToken_Minus,
 ParametricEquationToken_DoubleAsteriskOrCaret,
 ParametricEquationToken_OpenParen,
 ParametricEquationToken_CloseParen,
 ParametricEquationToken_OpenBrace,
 ParametricEquationToken_CloseBrace,
 ParametricEquationToken_Semicolon,
 ParametricEquationToken_Comma,
};

global string ParametricEquationIdentifierNames[] = {
#define X(_Func, _ArgCount, _Class) StrLitComp(#_Func),
 ParametricEquationIdentifiers
#undef X
};

global u32 ParametricEquationIdentifierArgCounts[] = {
#define X(_Func, _ArgCount, _Class) _ArgCount,
 ParametricEquationIdentifiers
#undef X
};

global read_only parametric_equation_identifier_class ParametricEquationIdentifierClass[] = {
#define X(_Func, _ArgCount, _Class) ParametricEquationIdentifierClass_##_Class,
 ParametricEquationIdentifiers
#undef X
};

enum parametric_equation_identifier_type
{
#define X(_Func, _ArgCount, _Class) ParametricEquationIdentifier_##_Func,
 ParametricEquationIdentifiers
#undef X
 ParametricEquationIdentifier_Count
};

struct parametric_equation_identifier
{
 parametric_equation_identifier_type Type;
 string Var;
};

struct parametric_equation_token
{
 parametric_equation_token_type Type;
 parametric_equation_identifier Identifier;
 f32 Number;
 string OriginalString;
};

enum parametric_equation_unary_operator
{
 ParametricEquationUnaryOp_Plus,
 ParametricEquationUnaryOp_Minus,
};
struct parametric_equation_unary_expr
{
 parametric_equation_unary_operator Operator;
 struct parametric_equation_expr *SubExpr;
};

enum parametric_equation_binary_operator
{
 ParametricEquationBinaryOp_Plus,
 ParametricEquationBinaryOp_Minus,
 ParametricEquationBinaryOp_Mult,
 ParametricEquationBinaryOp_Div,
 ParametricEquationBinaryOp_Mod,
 ParametricEquationBinaryOp_Pow,
};
struct parametric_equation_binary_expr
{
 parametric_equation_expr *Left;
 parametric_equation_expr *Right;
 parametric_equation_binary_operator Operator;
};

struct parametric_equation_number_expr
{
 f32 Number;
};

struct parametric_equation_application_expr
{
 parametric_equation_identifier Identifier;
 u32 ArgCount;
#define PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT 2
 parametric_equation_expr *Args[PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT];
};

enum parametric_equation_expr_type
{
 ParametricEquationExpr_Number, // also naturally serves as "None" expression
 ParametricEquationExpr_Unary,
 ParametricEquationExpr_Binary,
 ParametricEquationExpr_Application,
};
struct parametric_equation_expr
{
 parametric_equation_expr_type Type;
 union {
  parametric_equation_unary_expr Unary;
  parametric_equation_binary_expr Binary;
  parametric_equation_number_expr Number;
  parametric_equation_application_expr Application;
 };
};
global parametric_equation_expr *NilExpr;

enum parametric_equation_t_mode
{
 ParametricEquationT_Unbound,
 ParametricEquationT_JustBound,
 ParametricEquationT_BoundAndValueProvided,
};

struct parametric_equation_env
{
 parametric_equation_t_mode T_Mode;
 f32 T_Value;
 
 u32 VarCount;
 string *VarNames;
 f32 *VarValues;
};

/*

// TODO(hbr): Things todo:
- I want to add subscopes - but to do that I think I need to introduce variable decalarations: := = syntax
otherwise it would be very confusing what is what
- in this sense I realized that the way I treat env is incorrect - I want variable declarations to be revoked at the end of the scope
but I want variable modifications to be persistent across scopes
- when parsing I need to treat special variables in special way - transform pi/tau into numbers
and transforms +/- div multiply power into functionInvocations
- I think it would be useful to make and id out of everything - i.e. store identifier_id instead of identifier and all that stuff

- compress types
- type checking
- evaluating in environment
- some expression optimization
- there is an issue that we store the identifier as text that we got externally, but then when I modify this text in my editor then it is no longer valid - I think ids might solve this
- number of points we have for parametric curve should not depend on number of control points it has
- when I recompute the curve_params and parametric_equation, I shouldn't only check for equality of curve_params struct but instead, work with everything
 
r := 10;

 my_func(x, r) {
my_var := 10;
r*sin(my_var*x) + r*cos(my_var*y)

}

2*pi*sin(t)*cos(t)*my_func(10)

*/

struct parametric_equation_parse_result
{
 b32 Fail;
 string ErrorMessage;
 parametric_equation_expr *ParsedExpr;
};

struct parametric_equation_eval_result
{
 b32 Fail;
 string ErrorMessage;
 f32 Value;
};

struct parametric_equation_parse_statements_result
{
 b32 Fail;
 string ErrorMessage;
 parametric_equation_env Env;
};

struct parametric_equation_parsing_error
{
 arena *Arena;
 string ErrorMessage;
};

struct parametric_equation_lexer
{
 char *CharAt;
 
 u32 MaxTokenCount;
 u32 TokenCount;
 parametric_equation_token *Tokens;
};

struct parametric_equation_token_array
{
 u32 TokenCount;
 parametric_equation_token *Tokens;
};

struct parametric_equation_parser
{
 parametric_equation_token *TokenAt;
 parametric_equation_parsing_error *ErrorCapture;
};

internal parametric_equation_parse_result ParametricEquationParse(arena *Arena, string Equation, u32 VarCount, string *VarNames, f32 *VarValues, b32 DontOptimize = false); // assumes that "t" is implicitly bound
internal parametric_equation_eval_result  ParametricEquationEval(arena *Arena, string Equation, u32 VarCount, string *VarNames, f32 *VarValues);
internal f32                              ParametricEquationEvalWithT(parametric_equation_expr *Expr, f32 T);

#endif //EDITOR_PARAMETRIC_EQUATION_H
