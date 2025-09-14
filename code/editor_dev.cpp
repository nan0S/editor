internal void
RenderDevConsoleUI(editor *Editor)
{
 camera *Camera = &Editor->Camera;
 
 if (DEBUG_Vars->DevConsole)
 {
  if (UI_BeginWindow(&DEBUG_Vars->DevConsole, 0, StrLit("Dev Console")))
  {
   if (UI_Button(StrLit("Add Notification")))
   {
    AddNotificationF(Editor, Notification_Error,
                     "This is a dev notification\nblablablablablablablabla"
                     "blablablabl ablablablablabla blabl abla blablablablab lablablabla"
                     "bla bla blablabl ablablablablab lablablablablablablablablablabla"
                     "blabla blab lablablablab lablabla blablablablablablablablablabla");
   }
   UI_Checkbox(&DEBUG_Vars->ParametricEquationDebugMode, StrLit("Parametric Equation Debug Mode Enabled"));
   UI_Checkbox(&DEBUG_Vars->ShowSampleCurvePoints, StrLit("Show Sample Curve Points"));
   UI_ExponentialAnimation(&Camera->Animation);
   UI_TextF(false, "DrawnGridLinesCount=%u", DEBUG_Vars->DrawnGridLinesCount);
   UI_TextF(false, "String Store String Count: %u\n", GetCtx()->StrStore->StrCount);
   UI_TextF(false, "TransformAction: %p", Editor->SelectedEntityTransformState.TransformAction);
   
   UI_SliderUnsigned(&DEBUG_Vars->MultiThreadedEvaluationBlockSize, 1, 10000, StrLit("MultiThreaded Evaluation Block Size"));
   
   UI_Checkbox(&DEBUG_Vars->NURBS_Benchmark, StrLit("NURBS Benchmark"));
   UI_Combo(SafeCastToPtr(DEBUG_Vars->NURBS_EvalMethod, u32), NURBS_Eval_Count, NURBS_Eval_Names, StrLit("NURBS Eval Method"));
   
   UI_Checkbox(&DEBUG_Vars->Parametric_Benchmark, StrLit("Parametric Benchmark"));
   UI_Combo(SafeCastToPtr(DEBUG_Vars->Parametric_EvalMethod, u32), Parametric_Eval_Count, Parametric_Eval_Names, StrLit("Parametric Eval Method"));
   
   UI_Checkbox(&DEBUG_Vars->Bezier_Benchmark, StrLit("Bezier Benchmark"));
   UI_Combo(SafeCastToPtr(DEBUG_Vars->Bezier_EvalMethod, u32), Bezier_Eval_Count, Bezier_Eval_Names, StrLit("Bezier Eval Method"));
   
   UI_Checkbox(&DEBUG_Vars->CubicSpline_Benchmark, StrLit("Cubic Spline Benchmark"));
   UI_Combo(SafeCastToPtr(DEBUG_Vars->CubicSpline_EvalMethod, u32), CubicSpline_Eval_Count, CubicSpline_Eval_Names, StrLit("Cubic Spline Eval Method"));
   
   UI_Combo(SafeCastToPtr(DEBUG_Vars->CubicSplinePeriodicM_EvalMethod, u32),
            CubicSplinePeriodicM_Eval_Count,
            CubicSplinePeriodicM_Eval_Names,
            StrLit("Cubic Spline Periodic M Eval Method"));
  }
  UI_EndWindow();
 }
}

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
    case ParametricEquationBinaryOp_Mod: {OpStr = "%";}break;
   }
   TypeStr = StrF(Temp.Arena, "BinaryOp(%s)", OpStr);
  }break;
  
  case ParametricEquationExpr_Number: {
   TypeStr = StrF(Temp.Arena, "Number(%f)", Expr->Number.Number);
  }break;
  
  case ParametricEquationExpr_Application: {
   parametric_equation_application_expr Application = Expr->Application;
   string IdentifierName = ParametricEquationIdentifierName(Application.Identifier);
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

internal void
UI_ExponentialAnimation(exponential_animation *Anim)
{
 UI_SliderFloat(&Anim->PowBase, 0.0f, 10.0f, StrLit("PowBase"));
 UI_SliderFloat(&Anim->ExponentMult, 0.0f, 10.0f, StrLit("ExponentMult"));
}

internal void
AppendMultipleControlPoints(editor *Editor, entity_with_modify_witness *Witness, u32 PointsPerSide)
{
 f32 Side = 1.0f;
 f32 Delta = Side / PointsPerSide;
 v2 Center = 0.5f * V2(Side, Side);
 
 for (u32 I = 1; I <= PointsPerSide; ++I)
 {
  v2 P = V2(I * Delta, 0.0f) - Center;
  AppendControlPoint(Editor, Witness, P);
 }
 
 for (u32 I = 1; I <= PointsPerSide; ++I)
 {
  v2 P = V2(Side, I * Delta) - Center;
  AppendControlPoint(Editor, Witness, P);
 }
 
 for (u32 I = 1; I <= PointsPerSide; ++I)
 {
  v2 P = V2(Side - I * Delta, Side) - Center;
  AppendControlPoint(Editor, Witness, P);
 }
 
 for (u32 I = 1; I <= PointsPerSide; ++I)
 {
  v2 P = V2(0, Side - I * Delta) - Center;
  AppendControlPoint(Editor, Witness, P);
 }
}

internal void
DevUpdateAndRender(editor *Editor)
{
 DEBUG_Vars = &Editor->DEBUG_Vars;
 
 if (!DEBUG_Vars->Initialized)
 {
  
#if BUILD_DEV
  // NOTE(hbr): Create NURBS curve benchmark
  {  
   entity *Entity = AddEntity(Editor);
   curve_params BSplineCurveParams = DefaultCurveParams();
   BSplineCurveParams.TotalSamples = 50000;
   BSplineCurveParams.Type = Curve_NURBS;
#if BUILD_DEBUG
   BSplineCurveParams.BSpline.KnotParams.Degree = 15;
#else
   BSplineCurveParams.BSpline.KnotParams.Degree = 26;
#endif
   InitEntityAsCurve(Entity, StrLit("NURBS Benchmark"), BSplineCurveParams);
   entity_with_modify_witness Witness = BeginEntityModify(Entity);
   AppendMultipleControlPoints(Editor, &Witness, 100);
   EndEntityModify(Witness);
   
   DEBUG_Vars->NURBS_BenchmarkEntity = Entity;
  }
#endif
  
#if BUILD_DEV
  // NOTE(hbr): Create Parametric curve benchmark
  {
   entity *Entity = AddEntity(Editor);
   
   curve_params ParametricCurveParams = DefaultCurveParams();
   ParametricCurveParams.Type = Curve_Parametric;
   InitEntityAsCurve(Entity, StrLit("Parametric Benchmark"), ParametricCurveParams);
   
   curve *Curve = SafeGetCurve(Entity);
   entity_with_modify_witness Witness = BeginEntityModify(Entity);
   LoadParametricCurvePredefinedExample(Curve, ParametricCurvePredefinedExample_ButterflyCurve);
   MarkEntityModified(&Witness);
   EndEntityModify(Witness);
   
   DEBUG_Vars->Parametric_BenchmarkEntity = Entity;
  }
#endif
  
#if BUILD_DEV
  // NOTE(hbr): Create Bezier curve benchmark
  {
   entity *Entity = AddEntity(Editor);
   
   curve_params BezierCurveParams = DefaultCurveParams();
   BezierCurveParams.Type = Curve_Bezier;
   BezierCurveParams.Bezier = Bezier_Rational;
   
   InitEntityAsCurve(Entity, StrLit("Bezier Benchmark"), BezierCurveParams);
   entity_with_modify_witness Witness = BeginEntityModify(Entity);
   AppendMultipleControlPoints(Editor, &Witness, 100);
   EndEntityModify(Witness);
   
   DEBUG_Vars->Bezier_BenchmarkEntity = Entity;
  }
#endif
  
#if BUILD_DEV
  // NOTE(hbr): Create Cubic Spline curve benchmark
  {
   entity *Entity = AddEntity(Editor);
   
   curve_params CubicSplineCurveParams = DefaultCurveParams();
   CubicSplineCurveParams.Type = Curve_CubicSpline;
   CubicSplineCurveParams.CubicSpline = CubicSpline_Natural;
   
   InitEntityAsCurve(Entity, StrLit("Cubic Spline Benchmark"), CubicSplineCurveParams);
   entity_with_modify_witness Witness = BeginEntityModify(Entity);
   AppendMultipleControlPoints(Editor, &Witness, 500);
   EndEntityModify(Witness);
   
   DEBUG_Vars->CubicSpline_BenchmarkEntity = Entity;
  }
#endif
  
#if BUILD_DEV
  DEBUG_Vars->ParametricCurveMaxTotalSamples = 50000;
  DEBUG_Vars->DevConsole = true;
  Editor->Profiler.ReferenceMs = 1000.0f / 30.0f;
  Editor->ProfilerWindow = true;
  Editor->DiagnosticsWindow = true;
#else
  DEBUG_Vars->ParametricCurveMaxTotalSamples = 5000;
#endif
  
  DEBUG_Vars->NURBS_EvalMethod = NURBS_Eval_Adaptive_MultiThreaded;
  DEBUG_Vars->Parametric_EvalMethod = Parametric_Eval_MultiThreaded;
  DEBUG_Vars->Bezier_EvalMethod = Bezier_Eval_Adaptive_MultiThreaded;
  DEBUG_Vars->CubicSpline_EvalMethod = CubicSpline_Eval_ScalarWithBinarySearch_MultiThreaded;
  DEBUG_Vars->CubicSplinePeriodicM_EvalMethod = CubicSplinePeriodicM_Eval_Base;
  DEBUG_Vars->MultiThreadedEvaluationBlockSize = 1024;
  
  DEBUG_Vars->Initialized = true;
 }
 
 if (DEBUG_Vars->NURBS_BenchmarkEntity)
 {
  SetEntityVisibility(DEBUG_Vars->NURBS_BenchmarkEntity, DEBUG_Vars->NURBS_Benchmark);
  if (DEBUG_Vars->NURBS_Benchmark)
  { 
   entity_with_modify_witness Witness = BeginEntityModify(DEBUG_Vars->NURBS_BenchmarkEntity);
   MarkEntityModified(&Witness);
   EndEntityModify(Witness);
  }
 }
 
 if (DEBUG_Vars->Parametric_BenchmarkEntity)
 {
  SetEntityVisibility(DEBUG_Vars->Parametric_BenchmarkEntity, DEBUG_Vars->Parametric_Benchmark);
  if (DEBUG_Vars->Parametric_Benchmark)
  { 
   entity_with_modify_witness Witness = BeginEntityModify(DEBUG_Vars->Parametric_BenchmarkEntity);
   MarkEntityModified(&Witness);
   EndEntityModify(Witness);
  }
 }
 
 if (DEBUG_Vars->Bezier_BenchmarkEntity)
 {
  SetEntityVisibility(DEBUG_Vars->Bezier_BenchmarkEntity, DEBUG_Vars->Bezier_Benchmark);
  if (DEBUG_Vars->Bezier_Benchmark)
  { 
   entity_with_modify_witness Witness = BeginEntityModify(DEBUG_Vars->Bezier_BenchmarkEntity);
   MarkEntityModified(&Witness);
   EndEntityModify(Witness);
  }
 }
 
 if (DEBUG_Vars->CubicSpline_BenchmarkEntity)
 {
  SetEntityVisibility(DEBUG_Vars->CubicSpline_BenchmarkEntity, DEBUG_Vars->CubicSpline_Benchmark);
  if (DEBUG_Vars->CubicSpline_Benchmark)
  { 
   entity_with_modify_witness Witness = BeginEntityModify(DEBUG_Vars->CubicSpline_BenchmarkEntity);
   MarkEntityModified(&Witness);
   EndEntityModify(Witness);
  }
 }
 
 RenderDevConsoleUI(Editor);
}
