#ifndef EDITOR_PARAMETRIC_EQUATION_H
#define EDITOR_PARAMETRIC_EQUATION_H

enum parametric_equation_token_type
{
 ParametricEquationToken_None,
 ParametricEquationToken_Identifier,
 ParametricEquationToken_Number,
 ParametricEquationToken_Definition,
 ParametricEquationToken_Slash,
 ParametricEquationToken_Asterisk,
 ParametricEquationToken_Plus,
 ParametricEquationToken_Minus,
 ParametricEquationToken_Caret,
 ParametricEquationToken_OpenParen,
 ParametricEquationToken_CloseParen,
 ParametricEquationToken_OpenBrace,
 ParametricEquationToken_CloseBrace,
 ParametricEquationToken_Semicolon,
 ParametricEquationToken_Comma,
};

struct parametric_equation_token
{
 parametric_equation_token_type Type;
 union {
  string Identifier;
  f32 Number;
 };
};

struct parametric_equation_tokenize_result
{
 parametric_equation_token *Tokens;
 char const *ErrorMsg;
};

struct parametric_equation_tokenizer
{
 arena *Arena;
 parametric_equation_token *Tokens;
 u32 TokenCount;
 u32 MaxTokenCount;
 
 char *At;
 u64 Left;
};

struct parametric_equation_token_array
{
 parametric_equation_token *Tokens;
 u32 TokenCount;
};

struct parametric_equation_function
{
};

enum parametric_equation_env_value_type
{
 ParametricEquationEnvValue_Number,
 ParametricEquationEnvValue_Function,
};
struct parametric_equation_env_value
{
 parametric_equation_env_value_type Type;
 union {
  parametric_equation_function *Function;
  f32 Number;
 };
};
struct parametric_equation_env
{
 u32 Capacity;
 u32 Count;
 string *Keys;
 parametric_equation_env_value *Values;
};

struct parametric_equation_expr;

enum parametric_equation_op_type
{
 ParametricEquationOp_None,
 
 ParametricEquationOp_Plus,
 ParametricEquationOp_Minus,
 ParametricEquationOp_Mult,
 ParametricEquationOp_Div,
 ParametricEquationOp_Pow,
};

struct parametric_equation_expr_negation
{
 parametric_equation_expr *SubExpr;
};

struct parametric_equation_expr_binary_op
{
 parametric_equation_op_type Type;
 
 parametric_equation_expr *Parent;
 parametric_equation_expr *Left;
 parametric_equation_expr *Right;
};

struct parametric_equation_expr_number
{
 f32 Number;
};

enum parametric_equation_identifier_type
{
 ParametricEquationIdentifier_Sin,
 ParametricEquationIdentifier_Cos,
 ParametricEquationIdentifier_Tan,
 ParametricEquationIdentifier_Sqrt,
 ParametricEquationIdentifier_Log,
 ParametricEquationIdentifier_Log10,
 ParametricEquationIdentifier_Floor,
 ParametricEquationIdentifier_Ceil,
 ParametricEquationIdentifier_Round,
 ParametricEquationIdentifier_Pow,
 ParametricEquationIdentifier_Tanh,
 ParametricEquationIdentifier_Exp,
 
 ParametricEquationIdentifier_Pi,
 ParametricEquationIdentifier_Tau,
 ParametricEquationIdentifier_Euler,
 
 ParametricEquationIdentifier_Custom,
};
struct parametric_equation_identifier
{
 parametric_equation_identifier_type Type;
 string Custom;
};
struct parametric_equation_expr_application
{
 parametric_equation_identifier Identifier;
 u32 ArgCount;
 parametric_equation_expr *ArgExprs;
};

enum parametric_equation_expr_type
{
 ParametricEquationExpr_None,
 ParametricEquationExpr_Negation,
 ParametricEquationExpr_BinaryOp,
 ParametricEquationExpr_Number,
 ParametricEquationExpr_Application,
};
struct parametric_equation_expr
{
 parametric_equation_expr_type Type;
 union {
  parametric_equation_expr_negation Negation;
  parametric_equation_expr_binary_op Binary;
  parametric_equation_expr_number Number;
  parametric_equation_expr_application Application;
 };
};

struct parametric_equation_env2
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

*/

struct parametric_equation_parser
{
 parametric_equation_token *At;
 char const *ErrorMsg;
};

struct parametric_equation_expr_array
{
 u32 Count;
 u32 MaxCount;
 parametric_equation_expr *Exprs;
};

struct parametric_equation_parse_result
{
 parametric_equation_expr *ParsedExpr;
 char const *ErrorMsg;
};

struct parametric_equation_eval_result
{
 f32 Value;
 char const *ErrorMsg;
};

struct parametric_equation_type_checker
{
 char const *ErrorMsg;
};

internal parametric_equation_parse_result ParametricEquationParse(arena *Arena, string Equation);
internal parametric_equation_eval_result  ParametricEquationEval(arena *Arena, string Equation);
internal f32                              ParametricEquationEvalWithT(parametric_equation_expr Expr, f32 T);

#endif //EDITOR_PARAMETRIC_EQUATION_H
