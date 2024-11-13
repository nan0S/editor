#if 0
if (Collision.Entity)
{
 curve *Curve = GetCurve(Collision.Entity);
 b32 ClickActionDone = false; 
 
 if (Editor->CurveAnimation.Stage == AnimateCurveAnimation_PickingTarget &&
     Collision.Entity != Editor->CurveAnimation.FromCurveEntity)
 {
  Editor->CurveAnimation.ToCurveEntity = Collision.Entity;
  ClickActionDone = true;
 }
 
 curve_combining_state *Combining = &Editor->CurveCombining;
 if (Combining->SourceEntity)
 {
  if (Collision.Entity != Combining->SourceEntity)
  {
   Combining->WithEntity = Collision.Entity;
  }
  
  // NOTE(hbr): Logic to recognize whether first/last point of
  // combine/target curve was clicked. If so then change which
  // point should be combined with which one (first with last,
  // first with first, ...). On top of that if user clicked
  // on already selected point but in different direction
  // (target->combine rather than combine->target) then swap
  // target and combine curves to combine in the other direction.
  
  // TODO(hbr): Probably simplify this logic
  if (Collision.Type == CurveCollision_ControlPoint)
  {
   if (Collision.Entity == Combining->SourceEntity)
   {
    curve *SourceCurve = GetCurve(Combining->SourceEntity);
    if (Collision.PointIndex == 0)
    {
     if (!Combining->SourceCurveLastControlPoint)
     {
      Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
      Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
      Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
      Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
     }
     else
     {
      Combining->SourceCurveLastControlPoint = false;
     }
    }
    else if (Collision.PointIndex == SourceCurve->ControlPointCount - 1)
    {
     if (Combining->SourceCurveLastControlPoint)
     {
      Swap(Combining->SourceEntity, Combining->WithEntity, entity *);
      Swap(Combining->SourceCurveLastControlPoint, Combining->WithCurveFirstControlPoint, b32);
      Combining->SourceCurveLastControlPoint = !Combining->SourceCurveLastControlPoint;
      Combining->WithCurveFirstControlPoint = !Combining->WithCurveFirstControlPoint;
     }
     else
     {
      Combining->SourceCurveLastControlPoint = true;
     }
    }
   }
   else if (Collision.Entity == Combining->WithEntity)
   {
    curve *WithCurve = GetCurve(Combining->WithEntity);
    if (Collision.PointIndex == 0)
    {
     Combining->WithCurveFirstControlPoint = true;
    }
    else if (Collision.PointIndex == WithCurve->ControlPointCount - 1)
    {
     Combining->WithCurveFirstControlPoint = false;
    }
   }
  }
  
  ClickActionDone = true;
 }
}
else
{
 
}

ImGui::CreateContext();
ImGui_ImplWin32_Init(Window);
ImGui_ImplOpenGL3_Init();

ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();

ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#endif