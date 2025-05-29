internal parametric_equation_token
MakeToken(parametric_equation_token_type Type)
{
 parametric_equation_token Token = {};
 Token.Type = Type;
 return Token;
}

internal parametric_equation_token *
AddToken(parametric_equation_lexer *Lexer,
         parametric_equation_token_type Type,
         char *TokenBeginChar)
{
 Assert(Lexer->TokenCount < Lexer->MaxTokenCount);
 parametric_equation_token *Token = Lexer->Tokens + Lexer->TokenCount;
 ++Lexer->TokenCount;
 
 *Token = MakeToken(Type);
 Token->OriginalString = MakeStr(TokenBeginChar, Lexer->CharAt - TokenBeginChar);
 
 return Token;
}

inline internal void
AdvanceChar(parametric_equation_lexer *Lexer, u32 Count)
{
 Lexer->CharAt += Count;
}

internal b32
ParseIdentifier(parametric_equation_lexer *Lexer, string *OutIdentifier)
{
 char *SaveAt = Lexer->CharAt;
 
 b32 Parsing = true;
 b32 FirstChar = true;
 do {
  char C = Lexer->CharAt[0];
  
  b32 AlwaysPossible = (CharIsAlpha(C) || C == '_');
  b32 OnlyNotAtTheBeginning = (CharIsDigit(C) || C == '\'');
  b32 IsEligibleForIdentifier = (AlwaysPossible || (!FirstChar && OnlyNotAtTheBeginning));
  
  if (IsEligibleForIdentifier)
  {
   AdvanceChar(Lexer, 1);
  }
  else
  {
   Parsing = false;
  }
  
  FirstChar = false;
 } while (Parsing);
 
 b32 Parsed = false;
 if (Lexer->CharAt != SaveAt)
 {
  Parsed = true;
  *OutIdentifier = MakeStr(SaveAt, Lexer->CharAt - SaveAt);
 }
 
 return Parsed;
}

internal b32
ParseInteger(parametric_equation_lexer *Lexer, u64 *OutInteger)
{
 b32 Parsed = false;
 u64 N = 0;
 b32 Parsing = true;
 b32 FirstChar = true;
 
 do {
  char C = Lexer->CharAt[0];
  
  b32 IsDigit = CharIsDigit(C);
  b32 AlwaysPossible = (IsDigit);
  b32 OnlyNotAtTheBeginning = (C == '_');
  b32 IsEligibleForInteger = (AlwaysPossible || (!FirstChar && OnlyNotAtTheBeginning));
  
  if (IsEligibleForInteger)
  {
   if (IsDigit)
   {
    u64 Digit = C - '0';
    N = N * 10 + Digit;
   }
   AdvanceChar(Lexer, 1);
   Parsed = true;
  }
  else
  {
   Parsing = false;
  }
  
  FirstChar = false;
 } while (Parsing);
 
 if (Parsed)
 {
  *OutInteger = N;
 }
 
 return Parsed;
}

internal void
MaybeEatChar(parametric_equation_lexer *Lexer, char C)
{
 char Actual = Lexer->CharAt[0];
 if (Actual == C)
 {
  AdvanceChar(Lexer, 1);
 }
}

internal b32
ParseNumber(parametric_equation_lexer *Lexer, f32 *OutNumber)
{
 u64 PreDot = 0;
 u64 PostDot = 0;
 
 b32 PreDotParsed = ParseInteger(Lexer, &PreDot);
 MaybeEatChar(Lexer, '.');
 b32 PostDotParsed = ParseInteger(Lexer, &PostDot);
 
 b32 Parsed = false;
 if (PreDotParsed || PostDotParsed)
 {
  f32 PreDotFloat = Cast(f32)PreDot;
  
  u64 PostDotDiv = 1;
  while (PostDotDiv <= PostDot)
  {
   PostDotDiv *= 10;
  }
  f32 PostDotFloat = Cast(f32)PostDot / PostDotDiv;
  
  f32 Number = PreDotFloat + PostDotFloat;
  *OutNumber = Number;
  
  Parsed = true;
 }
 
 return Parsed;
}

internal b32
IsError(parametric_equation_parsing_error *ErrorCapture)
{
 b32 Result = (ErrorCapture->ErrorMessage.Data != 0);
 return Result;
}

internal void
SetError(parametric_equation_parsing_error *ErrorCapture,
         string ErrorMessage)
{
 // TODO(hbr): Check whether it's better to accumulate errors here or just save the first one.
 // The advantage for the second case is that there is more information passed to the user.
 // The disadvantage is that the information could be lower quality. Hmmm, maybe we should do
 // best case parsing? Meaning if there is something we expect, but it's not there, assume it's there,
 // continue, but also set the error.
 if (!IsError(ErrorCapture))
 {
  ErrorCapture->ErrorMessage = StrCopy(ErrorCapture->Arena, ErrorMessage);
 }
}

internal void
SetErrorF(parametric_equation_parsing_error *ErrorCapture,
          char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 
 temp_arena Temp = TempArena(ErrorCapture->Arena);
 
 string ErrorMessage = StrFV(Temp.Arena, Format, Args);
 SetError(ErrorCapture, ErrorMessage);
 
 EndTemp(Temp);
 
 va_end(Args);
}

