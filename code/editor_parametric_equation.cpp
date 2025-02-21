internal parametric_equation_token *
AddToken(parametric_equation_parser *Parser,
         parametric_equation_token_type Type,
         char *TokenBeginChar,
         parametric_equation_source_location TokenLocation)
{
 Assert(Parser->TokenCount < Parser->MaxTokenCount);
 parametric_equation_token *Token = Parser->Tokens + Parser->TokenCount;
 ++Parser->TokenCount;
 
 Token->Type = Type;
 Token->OriginalString = MakeStr(TokenBeginChar, Parser->CharAt - TokenBeginChar);
 Token->Location = TokenLocation;
 
 return Token;
}

inline internal void
AdvanceChar(parametric_equation_parser *Parser, u32 Count)
{
 while (Count--)
 {
  if (Parser->CharAt[0] == '\n')
  {
   ++Parser->CharLocation.LineAt;
   Parser->CharLocation.CharWithinLineAt = 0;
  }
  else
  {
   ++Parser->CharLocation.CharWithinLineAt;
  }
  ++Parser->CharAt;
 }
}

internal b32
ParseIdentifier(parametric_equation_parser *Parser, string *OutIdentifier)
{
 char *SaveAt = Parser->CharAt;
 
 b32 Parsing = true;
 b32 FirstChar = true;
 do {
  char C = Parser->CharAt[0];
  
  b32 AlwaysPossible = (CharIsAlpha(C) || C == '_');
  b32 OnlyNotAtTheBeginning = (CharIsDigit(C) || C == '\'');
  b32 IsEligibleForIdentifier = (AlwaysPossible || (!FirstChar && OnlyNotAtTheBeginning));
  
  if (IsEligibleForIdentifier)
  {
   AdvanceChar(Parser, 1);
  }
  else
  {
   Parsing = false;
  }
  
  FirstChar = false;
 } while (Parsing);
 
 b32 Parsed = false;
 if (Parser->CharAt != SaveAt)
 {
  Parsed = true;
  *OutIdentifier = MakeStr(SaveAt, Parser->CharAt - SaveAt);
 }
 
 return Parsed;
}

