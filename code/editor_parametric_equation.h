#ifndef EDITOR_PARAMETRIC_EQUATION_H
#define EDITOR_PARAMETRIC_EQUATION_H

enum parametric_equation_token_type
{
 ParametricEquationToken_None,
 ParametricEquationToken_Identifier,
 ParametricEquationToken_Number,
 ParametricEquationToken_Equal,
 ParametricEquationToken_Slash,
 ParametricEquationToken_Mult,
 ParametricEquationToken_Plus,
 ParametricEquationToken_Minus,
 ParametricEquationToken_Pow,
 ParametricEquationToken_OpenParen,
 ParametricEquationToken_CloseParen,
 ParametricEquationToken_OpenBrace,
 ParametricEquationToken_CloseBrace,
 ParametricEquationToken_Semicolon,
 ParametricEquationToken_Comma,
};

struct parametric_equation_source_location
{
 u32 LineAt;
 u32 CharWithinLineAt;
};

struct parametric_equation_token
{
 parametric_equation_token_type Type;
 union {
  string Identifier;
  f32 Number;
 };
 
 parametric_equation_source_location Location;
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

#define ParametricEquationIdentifiers \
X(sin, 1) \
X(cos, 1) \
X(tan, 1) \
X(sqrt, 1) \
X(log, 1) \
X(log10, 1) \
X(floor, 1) \
X(ceil, 1) \
X(round, 1) \
X(pow, 2) \
X(tanh, 1) \
X(exp, 1) \
X(pi, 0) \
X(tau, 0) \
X(euler, 0) \
X(var, 0) // this is just regular user-defined variable but we have to define all the types

global char const *ParametricEquationIdentifierNames[] = {
#define X(_Func, _ArgCount) #_Func,
 ParametricEquationIdentifiers
#undef X
};

global u32 ParametricEquationIdentifierArgCounts[] = {
#define X(_Func, _ArgCount) _ArgCount,
 ParametricEquationIdentifiers
#undef X
};

enum parametric_equation_identifier_type
{
#define X(_Func, _ArgCount) ParametricEquationIdentifier_##_Func,
 ParametricEquationIdentifiers
#undef X
};

struct parametric_equation_application_expr_identifier
{
 parametric_equation_identifier_type Type;
 string Var;
 parametric_equation_source_location Location;
};
struct parametric_equation_application_expr
{
 parametric_equation_application_expr_identifier Identifier;
 u32 ArgCount;
 parametric_equation_expr *Args;
};

enum parametric_equation_expr_type
{
 ParametricEquationExpr_Number,
 ParametricEquationExpr_Unary,
 ParametricEquationExpr_Binary,
 ParametricEquationExpr_Application,
};
struct parametric_equation_expr
{
 parametric_equation_expr *Next;
 parametric_equation_expr_type Type;
 union {
  parametric_equation_unary_expr Unary;
  parametric_equation_binary_expr Binary;
  parametric_equation_number_expr Number;
  parametric_equation_application_expr Application;
 };
};
read_only global parametric_equation_expr NilExpr = { &NilExpr };

struct parametric_equation_env
{
 u32 Count;
 // TODO(hbr): add dynamic growing
#define MAX_ENV_VARIABLE_COUNT 16
 f32 Values[MAX_ENV_VARIABLE_COUNT];
 string Keys[MAX_ENV_VARIABLE_COUNT];
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
 b32 Ok;
 string ErrorMessage;
 parametric_equation_expr *ParsedExpr;
};

struct parametric_equation_eval_result
{
 b32 Ok;
 string ErrorMessage;
 f32 Value;
};

struct parametric_equation_parse_env_result
{
 b32 Ok;
 string ErrorMessage;
 parametric_equation_env Env;
};

struct parametric_equation_parser
{
 arena *Arena;
 
 char *CharBegin;
 char *CharAt;
 parametric_equation_source_location CharLocation;
 
 parametric_equation_token *Tokens;
 u32 TokenCount;
 u32 MaxTokenCount;
 
 parametric_equation_token *TokenAt;
 
 b32 T_ImplicitlyBound;
 
 string ErrorMessage;
};


internal parametric_equation_parse_result ParametricEquationParse(arena *Arena, string Equation, b32 T_ImplicitlyBound);
internal parametric_equation_eval_result  ParametricEquationEval(arena *Arena, string Equation);
internal f32                              ParametricEquationEvalWithT(parametric_equation_expr *Expr, f32 T);

#endif //EDITOR_PARAMETRIC_EQUATION_H
