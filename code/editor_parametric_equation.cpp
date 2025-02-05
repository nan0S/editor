internal char
ParametricEquation_Tokenizer_NextChar(parametric_equation_tokenizer *Tokenizer)
{
 char C = 0;
 if (Tokenizer->Left)
 {
  C = Tokenizer->At[0];
 }
 return C;
}

internal parametric_equation_token *
ParametricEquation_Tokenizer_AddToken(parametric_equation_tokenizer *Tokenizer,
                                      parametric_equation_token_type Type)
{
 if (Tokenizer->TokenCount == Tokenizer->MaxTokenCount)
 {
  u32 NewMaxTokenCount = Max(8, 2 * Tokenizer->MaxTokenCount);
  parametric_equation_token *NewTokens = PushArrayNonZero(Tokenizer->Arena, NewMaxTokenCount, parametric_equation_token);
  ArrayCopy(NewTokens, Tokenizer->Tokens, Tokenizer->TokenCount);
  Tokenizer->Tokens = NewTokens;
  Tokenizer->MaxTokenCount = NewMaxTokenCount;
 }
 Assert(Tokenizer->TokenCount < Tokenizer->MaxTokenCount);
 
 parametric_equation_token *Token = Tokenizer->Tokens + Tokenizer->TokenCount;
 ++Tokenizer->TokenCount;
 Token->Type = Type;
 
 return Token;
}

internal void
ParametricEquation_Tokenizer_Advance(parametric_equation_tokenizer *Tokenizer)
{
 Assert(Tokenizer->Left);
 --Tokenizer->Left;
 ++Tokenizer->At;
}

internal b32
ParametricEquation_Tokenizer_ParseIdentifier(parametric_equation_tokenizer *Tokenizer, string *OutIdentifier)
{
 char *SaveAt = Tokenizer->At;
 
 b32 Parsing = true;
 b32 FirstChar = true;
 do {
  char C = ParametricEquation_Tokenizer_NextChar(Tokenizer);
  
  b32 AlwaysPossible = (CharIsAlpha(C) || C == '_');
  b32 OnlyNotAtTheBeginning = (CharIsDigit(C) || C == '\'');
  b32 IsEligibleForIdentifier = (AlwaysPossible || (!FirstChar && OnlyNotAtTheBeginning));
  if (IsEligibleForIdentifier)
  {
   ParametricEquation_Tokenizer_Advance(Tokenizer);
  }
  else
  {
   Parsing = false;
  }
  
  FirstChar = false;
  
 } while (Parsing);
 
 b32 Parsed = false;
 if (Tokenizer->At != SaveAt)
 {
  Parsed = true;
  *OutIdentifier = MakeStr(SaveAt, Tokenizer->At - SaveAt);
 }
 
 return Parsed;
}