internal parametric_equation_lexer
BeginLexing(arena *Arena, string Equation)
{
 parametric_equation_lexer Lexer = {};
 
 Lexer.CharAt = Equation.Data;
 // NOTE(hbr): One additional space for Token_None
 Lexer.MaxTokenCount = SafeCastU32(Equation.Count + 1);
 Lexer.Tokens = PushArrayNonZero(Arena, Lexer.MaxTokenCount, parametric_equation_token);
 
 return Lexer;
}

internal parametric_equation_token_array
MakeTokenArray(u32 TokenCount, parametric_equation_token *Tokens)
{
 parametric_equation_token_array Array = {};
 Array.TokenCount = TokenCount;
 Array.Tokens = Tokens;
 
 return Array;
}

internal parametric_equation_identifier
MakeParametricEquationIdentifier(string IdentifierStr)
{
 parametric_equation_identifier Identifier = {};
 
 b32 Found = false;
 for (u32 IdentifierIndex = 0;
      IdentifierIndex < ParametricEquationIdentifier_Count;
      ++IdentifierIndex)
 {
  string IdentifierName = ParametricEquationIdentifierNames[IdentifierIndex];
  if (IdentifierIndex != ParametricEquationIdentifier_var)
  {
   if (StrMatch(IdentifierStr, IdentifierName, true))
   {
    Identifier.Type = Cast(parametric_equation_identifier_type)IdentifierIndex;
    Found = true;
    break;
   }
  }
 }
 
 if (!Found)
 {
  Identifier.Type = ParametricEquationIdentifier_var;
  Identifier.Var = IdentifierStr;
 }
 
 return Identifier;
}

internal parametric_equation_token_array
TokenizeParametricEquation(arena *Arena, string Equation,
                           parametric_equation_parsing_error *ErrorCapture)
{
 parametric_equation_lexer Lexer = BeginLexing(Arena, Equation);
 
 b32 Lexing = true;
 do {
  //- multiline comment
  b32 MultiLineComment = true;
  u32 NestLevel = 0;
  do {
   if (Lexer.CharAt[0] == '\0')
   {
    if (NestLevel > 0)
    {
     SetError(ErrorCapture, StrLit("non-terminated multiline comment, unexpected end of input"));
    }
    MultiLineComment = false;
   }
   else
   {
    if (Lexer.CharAt[0] == '/' &&
        Lexer.CharAt[1] == '*')
    {
     ++NestLevel;
     AdvanceChar(&Lexer, 2);
    }
    else if (NestLevel > 0 &&
             Lexer.CharAt[0] == '*' &&
             Lexer.CharAt[1] == '/')
    {
     --NestLevel;
     AdvanceChar(&Lexer, 2);
    }
    else if (NestLevel > 0)
    {
     AdvanceChar(&Lexer, 1);
    }
   }
   
   if (NestLevel == 0)
   {
    MultiLineComment = false;
   }
   
  } while (MultiLineComment);
  
  if (CharIsWhiteSpace(Lexer.CharAt[0]))
  {
   AdvanceChar(&Lexer, 1);
  }
  else if (Lexer.CharAt[0] == '/' &&
           Lexer.CharAt[1] == '/')
  {
   AdvanceChar(&Lexer, 2);
   
   b32 Comment = true;
   do {
    if (Lexer.CharAt[0] == '\0')
    {
     Comment = false;
    }
    else
    {
     if (Lexer.CharAt[0] == '\n')
     {
      Comment = false;
     }
     AdvanceChar(&Lexer, 1);
    }
   } while (Comment);
   // NOTE(hbr): If this is the end of input,
   // then so be it. Just finish parsing.
  }
  else
  {
   char *SaveCharAt = Lexer.CharAt;
   
   switch (Lexer.CharAt[0])
   {
    case '\0': {
     Lexing = false;
    }break;
    
    case '-': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Minus, SaveCharAt);
    }break;
    
    case '+': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Plus, SaveCharAt);
    }break;
    
    case '/': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Slash, SaveCharAt);
    }break;
    
    case '%': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Percent, SaveCharAt);
    }break;
    
    case '*': {
     AdvanceChar(&Lexer, 1);
     if (Lexer.CharAt[0] == '*')
     {
      AdvanceChar(&Lexer, 1);
      AddToken(&Lexer, ParametricEquationToken_DoubleAsteriskOrCaret, SaveCharAt);
     }
     else
     {
      AddToken(&Lexer, ParametricEquationToken_Asterisk, SaveCharAt);
     }
    }break;
    
    case '^': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_DoubleAsteriskOrCaret, SaveCharAt);
    }break;
    
    case '(': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_OpenParen, SaveCharAt);
    }break;
    
    case ')': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_CloseParen, SaveCharAt);
    }break;
    
    case '{': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_OpenBrace, SaveCharAt);
    }break;
    
    case '}': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_CloseBrace, SaveCharAt);
    }break;
    
    case ',': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Comma, SaveCharAt);
    }break;
    
    case ':': {
     AdvanceChar(&Lexer, 1);
     if (Lexer.CharAt[0] == '=')
     {
      AdvanceChar(&Lexer, 1);
      AddToken(&Lexer, ParametricEquationToken_Equal, SaveCharAt);
     }
     else
     {
      SetError(ErrorCapture, StrLit("missing '=' after ':'"));
     }
    }break;
    
    case '=': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Equal, SaveCharAt);
    }break;
    
    case ';': {
     AdvanceChar(&Lexer, 1);
     AddToken(&Lexer, ParametricEquationToken_Semicolon, SaveCharAt);
    }break;
    
    default: {
     string Identifier = {};
     f32 Number = 0;
     if (ParseIdentifier(&Lexer, &Identifier))
     {
      parametric_equation_token *Token = AddToken(&Lexer, ParametricEquationToken_Identifier, SaveCharAt);
      Token->Identifier = MakeParametricEquationIdentifier(Identifier);
     }
     else if (ParseNumber(&Lexer, &Number))
     {
      parametric_equation_token *Token = AddToken(&Lexer, ParametricEquationToken_Number, SaveCharAt);
      Token->Number = Number;
     }
     else
     {
      SetErrorF(ErrorCapture, "unexpected character '%c'", Lexer.CharAt[0]);
     }
    }break;
   }
  }
  
  if (IsError(ErrorCapture))
  {
   Lexing = false;
  }
 } while (Lexing);
 
 AddToken(&Lexer, ParametricEquationToken_None, Lexer.CharAt);
 
 parametric_equation_token_array Tokens = MakeTokenArray(Lexer.TokenCount, Lexer.Tokens);
 return Tokens;
}

