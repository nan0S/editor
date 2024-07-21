internal frame_stats
MakeFrameStats(void)
{
   frame_stats Result = {};
   Result.Calculation.MinFrameTime = INF_F32;
   Result.Calculation.MaxFrameTime = -INF_F32;
   
   return Result;
}

internal void
FrameStatsUpdate(frame_stats *Stats, f32 FrameTime)
{
   Stats->Calculation.FrameCount += 1;
   Stats->Calculation.SumFrameTime += FrameTime;
   Stats->Calculation.MinFrameTime = Min(Stats->Calculation.MinFrameTime, FrameTime);
   Stats->Calculation.MaxFrameTime = Max(Stats->Calculation.MaxFrameTime, FrameTime);
   
   if (Stats->Calculation.SumFrameTime >= 1.0f)
   {
      Stats->FPS = Stats->Calculation.FrameCount / Stats->Calculation.SumFrameTime;
      Stats->MinFrameTime = Stats->Calculation.MinFrameTime;
      Stats->MaxFrameTime = Stats->Calculation.MaxFrameTime;
      Stats->AvgFrameTime = Stats->Calculation.SumFrameTime / Stats->Calculation.FrameCount;
      
      Stats->Calculation.FrameCount = 0;
      Stats->Calculation.MinFrameTime = INF_F32;
      Stats->Calculation.MaxFrameTime = -INF_F32;
      Stats->Calculation.SumFrameTime = 0.0f;
   }
}

internal void
RenderDebugWindow(editor *Editor)
{
   bool ViewDebugWindowAsBool = Cast(bool)Editor->UI_Config.ViewDebugWindow;
   DeferBlock(ImGui::Begin("Debug", &ViewDebugWindowAsBool), ImGui::End())
   {
      Editor->UI_Config.ViewDebugWindow = Cast(b32)ViewDebugWindowAsBool;
      
      if (Editor->UI_Config.ViewDebugWindow)
      {
         ImGui::Text("Number of entities = %lu", Editor->Entities.EntityCount);
         
         if (Editor->SelectedEntity &&
             Editor->SelectedEntity->Type == Entity_Curve)
         {
            curve *Curve = &Editor->SelectedEntity->Curve;
            
            ImGui::Text("Number of control points = %lu", Curve->ControlPointCount);
            ImGui::Text("Number of curve points = %lu", Curve->CurvePointCount);
         }
         
         ImGui::Text("Min Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MinFrameTime);
         ImGui::Text("Max Frame Time = %.3fms", 1000.0f * Editor->FrameStats.MaxFrameTime);
         ImGui::Text("Average Frame Time = %.3fms", 1000.0f * Editor->FrameStats.AvgFrameTime);
      }
   }
}

internal void
DebugUpdateAndRender(editor *Editor)
{
   if (Editor->UI_Config.ViewDebugWindow)
   {
      RenderDebugWindow(Editor);
   }
   ImGui::ShowDemoWindow();
}