internal b32
ParseInteger(parametric_equation_parser *Parser, u64 *OutInteger)
{
 b32 Parsed = false;
 u64 N = 0;
 b32 Parsing = true;
 b32 FirstChar = true;
 
 do {
  char C = Parser->CharAt[0];
  
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
   AdvanceChar(Parser, 1);
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
MaybeEatChar(parametric_equation_parser *Parser, char C)
{
 char Actual = Parser->CharAt[0];
 if (Actual == C)
 {
  AdvanceChar(Parser, 1);
 }
}

internal b32
ParseNumber(parametric_equation_parser *Parser, f32 *OutNumber)
{
 u64 PreDot = 0;
 u64 PostDot = 0;
 
 b32 PreDotParsed = ParseInteger(Parser, &PreDot);
 MaybeEatChar(Parser, '.');
 b32 PostDotParsed = ParseInteger(Parser, &PostDot);
 
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
IsError(parametric_equation_parser *Parser)
{
 b32 Result = (Parser->ErrorMessage.Data != 0);
 return Result;
}

internal void
SetErrorAtLocation(parametric_equation_parser *Parser,
                   parametric_equation_source_location Location,
                   string BaseErrorMessage)
{
 temp_arena Temp = TempArena(Parser->Arena);
 
 string ErrorMessage = StrF(Temp.Arena,
                            "(%u,%u) %S",
                            Location.LineAt + 1,
                            Location.CharWithinLineAt + 1,
                            BaseErrorMessage);
 
 // TODO(hbr): Check whether it's better to accumulate errors here or just save the first one.
 // The advantage for the second case is that there is more information passed to the user.
 // The disadvantage is that the information could be lower quality. Hmmm, maybe we should do
 // best case parsing? Meaning if there is something we expect, but it's not there, assume it's there,
 // continue, but also set the error.
 if (!IsError(Parser))
 {
  Parser->ErrorMessage = StrCopy(Parser->Arena, ErrorMessage);
 }
 
 EndTemp(Temp);
}

internal void
SetErrorAtLocationF(parametric_equation_parser *Parser,
                    parametric_equation_source_location Location,
                    char const *Format,
                    ...)
{
 va_list Args;
 va_start(Args, Format);
 
 temp_arena Temp = TempArena(Parser->Arena);
 
 string BaseErrorMessage = StrFV(Temp.Arena, Format, Args);
 SetErrorAtLocation(Parser, Location, BaseErrorMessage);
 
 EndTemp(Temp);
 
 va_end(Args);
}

internal parametric_equation_token
GetLastToken(parametric_equation_parser *Parser)
{
 parametric_equation_token Token = {};
 if (Parser->TokenCount > 0)
 {
  Token = Parser->Tokens[Parser->TokenCount - 1];
 }
 return Token;
}

internal void
TokenizeParametricEquation(parametric_equation_parser *Parser,
                           string Equation)
{
 parametric_equation_lexer Lexer = {};
 
 
 // NOTE(hbr): Copy the equation because tokens/parse tree point to that original text
 Equation = StrCopy(Parser->Arena, Equation);
 
 Parser->CharAt = Equation.Data;
 // NOTE(hbr): One additional space for Token_None, twice for virtual multiply tokens
 Parser->MaxTokenCount = SafeCastU32(2 * Equation.Count + 1);
 Parser->Tokens = PushArrayNonZero(Parser->Arena, Parser->MaxTokenCount, parametric_equation_token);
 
 b32 Tokenizing = true;
 do {
  //- multiline comment
  b32 MultiLineComment = true;
  u32 NestLevel = 0;
  do {
   if (Parser->CharAt[0] == '\0')
   {
    if (NestLevel > 0)
    {
     SetErrorAtLocation(Parser,
                        Parser->CharLocation,
                        StrLit("non-terminated multiline comment, unexpected end of input"));
    }
    MultiLineComment = false;
   }
   else
   {
    if (Parser->CharAt[0] == '/' &&
        Parser->CharAt[1] == '*')
    {
     ++NestLevel;
     AdvanceChar(Parser, 2);
    }
    else if (NestLevel > 0 &&
             Parser->CharAt[0] == '*' &&
             Parser->CharAt[1] == '/')
    {
     --NestLevel;
     AdvanceChar(Parser, 2);
    }
    else if (NestLevel > 0)
    {
     AdvanceChar(Parser, 1);
    }
   }
   
   if (NestLevel == 0)
   {
    MultiLineComment = false;
   }
   
  } while (MultiLineComment);
  
  if (CharIsWhiteSpace(Parser->CharAt[0]))
  {
   AdvanceChar(Parser, 1);
  }
  else if (Parser->CharAt[0] == '/' &&
           Parser->CharAt[1] == '/')
  {
   AdvanceChar(Parser, 2);
   
   b32 Comment = true;
   do {
    if (Parser->CharAt[0] == '\0')
    {
     Comment = false;
    }
    else
    {
     if (Parser->CharAt[0] == '\n')
     {
      Comment = false;
     }
     AdvanceChar(Parser, 1);
    }
   } while (Comment);
   // NOTE(hbr): If this is the end of input,
   // then so be it. Just finish parsing.
  }
  else
  {
   char *SaveCharAt = Parser->CharAt;
   parametric_equation_source_location SaveCharLocation = Parser->CharLocation;
   
   switch (Parser->CharAt[0])
   {
    case '\0': {
     Tokenizing = false;
    }break;
    
    case '-': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Minus, SaveCharAt, SaveCharLocation);
    }break;
    
    case '+': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Plus, SaveCharAt, SaveCharLocation);
    }break;
    
    case '/': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Slash, SaveCharAt, SaveCharLocation);
    }break;
    
    case '*': {
     AdvanceChar(Parser, 1);
     if (Parser->CharAt[0] == '*')
     {
      AdvanceChar(Parser, 1);
      AddToken(Parser, ParametricEquationToken_Pow, SaveCharAt, SaveCharLocation);
     }
     else
     {
      AddToken(Parser, ParametricEquationToken_Mult, SaveCharAt, SaveCharLocation);
     }
    }break;
    
    case '^': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Pow, SaveCharAt, SaveCharLocation);
    }break;
    
    case '(': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_OpenParen, SaveCharAt, SaveCharLocation);
    }break;
    
    case ')': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_CloseParen, SaveCharAt, SaveCharLocation);
    }break;
    
    case '{': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_OpenBrace, SaveCharAt, SaveCharLocation);
    }break;
    
    case '}': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_CloseBrace, SaveCharAt, SaveCharLocation);
    }break;
    
    case ',': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Comma, SaveCharAt, SaveCharLocation);
    }break;
    
    case ':': {
     AdvanceChar(Parser, 1);
     if (Parser->CharAt[0] == '=')
     {
      AdvanceChar(Parser, 1);
      AddToken(Parser, ParametricEquationToken_Equal, SaveCharAt, SaveCharLocation);
     }
     else
     {
      SetErrorAtLocation(Parser, Parser->CharLocation, StrLit("missing '=' after ':'"));
     }
    }break;
    
    case '=': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Equal, SaveCharAt, SaveCharLocation);
    }break;
    
    case ';': {
     AdvanceChar(Parser, 1);
     AddToken(Parser, ParametricEquationToken_Semicolon, SaveCharAt, SaveCharLocation);
    }break;
    
    default: {
     string Identifier = {};
     f32 Number = 0;
     if (ParseIdentifier(Parser, &Identifier))
     {
      // NOTE(hbr): If the last token was a number, insert imaginary multiply token so that for example "12t" parses.
      parametric_equation_token LastToken = GetLastToken(Parser);
      if (LastToken.Type == ParametricEquationToken_Number)
      {
       parametric_equation_token *Token = AddToken(Parser, ParametricEquationToken_Mult, Parser->CharAt, Parser->CharLocation);
      }
      
      parametric_equation_token *Token = AddToken(Parser, ParametricEquationToken_Identifier, SaveCharAt, SaveCharLocation);
      Token->Identifier = Identifier;
     }
     else if (ParseNumber(Parser, &Number))
     {
      parametric_equation_token *Token = AddToken(Parser, ParametricEquationToken_Number, SaveCharAt, SaveCharLocation);
      Token->Number = Number;
     }
     else
     {
      SetErrorAtLocationF(Parser, Parser->CharLocation, "unexpected character '%c'", Parser->CharAt[0]);
     }
    }break;
   }
  }
  
  if (IsError(Parser))
  {
   Tokenizing = false;
  }
 } while (Tokenizing);
 
 AddToken(Parser, ParametricEquationToken_None, Parser->CharAt, Parser->CharLocation);
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
AllocExpr(parametric_equation_parser *Parser, parametric_equation_expr_type Type)
{
 parametric_equation_expr *Expr = PushStruct(Parser->Arena, parametric_equation_expr);
 Expr->Type = Type;
 
 return Expr;
}