internal b32
EatIfMatch(parametric_equation_parser *Parser, parametric_equation_token_type Type)
{
 b32 Eaten = false;
 parametric_equation_token Token = Parser->TokenAt[0];
 if (Token.Type == Type)
 {
  ++Parser->TokenAt;
  Eaten = true;
 }
 
 return Eaten;
}

internal parametric_equation_expr *
AllocExpr(arena *Arena, parametric_equation_expr_type Type)
{
 parametric_equation_expr *Expr = PushStruct(Arena, parametric_equation_expr);
 Expr->Type = Type;
 
 return Expr;
}

internal parametric_equation_expr *
MakeUnaryExpr(arena *Arena,
              parametric_equation_unary_operator UnaryOp,
              parametric_equation_expr *SubExpr)
{
 parametric_equation_expr *Expr = AllocExpr(Arena, ParametricEquationExpr_Unary);
 Expr->Unary.Operator = UnaryOp;
 Expr->Unary.SubExpr = SubExpr;
 
 return Expr;
}

internal parametric_equation_expr *
MakeBinaryExpr(arena *Arena,
               parametric_equation_expr *Left,
               parametric_equation_binary_operator BinaryOp,
               parametric_equation_expr *Right)
{
 parametric_equation_expr *Expr = AllocExpr(Arena, ParametricEquationExpr_Binary);
 Expr->Binary.Left = Left;
 Expr->Binary.Right = Right;
 Expr->Binary.Operator = BinaryOp;
 
 return Expr;
}

internal parametric_equation_expr *
MakeNumberExpr(arena *Arena, f32 Number)
{
 parametric_equation_expr *Expr = AllocExpr(Arena, ParametricEquationExpr_Number);
 Expr->Number.Number = Number;
 
 return Expr;
}

internal parametric_equation_expr *
MakeApplicationExpr(arena *Arena,
                    parametric_equation_identifier Identifier,
                    u32 ArgCount,
                    parametric_equation_expr **Args)
{
 parametric_equation_expr *Expr = AllocExpr(Arena, ParametricEquationExpr_Application);
 Expr->Application.Identifier = Identifier;
 Expr->Application.ArgCount = ArgCount;
 Assert(ArgCount <= PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT);
 ArrayCopy(Expr->Application.Args, Args, ArgCount);
 
 return Expr;
}

struct unary_operator_detection
{
 b32 IsUnary;
 parametric_equation_unary_operator Operator;
};
internal void
UnaryOperatorDetected(unary_operator_detection *Detection, parametric_equation_unary_operator Operator)
{
 Assert(!Detection->IsUnary);
 Detection->IsUnary = true;
 Detection->Operator = Operator;
}
internal unary_operator_detection
IsUnaryOperator(parametric_equation_token Token)
{
 unary_operator_detection Result = {};
 switch (Token.Type)
 {
  case ParametricEquationToken_Plus:  {UnaryOperatorDetected(&Result, ParametricEquationUnaryOp_Plus);}break;
  case ParametricEquationToken_Minus: {UnaryOperatorDetected(&Result, ParametricEquationUnaryOp_Minus);}break;
  default: break;
 }
 return Result;
}

struct binary_operator_detection
{
 b32 IsBinary;
 parametric_equation_binary_operator Operator;
};
internal void
BinaryOperatorDetected(binary_operator_detection *Detection, parametric_equation_binary_operator Operator)
{
 Assert(!Detection->IsBinary);
 Detection->IsBinary = true;
 Detection->Operator = Operator;
}
internal binary_operator_detection
IsBinaryOperator(parametric_equation_token Token)
{
 binary_operator_detection Result = {};
 switch (Token.Type)
 {
  case ParametricEquationToken_Plus:                      {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Plus);}break;
  case ParametricEquationToken_Minus:                     {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Minus);}break;
  case ParametricEquationToken_Asterisk:                  {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Mult);}break;
  case ParametricEquationToken_Slash:                     {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Div);}break;
  case ParametricEquationToken_Percent:                   {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Mod);}break;
  case ParametricEquationToken_DoubleAsteriskOrCaret:     {BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Pow);}break;
  
  case ParametricEquationToken_Identifier: {
   if (Token.Identifier.Type == ParametricEquationIdentifier_mod)
   {
    BinaryOperatorDetected(&Result, ParametricEquationBinaryOp_Mod);
   }
  }break;
  
  default: break;
 }
 return Result;
}