internal b32
ParametricEquation_Tokenizer_ParseInteger(parametric_equation_tokenizer *Tokenizer, u64 *OutInteger)
{
 b32 Parsed = false;
 u64 N = 0;
 
 b32 Parsing = true;
 b32 FirstChar = true;
 do {
  char C = ParametricEquation_Tokenizer_NextChar(Tokenizer);
  
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
   ParametricEquation_Tokenizer_Advance(Tokenizer);
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
ParametricEquation_Tokenizer_MaybeEatChar(parametric_equation_tokenizer *Tokenizer, char C)
{
 char Actual = ParametricEquation_Tokenizer_NextChar(Tokenizer);
 if (Actual == C)
 {
  ParametricEquation_Tokenizer_Advance(Tokenizer);
 }
}

internal b32
ParametricEquation_Tokenizer_ParseNumber(parametric_equation_tokenizer *Tokenizer, f32 *OutNumber)
{
 u64 PreDot = 0;
 u64 PostDot = 0;
 
 b32 PreDotParsed = ParametricEquation_Tokenizer_ParseInteger(Tokenizer, &PreDot);
 ParametricEquation_Tokenizer_MaybeEatChar(Tokenizer, '.');
 b32 PostDotParsed = ParametricEquation_Tokenizer_ParseInteger(Tokenizer, &PostDot);
 
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

internal parametric_equation_tokenize_result
TokenizeParametricEquation(arena *Arena, string Code)
{
 parametric_equation_tokenizer _Tokenizer = {};
 parametric_equation_tokenizer *Tokenizer = &_Tokenizer;
 Tokenizer->MaxTokenCount = SafeCastU32(Code.Count + 1);
 Tokenizer->Tokens = PushArrayNonZero(Arena, Tokenizer->MaxTokenCount, parametric_equation_token);
 Tokenizer->At = Code.Data;
 Tokenizer->Left = Code.Count;
 Tokenizer->Arena = Arena;
 
 char const *ErrorMsg = 0;
 
 b32 Parsing = true;
 do {
  char C = ParametricEquation_Tokenizer_NextChar(Tokenizer);
  if (CharIsWhiteSpace(C))
  {
   ParametricEquation_Tokenizer_Advance(Tokenizer);
  }
  else
  {
   switch (C)
   {
    case '\0': {
     Parsing = false;
    }break;
    
    case '-': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Minus);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '+': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Plus);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '/': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Slash);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '*': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Asterisk);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '^': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Caret);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '(': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_OpenParen);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case ')': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_CloseParen);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '{': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_OpenBrace);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case '}': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_CloseBrace);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case ',': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Comma);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    case ':': {
     ParametricEquation_Tokenizer_Advance(Tokenizer);
     if (ParametricEquation_Tokenizer_NextChar(Tokenizer) == '=')
     {
      ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Definition);
      ParametricEquation_Tokenizer_Advance(Tokenizer);
     }
     else
     {
      ErrorMsg = "TODO";
      Parsing = false;
     }
    }break;
    
    case ';': {
     ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Semicolon);
     ParametricEquation_Tokenizer_Advance(Tokenizer);
    }break;
    
    default: {
     string Identifier = {};
     f32 Number = 0;
     if (ParametricEquation_Tokenizer_ParseIdentifier(Tokenizer, &Identifier))
     {
      parametric_equation_token *Token = ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Identifier);
      Token->Identifier = Identifier;
     }
     else if (ParametricEquation_Tokenizer_ParseNumber(Tokenizer, &Number))
     {
      parametric_equation_token *Token = ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_Number);
      Token->Number = Number;
     }
     else
     {
      ErrorMsg = "TODO";
      Parsing = false;
     }
    }break;
   }
  }
 } while (Parsing);
 
 ParametricEquation_Tokenizer_AddToken(Tokenizer, ParametricEquationToken_None);
 
 parametric_equation_tokenize_result Result = {};
 Result.ErrorMsg = ErrorMsg;
 Result.Tokens = Tokenizer->Tokens;
 
 return Result;
}

internal b32
ParametricEquation_Env_Lookup(parametric_equation_env *Env,
                              string Identifier,
                              parametric_equation_env_value *OutValue)
{
 b32 Result = false;
 for (u32 Index = 0;
      Index < Env->Count;
      ++Index)
 {
  string Key = Env->Keys[Index];
  if (StrEqual(Identifier, Key))
  {
   Result = true;
   *OutValue = Env->Values[Index];
   break;
  }
 }
 
 return Result;
}

internal f32
ParametricEquation_Env_LookupNumber(parametric_equation_env *Env,
                                    string Identifier)
{
 parametric_equation_env_value Value = {};
 b32 Found = ParametricEquation_Env_Lookup(Env, Identifier, &Value);
 Assert(Found);
 Assert(Value.Type == ParametricEquationEnvValue_Number);
 return Value.Number;
}

internal parametric_equation_function *
ParametricEquation_Env_LookupFunction(parametric_equation_env *Env,
                                      string Identifier)
{
 parametric_equation_env_value Value = {};
 b32 Found = ParametricEquation_Env_Lookup(Env, Identifier, &Value);
 Assert(Found);
 Assert(Value.Type == ParametricEquationEnvValue_Function);
 return Value.Function;
}

