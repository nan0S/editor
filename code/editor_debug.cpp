internal void
UI_ParametricEquationExpr(parametric_equation_expr *Expr, u32 *Id)
{
 temp_arena Temp = TempArena(0);
 
 UI_PushId(*Id);
 *Id += 1;
 
 string TypeStr = {};
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   TypeStr = StrLit("Unary");
  }break;
  
  case ParametricEquationExpr_Binary: {
   char const *OpStr = "null";
   switch (Expr->Binary.Operator)
   {
    case ParametricEquationBinaryOp_Pow: {OpStr = "^";}break;
    case ParametricEquationBinaryOp_Plus: {OpStr = "+";}break;
    case ParametricEquationBinaryOp_Minus: {OpStr = "-";}break;
    case ParametricEquationBinaryOp_Mult: {OpStr = "*";}break;
    case ParametricEquationBinaryOp_Div: {OpStr = "/";}break;
   }
   TypeStr = StrF(Temp.Arena, "BinaryOp(%s)", OpStr);
  }break;
  
  case ParametricEquationExpr_Number: {
   TypeStr = StrF(Temp.Arena, "Number(%f)", Expr->Number.Number);
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   
   string IdentifierName = {};
   if (Application.Identifier.Type == ParametricEquationIdentifier_var)
   {
    IdentifierName = Application.Identifier.Var;
   }
   else
   {
    IdentifierName = StrFromCStr(ParametricEquationIdentifierNames[Application.Identifier.Type]);
   }
   
   TypeStr = StrF(Temp.Arena, "Application(%S)", IdentifierName);
  }break;
 }
 UI_Text(false, TypeStr);
 
 switch (Expr->Type)
 {
  case ParametricEquationExpr_Unary: {
   parametric_equation_unary_expr Unary = Expr->Unary;
   
   UI_SetNextItemOpen(true, UICond_Once);
   if (UI_BeginTreeF("Sub"))
   {
    UI_ParametricEquationExpr(Unary.SubExpr, Id);
    UI_EndTree();
   }
  }break;
  
  case ParametricEquationExpr_Binary: {
   parametric_equation_binary_expr Binary = Expr->Binary;
   
   UI_SetNextItemOpen(true, UICond_Once);
   if (UI_BeginTreeF("Left"))
   {
    UI_ParametricEquationExpr(Binary.Left, Id);
    UI_EndTree();
   }
   
   UI_SetNextItemOpen(true, UICond_Once);
   if (UI_BeginTreeF("Right"))
   {
    UI_ParametricEquationExpr(Binary.Right, Id);
    UI_EndTree();
   }
  }break;
  
  case ParametricEquationExpr_Number: {}break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   
   u32 ArgIndex = 0;
   for (u32 ArgIndex = 0;
        ArgIndex < Application.ArgCount;
        ++ArgIndex)
   {
    parametric_equation_expr *Arg = Application.Args[ArgIndex];
    
    UI_SetNextItemOpen(true, UICond_Once);
    if (UI_BeginTreeF("%u", ArgIndex))
    {
     UI_ParametricEquationExpr(Arg, Id);
     UI_EndTree();
    }
   }
  }break;
 }
 
 UI_PopId();
 
 EndTemp(Temp);
}

internal void
UI_ParametricEquationExpr(parametric_equation_expr *Expr, string Label)
{
 UI_PushLabel(Label);
 u32 Id = 0;
 UI_ParametricEquationExpr(Expr, &Id);
 UI_PopLabel();
}