internal b32
IsNumber(parametric_equation_token Token)
{
 b32 Result = (Token.Type == ParametricEquationToken_Number);
 return Result;
}

internal b32
IsIdentifier(parametric_equation_token Token)
{
 b32 Result = (Token.Type == ParametricEquationToken_Identifier);
 return Result;
}

internal b32
IsValid(parametric_equation_expr *Expr)
{
 b32 Valid = (Expr != NilExpr);
 return Valid;
}

internal u8
BinaryOperatorToPrecedence(parametric_equation_binary_operator Operator)
{
 u8 Result = 0;
 switch (Operator)
 {
  case ParametricEquationBinaryOp_Plus: 
  case ParametricEquationBinaryOp_Minus: {Result = 1;}break;
  case ParametricEquationBinaryOp_Mult:
  case ParametricEquationBinaryOp_Div:
  case ParametricEquationBinaryOp_Mod: {Result = 2;}break;
  case ParametricEquationBinaryOp_Pow: {Result = 4;}break;
 }
 return Result;
}

internal u8
UnaryOperatorToPrecedence(parametric_equation_unary_operator Operator)
{
 u8 Result = 0;
 switch (Operator)
 {
  case ParametricEquationUnaryOp_Plus:
  case ParametricEquationUnaryOp_Minus: {Result=3;}break;
 }
 return Result;
}

internal parametric_equation_expr *ParseExpr(arena *Arena, parametric_equation_parser *Parser, u8 MinPrecedence);

internal parametric_equation_expr *
ParseLeafExpr(arena *Arena, parametric_equation_parser *Parser)
{
 parametric_equation_expr *Expr = NilExpr;
 
 parametric_equation_token NextToken = Parser->TokenAt[0];
 unary_operator_detection UnaryOp = IsUnaryOperator(NextToken);
 if (UnaryOp.IsUnary)
 {
  ++Parser->TokenAt;
  
  u8 NextPrecedence = UnaryOperatorToPrecedence(UnaryOp.Operator);
  
  parametric_equation_expr *SubExpr = ParseExpr(Arena, Parser, NextPrecedence);
  
  Expr = MakeUnaryExpr(Arena, UnaryOp.Operator, SubExpr);
 }
 else if (IsNumber(NextToken))
 {
  ++Parser->TokenAt;
  
  Expr = MakeNumberExpr(Arena, NextToken.Number);
 }
 else if (IsIdentifier(NextToken))
 {
  ++Parser->TokenAt;
  
  parametric_equation_token IdentifierToken = NextToken;
  parametric_equation_identifier Identifier = IdentifierToken.Identifier;
  
  u32 ArgCount = 0;
  parametric_equation_expr *Args[PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT];
  if (EatIfMatch(Parser, ParametricEquationToken_OpenParen))
  {
   b32 Parsing = true;
   do {
    // TODO(hbr): try to flatten this kind of horrible logic
    parametric_equation_token NextToken = Parser->TokenAt[0];
    if (NextToken.Type == ParametricEquationToken_CloseParen)
    {
     Parsing = false;
    }
    else
    {
     parametric_equation_expr *Arg = ParseExpr(Arena, Parser, 0);
     // TODO(hbr): Try to get rid of this if by using NilExpr
     if (IsValid(Arg))
     {
      if (ArgCount < PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT)
      {
       Args[ArgCount] = Arg;
       ++ArgCount;
       
       if (!EatIfMatch(Parser, ParametricEquationToken_Comma))
       {
        Parsing = false;
       }
      }
      else
      {
       SetErrorF(Parser->ErrorCapture, "too many arguments for '%S'", IdentifierToken.OriginalString);
       Parsing = false;
      }
     }
     else
     {
      Parsing = false;
     }
    }
   } while (Parsing);
   
   if (!EatIfMatch(Parser, ParametricEquationToken_CloseParen))
   {
    SetError(Parser->ErrorCapture, StrLit("'(' is missing ')'"));
   }
  }
  
  Expr = MakeApplicationExpr(Arena, Identifier, ArgCount, Args);
 }
 else if (NextToken.Type == ParametricEquationToken_OpenParen)
 {
  ++Parser->TokenAt;
  
  Expr = ParseExpr(Arena, Parser, 0);
  
  if (!EatIfMatch(Parser, ParametricEquationToken_CloseParen))
  {
   SetError(Parser->ErrorCapture, StrLit("'(' is missing ')'"));
  }
 }
 else
 {
  parametric_equation_token Token = Parser->TokenAt[0];
  if (Token.Type == ParametricEquationToken_None)
  {
   SetError(Parser->ErrorCapture, StrLit("unexpected end of input"));
  }
  else
  {
   SetErrorF(Parser->ErrorCapture, "unexpected '%S'", Token.OriginalString);
  }
 }
 
 return Expr;
}