internal parametric_equation_env
ParametricEquation_Env_CopyWithCapacity(arena *Arena, u32 NewCapacity, parametric_equation_env *From)
{
 Assert(From->Count <= NewCapacity);
 
 string *NewKeys = PushArrayNonZero(Arena, NewCapacity, string);
 parametric_equation_env_value *NewValues = PushArrayNonZero(Arena, NewCapacity, parametric_equation_env_value);
 
 ArrayCopy(NewKeys, From->Keys, From->Count);
 ArrayCopy(NewValues, From->Values, From->Count);
 
 parametric_equation_env Result = {};
 Result.Keys = NewKeys;
 Result.Values = NewValues;
 Result.Capacity = NewCapacity;
 Result.Count = From->Count;
 
 return Result;
}

internal parametric_equation_env_value *
ParametricEquation_Env_Add(arena *Arena, parametric_equation_env *Env, string Identifier)
{
 if (Env->Count == Env->Capacity)
 {
  u32 NewCapacity = Max(8, 2 * Env->Capacity);
  *Env = ParametricEquation_Env_CopyWithCapacity(Arena, NewCapacity, Env);
 }
 Assert(Env->Count < Env->Capacity);
 
 Env->Keys[Env->Count] = Identifier;
 
 parametric_equation_env_value *Result = Env->Values + Env->Count;
 ++Env->Count;
 
 return Result;
}

internal parametric_equation_env
ParametricEquation_Env_Copy(arena *Arena, parametric_equation_env *From)
{
 parametric_equation_env Result = ParametricEquation_Env_CopyWithCapacity(Arena, From->Capacity, From);
 return Result;
}

internal void
ParametricEquation_AddExpr(arena *Arena, parametric_equation_expr_array *Array, parametric_equation_expr Expr)
{
 if (Array->Count == Array->MaxCount)
 {
  u32 NewMaxCount = Max(8, 2 * Array->MaxCount);
  parametric_equation_expr *NewExprs = PushArrayNonZero(Arena, NewMaxCount, parametric_equation_expr);
  ArrayCopy(NewExprs, Array->Exprs, Array->Count);
  Array->Exprs = NewExprs;
  Array->MaxCount = NewMaxCount;
 }
 Assert(Array->Count < Array->MaxCount);
 
 Array->Exprs[Array->Count] = Expr;
 ++Array->Count;
}

internal parametric_equation_expr *ParametricEquation_ParseExpr(arena *Arena, parametric_equation_parser *Parser);

internal parametric_equation_identifier
MakeSpecialIdentifier(parametric_equation_identifier_type Special)
{
 parametric_equation_identifier Result = {};
 Result.Type = Special;
 
 return Result;
}

internal parametric_equation_identifier
MakeCustomIdentifier(string Custom)
{
 parametric_equation_identifier Result = {};
 Result.Type = ParametricEquationIdentifier_Custom;
 Result.Custom = Custom;
 
 return Result;
}

internal parametric_equation_token
ParametricEquation_Parser_PeekToken(parametric_equation_parser *Parser)
{
 parametric_equation_token Result = Parser->At[0];
 return Result;
}

internal void
ParametricEquation_Parser_AdvanceToken(parametric_equation_parser *Parser)
{
 ++Parser->At;
}

internal b32
ParametricEquation_Parser_CheckAndEat(parametric_equation_parser *Parser, parametric_equation_token_type Type)
{
 b32 Eaten = false;
 parametric_equation_token Token = ParametricEquation_Parser_PeekToken(Parser);
 if (Token.Type == Type)
 {
  ParametricEquation_Parser_AdvanceToken(Parser);
  Eaten = true;
 }
 
 return Eaten;
}

internal b32
ParametricEquation_Parser_IsError(parametric_equation_parser *Parser)
{
 b32 Result = (Parser->ErrorMsg != 0);
 return Result;
}

internal void
ParametricEquation_Parser_SetError(parametric_equation_parser *Parser, char const *Error)
{
 Parser->ErrorMsg = Error;
}