internal parametric_equation_expr *
MakeUnaryExpr(parametric_equation_parser *Parser,
              parametric_equation_unary_operator UnaryOp,
              parametric_equation_expr *SubExpr)
{
 parametric_equation_expr *Expr = AllocExpr(Parser, ParametricEquationExpr_Unary);
 Expr->Unary.Operator = UnaryOp;
 Expr->Unary.SubExpr = SubExpr;
 
 return Expr;
}

internal parametric_equation_expr *
MakeBinaryExpr(parametric_equation_parser *Parser,
               parametric_equation_expr *Left,
               parametric_equation_binary_operator BinaryOp,
               parametric_equation_expr *Right)
{
 parametric_equation_expr *Expr = AllocExpr(Parser, ParametricEquationExpr_Binary);
 Expr->Binary.Left = Left;
 Expr->Binary.Right = Right;
 Expr->Binary.Operator = BinaryOp;
 
 return Expr;
}

internal parametric_equation_expr *
MakeNumberExpr(parametric_equation_parser *Parser,
               f32 Number)
{
 parametric_equation_expr *Expr = AllocExpr(Parser, ParametricEquationExpr_Number);
 Expr->Number.Number = Number;
 
 return Expr;
}

internal parametric_equation_application_expr_identifier
MakeSpecialIdentifier(parametric_equation_identifier_type Special)
{
 parametric_equation_application_expr_identifier Identifier = {};
 Identifier.Type = Special;
 
 return Identifier;
}

internal parametric_equation_application_expr_identifier
MakeVarIdentifier(string Var)
{
 parametric_equation_application_expr_identifier Identifier = {};
 Identifier.Type = ParametricEquationIdentifier_var;
 Identifier.Var = Var;
 
 return Identifier;
}