internal b32
IsRightAssociative(parametric_equation_binary_operator Operator)
{
 b32 Result = false;
 switch (Operator)
 {
  case ParametricEquationBinaryOp_Plus:
  case ParametricEquationBinaryOp_Minus:
  case ParametricEquationBinaryOp_Mult:
  case ParametricEquationBinaryOp_Div: 
  case ParametricEquationBinaryOp_Mod: {}break;
  
  case ParametricEquationBinaryOp_Pow: {Result=true;}break;
 }
 
 return Result;
}

// Expressions of increasing operator precedence are right-leaning.
// This function creates exactly right leaning trees.
internal parametric_equation_expr *
ParseExprIncreasingPrecedence(arena *Arena,
                              parametric_equation_parser *Parser,
                              parametric_equation_expr *Left,
                              u8 MinPrecedence)
{
 parametric_equation_expr *Expr = Left;
 
 parametric_equation_token NextToken = Parser->TokenAt[0];
 binary_operator_detection BinaryOp = IsBinaryOperator(NextToken);
 if (BinaryOp.IsBinary)
 {
  u8 NextPrecedence = BinaryOperatorToPrecedence(BinaryOp.Operator);
  
  if ((NextPrecedence > MinPrecedence) ||
      (NextPrecedence == MinPrecedence && IsRightAssociative(BinaryOp.Operator)))
  {
   ++Parser->TokenAt;
   
   parametric_equation_expr *Right = ParseExpr(Arena, Parser, NextPrecedence);
   Expr = MakeBinaryExpr(Arena, Left, BinaryOp.Operator, Right);
  }
 }
 
 return Expr;
}

// This function creates left-leaning trees (but also calls ParseExprIncreasingPrecedence,
// which creates right-leaning trees. Interplay of these two functions correctly parse
// an expression.
internal parametric_equation_expr *
ParseExpr(arena *Arena, parametric_equation_parser *Parser, u8 MinPrecedence)
{
 parametric_equation_expr *Left = ParseLeafExpr(Arena, Parser);
 
 b32 Parsing = true;
 do
 {
  parametric_equation_expr *Expr = ParseExprIncreasingPrecedence(Arena, Parser, Left, MinPrecedence);
  if (Expr == Left)
  {
   Parsing = false;
  }
  else
  {
   Left = Expr;
  }
 } while (Parsing);
 
 return Left;
}

internal b32
IsBound(parametric_equation_env *Env, string Var)
{
 b32 Bound = false;
 for (u32 Index = 0;
      Index < Env->VarCount;
      ++Index)
 {
  if (StrEqual(Var, Env->VarNames[Index]))
  {
   Bound = true;
   break;
  }
 }
 return Bound;
}

internal string
ParametricEquationIdentifierName(parametric_equation_identifier Identifier)
{
 string Name = (Identifier.Type == ParametricEquationIdentifier_var ?
                Identifier.Var :
                ParametricEquationIdentifierNames[Identifier.Type]);
 return Name;
}

internal void
TypeCheckExpr(parametric_equation_expr *Expr,
              parametric_equation_env *Env,
              parametric_equation_parsing_error *ErrorCapture)
{
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   TypeCheckExpr(Expr->Unary.SubExpr, Env, ErrorCapture);
  }break;
  
  case ParametricEquationExpr_Binary: {
   TypeCheckExpr(Expr->Binary.Left, Env, ErrorCapture);
   TypeCheckExpr(Expr->Binary.Right, Env, ErrorCapture);
  }break;
  
  case ParametricEquationExpr_Number: {}break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   parametric_equation_identifier Identifier = Application.Identifier;
   
   if (Identifier.Type == ParametricEquationIdentifier_var)
   {
    string Var = Identifier.Var;
    
    if (!IsBound(Env, Var))
    {
     SetErrorF(ErrorCapture, "'%S' identifier not bound", Var);
    }
   }
   else if (Identifier.Type == ParametricEquationIdentifier_t)
   {
    if (Env->T_Mode == ParametricEquationT_Unbound)
    {
     SetError(ErrorCapture, StrLit("'t' identifier not bound"));
    }
   }
   
   u32 ExpectedArgCount = ParametricEquationIdentifierArgCounts[Identifier.Type];
   if (Application.ArgCount != ExpectedArgCount)
   {
    string IdentifierName = ParametricEquationIdentifierName(Identifier);
    if (ExpectedArgCount == 0)
    {
     SetErrorF(ErrorCapture,
               "'%S' is not a function, no arguments expected (got %u)",
               IdentifierName, Application.ArgCount);
    }
    else
    {
     char const *ArgumentStr = (ExpectedArgCount == 1 ? "argument" : "arguments");
     SetErrorF(ErrorCapture,
               "'%S' expects %u %s (got %u)",
               IdentifierName, ExpectedArgCount, ArgumentStr, Application.ArgCount);
    }
   }
   
   for (u32 ArgIndex = 0;
        ArgIndex < Application.ArgCount;
        ++ArgIndex)
   {
    parametric_equation_expr *Arg = Application.Args[ArgIndex];
    TypeCheckExpr(Arg, Env, ErrorCapture);
   }
  }break;
 }
}