internal parametric_equation_op_type
ParametricEquation_TokenToOpType(parametric_equation_token Token)
{
 parametric_equation_op_type Op = ParametricEquationOp_None;
 switch (Token.Type)
 {
  case ParametricEquationToken_Caret: {Op = ParametricEquationOp_Pow;}break;
  case ParametricEquationToken_Minus: {Op = ParametricEquationOp_Minus;}break;
  case ParametricEquationToken_Plus: {Op = ParametricEquationOp_Plus;}break;
  case ParametricEquationToken_Asterisk: {Op = ParametricEquationOp_Mult;}break;
  case ParametricEquationToken_Slash: {Op = ParametricEquationOp_Div;}break;
  
  default: InvalidPath;
 }
 
 return Op;
}

internal parametric_equation_expr *
ParametricEquation_ParseOneExpr(arena *Arena, parametric_equation_parser *Parser)
{
 parametric_equation_expr *MainExpr = 0;
 
 parametric_equation_op_type UnaryOp = ParametricEquationOp_None;
 {
  parametric_equation_token Token = ParametricEquation_Parser_PeekToken(Parser);
  if (Token.Type == ParametricEquationToken_Plus ||
      Token.Type == ParametricEquationToken_Minus)
  {
   ParametricEquation_Parser_AdvanceToken(Parser);
   UnaryOp = ParametricEquation_TokenToOpType(Token);
  }
 }
 
 parametric_equation_token Token = ParametricEquation_Parser_PeekToken(Parser);
 if (Token.Type == ParametricEquationToken_Number)
 {
  ParametricEquation_Parser_AdvanceToken(Parser);
  
  MainExpr = PushStruct(Arena, parametric_equation_expr);
  MainExpr->Type = ParametricEquationExpr_Number;
  MainExpr->Number.Number = Token.Number;
 }
 else if (Token.Type == ParametricEquationToken_Identifier)
 {
  ParametricEquation_Parser_AdvanceToken(Parser);
  
  u32 ApplyArgCount = 0;
  parametric_equation_expr *ApplyArgs = 0;
  if (ParametricEquation_Parser_CheckAndEat(Parser, ParametricEquationToken_OpenParen))
  {
   b32 Parsing = true;
   parametric_equation_expr_array ArgsArray = {};
   do {
    parametric_equation_token Token = ParametricEquation_Parser_PeekToken(Parser);
    if (Token.Type == ParametricEquationToken_CloseParen)
    {
     Parsing = false;
    }
    else
    {
     parametric_equation_expr *ArgExpr = ParametricEquation_ParseExpr(Arena, Parser);
     if (ParametricEquation_Parser_IsError(Parser))
     {
      Parsing = false;
     }
     else
     {
      ParametricEquation_AddExpr(Arena, &ArgsArray, *ArgExpr);
      if (!ParametricEquation_Parser_CheckAndEat(Parser, ParametricEquationToken_Comma))
      {
       Parsing = false;
      }
     }
    }
   } while (Parsing);
   
   if (ParametricEquation_Parser_CheckAndEat(Parser, ParametricEquationToken_CloseParen))
   {
    ApplyArgCount = ArgsArray.Count;
    ApplyArgs = ArgsArray.Exprs;
   }
   else
   {
    ParametricEquation_Parser_SetError(Parser, "TODO");
   }
  }
  
  string BaseIdentifier = Token.Identifier;
  parametric_equation_identifier Identifier = {};
  if (StrMatch(BaseIdentifier, StrLit("Sin"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Sin);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Cos"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Cos);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Tan"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Tan);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Sqrt"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Sqrt);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Log"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Log);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Log10"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Log10);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Floor"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Floor);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Ceil"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Ceil);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Round"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Round);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Pow"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Pow);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Tanh"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Tanh);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Exp"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Exp);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Pi"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Pi);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Tau"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Tau);
  }
  else if (StrMatch(BaseIdentifier, StrLit("Euler"), true))
  {
   Identifier = MakeSpecialIdentifier(ParametricEquationIdentifier_Euler);
  }
  else
  {
   Identifier = MakeCustomIdentifier(BaseIdentifier);
  }
  
  if (!ParametricEquation_Parser_IsError(Parser))
  {
   MainExpr = PushStruct(Arena, parametric_equation_expr);
   MainExpr->Type = ParametricEquationExpr_Application;
   MainExpr->Application.Identifier = Identifier;
   MainExpr->Application.ArgCount = ApplyArgCount;
   MainExpr->Application.ArgExprs = ApplyArgs;
  }
 }
 else if (Token.Type == ParametricEquationToken_OpenParen)
 {
  ParametricEquation_Parser_AdvanceToken(Parser);
  MainExpr = ParametricEquation_ParseExpr(Arena, Parser);
  if (!ParametricEquation_Parser_CheckAndEat(Parser, ParametricEquationToken_CloseParen))
  {
   ParametricEquation_Parser_SetError(Parser, "TODO");
  }
 }
 else
 {
  ParametricEquation_Parser_SetError(Parser, "TODO");
 }
 
 parametric_equation_expr *Result = 0;
 // NOTE(hbr): Plus doesn't do anything computationally
 if (UnaryOp == ParametricEquationOp_Minus)
 {
  Result = PushStruct(Arena, parametric_equation_expr);
  Result->Type = ParametricEquationExpr_Negation;
  Result->Negation.SubExpr = MainExpr;
 }
 else
 {
  Assert(UnaryOp == ParametricEquationOp_Plus ||
         UnaryOp == ParametricEquationOp_None);
  Result = MainExpr;
 }
 
 return Result;
}