internal parametric_equation_application_expr_identifier
RawIdentifierToApplicationExprIdentifier(string RawIdentifier,
                                         parametric_equation_source_location Location)
{
 parametric_equation_application_expr_identifier Identifier = {};
 
 b32 Found = false;
 for (u32 IdentifierIndex = 0;
      IdentifierIndex < ParametricEquationIdentifier_var;
      ++IdentifierIndex)
 {
  if (IdentifierIndex != ParametricEquationIdentifier_var)
  {
   string IdentifierName = StrFromCStr(ParametricEquationIdentifierNames[IdentifierIndex]);
   if (StrMatch(RawIdentifier, IdentifierName, true))
   {
    parametric_equation_identifier_type Type = Cast(parametric_equation_identifier_type)IdentifierIndex;
    Identifier = MakeSpecialIdentifier(Type);
    Found = true;
    
    break;
   }
  }
 }
 
 if (!Found)
 {
  Identifier = MakeVarIdentifier(RawIdentifier);
 }
 
 Identifier.Location = Location;
 
 return Identifier;
}

internal parametric_equation_expr *
MakeApplicationExpr(parametric_equation_parser *Parser,
                    string RawIdentifier,
                    parametric_equation_source_location IdentifierLocation,
                    u32 ArgCount,
                    parametric_equation_expr *Args)
{
 parametric_equation_expr *Expr = AllocExpr(Parser, ParametricEquationExpr_Application);
 Expr->Application.Identifier = RawIdentifierToApplicationExprIdentifier(RawIdentifier, IdentifierLocation);
 Expr->Application.ArgCount = ArgCount;
 Expr->Application.Args = Args;
 
 return Expr;
}

internal b32
IsUnaryOperator(parametric_equation_token Token)
{
 b32 Result = ((Token.Type == ParametricEquationToken_Plus) ||
               (Token.Type == ParametricEquationToken_Minus));
 return Result;
}

internal parametric_equation_unary_operator
TokenToUnaryOperator(parametric_equation_token Token)
{
 parametric_equation_unary_operator Result = Cast(parametric_equation_unary_operator)0;
 switch (Token.Type)
 {
  case ParametricEquationToken_Plus: {Result = ParametricEquationUnaryOp_Plus;}break;
  case ParametricEquationToken_Minus: {Result = ParametricEquationUnaryOp_Minus;}break;
  
  default: InvalidPath;
 }
 
 return Result;
}

internal b32
IsBinaryOperator(parametric_equation_token Token)
{
 b32 Result = ((Token.Type == ParametricEquationToken_Plus) ||
               (Token.Type == ParametricEquationToken_Minus) ||
               (Token.Type == ParametricEquationToken_Mult) ||
               (Token.Type == ParametricEquationToken_Slash) ||
               (Token.Type == ParametricEquationToken_Pow));
 return Result;
}