internal parametric_equation_parser
BeginParsing(parametric_equation_token_array Tokens,
             parametric_equation_parsing_error *ErrorCapture)
{
 parametric_equation_parser Parser = {};
 Parser.TokenAt = Tokens.Tokens;
 Parser.ErrorCapture = ErrorCapture;
 
 return Parser;
}

struct expr_value
{
 b32 IsValue;
 f32 Value;
};
internal expr_value
ToValue(parametric_equation_expr *Expr)
{
 expr_value Value = {};
 Value.IsValue = (Expr->Type == ParametricEquationExpr_Number);
 Value.Value = (Expr->Number.Number);
 return Value;
}
internal b32
ValueEqual(expr_value ExprValue, f32 Value)
{
 b32 Result = (ExprValue.IsValue && Value == ExprValue.Value);
 return Result;
}
internal void
SetValue(parametric_equation_expr *Expr, f32 Value)
{
 Expr->Type = ParametricEquationExpr_Number;
 Expr->Number.Number = Value;
}

internal f32
EvalUnaryExpr(parametric_equation_unary_operator UnaryOp, f32 SubExprValue)
{
 f32 Result = 0;
 switch (UnaryOp)
 {
  case ParametricEquationUnaryOp_Plus: {Result = SubExprValue;}break;
  case ParametricEquationUnaryOp_Minus: {Result = -SubExprValue;}break;
 }
 return Result;
}

internal f32
EvalBinaryExpr(f32 LeftValue, parametric_equation_binary_operator BinaryOp, f32 RightValue)
{
 f32 Result = 0;
 switch (BinaryOp)
 {
  case ParametricEquationBinaryOp_Plus: {Result = LeftValue + RightValue;}break;
  case ParametricEquationBinaryOp_Minus: {Result = LeftValue - RightValue;}break;
  case ParametricEquationBinaryOp_Mult: {Result = LeftValue * RightValue;}break;
  case ParametricEquationBinaryOp_Div: {Result = SafeDiv0(LeftValue, RightValue);}break;
  case ParametricEquationBinaryOp_Mod: {Result = ModF32(LeftValue, RightValue);}break;
  case ParametricEquationBinaryOp_Pow: {Result = PowF32(LeftValue, RightValue);}break;
 }
 return Result;
}

internal f32
EnvLookup(parametric_equation_env *Env, string Key)
{
 f32 Result = 0;
 for (u32 Index = 0;
      Index < Env->VarCount;
      ++Index)
 {
  if (StrEqual(Key, Env->VarNames[Index]))
  {
   Result = Env->VarValues[Index];
   break;
  }
 }
 
 return Result;
}