internal parametric_equation_expr *
ParametricEquation_ParseExpr(arena *Arena, parametric_equation_parser *Parser)
{
 parametric_equation_expr *Root = 0;
 parametric_equation_expr *LastExpr = 0;
 parametric_equation_expr *LastNode = 0;
 parametric_equation_op_type LastOp = ParametricEquationOp_None;
 
 b32 Parsing = true;
 do {
  parametric_equation_expr *ThisExpr = ParametricEquation_ParseOneExpr(Arena, Parser);
  if (!ParametricEquation_Parser_IsError(Parser))
  {
   if (LastExpr != 0)
   {
    parametric_equation_expr *Node = PushStruct(Arena, parametric_equation_expr);
    
    Node->Type = ParametricEquationExpr_BinaryOp;
    Node->Binary.Type = LastOp;
    Node->Binary.Left = LastExpr;
    Node->Binary.Right = ThisExpr;
    Node->Binary.Parent = LastNode;
    if (LastNode)
    {
     LastNode->Binary.Right = Node;
    }
    
    LastNode = Node;
    if (Root == 0)
    {
     Root = Node;
    }
   }
   LastExpr = ThisExpr;
   
   parametric_equation_token Token = ParametricEquation_Parser_PeekToken(Parser);
   if (Token.Type == ParametricEquationToken_Asterisk ||
       Token.Type == ParametricEquationToken_Minus ||
       Token.Type == ParametricEquationToken_Plus ||
       Token.Type == ParametricEquationToken_Slash ||
       Token.Type == ParametricEquationToken_Caret)
   {
    ParametricEquation_Parser_AdvanceToken(Parser);
    LastOp = ParametricEquation_TokenToOpType(Token);
   }
   else
   {
    Parsing = false;
   }
  }
  else
  {
   Parsing = false;
  }
 } while (Parsing);
 
 if (Root == 0)
 {
  Root = LastExpr;
 }
 if (Root == 0)
 {
  ParametricEquation_Parser_SetError(Parser, "TODO");
 }
 
 return Root;
}

internal void
ParametricEquation_TypeChecker_SetError(parametric_equation_type_checker *TypeChecker,
                                        char const *ErrorMsg)
{
 TypeChecker->ErrorMsg = ErrorMsg;
}