internal parametric_equation_binary_operator
TokenToBinaryOperator(parametric_equation_token Token)
{
 parametric_equation_binary_operator Result = Cast(parametric_equation_binary_operator)0;
 switch (Token.Type)
 {
  case ParametricEquationToken_Plus: {Result = ParametricEquationBinaryOp_Plus;}break;
  case ParametricEquationToken_Minus: {Result = ParametricEquationBinaryOp_Minus;}break;
  case ParametricEquationToken_Mult: {Result = ParametricEquationBinaryOp_Mult;}break;
  case ParametricEquationToken_Slash: {Result = ParametricEquationBinaryOp_Div;}break;
  case ParametricEquationToken_Pow: {Result = ParametricEquationBinaryOp_Pow;}break;
  
  default: InvalidPath;
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
 b32 Valid = (Expr != &NilExpr);
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
  case ParametricEquationBinaryOp_Div: {Result = 2;}break;
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

internal parametric_equation_expr *ParseExpr(parametric_equation_parser *Parser, u8 MinPrecedence);

internal parametric_equation_expr *
ParseLeafExpr(parametric_equation_parser *Parser)
{
 parametric_equation_expr *Expr = &NilExpr;
 
 parametric_equation_token NextToken = Parser->TokenAt[0];
 if (IsUnaryOperator(NextToken))
 {
  ++Parser->TokenAt;
  
  parametric_equation_unary_operator UnaryOp = TokenToUnaryOperator(NextToken);
  u8 NextPrecedence = UnaryOperatorToPrecedence(UnaryOp);
  
  parametric_equation_expr *SubExpr = ParseExpr(Parser, NextPrecedence);
  
  Expr = MakeUnaryExpr(Parser, UnaryOp, SubExpr);
 }
 else if (IsNumber(NextToken))
 {
  ++Parser->TokenAt;
  
  Expr = MakeNumberExpr(Parser, NextToken.Number);
 }
 else if (IsIdentifier(NextToken))
 {
  ++Parser->TokenAt;
  
  string RawIdentifier = NextToken.Identifier;
  parametric_equation_source_location RawIdentifierLocation = NextToken.Location;
  
  u32 ArgCount = 0;
  parametric_equation_expr *ArgHead = 0;
  parametric_equation_expr *ArgTail = 0;
  if (EatIfMatch(Parser, ParametricEquationToken_OpenParen))
  {
   b32 Parsing = true;
   do {
    parametric_equation_token NextToken = Parser->TokenAt[0];
    if (NextToken.Type == ParametricEquationToken_CloseParen)
    {
     Parsing = false;
    }
    else
    {
     parametric_equation_expr *Arg = ParseExpr(Parser, 0);
     // TODO(hbr): Try to get rid of this if by using NilExpr
     if (IsValid(Arg))
     {
      QueuePushCounted(ArgHead, ArgTail, Arg, ArgCount);
      if (!EatIfMatch(Parser, ParametricEquationToken_Comma))
      {
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
    SetErrorAtLocation(Parser, NextToken.Location, StrLit("'(' is missing ')'"));
   }
  }
  
  Expr = MakeApplicationExpr(Parser, RawIdentifier, RawIdentifierLocation, ArgCount, ArgHead);
 }
 else if (NextToken.Type == ParametricEquationToken_OpenParen)
 {
  ++Parser->TokenAt;
  
  Expr = ParseExpr(Parser, 0);
  
  if (!EatIfMatch(Parser, ParametricEquationToken_CloseParen))
  {
   SetErrorAtLocation(Parser, NextToken.Location, StrLit("'(' is missing ')'"));
  }
 }
 else
 {
  parametric_equation_token Token = Parser->TokenAt[0];
  if (Token.Type == ParametricEquationToken_None)
  {
   SetErrorAtLocation(Parser, Token.Location, StrLit("unexpected end of input"));
  }
  else
  {
   SetErrorAtLocationF(Parser, Token.Location, "unexpected token '%S'", Token.OriginalString);
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
  case ParametricEquationBinaryOp_Div: {}break;
  
  case ParametricEquationBinaryOp_Pow: {Result=true;}break;
 }
 
 return Result;
}

// Expressions of increasing operator precedence are right-leaning.
// This function creates exactly right leaning trees.
internal parametric_equation_expr *
ParseExprIncreasingPrecedence(parametric_equation_parser *Parser,
                              parametric_equation_expr *Left,
                              u8 MinPrecedence)
{
 parametric_equation_expr *Expr = Left;
 
 parametric_equation_token NextToken = Parser->TokenAt[0];
 if (IsBinaryOperator(NextToken))
 {
  parametric_equation_binary_operator BinaryOp = TokenToBinaryOperator(NextToken);
  u8 NextPrecedence = BinaryOperatorToPrecedence(BinaryOp);
  
  if ((NextPrecedence > MinPrecedence) ||
      (NextPrecedence == MinPrecedence && IsRightAssociative(BinaryOp)))
  {
   ++Parser->TokenAt;
   
   parametric_equation_expr *Right = ParseExpr(Parser, NextPrecedence);
   Expr = MakeBinaryExpr(Parser, Left, BinaryOp, Right);
  }
 }
 
 return Expr;
}

// This function creates left-leaning trees (but also calls ParseExprIncreasingPrecedence,
// which creates right-leaning trees. Interplay of these two functions correctly parse
// an expression.
internal parametric_equation_expr *
ParseExpr(parametric_equation_parser *Parser, u8 MinPrecedence)
{
 parametric_equation_expr *Left = ParseLeafExpr(Parser);
 
 b32 Parsing = true;
 do
 {
  parametric_equation_expr *Expr = ParseExprIncreasingPrecedence(Parser, Left, MinPrecedence);
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
 if (Env->T_Bounded && StrEqual(Var, StrLit("t")))
 {
  Bound = true;
 }
 else
 {
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
 }
 return Bound;
}

internal void
TypeCheckExpr(parametric_equation_parser *Parser,
              parametric_equation_expr *Expr,
              parametric_equation_env *Env)
{
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   TypeCheckExpr(Parser, Expr->Unary.SubExpr, Env);
  }break;
  
  case ParametricEquationExpr_Binary: {
   TypeCheckExpr(Parser, Expr->Binary.Left, Env);
   TypeCheckExpr(Parser, Expr->Binary.Right, Env);
  }break;
  
  case ParametricEquationExpr_Number: {}break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   parametric_equation_application_expr_identifier Identifier = Application.Identifier;
   
   if (Identifier.Type == ParametricEquationIdentifier_var)
   {
    string Var = Identifier.Var;
    
    if (!IsBound(Env, Var))
    {
     SetErrorAtLocationF(Parser, Identifier.Location, "'%S' identifier not bound", Var);
    }
   }
   
   u32 ExpectedArgCount = ParametricEquationIdentifierArgCounts[Identifier.Type];
   if (Application.ArgCount != ExpectedArgCount)
   {
    string IdentifierStr = {};
    if (Identifier.Type == ParametricEquationIdentifier_var)
    {
     IdentifierStr = Identifier.Var;
    }
    else
    {
     char const *IdentifierName = ParametricEquationIdentifierNames[Identifier.Type];
     IdentifierStr = StrFromCStr(IdentifierName);
    }
    
    if (ExpectedArgCount == 0)
    {
     SetErrorAtLocationF(Parser, Identifier.Location,
                         "'%S' is not a function, no arguments expected (got %u)",
                         IdentifierStr, Application.ArgCount);
    }
    else
    {
     char const *ArgumentStr = (ExpectedArgCount == 1 ? "argument" : "arguments");
     SetErrorAtLocationF(Parser, Identifier.Location,
                         "'%S' expects %u %s (got %u)",
                         IdentifierStr, ExpectedArgCount, ArgumentStr, Application.ArgCount);
    }
   }
   
   ListIter(ArgExpr, Application.Args, parametric_equation_expr)
   {
    TypeCheckExpr(Parser, ArgExpr, Env);
   }
  }break;
 }
}

internal parametric_equation_parser
MakeParser(arena *Arena)
{
 parametric_equation_parser Parser = {};
 Parser.Arena = Arena;
 
 return Parser;
}

internal void
BeginParsing(parametric_equation_parser *Parser)
{
 Parser->TokenAt = Parser->Tokens;
}

// TODO(hbr): Maybe keep one convention - either we do parse parametric equation or parametric equation parse
internal parametric_equation_parse_result
ParseEquation(arena *Arena, string Equation,
              u32 VarCount, string *VarNames, f32 *VarValues,
              b32 T_Bounded)
{
 parametric_equation_parse_result Result = {};
 
 parametric_equation_parser Parser = {};
 Parser.Arena = Arena;
 
 TokenizeParametricEquation(&Parser, Equation);
 
 BeginParsing(&Parser);
 
 parametric_equation_expr *Parsed = ParseExpr(&Parser, 0);
 
 if (!EatIfMatch(&Parser, ParametricEquationToken_None))
 {
  parametric_equation_token Token = Parser.TokenAt[0];
  Assert(Token.Type != ParametricEquationToken_None);
  SetErrorAtLocationF(&Parser, Token.Location, "unexpected token '%S', expected end of input", Token.OriginalString);
 }
 
 parametric_equation_env Env = {};
 Env.T_Bounded = T_Bounded;
 Env.VarCount = VarCount;
 Env.VarNames = VarNames;
 Env.VarValues = VarValues;
 TypeCheckExpr(&Parser, Parsed, &Env);
 
 Result.ParsedExpr = Parsed;
 Result.Ok = (!IsError(&Parser));
 Result.ErrorMessage = Parser.ErrorMessage;
 
 return Result;
}

internal parametric_equation_parse_result
ParametricEquationParse(arena *Arena, string Equation,
                        u32 VarCount, string *VarNames, f32 *VarValues)
{
 parametric_equation_parse_result Result = ParseEquation(Arena, Equation,
                                                         VarCount, VarNames, VarValues,
                                                         true);
 return Result;
}

internal f32
EnvLookup(parametric_equation_env *Env, string Key)
{
 f32 Result = 0.0f;
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
EvalExpr(parametric_equation_expr *Expr,
         parametric_equation_env *Env)
{
 f32 Result = 0;
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   f32 Value = EvalExpr(Expr->Unary.SubExpr, Env);
   switch (Expr->Unary.Operator)
   {
    case ParametricEquationUnaryOp_Plus: {Result = Value;}break;
    case ParametricEquationUnaryOp_Minus: {Result = -Value;}break;
   }
  }break;
  
  case ParametricEquationExpr_Binary: {
   f32 Left = EvalExpr(Expr->Binary.Left, Env);
   f32 Right = EvalExpr(Expr->Binary.Right, Env);
   switch (Expr->Binary.Operator)
   {
    case ParametricEquationBinaryOp_Plus: {Result = Left + Right;}break;
    case ParametricEquationBinaryOp_Minus: {Result = Left - Right;}break;
    case ParametricEquationBinaryOp_Mult: {Result = Left * Right;}break;
    case ParametricEquationBinaryOp_Div: {Result = SafeDiv0(Left, Right);}break;
    case ParametricEquationBinaryOp_Pow: {Result = PowF32(Left, Right);}break;
   }
  }break;
  
  case ParametricEquationExpr_Number: {
   Result = Expr->Number.Number;
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   parametric_equation_expr *Arg0 = Application.Args;
   f32 Arg = ((Application.ArgCount == 0) ? 0 : EvalExpr(Application.Args, Env));
   
   switch (Application.Identifier.Type)
   {
    case ParametricEquationIdentifier_sin:   {Result =   SinF32(Arg);}break;
    case ParametricEquationIdentifier_cos:   {Result =   CosF32(Arg);}break;
    case ParametricEquationIdentifier_tan:   {Result =   TanF32(Arg);}break;
    case ParametricEquationIdentifier_sqrt:  {Result =  SqrtF32(Arg);}break;
    case ParametricEquationIdentifier_log:   {Result =   LogF32(Arg);}break;
    case ParametricEquationIdentifier_log10: {Result = Log10F32(Arg);}break;
    case ParametricEquationIdentifier_floor: {Result = FloorF32(Arg);}break;
    case ParametricEquationIdentifier_ceil:  {Result =  CeilF32(Arg);}break;
    case ParametricEquationIdentifier_round: {Result = RoundF32(Arg);}break;
    case ParametricEquationIdentifier_tanh:  {Result =  TanhF32(Arg);}break;
    case ParametricEquationIdentifier_exp:   {Result =   ExpF32(Arg);}break;
    
    case ParametricEquationIdentifier_pow: {
     parametric_equation_expr *Arg1 = Arg0->Next;
     
     f32 Base = Arg;
     f32 Exp = EvalExpr(Arg1, Env);
     
     Result = PowF32(Base, Exp);
    }break;
    
    case ParametricEquationIdentifier_pi:    {Result =    Pi32;}break;
    case ParametricEquationIdentifier_tau:   {Result =   Tau32;}break;
    case ParametricEquationIdentifier_euler: {Result = Euler32;}break;
    
    case ParametricEquationIdentifier_var: {
     Assert(Application.ArgCount == 0);
     Result = EnvLookup(Env, Application.Identifier.Var);
    }break;
   }break;
  }
 }
 
 return Result;
}

internal f32
ParametricEquationEvalWithT(parametric_equation_expr *Expr, f32 T)
{
 parametric_equation_env Env = {};
 Env.T_Bounded = true;
 Env.T_Value = T;
 
 f32 Result = EvalExpr(Expr, &Env);
 
 return Result;
}

internal parametric_equation_eval_result
ParametricEquationEval(arena *Arena, string Equation,
                       u32 VarCount, string *VarNames, f32 *VarValues)
{
 parametric_equation_eval_result Result = {};
 
 parametric_equation_parse_result Parse = ParseEquation(Arena, Equation, VarCount, VarNames, VarValues, false);
 if (Parse.Ok)
 {
  // TODO(hbr): We shouldn't set these things two times
  parametric_equation_env Env = {};
  Env.VarCount = VarCount;
  Env.VarNames = VarNames;
  Env.VarValues = VarValues;
  
  f32 Value = EvalExpr(Parse.ParsedExpr, &Env);
  Result.Value = Value;
 }
 
 Result.Ok = Parse.Ok;
 Result.ErrorMessage = Parse.ErrorMessage;
 
 return Result;
}