internal f32
EvalApplicationExpr(parametric_equation_identifier Identifier,
                    parametric_equation_env *Env,
                    u32 ArgCount, f32 *ArgValues)
{
 f32 Result = 0;
 switch (Identifier.Type)
 {
  case ParametricEquationIdentifier_sin:   {Result =   SinF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_cos:   {Result =   CosF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_tan:   {Result =   TanF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_asin:  {Result =  AsinF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_acos:  {Result =  AcosF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_atan:  {Result =  AtanF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_sqrt:  {Result =  SqrtF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_log:   {Result =   LogF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_log10: {Result = Log10F32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_floor: {Result = FloorF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_ceil:  {Result =  CeilF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_round: {Result = RoundF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_tanh:  {Result =  TanhF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_sinh:  {Result =  SinhF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_cosh:  {Result =  CoshF32(ArgValues[0]);}break;
  case ParametricEquationIdentifier_exp:   {Result =   ExpF32(ArgValues[0]);}break;
  
  case ParametricEquationIdentifier_pow:   {Result =   PowF32(ArgValues[0], ArgValues[1]);}break;
  case ParametricEquationIdentifier_mod:   {Result =   ModF32(ArgValues[0], ArgValues[1]);}break;
  case ParametricEquationIdentifier_atan2:  {Result = Atan2F32(ArgValues[0], ArgValues[1]);}break;
  
  case ParametricEquationIdentifier_pi:    {Result =        Pi32;}break;
  case ParametricEquationIdentifier_tau:   {Result =       Tau32;}break;
  case ParametricEquationIdentifier_euler: {Result =     Euler32;}break;
  
  case ParametricEquationIdentifier_t:     {
   Assert(Env->T_Mode == ParametricEquationT_BoundAndValueProvided);
   Result = Env->T_Value;
  }break;
  
  case ParametricEquationIdentifier_var: {
   Assert(ArgCount == 0);
   Result = EnvLookup(Env, Identifier.Var);
  }break;
  
  case ParametricEquationIdentifier_Count: InvalidPath;
 }
 
 return Result;
}

internal parametric_equation_env
MakeEnv(u32 VarCount, string *VarNames, f32 *VarValues, parametric_equation_t_mode T_Mode, f32 T_Value)
{
 parametric_equation_env Env = {};
 Env.T_Mode = T_Mode;
 Env.T_Value = T_Value;
 Env.VarCount = VarCount;
 Env.VarNames = VarNames;
 Env.VarValues = VarValues;
 
 return Env;
}

internal void
OptimizeExpr(parametric_equation_expr *Expr, parametric_equation_env *Env)
{
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Number: {}break;
  
  case ParametricEquationExpr_Unary: {
   parametric_equation_unary_expr Unary = Expr->Unary;
   
   OptimizeExpr(Unary.SubExpr, Env);
   
   expr_value SubExprValue = ToValue(Unary.SubExpr);
   if (SubExprValue.IsValue)
   {
    f32 NewValue = EvalUnaryExpr(Unary.Operator, SubExprValue.Value);
    SetValue(Expr, NewValue);
   }
   else
   {
    if (Unary.Operator == ParametricEquationUnaryOp_Plus)
    {
     // NOTE(hbr): Special case for unary +, which doesn't do anything
     *Expr = *Unary.SubExpr;
    }
   }
  }break;
  
  case ParametricEquationExpr_Binary: {
   parametric_equation_binary_expr Binary = Expr->Binary;
   
   OptimizeExpr(Binary.Left, Env);
   OptimizeExpr(Binary.Right, Env);
   
   expr_value LeftValue = ToValue(Binary.Left);
   expr_value RightValue = ToValue(Binary.Right);
   
   if (LeftValue.IsValue && RightValue.IsValue)
   {
    f32 NewValue = EvalBinaryExpr(LeftValue.Value, Binary.Operator, RightValue.Value);
    SetValue(Expr, NewValue);
   }
   else
   {
    switch (Binary.Operator)
    {
     case ParametricEquationBinaryOp_Plus: {
      if (ValueEqual(LeftValue, 0))
      {
       *Expr = *Binary.Right;
      }
      else if (ValueEqual(RightValue, 0))
      {
       *Expr = *Binary.Left;
      }
     }break;
     
     case ParametricEquationBinaryOp_Minus: {
      if (ValueEqual(RightValue, 0))
      {
       *Expr = *Binary.Right;
      }
     }break;
     
     case ParametricEquationBinaryOp_Mult: {
      if (ValueEqual(LeftValue, 0) || ValueEqual(RightValue, 0))
      {
       SetValue(Expr, 0);
      }
      else if (ValueEqual(LeftValue, 1))
      {
       *Expr = *Binary.Right;
      }
      else if (ValueEqual(RightValue, 1))
      {
       *Expr = *Binary.Left;
      }
     }break;
     
     case ParametricEquationBinaryOp_Div: {
      if (ValueEqual(RightValue, 1))
      {
       *Expr = *Binary.Left;
      }
      else if (ValueEqual(RightValue, 0)) // SafeDiv0
      {
       SetValue(Expr, 0);
      }
     }break;
     
     case ParametricEquationBinaryOp_Mod: {
      if (ValueEqual(RightValue, 0))
      {
       SetValue(Expr, 0);
      }
     }break;
     
     case ParametricEquationBinaryOp_Pow: {
      if (ValueEqual(LeftValue, 0))
      {
       SetValue(Expr, 0);
      }
      else if (ValueEqual(LeftValue, 1))
      {
       SetValue(Expr, 1);
      }
      else if (ValueEqual(RightValue, 0))
      {
       SetValue(Expr, 1);
      }
      else if (ValueEqual(RightValue, 1))
      {
       *Expr = *Binary.Left;
      }
     }break;
    }
   }
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   
   f32 ArgValues[PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT];
   b32 AllArgValues = true;
   for (u32 ArgIndex = 0;
        ArgIndex < Application.ArgCount;
        ++ArgIndex)
   {
    parametric_equation_expr *Arg = Application.Args[ArgIndex];
    
    OptimizeExpr(Arg, Env);
    
    expr_value ArgValue = ToValue(Arg);
    ArgValues[ArgIndex] = ArgValue.Value;
    AllArgValues = (AllArgValues && ArgValue.IsValue);
   }
   
   if (AllArgValues && Application.Identifier.Type != ParametricEquationIdentifier_t)
   {
    f32 NewValue = EvalApplicationExpr(Application.Identifier, Env, Application.ArgCount, ArgValues);
    SetValue(Expr, NewValue);
   }
  }break;
 }
}

// NOTE(hbr): Parse expr. If it doesn't parse the whole thing, assume
// virtual binary multiplication operator and try again.
internal parametric_equation_expr *
ParseExprOptimistic(arena *Arena,
                    parametric_equation_token_array Tokens,
                    parametric_equation_parsing_error *ErrorCapture)
{
 parametric_equation_expr *Parsed = NilExpr;
 
 temp_arena Temp = TempArena(Arena);
 temp_arena Checkpoint = BeginTemp(Arena);
 parametric_equation_token_array CurrentTokens = Tokens;
 b32 Parsing = true;
 
 while (Parsing)
 {
  EndTemp(Checkpoint);
  
  parametric_equation_parser Parser = BeginParsing(CurrentTokens, ErrorCapture);
  // NOTE(hbr): Parse empty expression
  if (!EatIfMatch(&Parser, ParametricEquationToken_None))
  {
   parametric_equation_expr *Expr = ParseExpr(Arena, &Parser, 0);
   if (IsError(ErrorCapture))
   {
    Parsing = false;
   }
   else if (EatIfMatch(&Parser, ParametricEquationToken_None))
   {
    Parsing = false;
    Parsed = Expr;
   }
   else
   {
    u32 TokenInsertPoint = Cast(u32)(Parser.TokenAt - CurrentTokens.Tokens);
    Assert(TokenInsertPoint < CurrentTokens.TokenCount);
    
    parametric_equation_token FakeToken = MakeToken(ParametricEquationToken_Asterisk);
    parametric_equation_token *NewTokens = PushArrayNonZero(Temp.Arena, CurrentTokens.TokenCount + 1, parametric_equation_token);
    
    // NOTE(hbr): This isn't particularly performant (both that we restart from the beginning
    // and that we insert into array) in general, but I bet for our use cases, this is fine.
    ArrayCopy(NewTokens, CurrentTokens.Tokens, TokenInsertPoint);
    NewTokens[TokenInsertPoint] = FakeToken;
    ArrayCopy(NewTokens + TokenInsertPoint + 1,
              CurrentTokens.Tokens + TokenInsertPoint,
              CurrentTokens.TokenCount - TokenInsertPoint);
    
    CurrentTokens = MakeTokenArray(CurrentTokens.TokenCount + 1, NewTokens);
   }
  }
  else
  {
   Parsing = false;
  }
 }
 
 EndTemp(Temp);
 
 return Parsed;
}

// TODO(hbr): Maybe keep one convention - either we do parse parametric equation or parametric equation parse
internal parametric_equation_parse_result
ParseEquation(arena *Arena, string Equation, parametric_equation_env Env, b32 DontOptimize)
{
 temp_arena Temp = TempArena(Arena);
 
 parametric_equation_parsing_error ErrorCapture = {};
 ErrorCapture.Arena = Arena;
 
 parametric_equation_token_array Tokens = TokenizeParametricEquation(Temp.Arena, Equation, &ErrorCapture);
 parametric_equation_expr *Parsed = ParseExprOptimistic(Arena, Tokens, &ErrorCapture);
 TypeCheckExpr(Parsed, &Env, &ErrorCapture);
 
 if (!IsError(&ErrorCapture))
 {
  if (!DontOptimize)
  {
   OptimizeExpr(Parsed, &Env);
  }
 }
 
 parametric_equation_parse_result Result = {};
 Result.ParsedExpr = (IsError(&ErrorCapture) ? NilExpr : Parsed);
 Result.Fail = IsError(&ErrorCapture);
 Result.ErrorMessage = ErrorCapture.ErrorMessage;
 
 EndTemp(Temp);
 
 return Result;
}

internal parametric_equation_parse_result
ParametricEquationParse(arena *Arena, string Equation,
                        u32 VarCount, string *VarNames, f32 *VarValues,
                        b32 DontOptimize)
{
 parametric_equation_env Env = MakeEnv(VarCount, VarNames, VarValues, ParametricEquationT_JustBound, 0);
 parametric_equation_parse_result Result = ParseEquation(Arena, Equation, Env, DontOptimize);
 
 return Result;
}

internal f32
EvalExpr(parametric_equation_expr *Expr,
         parametric_equation_env *Env)
{
 f32 Result = 0;
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   f32 Value = EvalExpr(Expr->Unary.SubExpr, Env);
   Result = EvalUnaryExpr(Expr->Unary.Operator, Value);
  }break;
  
  case ParametricEquationExpr_Binary: {
   f32 Left = EvalExpr(Expr->Binary.Left, Env);
   f32 Right = EvalExpr(Expr->Binary.Right, Env);
   Result = EvalBinaryExpr(Left, Expr->Binary.Operator, Right);
  }break;
  
  case ParametricEquationExpr_Number: {
   Result = Expr->Number.Number;
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   
   f32 ArgValues[PARAMETRIC_EQUATION_APPLICATION_MAX_ARG_COUNT];
   for (u32 ArgIndex = 0;
        ArgIndex < Application.ArgCount;
        ++ArgIndex)
   {
    parametric_equation_expr *Arg = Application.Args[ArgIndex];
    ArgValues[ArgIndex] = EvalExpr(Arg, Env);
   }
   
   Result = EvalApplicationExpr(Application.Identifier, Env, Application.ArgCount, ArgValues);
  }break;
 }
 
 return Result;
}

internal f32
ParametricEquationEvalWithT(parametric_equation_expr *Expr, f32 T)
{
 parametric_equation_env Env = MakeEnv(0, 0, 0, ParametricEquationT_BoundAndValueProvided, T);
 f32 Result = EvalExpr(Expr, &Env);
 
 return Result;
}

internal parametric_equation_eval_result
ParametricEquationEval(arena *Arena, string Equation,
                       u32 VarCount, string *VarNames, f32 *VarValues)
{
 parametric_equation_env Env = MakeEnv(VarCount, VarNames, VarValues, ParametricEquationT_Unbound, 0);
 parametric_equation_parse_result Parse = ParseEquation(Arena, Equation, Env, false);
 
 parametric_equation_eval_result Result = {};
 Result.Value = EvalExpr(Parse.ParsedExpr, &Env);
 Result.Fail = Parse.Fail;
 Result.ErrorMessage = Parse.ErrorMessage;
 
 return Result;
}