internal void
TypeCheckParametricExpression(parametric_equation_type_checker *TypeChecker,
                              parametric_equation_expr *Expr)
{
 switch (Expr->Type)
 {
  case ParametricEquationExpr_None: {}break;
  
  case ParametricEquationExpr_Negation: {
   TypeCheckParametricExpression(TypeChecker, Expr->Negation.SubExpr);
  }break;
  
  case ParametricEquationExpr_BinaryOp: {
   parametric_equation_expr_binary_op Binary = Expr->Binary;
   TypeCheckParametricExpression(TypeChecker, Binary.Left);
   TypeCheckParametricExpression(TypeChecker, Binary.Right);
  }break;
  
  case ParametricEquationExpr_Number: {}break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_expr_application Application = Expr->Application;
   parametric_equation_identifier Identifier = Application.Identifier;
   
   switch (Identifier.Type)
   {
    case ParametricEquationIdentifier_Sin:
    case ParametricEquationIdentifier_Cos:
    case ParametricEquationIdentifier_Tan:
    case ParametricEquationIdentifier_Sqrt:
    case ParametricEquationIdentifier_Log:
    case ParametricEquationIdentifier_Log10:
    case ParametricEquationIdentifier_Floor:
    case ParametricEquationIdentifier_Ceil:
    case ParametricEquationIdentifier_Round:
    case ParametricEquationIdentifier_Tanh:
    case ParametricEquationIdentifier_Exp: {
     if (Application.ArgCount != 1) {
      ParametricEquation_TypeChecker_SetError(TypeChecker, "expected 1 argument");
     }
    }break;
    
    case ParametricEquationIdentifier_Pow: {
     if (Application.ArgCount != 2) {
      ParametricEquation_TypeChecker_SetError(TypeChecker, "expected 2 arguments");
     }
    }break;
    
    case ParametricEquationIdentifier_Pi:
    case ParametricEquationIdentifier_Tau: 
    case ParametricEquationIdentifier_Euler: {
     if (Application.ArgCount != 0) {
      ParametricEquation_TypeChecker_SetError(TypeChecker, "no arguments expected");
     }
    }break;
    
    case ParametricEquationIdentifier_Custom: {
     string Custom = Identifier.Custom;
     // TODO(hbr): I should actually check if this is in the environment, not just t
     if (!StrEqual(StrLit("t"), Custom)) {
      ParametricEquation_TypeChecker_SetError(TypeChecker, "only 't' free variable is allowed");
     }
     if (Application.ArgCount != 0)
     {
      ParametricEquation_TypeChecker_SetError(TypeChecker, "no arguments expected");
     }
    }break;
   }
  }break;
 }
}

internal parametric_equation_parse_result
ParametricEquationParse(arena *Arena, string Equation)
{
 parametric_equation_parse_result Result = {};
 
 // NOTE(hbr): Copy the equation because tokens/parse tree point to that original text
 Equation = StrCopy(Arena, Equation);
 
 
 parametric_equation_tokenize_result Tokenize = TokenizeParametricEquation(Arena, Equation);
 if (!Tokenize.ErrorMsg)
 {
  parametric_equation_parser Parser = {};
  Parser.At = Tokenize.Tokens;
  
  parametric_equation_expr *Parsed = ParametricEquation_ParseExpr(Arena, &Parser);
  if (!ParametricEquation_Parser_CheckAndEat(&Parser, ParametricEquationToken_None))
  {
   ParametricEquation_Parser_SetError(&Parser, "TODO");
  }
  
  if (!Parser.ErrorMsg)
  {
   parametric_equation_type_checker TypeChecker = {};
   TypeCheckParametricExpression(&TypeChecker, Parsed);
   
   if (!TypeChecker.ErrorMsg)
   {
    Result.ParsedExpr = Parsed;
   }
   else
   {
    Result.ErrorMsg = TypeChecker.ErrorMsg;
   }
  }
  else
  {
   Result.ErrorMsg = Parser.ErrorMsg;
  }
 }
 else
 {
  Result.ErrorMsg = Tokenize.ErrorMsg;
 }
 
 return Result;
}

internal f32
ParametricEquation_Env_LookupAndAssert(parametric_equation_env2 *Env, string Key)
{
 f32 Result = 0.0f;
 b32 Found = false;
 for (u32 Index = 0;
      Index < Env->Count;
      ++Index)
 {
  if (StrEqual(Key, Env->Keys[Index]))
  {
   Result = Env->Values[Index];
   Found = true;
   break;
  }
 }
 Assert(Found);
 
 return Result;
}

internal f32
ParametricEquation_EvaluateExpr(parametric_equation_expr *Expr, parametric_equation_env2 *Env)
{
 f32 Result = 0;
 switch (Expr->Type)
 {
  case ParametricEquationExpr_None: {}break;
  
  case ParametricEquationExpr_Negation: {
   parametric_equation_expr_negation Negation = Expr->Negation;
   f32 Sub = ParametricEquation_EvaluateExpr(Negation.SubExpr, Env);
   Result = -Sub;
  }break;
  
  case ParametricEquationExpr_BinaryOp: {
   parametric_equation_expr_binary_op Binary = Expr->Binary;
   
   f32 Left = ParametricEquation_EvaluateExpr(Binary.Left, Env);
   f32 Right = ParametricEquation_EvaluateExpr(Binary.Right, Env);
   
   switch (Binary.Type)
   {
    case ParametricEquationOp_Plus: {
     Result = Left + Right;
    }break;
    
    case ParametricEquationOp_Minus: {
     Result = Left - Right;
    }break;
    
    case ParametricEquationOp_Mult: {
     Result = Left * Right;
    }break;
    
    case ParametricEquationOp_Div: {
     Result = SafeDiv0(Left, Right);
    }break;
    
    case ParametricEquationOp_Pow: {
     Result = PowF32(Left, Right);
    }break;
   }
  }break;
  
  case ParametricEquationExpr_Number: {
   Result = Expr->Number.Number;
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_expr_application Application = Expr->Application;
   switch (Application.Identifier.Type)
   {
    case ParametricEquationIdentifier_Sin: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = SinF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Cos: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = CosF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Tan: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = TanF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Sqrt: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = SqrtF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Log: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = LogF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Log10: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = Log10F32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Floor: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = FloorF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Ceil: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = CeilF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Round: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = RoundF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Tanh: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = TanhF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Exp: {
     f32 Arg = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     Result = ExpF32(Arg);
    }break;
    
    case ParametricEquationIdentifier_Pow: {
     f32 Base = ParametricEquation_EvaluateExpr(Application.ArgExprs + 0, Env);
     f32 Exp = ParametricEquation_EvaluateExpr(Application.ArgExprs + 1, Env);
     Result = PowF32(Base, Exp);
    }break;
    
    case ParametricEquationIdentifier_Pi: {
     Result = Pi32;
    }break;
    
    case ParametricEquationIdentifier_Tau: {
     Result = Tau32;
    }break;
    
    case ParametricEquationIdentifier_Euler: {
     Result = Euler32;
    }break;
    
    case ParametricEquationIdentifier_Custom: {
     if (Application.ArgCount == 0)
     {
      f32 Value = ParametricEquation_Env_LookupAndAssert(Env, Application.Identifier.Custom);
      Result = Value;
     }
     else
     {
      NotImplemented;
     }
    }break;
   }
  }break;
 }
 
 return Result;
}

internal void
ParametricEquation_Env_Add(parametric_equation_env2 *Env, string Key, f32 Value)
{
 Assert(Env->Count < MAX_ENV_VARIABLE_COUNT);
 Env->Keys[Env->Count] = Key;
 Env->Values[Env->Count] = Value;
 ++Env->Count;
}

internal f32
ParametricEquationEvalWithT(parametric_equation_expr Expr, f32 T)
{
 parametric_equation_env2 Env = {};
 ParametricEquation_Env_Add(&Env, StrLit("t"), T);
 f32 Result = ParametricEquation_EvaluateExpr(&Expr, &Env);
 return Result;
}

internal parametric_equation_eval_result
ParametricEquationEval(arena *Arena, string Equation)
{
 parametric_equation_eval_result Result = {};
 
 parametric_equation_parse_result Parse = ParametricEquationParse(Arena, Equation);
 if (!Parse.ErrorMsg)
 {
  parametric_equation_env2 Env = {};
  f32 Value = ParametricEquation_EvaluateExpr(Parse.ParsedExpr, &Env);
  Result.Value = Value;
 }
 else
 {
  Result.ErrorMsg = Parse.ErrorMsg;
 }
 
 return Result;
}