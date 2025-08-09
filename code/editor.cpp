#include "editor.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_arena.cpp"
#include "base/base_memory.cpp"

#include "editor_profiler.cpp"
#include "editor_math.cpp"
#include "editor_renderer.cpp"
#include "editor_parametric_equation.cpp"
#include "editor_ui.cpp"
#include "editor_stb.cpp"
#include "editor_debug.cpp"
#include "editor_camera.cpp"
#include "editor_sort.cpp"
#include "editor_editor.cpp"
#include "editor_platform.cpp"
#include "editor_ctx.cpp"

#if BUILD_HOT_RELOAD
platform_api Platform;
#endif

internal void
MovePointAlongCurve(curve *Curve, v2 *TranslateInput, f32 *PointFractionInput, b32 Forward)
{
 v2 Translate = *TranslateInput;
 f32 Fraction = *PointFractionInput;
 
 u32 SampleCount = Curve->CurveSampleCount;
 v2 *Samples = Curve->CurveSamples;
 f32 DeltaFraction = 1.0f / (SampleCount - 1);
 
 f32 PointIndexFloat = Fraction * (SampleCount - 1);
 u32 PointIndex = Cast(u32)(Forward ? FloorF32(PointIndexFloat) : CeilF32(PointIndexFloat));
 PointIndex = ClampTop(PointIndex, SampleCount - 1);
 
 i32 DirSign = (Forward ? 1 : -1);
 
 b32 Moving = true;
 while (Moving)
 {
  u32 PrevPointIndex = PointIndex;
  PointIndex += DirSign;
  if (PointIndex >= SampleCount)
  {
   Moving = false;
  }
  
  if (Moving)
  {
   v2 AlongCurve = Samples[PointIndex] - Samples[PrevPointIndex];
   f32 AlongCurveLength = Norm(AlongCurve);
   f32 InvAlongCurveLength = 1.0f / AlongCurveLength;
   f32 Projection = Clamp01(Dot(AlongCurve, Translate) * InvAlongCurveLength * InvAlongCurveLength);
   Translate -= Projection * AlongCurve;
   Fraction += DirSign * Projection * DeltaFraction;
   
   if (Projection < 1.0f)
   {
    Moving = false;
   }
  }
 }
 
 *TranslateInput = Translate;
 *PointFractionInput = Fraction;
}

internal b32
ResetCtxMenu(string Label)
{
 b32 Reset = false;
 if (UI_IsItemHovered() && UI_IsMouseClicked(UIMouseButton_Right))
 {
  UI_OpenPopup(Label);
 }
 if (UI_BeginPopup(Label, UIWindowFlag_NoMove))
 {
  Reset = UI_MenuItem(0, NilStr, StrLit("Reset"));
  UI_EndPopup();
 }
 
 return Reset;
}

internal rendering_entity_handle
BeginRenderingEntity(entity *Entity, render_group *RenderGroup)
{
 rendering_entity_handle Handle = {};
 Handle.Entity = Entity;
 Handle.RenderGroup = RenderGroup;
 
 mat3 Model = ModelTransform(Entity->XForm.P, Entity->XForm.Rotation, Entity->XForm.Scale);
 SetTransform(RenderGroup, Model, Cast(f32)Entity->SortingLayer);
 
 return Handle;
}
internal void
EndRenderingEntity(rendering_entity_handle Handle)
{
 ResetTransform(Handle.RenderGroup);
}

internal void
UpdateAndRenderPointTracking(rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
 curve *Curve = &Entity->Curve;
 curve_params *CurveParams = &Curve->Params;
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 
 
 switch (Tracking->Type)
 {
  case PointTrackingAlongCurve_None: {}break;
  
  case PointTrackingAlongCurve_BezierCurveSplit: {
   f32 Radius = GetCurveTrackedPointRadius(Curve);
   rgba Color = RGBA(0.0f, 1.0f, 0.0f, 0.5f);
   f32 OutlineThickness = 0.3f * Radius;
   rgba OutlineColor = RGBA_Darken(Color, 0.5f);
   
   PushCircle(RenderGroup,
              Tracking->LocalSpaceTrackedPoint,
              Radius, Color,
              GetCurvePartVisibilityZOffset(CurvePartVisibility_BezierSplitPoint),
              OutlineThickness, OutlineColor);
  }break;
  
  case PointTrackingAlongCurve_DeCasteljauVisualization: {
   u32 IterationCount = Tracking->Intermediate.IterationCount;
   f32 P = 0.0f;
   f32 Delta_P = 1.0f / (IterationCount - 1);
   rgba GradientA = RGBA_U8(255, 0, 144);
   rgba GradientB = RGBA_U8(155, 200, 0);
   // TODO(hbr): Shouldn't this use some of the functions that are already used for drwaing curve points to be consistent?
   f32 PointSize = CurveParams->DrawParams.Points.Radius;
   
   u32 PointIndex = 0;
   for (u32 Iteration = 0;
        Iteration < IterationCount;
        ++Iteration)
   {
    rgba IterationColor = Lerp(GradientA, GradientB, P);
    
    PushVertexArray(RenderGroup,
                    Tracking->LineVerticesPerIteration[Iteration].Vertices,
                    Tracking->LineVerticesPerIteration[Iteration].VertexCount,
                    Tracking->LineVerticesPerIteration[Iteration].Primitive,
                    IterationColor, GetCurvePartVisibilityZOffset(CurvePartVisibility_DeCasteljauAlgorithmLines));
    
    for (u32 I = 0; I < IterationCount - Iteration; ++I)
    {
     v2 Point = Tracking->Intermediate.P[PointIndex];
     PushCircle(RenderGroup,
                Point, PointSize, IterationColor,
                GetCurvePartVisibilityZOffset(CurvePartVisibility_DeCasteljauAlgorithmPoints));
     ++PointIndex;
    }
    
    P += Delta_P;
   }
  }break;
 }
}

internal void
UpdateAndRenderDegreeLowering(editor *Editor, rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
 if (Entity->Type == Entity_Curve)
 {
  curve *Curve = SafeGetCurve(Entity);
  curve_params *CurveParams = &Curve->Params;
  curve_degree_lowering_state *Lowering = &Curve->DegreeLowering;
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  
  if (Lowering->Active)
  {
   Assert(CurveParams->Type == Curve_Bezier);
   
   b32 IsDegreeLoweringWindowOpen = true;
   b32 MixChanged = false;
   b32 Ok = false;
   b32 Revert = false;
   
   if(UI_BeginWindow(&IsDegreeLoweringWindowOpen, 0, StrLit("Degree Lowering")))
   {          
    UI_Text(true, StrLit("Degree lowering failed. Tweak the middle point to fit the curve manually."));
    UI_SeparatorText(NilStr);
    MixChanged = UI_SliderFloat(&Lowering->MixParameter, 0.0f, 1.0f, StrLit("Middle Point Mix"));
    
    Ok = UI_Button(StrLit("OK"));
    UI_SameRow();
    Revert = UI_Button(StrLit("Revert"));
   }
   UI_EndWindow();
   
   //-
   curve_points_static *Points = GetCurvePoints(Curve);
   Assert(Lowering->LowerDegree.MiddlePointIndex < Points->ControlPointCount);
   
   if (MixChanged)
   {
    v2 NewControlPoint = Lerp(Lowering->LowerDegree.P_I, Lowering->LowerDegree.P_II, Lowering->MixParameter);
    f32 NewControlPointWeight = Lerp(Lowering->LowerDegree.W_I, Lowering->LowerDegree.W_II, Lowering->MixParameter);
    
    control_point_handle MiddlePoint = ControlPointHandleFromIndex(Lowering->LowerDegree.MiddlePointIndex);
    SetControlPoint(&EntityWitness,
                    MiddlePoint,
                    NewControlPoint,
                    NewControlPointWeight);
   }
   
   if (Revert)
   {
    curve_points_static *Points = Lowering->OriginalCurvePoints;
    Assert(Points);
    SetCurvePointsTracked(Editor, &EntityWitness, CurvePointsHandleFromCurvePointsStatic(Points));
   }
   
   if (Ok || Revert || !IsDegreeLoweringWindowOpen)
   {
    EndLoweringBezierCurveDegree(Editor, Lowering);
   }
  }
  
  if (Lowering->Active)
  {
   rgba Color = Curve->Params.DrawParams.Line.Color;
   Color.A *= 0.5f;
   PushVertexArray(RenderGroup,
                   Lowering->OriginalCurveVertices.Vertices,
                   Lowering->OriginalCurveVertices.VertexCount,
                   Lowering->OriginalCurveVertices.Primitive,
                   Color,
                   GetCurvePartVisibilityZOffset(CurvePartVisibility_LineShadow));
  }
  
  EndEntityModify(EntityWitness);
 }
}

internal f32
UpdateBouncingParam(bouncing_parameter *Bouncing, f32 dt)
{
 f32 T = Bouncing->T + dt * Bouncing->Speed * Bouncing->Sign;
 f32 Sign = Bouncing->Sign;
 if (T < 0.0f || T > 1.0f)
 {
  T = Clamp01(T);
  Sign = -Sign;
 }
 Bouncing->T = T;
 Bouncing->Sign = Sign;
 
 return T;
}

internal void
RenderEntity(rendering_entity_handle Handle)
{
 entity *Entity = Handle.Entity;
 render_group *RenderGroup = Handle.RenderGroup;
 
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   curve_params *CurveParams = &Curve->Params;
   
   if (CurveParams->DrawParams.Line.Enabled)
   {
    PushVertexArray(RenderGroup,
                    Curve->CurveVertices.Vertices,
                    Curve->CurveVertices.VertexCount,
                    Curve->CurveVertices.Primitive,
                    Curve->Params.DrawParams.Line.Color,
                    GetCurvePartVisibilityZOffset(CurvePartVisibility_CurveLine));
   }
   
   if (IsPolylineVisible(Curve))
   {
    PushVertexArray(RenderGroup,
                    Curve->PolylineVertices.Vertices,
                    Curve->PolylineVertices.VertexCount,
                    Curve->PolylineVertices.Primitive,
                    Curve->Params.DrawParams.Polyline.Color,
                    GetCurvePartVisibilityZOffset(CurvePartVisibility_CurvePolyline));
   }
   
   if (IsConvexHullVisible(Curve))
   {
    PushVertexArray(RenderGroup,
                    Curve->ConvexHullVertices.Vertices,
                    Curve->ConvexHullVertices.VertexCount,
                    Curve->ConvexHullVertices.Primitive,
                    Curve->Params.DrawParams.ConvexHull.Color,
                    GetCurvePartVisibilityZOffset(CurvePartVisibility_CurveConvexHull));
   }
   
   if (AreBSplineConvexHullsVisible(Curve))
   {
    if (IsControlPointSelected(Curve))
    {
     control_point_handle SelectedControlPoint = Curve->SelectedControlPoint;
     u32 ControlPointIndex = IndexFromControlPointHandle(SelectedControlPoint);
     u32 HullCount = Curve->BSplineConvexHullCount;
     if (HullCount > 0)
     {
      u32 HullIndex = Clamp(ControlPointIndex, 0, HullCount - 1);
      b_spline_convex_hull *Hulls = Curve->BSplineConvexHulls;
      b_spline_convex_hull *Hull = Hulls + HullIndex;
      rgba Color = CurveParams->DrawParams.BSplinePartialConvexHull.Color;
      PushVertexArray(RenderGroup,
                      Hull->Vertices.Vertices, 
                      Hull->Vertices.VertexCount,
                      Hull->Vertices.Primitive,
                      Color,
                      GetCurvePartVisibilityZOffset(CurvePartVisibility_CurveConvexHull));
     }
    }
   }
   
   if (AreCurvePointsVisible(Curve))
   {
    visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
    for (u32 Index = 0;
         Index < VisibleBeziers.Count;
         ++Index)
    {
     cubic_bezier_point_handle Bezier = VisibleBeziers.Indices[Index];
     
     v2 BezierPoint = GetCubicBezierPoint(Curve, Bezier);
     v2 CenterPoint = GetCenterPointFromCubicBezierPoint(Curve, Bezier);
     
     f32 BezierPointRadius = GetCurveCubicBezierPointRadius(Curve);
     f32 HelperLineWidth = 0.2f * CurveParams->DrawParams.Line.Width;
     
     // TODO(hbr): It is really fucked up I have to add Entity->P when I do [SetTransform] above.
     // clean up this mess
     PushLine(RenderGroup,
              Entity->XForm.P + BezierPoint,
              Entity->XForm.P + CenterPoint,
              HelperLineWidth,
              CurveParams->DrawParams.Line.Color,
              GetCurvePartVisibilityZOffset(CurvePartVisibility_CubicBezierHelperLines));
     PushCircle(RenderGroup,
                BezierPoint,
                BezierPointRadius, CurveParams->DrawParams.Points.Color,
                GetCurvePartVisibilityZOffset(CurvePartVisibility_CubicBezierHelperPoints));
    }
    
    curve_points_static *Points = GetCurvePoints(Curve);
    u32 ControlPointCount = Points->ControlPointCount;
    v2 *ControlPoints = Points->ControlPoints;
    for (u32 PointIndex = 0;
         PointIndex < ControlPointCount;
         ++PointIndex)
    {
     point_draw_info PointInfo = GetCurveControlPointDrawInfo(Entity, ControlPointHandleFromIndex(PointIndex));
     PushCircle(RenderGroup,
                ControlPoints[PointIndex],
                PointInfo.Radius,
                PointInfo.Color,
                GetCurvePartVisibilityZOffset(CurvePartVisibility_CurveControlPoint),
                PointInfo.OutlineThickness,
                PointInfo.OutlineColor);
    }
   }
   
   if (AreBSplineKnotsVisible(Curve))
   {
    u32 PartitionSize = GetBSplineParams(Curve).KnotParams.PartitionSize;
    v2 *PartitionKnotPoints = Curve->BSplinePartitionKnots;
    b_spline_params BSpline = CurveParams->BSpline;
    point_draw_info KnotPointInfo = GetBSplinePartitionKnotPointDrawInfo(Entity);
    
    for (u32 KnotIndex = 0;
         KnotIndex < PartitionSize;
         ++KnotIndex)
    {
     v2 Knot = PartitionKnotPoints[KnotIndex];
     PushCircle(RenderGroup,
                Knot,
                KnotPointInfo.Radius,
                KnotPointInfo.Color,
                GetCurvePartVisibilityZOffset(CurvePartVisibility_BSplineKnot),
                KnotPointInfo.OutlineThickness,
                KnotPointInfo.OutlineColor);
    }
   }
   
   // TODO(hbr): remove
   if (DEBUG_Vars->ShowSampleCurvePoints)
   {
    v2 *Samples = Curve->CurveSamples;
    u32 SampleCount = Curve->CurveSampleCount;
    rgba GradientA = RGBA_U8(255, 0, 144);
    rgba GradientB = RGBA_U8(155, 200, 0);
    ForEachIndex(SampleIndex, SampleCount)
    {
     v2 Sample = Samples[SampleIndex];
     f32 T = Cast(f32)SampleIndex / (SampleCount - 1);
     rgba Color = Lerp(GradientA, GradientB, T);
     PushCircle(RenderGroup,
                Sample,
                CurveParams->DrawParams.Points.Radius,
                Color,
                GetCurvePartVisibilityZOffset(CurvePartVisibility_CurveSamplePoint));
    }
   }
  } break;
  
  case Entity_Image: {
   image *Image = &Entity->Image;
   PushImage(RenderGroup, Image->Dim, Image->TextureHandle);
  }break;
  
  case Entity_Count: InvalidPath; break;
 }
}

internal void
InterpolateAnimationCurve(animating_curves_state *Animation, v2 *Samples)
{
 temp_arena Temp = TempArena(0);
 
 entity *Entity0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
 entity *Entity1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
 
 entity *Entity = Animation->ExtractEntity;
 entity_with_modify_witness Witness = BeginEntityModify(Entity);
 
 if (Entity0 && Entity1)
 {
  string Name = StrF(Temp.Arena, "%S+%S extracted", GetEntityName(Entity0), GetEntityName(Entity1));
  InitEntityAsCurve(Entity, Name, DefaultCubicSplineCurveParams());
  curve *Curve = SafeGetCurve(Entity);
  
  u32 SampleCount = Animation->AnimationCurveSamplePointCount;
  u32 ControlPointCount = Animation->ExtractedCurveDetail;
  ControlPointCount = ClampTop(ControlPointCount, SampleCount);
  
  curve_points_modify_handle ModifyHandle = BeginModifyCurvePoints(&Witness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsOnly);
  
  f32 SamplePointsStep = SafeDiv0(Cast(f32)(SampleCount - 1), ControlPointCount - 1);
  u32 OutControlCount = 0;
  v2 *OutControls = ModifyHandle.ControlPoints;
  
  for (u32 ControlPointIndex = 0;
       ControlPointIndex < ControlPointCount;
       ++ControlPointIndex)
  {
   u32 SampleIndex = Cast(u32)RoundF32(ControlPointIndex * SamplePointsStep);
   SampleIndex = Clamp(SampleIndex, 0, SampleIndex - 1);
   OutControls[OutControlCount++] = Samples[SampleIndex];
  }
  Assert(OutControlCount == ControlPointCount);
  
  EndModifyCurvePoints(ModifyHandle);
 }
 else
 {
  curve_points_handle Points = CurvePointsHandleZero();
  SetCurvePoints(&Witness, Points);
 }
 
 EndEntityModify(Witness);
 EndTemp(Temp);
}

struct compute_animation_samples_result
{
 v2 *Samples;
 u32 SampleCount;
 f32 MappedT;
};
internal compute_animation_samples_result
ComputeAnimationSamples(arena *Arena, animating_curves_state *Animation)
{
 compute_animation_samples_result Result = {};
 
 entity *Entity0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
 entity *Entity1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
 
 if (Entity0 && Entity1)
 {
  curve *Curve0 = SafeGetCurve(Entity0);
  curve *Curve1 = SafeGetCurve(Entity1);
  
  v2 Points[4] = {
   V2(0.0f, 0.0f),
   V2(1.0f, 0.0f),
   V2(0.0f, 1.0f),
   V2(1.0f, 1.0f),
  };
  f32 MappedT = BezierCurveEvaluate(Animation->Bouncing.T, Points, 4).Y;
  
  v2 *CurveSamples0 = Curve0->CurveSamples;
  v2 *CurveSamples1 = Curve1->CurveSamples;
  
  u32 CurveSampleCount0 = Curve0->CurveSampleCount;
  u32 CurveSampleCount1 = Curve1->CurveSampleCount;
  
  // TODO(hbr): Multithread this
  // TODO(hbr): Use max instead
  u32 CurveSampleCount = Min(CurveSampleCount0, CurveSampleCount1);
  v2 *CurveSamples = PushArrayNonZero(Arena, CurveSampleCount, v2);
  for (u32 LinePointIndex = 0;
       LinePointIndex < CurveSampleCount;
       ++LinePointIndex)
  {
   u32 LinePointIndex0 = ClampTop(SafeDiv0(CurveSampleCount0 * LinePointIndex, CurveSampleCount - 1), CurveSampleCount0 - 1);
   u32 LinePointIndex1 = ClampTop(SafeDiv0(CurveSampleCount1 * LinePointIndex, CurveSampleCount - 1), CurveSampleCount1 - 1);
   
   if (IsCurveReversed(Entity0)) LinePointIndex0 = (CurveSampleCount0-1 - LinePointIndex0);
   if (IsCurveReversed(Entity1)) LinePointIndex1 = (CurveSampleCount1-1 - LinePointIndex1);
   
   v2 PointLocal0 = CurveSamples0[LinePointIndex0];
   v2 PointLocal1 = CurveSamples1[LinePointIndex1];
   
   v2 Point0 = LocalToWorldEntityPosition(Entity0, PointLocal0);
   v2 Point1 = LocalToWorldEntityPosition(Entity1, PointLocal1);
   
   v2 Point  = Lerp(Point0, Point1, MappedT);
   CurveSamples[LinePointIndex] = Point;
  }
  Animation->AnimationCurveSamplePointCount = CurveSampleCount;
  
  if (!Animation->ExtractedCurveDetailCustom)
  {
   curve_points_static *Points0 = GetCurvePoints(Curve0);
   curve_points_static *Points1 = GetCurvePoints(Curve1);
   Animation->ExtractedCurveDetail = Max(Points0->ControlPointCount, Points1->ControlPointCount);
  }
  
  Result.Samples = CurveSamples;
  Result.SampleCount = CurveSampleCount;
  Result.MappedT = MappedT;
 }
 
 return Result;
}

internal void
UpdateAndRenderAnimatingCurves(editor *Editor,
                               platform_input_output *Input,
                               render_group *RenderGroup)
{
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 
 entity *Entity0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
 entity *Entity1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
 
 if ((Animation->Flags & AnimatingCurves_Active) && Entity0 && Entity1)
 {
  temp_arena Temp = TempArena(0);
  
  if (Animation->Flags & AnimatingCurves_Animating)
  {
   f32 dt = UseAndExtractDeltaTime(Input);
   UpdateBouncingParam(&Animation->Bouncing, dt);
  }
  
  compute_animation_samples_result AnimationSamples = ComputeAnimationSamples(Temp.Arena, Animation);
  if (Animation->Flags & AnimatingCurves_Preview)
  {
   InterpolateAnimationCurve(Animation, AnimationSamples.Samples);
   rendering_entity_handle RenderingHandle = BeginRenderingEntity(Animation->ExtractEntity, RenderGroup);
   RenderEntity(RenderingHandle);
   EndRenderingEntity(RenderingHandle);
  }
  else
  {
   curve *Curve0 = SafeGetCurve(Entity0);
   curve *Curve1 = SafeGetCurve(Entity1);
   f32 MappedT = AnimationSamples.MappedT;
   
   f32 LineWidth0 = Curve0->Params.DrawParams.Line.Width;
   f32 LineWidth1 = Curve1->Params.DrawParams.Line.Width;
   f32 LineWidth  = Lerp(LineWidth0, LineWidth1, MappedT);
   
   rgba LineColor0 = Curve0->Params.DrawParams.Line.Color;
   rgba LineColor1 = Curve1->Params.DrawParams.Line.Color;
   rgba LineColor  = Lerp(LineColor0, LineColor1, MappedT);
   
   f32 ZOffset0 = Cast(f32)Entity0->SortingLayer;
   f32 ZOffset1 = Cast(f32)Entity1->SortingLayer;
   f32 ZOffset  = Lerp(ZOffset0, ZOffset1, MappedT);
   
   vertex_array LineVertices = ComputeVerticesOfThickLine(Temp.Arena,
                                                          AnimationSamples.SampleCount,
                                                          AnimationSamples.Samples,
                                                          LineWidth, false);
   PushVertexArray(RenderGroup,
                   LineVertices.Vertices, LineVertices.VertexCount, LineVertices.Primitive,
                   LineColor,
                   ZOffset);
  }
  
  EndTemp(Temp);
 }
}

internal void
UpdateAndRenderEntities(editor *Editor, render_group *RenderGroup)
{
 entity_array Entities = AllEntityArrayFromStore(Editor->EntityStore);
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  if (IsEntityVisible(Entity))
  {
   rendering_entity_handle Handle = BeginRenderingEntity(Entity, RenderGroup);
   
   RenderEntity(Handle);
   UpdateAndRenderDegreeLowering(Editor, Handle);
   UpdateAndRenderPointTracking(Handle);
   
   EndRenderingEntity(Handle);
  }
 }
}

#if 0
struct update_and_render_parametric_curve_var_result
{
 b32 Changed;
 b32 Remove;
};
enum update_parametric_curve_var_mode
{
 UpdateParametricCurveVar_Static,
 UpdateParametricCurveVar_Dynamic,
};
internal update_and_render_parametric_curve_var_result
UpdateAndRenderParametricCurveVar(parametric_curve_field *Var,
                                  update_parametric_curve_var_mode Mode)
{
 b32 VarChanged = false;
 
 if (Var->EquationOrDragFloatMode_Equation)
 {
  char_buffer *VarEquation = CharBufferFromStringId(GetCtx()->StrStore, Var->VarEquation);
  VarChanged = UI_InputText2(VarEquation, 0, StrLit("##Equation"));
 }
 else
 {
  VarChanged |= UI_DragFloat(&Var->DragValue, 0, 0, 0, StrLit("##Drag"));
 }
 
 b32 Remove = false;
 if (Mode == UpdateParametricCurveVar_Dynamic)
 {
  UI_SameRow();
  Remove = UI_Button(StrLit("-"));
 }
 
 b32 SwapMode = false;
 { 
  string ButtonLabel = {};
  string TooltipContents = {};
  if (Var->EquationOrDragFloatMode_Equation)
  {
   ButtonLabel = StrLit("E");
   TooltipContents = StrLit("Switch to drag");
  }
  else
  {
   ButtonLabel = StrLit("D");
   TooltipContents = StrLit("Switch to equation");
  }
  
  UI_SameRow();
  SwapMode = UI_Button(ButtonLabel);
  if (UI_IsItemHovered())
  {
   UI_Tooltip(TooltipContents);
  }
 }
 
 if ((Var->EquationOrDragFloatMode_Equation) && Var->Cached.EquationFail)
 {
  UI_Colored(UI_Color_Text, RedColor)
  {
   UI_SameRow();
   UI_Text(false, Var->Cached.EquationErrorMessage);
  }
 }
 
 if (SwapMode)
 {
  VarChanged = true;
  Var->EquationOrDragFloatMode_Equation = !Var->EquationOrDragFloatMode_Equation;
 }
 
 if (Remove)
 {
  VarChanged = true;
 }
 
 update_and_render_parametric_curve_var_result Result = {};
 Result.Changed = VarChanged;
 Result.Remove = Remove;
 
 return Result;
}
#endif

internal changed_b32
RenderParametricCurveFieldUI(parametric_curve_resources *Resources,
                             parametric_curve_field *Field,
                             b32 IsVar,
                             b32 IsStatic,
                             char *StaticVarName,
                             string Label)
{
 UI_PushLabel(Label);
 
 string_store *StrStore = GetCtx()->StrStore;
 u32 InputBoxWidthInChars = 4;
 
 b32 Disabled = IsStatic;
 b32 IsDynamic = !IsStatic;
 b32 Remove = false;
 b32 Changed = false;
 
 UI_Disabled(Disabled)
 {
  if (IsStatic)
  {
   UI_InputText(StaticVarName, 0, InputBoxWidthInChars, Label);
  }
  else
  {
   char_buffer *VarName = CharBufferFromStringId(Field->VarName);
   UI_InputText2(VarName, InputBoxWidthInChars, Label);
  }
 }
 
 UI_SameRow();
 UI_Text(false, StrLit(":= "));
 UI_SameRow();
 
 if (IsVar && !Field->EquationOrDragFloatMode_Equation)
 {
  Changed |= UI_DragFloat(&Field->DragValue, 0, 0, 0, StrLit("##Drag"));
 }
 else
 {
  char_buffer *VarEquation = CharBufferFromStringId(Field->Equation);
  Changed = UI_InputText2(VarEquation, 0, StrLit("##Equation"));
 }
 
 if (IsDynamic)
 {
  UI_SameRow();
  if (UI_Button(StrLit("-")))
  {
   Remove = true;
   Changed = true;
  }
 }
 
 if (IsVar)
 {
  string ButtonLabel = {};
  string TooltipContents = {};
  if (Field->EquationOrDragFloatMode_Equation)
  {
   ButtonLabel = StrLit("E");
   TooltipContents = StrLit("Switch to drag");
  }
  else
  {
   ButtonLabel = StrLit("D");
   TooltipContents = StrLit("Switch to equation");
  }
  
  UI_SameRow();
  if (UI_Button(ButtonLabel))
  {
   Changed = true;
   Field->EquationOrDragFloatMode_Equation = !Field->EquationOrDragFloatMode_Equation;
  }
  if (UI_IsItemHovered())
  {
   UI_Tooltip(TooltipContents);
  }
 }
 
 if (!IsVar || Field->EquationOrDragFloatMode_Equation)
 {
  UI_Colored(UI_Color_Text, RedColor)
  {
   UI_SameRow();
   UI_Text(false, Field->Cached.EvalErrorMessage);
  }
 }
 
 if (Remove)
 {
  Assert(IsVar && IsDynamic);
  DeallocParametricCurveVar(Resources, Field);
 }
 
 UI_PopLabel();
 
 return Changed;
}

internal void
FocusCameraOnEntity(camera *Camera, entity *Entity, render_group *RenderGroup)
{
 rect2 AABB = EntityAABB(Entity);
 if (IsNonEmpty(&AABB))
 {
  v2 TargetP = 0.5f * (AABB.Min + AABB.Max);
  v2 Size = AABB.Max - AABB.Min;
  v2 Expand = 0.6f * Size;
  AABB.Min += Expand;
  AABB.Max += Expand;
  
  // NOTE(hbr): We want AABB.Max, which is in world space, to be transformed
  // into (1, 1) in clip space. Calculate projection matrix (by creating camera and
  // clip matrices) and solve the equation. We get ZoomX and ZoomY. Take the minimal
  // one (because the window might be a rectangle) so that the whole AABB is visible.
  // Appendix: this isn't really true that we want to transform AABB.Max necessarily
  // into (1,1) when we take camera rotation into account. But this is a useful
  // step to understanding the idea behing the code below. In reality we transform
  // all 4 corners of AABB through projection matrix and take the biggest zoom that
  // encompasses all of them.
  // NOTE(hbr): I'm not 100% sure whether taking Abs is mathematically sound here
  // and whether this code actually mathematically is correct. Abs is sus to me
  // to be honest. But it seems to work fairly well.
  mat3_inv CameraT = CameraTransform(TargetP, Camera->Rotation, 1.0f);
  mat3_inv ClipT = ClipTransform(RenderGroup->AspectRatio);
  mat3 Proj = ClipT.Forward * CameraT.Forward;
  
  rect2_corners Corners = AABBCorners(AABB);
  v2 Q0 = Proj * Corners.Corners[Corner_00];
  v2 Q1 = Proj * Corners.Corners[Corner_01];
  v2 Q2 = Proj * Corners.Corners[Corner_10];
  v2 Q3 = Proj * Corners.Corners[Corner_11];
  
  f32 ZoomX0 = Abs(SafeDiv1(1.0f, Q0.X));
  f32 ZoomY0 = Abs(SafeDiv1(1.0f, Q0.Y));
  f32 ZoomX1 = Abs(SafeDiv1(1.0f, Q1.X));
  f32 ZoomY1 = Abs(SafeDiv1(1.0f, Q1.Y));
  f32 ZoomX2 = Abs(SafeDiv1(1.0f, Q2.X));
  f32 ZoomY2 = Abs(SafeDiv1(1.0f, Q2.Y));
  f32 ZoomX3 = Abs(SafeDiv1(1.0f, Q3.X));
  f32 ZoomY3 = Abs(SafeDiv1(1.0f, Q3.Y));
  
  f32 TargetZoom = ZoomX0;
  TargetZoom = Min(TargetZoom, ZoomY0);
  TargetZoom = Min(TargetZoom, ZoomX1);
  TargetZoom = Min(TargetZoom, ZoomY1);
  TargetZoom = Min(TargetZoom, ZoomX2);
  TargetZoom = Min(TargetZoom, ZoomY2);
  TargetZoom = Min(TargetZoom, ZoomX3);
  TargetZoom = Min(TargetZoom, ZoomY3);
  
  SetCameraTarget(Camera, TargetP, TargetZoom);
 }
}

internal b32
RenderDrawParamsUI(string Label, draw_params *Params, draw_params *Default, b32 IsPoint)
{
 b32 Changed = false;
 UI_Label(Label)
 {
  UI_Checkbox(&Params->Enabled, NilStr);
  UI_SameRow();
  UI_SeparatorText(Label);
  
  Changed |= UI_DragFloat(&Params->Float, 0.0f, FLT_MAX, 0, (IsPoint ? StrLit("Radius") : StrLit("Width")));
  if (ResetCtxMenu(StrLit("FloatReset")))
  {
   Params->Float = Default->Float;
   Changed = true;
  }
  
  Changed |= UI_ColorPicker(&Params->Color, StrLit("Color"));
  if (ResetCtxMenu(StrLit("ColorReset")))
  {
   Params->Color = Params->Color;
   Changed = true;
  }
 }
 return Changed;
}

internal void
RenderSelectedEntityUI(editor *Editor, render_group *RenderGroup)
{
 entity *Entity = GetSelectedEntity(Editor);
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 b32 DeleteEntity = false;
 b32 CrucialEntityParamChanged = false;
 selected_entity_transform_state *SelectedTransform = &Editor->SelectedEntityTransformState;
 string_store *StrStore = Editor->StrStore;
 
 if (Editor->SelectedEntityWindow && Entity)
 {
  curve *Curve = 0;
  image *Image = 0;
  switch (Entity->Type)
  {
   case Entity_Curve: {Curve = &Entity->Curve;}break;
   case Entity_Image: {Image = &Entity->Image;}break;
   case Entity_Count: InvalidPath; break;
  }
  
  curve_params *CurveParams = &Curve->Params;
  curve_params *DefaultParams = &Editor->CurveDefaultParams;
  
  if (UI_BeginWindow(&Editor->SelectedEntityWindow, UIWindowFlag_AutoResize, StrLit("Selected Entity")))
  {
   UI_PushLabelF("SelectedEntity %u", Entity->Id);
   
   if (UI_BeginTabBar(StrLit("SelectedEntityTabBar")))
   {
    if (UI_BeginTabItem(StrLit("General")))
    {
     char_buffer *NameBuffer = GetEntityNameBuffer(Entity, StrStore);
     UI_InputText2(NameBuffer, 0, StrLit("Name"));
     
     xform2d NewXForm = Entity->XForm;
     b32 ModifyActivated = false;
     b32 ModifyDeactivated = false;
     
     UI_DragFloat2(&NewXForm.P, 0.0f, 0.0f, 0, StrLit("Position"));
     ModifyActivated |= UI_IsItemActivated();
     ModifyDeactivated |= UI_IsItemDeactivated();
     if (ResetCtxMenu(StrLit("PositionReset")))
     {
      NewXForm.P = V2(0.0f, 0.0f);
      ModifyActivated = ModifyDeactivated = true;
     }
     
     UI_AngleSlider(&NewXForm.Rotation, StrLit("Rotation"));
     ModifyActivated |= UI_IsItemActivated();
     ModifyDeactivated |= UI_IsItemDeactivated();
     if (ResetCtxMenu(StrLit("RotationReset")))
     {
      NewXForm.Rotation = Rotation2DZero();
      ModifyActivated = ModifyDeactivated = true;
     }
     
     UI_DragFloat2(&NewXForm.Scale.V, 0.0f, 0.0f, 0, StrLit("Scale"));
     ModifyActivated |= UI_IsItemActivated();
     ModifyDeactivated |= UI_IsItemDeactivated();
     if (ResetCtxMenu(StrLit("ScaleReset")))
     {
      NewXForm.Scale = Scale2DOne();
      ModifyActivated = ModifyDeactivated = true;
     }
     
     UI_Label(StrLit("DragMe"))
     {
      f32 UniformScale = 0.0f;
      UI_DragFloat(&UniformScale, 0.0f, 0.0f, "Scale Uniformly", StrLit("##Uniform Scale"));
      ModifyActivated |= UI_IsItemActivated();
      ModifyDeactivated |= UI_IsItemDeactivated();
      f32 WidthOverHeight = NewXForm.Scale.V.X / NewXForm.Scale.V.Y;
      NewXForm.Scale.V = NewXForm.Scale.V + V2(WidthOverHeight * UniformScale, UniformScale);
      if (ResetCtxMenu(StrLit("DragMeReset")))
      {
       NewXForm.Scale = Scale2DOne();
       ModifyActivated = ModifyDeactivated = true;
      }
     }
     
     if (ModifyActivated && (SelectedTransform->TransformAction == 0))
     {
      SelectedTransform->TransformAction = BeginEntityTransform(Editor, Entity);
     }
     // NOTE(hbr): Modify the Entity transform in the middle in case modify has been
     // activated and deactivated in the same frame.
     Entity->XForm = NewXForm;
     if (ModifyDeactivated && (SelectedTransform->TransformAction != 0))
     {
      EndEntityTransform(Editor, SelectedTransform->TransformAction);
      SelectedTransform->TransformAction = 0;
     }
     
#if 0     
     if (ModifyActivated && ModifyDeactivated)
     {
      // NOTE(hbr): If two things happened in the same frame, then if we already were tracking
      // something, then we should just continue. Otherwise, quickly begin tracking and end.
      if (SelectedTransform->TransformAction == 0)
      {
       tracked_action *Transform = BeginEntityTransform(Editor, Entity);
       EndEntityTransform(Editor, Transform);
      }
      else
      {
       // NOTE(hbr): Nothing to do
      }
     }
     else if (ModifyActivated && !ModifyDeactivated)
     {
      Assert(SelectedTransform->TransformAction == 0);
      if (SelectedTransform->TransformAction == 0)
      {
       SelectedTransform->TransformAction = BeginEntityTransform(Editor, Entity);
      }
     }
     else if (!ModifyActivated && ModifyDeactivated)
     {
      Assert(SelectedTransform->TransformAction);
      // NOTE(hbr): Assert but be extra safe for production. Zero-is-initialization would be great here
      // but we don't have it everywhere unfortunately.
      if (SelectedTransform->TransformAction)
      {
       
      }
     }
     else
     {
      // NOTE(hbr): Nothing to do
     }
#endif
     
     {     
      b32 Visible = IsEntityVisible(Entity);
      UI_Checkbox(&Visible, StrLit("Visible"));
      SetEntityVisibility(Entity, Visible);
     }
     
     if (Curve)
     {
      if (UsesControlPoints(Curve))
      {
       temp_arena Temp = TempArena(0);
       control_point_handle Selected = Curve->SelectedControlPoint;
       string SelectedStr = (ControlPointHandleMatch(Selected, ControlPointHandleZero()) ?
                             StrLit("None") :
                             StrF(Temp.Arena, "%u", IndexFromControlPointHandle(Selected)));
       UI_TextF(false, "Selected Control Point Index: %S", SelectedStr);
       EndTemp(Temp);
      }
      
      UI_SeparatorText(StrLit("Curve"));
      UI_LabelF("Curve")
      {
       CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(CurveParams->Type, u32), Curve_Count, CurveTypeNames, StrLit("Interpolation"));
       if (ResetCtxMenu(StrLit("InterpolationReset")))
       {
        CurveParams->Type = DefaultParams->Type;
        CrucialEntityParamChanged = true;
       }
       
       switch (CurveParams->Type)
       {
        case Curve_Polynomial: {
         polynomial_interpolation_params *Polynomial = &CurveParams->Polynomial;
         
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(Polynomial->Type, u32), PolynomialInterpolation_Count, PolynomialInterpolationTypeNames, StrLit("Variant"));
         if (ResetCtxMenu(StrLit("PolynomialReset")))
         {
          Polynomial->Type = DefaultParams->Polynomial.Type;
          CrucialEntityParamChanged = true;
         }
         
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(Polynomial->PointSpacing, u32), PointSpacing_Count, PointSpacingNames, StrLit("Point Spacing"));
         if (ResetCtxMenu(StrLit("PointSpacingReset")))
         {
          Polynomial->PointSpacing = DefaultParams->Polynomial.PointSpacing;
          CrucialEntityParamChanged = true;
         }
        }break;
        
        case Curve_CubicSpline: {
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(CurveParams->CubicSpline, u32), CubicSpline_Count, CubicSplineNames, StrLit("Variant"));
         if (ResetCtxMenu(StrLit("SplineReset")))
         {
          CurveParams->CubicSpline = DefaultParams->CubicSpline;
          CrucialEntityParamChanged = true;
         }
        }break;
        
        case Curve_Bezier: {
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(CurveParams->Bezier, u32), Bezier_Count, BezierNames, StrLit("Variant"));
         if (ResetCtxMenu(StrLit("BezierReset")))
         {
          CurveParams->Bezier = DefaultParams->Bezier;
          CrucialEntityParamChanged = true;
         }
        }break;
        
        //@ render selected parametric curve ui
        case Curve_Parametric: {
         parametric_curve_resources *Resources = &Curve->ParametricResources;
         temp_arena Temp = TempArena(0);
         
         //- predefined examples
         // TODO(hbr): This is very adhoc, refactor this
         local parametric_curve_predefined_example_type PredefinedExample = {};
         UI_Combo(SafeCastToPtr(PredefinedExample, u32),
                  ParametricCurvePredefinedExample_Count,
                  ParametricCurvePredefinedExampleNames,
                  StrLit("##Predefined Example"));
         UI_SameRow();
         UI_Disabled(PredefinedExample == ParametricCurvePredefinedExample_None)
         {
          if (UI_Button(StrLit("Load Example")))
          {
           parametric_curve_predefined_example Example = ParametricCurvePredefinedExamples[PredefinedExample];
           
           FillCharBuffer(Resources->X_Equation.Equation, Example.X_Equation);
           FillCharBuffer(Resources->Y_Equation.Equation, Example.Y_Equation);
           
           FillCharBuffer(Resources->MinT_Var.VarName, Example.Min_T.Name);
           FillCharBuffer(Resources->MaxT_Var.VarName, Example.Max_T.Name);
           
           FillCharBuffer(Resources->MinT_Var.Equation, Example.Min_T.Equation);
           FillCharBuffer(Resources->MaxT_Var.Equation, Example.Max_T.Equation);
           
           Resources->MinT_Var.EquationOrDragFloatMode_Equation = Example.Min_T.EquationMode;
           Resources->MinT_Var.DragValue = Example.Min_T.Value;
           
           Resources->MaxT_Var.EquationOrDragFloatMode_Equation = Example.Max_T.EquationMode;
           Resources->MaxT_Var.DragValue = Example.Max_T.Value;
           
           // TODO(hbr): First deallocate everything
           ForEachElement(AdditionalVarIndex, Resources->AdditionalVars)
           {
            parametric_curve_field *Var = Resources->AdditionalVars + AdditionalVarIndex;
            if (IsParametricCurveVarActive(Var))
            {
             DeallocParametricCurveVar(Resources, Var);
            }
           }
           
           ForEachElement(AdditionalVar, Example.AdditionalVars)
           {
            parametric_curve_predefined_example_var *SrcVar = Example.AdditionalVars + AdditionalVar;
            if (SrcVar->Name.Count > 0)
            {
             parametric_curve_field *DstVar = AllocParametricCurveVar(Resources);
             Assert(DstVar);
             
             FillCharBuffer(DstVar->VarName, SrcVar->Name);
             FillCharBuffer(DstVar->Equation, SrcVar->Equation);
             
             DstVar->EquationOrDragFloatMode_Equation = SrcVar->EquationMode;
             DstVar->DragValue = SrcVar->Value;
            }
           }
           
           CrucialEntityParamChanged = true;
          }
         }
         
         UI_Text(false, StrLit("Additional Vars"));
         UI_SameRow();
         
         UI_Disabled(!ParametricCurveResourcesHasFreeAddditionalVar(Resources))
         {
          if (UI_Button(StrLit("+")))
          {
           AllocParametricCurveVar(Resources);
          }
         }
         
         ForEachElement(AdditionalVarIndex, Resources->AdditionalVars)
         {
          parametric_curve_field *Field = Resources->AdditionalVars + AdditionalVarIndex;
          if (IsParametricCurveVarActive(Field))
          {
           string Label = StrF(Temp.Arena, "##Var%u", Field->Id);
           CrucialEntityParamChanged |= RenderParametricCurveFieldUI(Resources, Field, true, false, 0, Label);
          }
         }
         
         CrucialEntityParamChanged |= RenderParametricCurveFieldUI(Resources, &Resources->MinT_Var, true, true, "t_min", StrLit("##t_min"));
         CrucialEntityParamChanged |= RenderParametricCurveFieldUI(Resources, &Resources->MaxT_Var, true, true, "t_max", StrLit("##t_max"));
         CrucialEntityParamChanged |= RenderParametricCurveFieldUI(Resources, &Resources->X_Equation, false, true, "x(t)", StrLit("##x(t)"));
         CrucialEntityParamChanged |= RenderParametricCurveFieldUI(Resources, &Resources->Y_Equation, false, true, "y(t)", StrLit("##y(t)"));
         
         EndTemp(Temp);
        }break;
        
        //-@ render selected b-spline curve ui
        case Curve_BSpline: {
         b_spline_params *BSpline = &CurveParams->BSpline;
         b_spline_knot_params *KnotParams = &BSpline->KnotParams;
         curve_points_static *Points = GetCurvePoints(Curve);
         b_spline_degree_bounds Bounds = BSplineDegreeBounds(Points->ControlPointCount);
         
         CrucialEntityParamChanged |= UI_SliderInteger(SafeCastToPtr(KnotParams->Degree, i32), Bounds.MinDegree, Bounds.MaxDegree, StrLit("Degree"));
         if (ResetCtxMenu(StrLit("Degree")))
         {
          KnotParams->Degree = 1;
          CrucialEntityParamChanged = true;
         }
         
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(BSpline->Partition, u32), BSplinePartition_Count, BSplinePartitionNames, StrLit("Partition"));
         if (ResetCtxMenu(StrLit("Partition")))
         {
          BSpline->Partition = DefaultParams->BSpline.Partition;
          CrucialEntityParamChanged = true;
         }
         
         if (BSpline->Partition == BSplinePartition_Custom)
         {
          UI_Colored(UI_Color_Text, YellowColor)
          {
           UI_Text(true, StrLit("WARNING: Custom partition mode is experimental."
                                " It is meant to preserve already existing knots, "
                                "when curve shape changes (new control point, etc.)."
                                " Knots are NOT recomputed in this mode."
                                " New knots might cause unexpected curve"
                                " shapes. Adjust them manually to fix it."));
          }
         }
         
         UI_Checkbox(&CurveParams->DrawParams.BSplineKnots.Enabled, StrLit("Partition Knots Visible"));
         
         UI_Checkbox(&CurveParams->DrawParams.BSplinePartialConvexHull.Enabled, StrLit("Partial Partition Convex Hull Visible"));
         if (UI_IsItemHovered())
         {
          UI_Tooltip(StrLit("Toggle convex hull that encompasses curve\n"
                            "points between two consecutive knot points.\n"
                            "Drawn convex hull is dependent on currently\n"
                            "selected control point and a degree of the\n"
                            "B-spline curve."));
         }
         
         //-@ render b-spline knots ui
         if (UI_BeginTree(StrLit("Knots")))
         {         
          b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
          curve_points_static *Points = GetCurvePoints(Curve);
          u32 KnotCount = KnotParams.KnotCount;
          u32 Degree = KnotParams.Degree;
          u32 PartitionSize = KnotParams.PartitionSize;
          f32 A = KnotParams.A;
          f32 B = KnotParams.B;
          f32 *Knots = Points->BSplineKnots;
          for (u32 KnotIndex = 0;
               KnotIndex < KnotCount;
               ++KnotIndex)
          {
           UI_PushId(KnotIndex);
           f32 Knot = Knots[KnotIndex];
           if (KnotIndex > Degree && KnotIndex + 1 < Degree + PartitionSize)
           {
            u32 RelativeIndex = KnotIndex - Degree - 1;
            if (UI_SliderFloatF(&Knot, A, B, "Knot %u", RelativeIndex))
            {
             SetBSplineKnotPoint(&EntityWitness, BSplineKnotHandleFromKnotIndex(KnotIndex), Knot);
            }
           }
           else
           {
            UI_Disabled(true)
            {
             b32 IsBorder = ((KnotIndex == Degree) || (KnotIndex + 1 == Degree + PartitionSize));
             UI_DragFloat(&Knot, A, B, 0, (IsBorder ? StrLit("Border Knot") : StrLit("External Knot")));
            }
           }
           UI_PopId();
          }
          UI_EndTree();
         }
        }break;
        
        case Curve_Count: InvalidPath;
       }
       
       if (CurveHasWeights(Curve))
       {
        b32 WeightChanged = false;
        curve_points_static *Points = GetCurvePoints(Curve);
        u32 PointCount = Points->ControlPointCount;
        f32 *Weights = Points->ControlPointWeights;
        
        u32 Selected = 0;
        if (IsControlPointSelected(Curve))
        {
         Selected = IndexFromControlPointHandle(Curve->SelectedControlPoint);
         WeightChanged |= UI_DragFloatF(&Weights[Selected], 0.0f, FLT_MAX, 0, "Weight (%u)", Selected);
         if (ResetCtxMenu(StrLit("WeightReset")))
         {
          Weights[Selected] = 1.0f;
          WeightChanged = true;
         }
        }
        
        if (UI_BeginTree(StrLit("Weights")))
        {
         // NOTE(hbr): Limit the number of points displayed in the case
         // curve has A LOT of them
         u32 ShowCount = 100;
         u32 ToIndex = ClampTop(Selected + ShowCount/2, PointCount);
         u32 FromIndex = Selected - Min(Selected, ShowCount/2);
         u32 LeftCount = ShowCount - (ToIndex - FromIndex);
         ToIndex = ClampTop(ToIndex + LeftCount, PointCount);
         FromIndex = FromIndex - Min(FromIndex, LeftCount);
         
         for (u32 PointIndex = FromIndex;
              PointIndex < ToIndex;
              ++PointIndex)
         {
          UI_Id(PointIndex)
          {                              
           WeightChanged |= UI_DragFloatF(&Weights[PointIndex], 0.0f, FLT_MAX, 0, "Point %u", PointIndex);
           if (ResetCtxMenu(StrLit("WeightReset")))
           {
            Weights[PointIndex] = 1.0f;
            WeightChanged = true;
           }
          }
         }
         
         UI_EndTree();
        }
        
        if (WeightChanged)
        {
         CrucialEntityParamChanged = true;
        }
       }
      }
     }
     
     UI_SeparatorText(StrLit("Actions"));
     UI_Label(StrLit("Actions"));
     {
      if (UI_Button(StrLit("Duplicate")))
      {
       DuplicateEntity(Editor, Entity);
      }
      if (UI_IsItemHovered())
      {
       UI_Tooltip(StrLit("Duplicate entity"));
      }
      
      UI_SameRow();
      
      DeleteEntity = UI_Button(StrLit("Delete"));
      if (UI_IsItemHovered())
      {
       UI_Tooltip(StrLit("Delete entity"));
      }
      
      UI_SameRow();
      
      if (UI_Button(StrLit("Focus")))
      {
       FocusCameraOnEntity(&Editor->Camera, Entity, RenderGroup);
      }
      if (UI_IsItemHovered())
      {
       UI_Tooltip(StrLit("Focus the camera on entity"));
      }
      
      if (Curve)
      {
       UI_SameRow();
       
       if (UI_Button(StrLit("Adjust Size")))
       {
        rect2 AABB = EntityAABB(Entity);
        f32 Diag = Norm(AABB.Max - AABB.Min);
        f32 InvBaseFloat = SafeDiv1(1, DefaultParams->DrawParams.Line.Float);
        ForEachElement(DrawParamsIndex, CurveParams->DrawParams.All)
        {
         draw_params *DrawParams = CurveParams->DrawParams.All + DrawParamsIndex;
         f32 MultToBase = DefaultParams->DrawParams.All[DrawParamsIndex].Float * InvBaseFloat;
         DrawParams->Width = Diag * 0.006f * MultToBase;
        }
        CrucialEntityParamChanged = true;
       }
       if (UI_IsItemHovered())
       {
        UI_Tooltip(StrLit("Automatically adjust curve's line widths/point sizes,\n"
                          "based on curve size in the world space"));
       }
       
       UI_Disabled(!IsControlPointSelected(Curve))
       {
        if (UI_Button(StrLit("Split at Control Point")))
        {
         SplitCurveOnControlPoint(Editor, Entity);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Split curve into two parts - from the beginning to the\n"
                           "selected control point, and from the selected control\n"
                           "point to the end"));
        }
       }
       
       UI_SameRow();
       
       if (UI_ButtonF("Swap Append"))
       {
        Entity->Flags ^= EntityFlag_CurveAppendFront;
       }
       if (UI_IsItemHovered())
       {
        UI_Tooltip(StrLit("Swap end to which new control points are appended"));
       }
       
       UI_Disabled(!IsRegularBezierCurve(Curve))
       {
        if (UI_Button(StrLit("Elevate Degree")))
        {
         ElevateBezierCurveDegree(Editor, Entity);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Elevate Bezier curve degree, maintain its shape"));
        }
        
        UI_SameRow();
        
        if (UI_Button(StrLit("Lower Degree")))
        {
         BeginLoweringBezierCurveDegree(Editor, Entity);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Lower Bezier curve degree, maintain its shape (if possible)"));
        }
       }
      }
     }
     
     if (Curve && IsCurveEligibleForPointTracking(Curve))
     {
      point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
      f32 Fraction = Tracking->Fraction;
      b32 Changed = false;
      
      b32 BezierTrackingActive = (Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization);
      b32 SplittingTrackingActive = (Tracking->Type == PointTrackingAlongCurve_BezierCurveSplit);
      
      UI_Label(StrLit("DeCasteljauVisualization"))
      {
       if (UI_Checkbox(&BezierTrackingActive, StrLit("##DeCasteljauEnabled")))
       {
        Changed = true;
        Tracking->Type = (BezierTrackingActive ?
                          PointTrackingAlongCurve_DeCasteljauVisualization :
                          PointTrackingAlongCurve_None);
       }
       UI_SameRow();
       UI_SeparatorText(StrLit("De Casteljau's Algorithm"));
       
       if (BezierTrackingActive)
       {
        Changed |= UI_SliderFloat(&Fraction, 0.0f, 1.0f, StrLit("t"));
       }
      }
      
      UI_Label(StrLit("BezierSplitting"))
      {
       if (UI_Checkbox(&SplittingTrackingActive, StrLit("##SplittingEnabled")))
       {
        Changed = true;
        Tracking->Type = (SplittingTrackingActive ?
                          PointTrackingAlongCurve_BezierCurveSplit :
                          PointTrackingAlongCurve_None);
       }
       UI_SameRow();
       UI_SeparatorText(StrLit("Split Bezier Curve"));
       
       if (SplittingTrackingActive)
       {
        Changed |= UI_SliderFloat(&Fraction, 0.0f, 1.0f, StrLit("t"));
        UI_SameRow();
        if (UI_Button(StrLit("Split!")))
        {
         PerformBezierCurveSplit(Editor, Entity);
        }
       }
      }
      
      if (Tracking->Type && Changed)
      {
       SetTrackingPointFraction(&EntityWitness, Fraction);
      }
     }
     
     UI_EndTabItem();
    }
    
    if (Curve)
    {
     if (UI_BeginTabItem(StrLit("Settings")))
     {
      UI_Label(StrLit("Line"))
      {
       CrucialEntityParamChanged |= RenderDrawParamsUI(StrLit("Line"),
                                                       &CurveParams->DrawParams.Line,
                                                       &DefaultParams->DrawParams.Line,
                                                       false);
       
       if (IsCurveTotalSamplesMode(Curve))
       {
        CrucialEntityParamChanged |= UI_SliderInteger(SafeCastToPtr(CurveParams->TotalSamples, i32), 1, 5000, StrLit("Total Samples"));
        if (ResetCtxMenu(StrLit("Samples")))
        {
         CurveParams->TotalSamples = DefaultParams->TotalSamples;
         CrucialEntityParamChanged = true;
        }
       }
       else
       {
        CrucialEntityParamChanged |= UI_SliderInteger(SafeCastToPtr(CurveParams->SamplesPerControlPoint, i32), 1, 500, StrLit("Samples per Control Point"));
        if (ResetCtxMenu(StrLit("Samples")))
        {
         CurveParams->SamplesPerControlPoint = DefaultParams->SamplesPerControlPoint;
         CrucialEntityParamChanged = true;
        }
       }
      }
      
      if (UsesControlPoints(Curve))
      {
       RenderDrawParamsUI(StrLit("Control Points"),
                          &CurveParams->DrawParams.Points,
                          &DefaultParams->DrawParams.Points,
                          true);
       
       CrucialEntityParamChanged |= RenderDrawParamsUI(StrLit("Polyline"),
                                                       &CurveParams->DrawParams.Polyline,
                                                       &DefaultParams->DrawParams.Polyline,
                                                       false);
       
       CrucialEntityParamChanged |= RenderDrawParamsUI(StrLit("Convex Hull"),
                                                       &CurveParams->DrawParams.ConvexHull,
                                                       &DefaultParams->DrawParams.ConvexHull,
                                                       false);
      }
      
      if (IsBSplineCurve(Curve))
      {
       CrucialEntityParamChanged |= RenderDrawParamsUI(StrLit("B-Spline Knots"),
                                                       &CurveParams->DrawParams.BSplineKnots,
                                                       &DefaultParams->DrawParams.BSplineKnots,
                                                       true);
       
       CrucialEntityParamChanged |= RenderDrawParamsUI(StrLit("B-Spline Partial Convex Hull"),
                                                       &CurveParams->DrawParams.BSplinePartialConvexHull,
                                                       &DefaultParams->DrawParams.BSplinePartialConvexHull,
                                                       false);
      }
      
      UI_EndTabItem();
     }
    }
    
    if (UI_BeginTabItem(StrLit("Details")))
    {
     if (Curve)
     {
      curve_points_static *Points = GetCurvePoints(Curve);
      UI_TextF(false, "Number of control points  %u", Points->ControlPointCount);
      UI_TextF(false, "Number of samples         %u", Curve->CurveSampleCount);
     }
     
     if (Image)
     {
      string ImageFilePath = StringFromStringId(Image->FilePath);
      UI_TextF(false, "File path  %S", ImageFilePath);
      UI_TextF(false, "Width      %u pixels", Image->OriginalWidth);
      UI_TextF(false, "Height     %u pixels", Image->OriginalHeight);
     }
     
     UI_EndTabItem();
    }
    
    UI_EndTabBar();
   }
   
   UI_PopLabel();
  }
  UI_EndWindow();
 }
 
 if (CrucialEntityParamChanged)
 {
  MarkEntityModified(&EntityWitness);
 }
 
 EndEntityModify(EntityWitness);
 
 if (DeleteEntity)
 {
  RemoveEntity(Editor, Entity);
 }
}

internal void
RenderMenuBarUI(editor *Editor)
{
 temp_arena Temp = TempArena(0);
 if (UI_BeginMainMenuBar())
 {
  if (UI_BeginMenu(StrLit("File")))
  {
   editor_command Cmds[] =
   {
    EditorCommand_New,
    EditorCommand_Open,
    EditorCommand_Save,
    EditorCommand_SaveAs,
    EditorCommand_Quit,
   };
   ForEachElement(CmdIndex, Cmds)
   {
    editor_command Cmd = Cmds[CmdIndex];
    editor_keyboard_shortcut_group *Group = EditorKeyboardShortcuts + Cmd;
    string_list ShortcutStrsList = {};
    ForEachIndex(ShortcutIndex, Group->Count)
    {
     editor_keyboard_shortcut *Shortcut = Group->Shortcuts + ShortcutIndex;
     string ShortcutStr = PlatformKeyCombinationToString(Temp.Arena, Shortcut->Key, Shortcut->Modifiers);
     StrListPush(Temp.Arena, &ShortcutStrsList, ShortcutStr);
    }
    string_list_join_options JoinOpts = {};
    JoinOpts.Sep = StrLit("|");
    string ShortcutString = StrListJoin(Temp.Arena, &ShortcutStrsList, JoinOpts);
    string CommandString = EditorCommandNames[Cmd];
    if (UI_MenuItem(0, ShortcutString, CommandString))
    {
     PushEditorCmd(Editor, Cmd);
    }
   }
   UI_EndMenu();
  }
  
  if(UI_BeginMenu(StrLit("Actions")))
  {
   if (UI_MenuItem(0, NilStr, StrLit("Animate Curves")))
   {
    BeginAnimatingCurves(&Editor->AnimatingCurves);
   }
   if (UI_MenuItem(0, NilStr, StrLit("Merge Curves")))
   {
    BeginMergingCurves(&Editor->MergingCurves);
   }
   UI_EndMenu();
  }
  
  if (UI_BeginMenu(StrLit("View")))
  {
   UI_MenuItem(&Editor->EntityListWindow, NilStr, StrLit("Entity List"));
   UI_MenuItem(&Editor->SelectedEntityWindow, NilStr, StrLit("Selected Entity"));
#if BUILD_DEBUG
   UI_MenuItem(&Editor->DiagnosticsWindow, NilStr, StrLit("Diagnostics"));
   UI_MenuItem(&Editor->ProfilerWindow, NilStr, StrLit("Profiler"));
#endif
   UI_MenuItem(&Editor->Grid, NilStr, StrLit("Grid"));
   UI_EndMenu();
  }
  
  if(UI_BeginMenu(StrLit("Settings")))
  {
   UI_SeparatorText(StrLit("Camera"));
   {
    camera *Camera = &Editor->Camera;
    UI_DragFloat2(&Camera->P, 0.0f, 0.0f, 0, StrLit("Position"));
    if (ResetCtxMenu(StrLit("PositionReset")))
    {
     TranslateCamera(Camera, -Camera->P);
    }
    UI_AngleSlider(&Camera->Rotation, StrLit("Rotation"));
    if (ResetCtxMenu(StrLit("RotationReset")))
    {
     RotateCameraAround(Camera, Rotation2DInverse(Camera->Rotation), Camera->P);
    }
    UI_DragFloat(&Camera->Zoom, 0.0f, 0.0f, 0, StrLit("Zoom"));
    if (ResetCtxMenu(StrLit("ZoomReset")))
    {
     SetCameraZoom(Camera, 1.0f);
    }
   }
   
   UI_SeparatorText(StrLit("Misc"));
   {
    UI_ColorPicker(&Editor->BackgroundColor, StrLit("Background Color"));
    if (ResetCtxMenu(StrLit("BackgroundColorReset")))
    {
     Editor->BackgroundColor = Editor->DefaultBackgroundColor;
    }
   }
   
   UI_EndMenu();
  }
  
  UI_MenuItem(&Editor->HelpWindow, NilStr, StrLit("Help"));
  
  UI_EndMainMenuBar();
 }
 EndTemp(Temp);
}

internal void
RenderEntityListWindowContents(editor *Editor, render_group *RenderGroup)
{
 string_store *StrStore = Editor->StrStore;
 
 ForEachEnumVal(EntityType, Entity_Count, entity_type)
 {
  entity_array Entities = EntityArrayFromType(Editor->EntityStore, EntityType);
  
  string EntityTypeLabel = {};
  switch (EntityType)
  {
   case Entity_Curve: {EntityTypeLabel = StrLit("Curve"); }break;
   case Entity_Image: {EntityTypeLabel = StrLit("Image"); }break;
   case Entity_Count: InvalidPath; break;
  }
  
  UI_PushLabel(EntityTypeLabel);
  
  if (UI_CollapsingHeader(EntityTypeLabel))
  {
   for (u32 EntityIndex = 0;
        EntityIndex < Entities.Count;
        ++EntityIndex)
   {
    entity *Entity = Entities.Entities[EntityIndex];
    UI_PushId(EntityIndex);
    b32 Selected = IsEntitySelected(Entity);
    
    if (UI_SelectableItem(Selected, GetEntityName(Entity)))
    {
     SelectEntity(Editor, Entity);
    }
    
    if (UI_IsItemHovered() && UI_IsMouseDoubleClicked(UIMouseButton_Left))
    {
     FocusCameraOnEntity(&Editor->Camera, Entity, RenderGroup);
    }
    
    string CtxMenu = StrLit("EntityContextMenu");
    if (UI_IsItemHovered() && UI_IsMouseClicked(UIMouseButton_Right))
    {
     UI_OpenPopup(CtxMenu);
    }
    if (UI_BeginPopup(CtxMenu, 0))
    {
     if(UI_MenuItem(0, NilStr, StrLit("Delete")))
     {
      RemoveEntity(Editor, Entity);
     }
     if(UI_MenuItem(0, NilStr, StrLit("Duplicate")))
     {
      DuplicateEntity(Editor, Entity);
     }
     if(UI_MenuItem(0, NilStr, (Entity->Flags & EntityFlag_Hidden) ? StrLit("Show") : StrLit("Hide")))
     {
      Entity->Flags ^= EntityFlag_Hidden;
     }
     if (UI_MenuItem(0, NilStr, (Selected ? StrLit("Deselect") : StrLit("Select"))))
     {
      SelectEntity(Editor, (Selected ? 0 : Entity));
     }
     if(UI_MenuItem(0, NilStr, StrLit("Focus")))
     {
      FocusCameraOnEntity(&Editor->Camera, Entity, RenderGroup);
     }
     
     UI_EndPopup();
    }
    
    UI_PopId();
   }
  }
  
  UI_PopLabel();
 }
}
internal void
RenderEntityListUI(editor *Editor, render_group *RenderGroup)
{
 if (Editor->EntityListWindow)
 {
  if (UI_BeginWindow(&Editor->EntityListWindow, UIWindowFlag_AutoResize, StrLit("Entities")))
  {
   RenderEntityListWindowContents(Editor, RenderGroup);
  }
  UI_EndWindow();
 }
}

internal void
UpdateFrameStats(frame_stats *Stats, platform_input_output *Input)
{
 // NOTE(hbr): Access directly (without setting RefreshRequested) purposefully
 f32 dt = ExtractDeltaTimeOnly(Input);
 
 Stats->Calculation.FrameCount += 1;
 Stats->Calculation.SumFrameTime += dt;
 Stats->Calculation.MinFrameTime = Min(Stats->Calculation.MinFrameTime, dt);
 Stats->Calculation.MaxFrameTime = Max(Stats->Calculation.MaxFrameTime, dt);
 
 if (Stats->Calculation.SumFrameTime >= 1.0f)
 {
  Stats->FPS = Stats->Calculation.FrameCount / Stats->Calculation.SumFrameTime;
  Stats->MinFrameTime = Stats->Calculation.MinFrameTime;
  Stats->MaxFrameTime = Stats->Calculation.MaxFrameTime;
  Stats->AvgFrameTime = Stats->Calculation.SumFrameTime / Stats->Calculation.FrameCount;
  
  Stats->Calculation.FrameCount = 0;
  Stats->Calculation.MinFrameTime = F32_INF;
  Stats->Calculation.MaxFrameTime = -F32_INF;
  Stats->Calculation.SumFrameTime = 0.0f;
 }
}

internal void
RenderDiagnosticsUI(editor *Editor, platform_input_output *Input)
{
 //- render diagnostics UI
 if (Editor->DiagnosticsWindow)
 {
  if (UI_BeginWindow(&Editor->DiagnosticsWindow, UIWindowFlag_AutoResize, StrLit("Diagnostics")))
  {
   frame_stats *Stats = &Editor->FrameStats;
   f32 dt = ExtractDeltaTimeOnly(Input);
   
   UI_TextF(false, "%-20s %.2f ms", "Frame time", 1000.0f * dt);
   UI_TextF(false, "%-20s %.0f", "FPS", Stats->FPS);
   UI_TextF(false, "%-20s %.2f ms", "Min frame time", 1000.0f * Stats->MinFrameTime);
   UI_TextF(false, "%-20s %.2f ms", "Max frame time", 1000.0f * Stats->MaxFrameTime);
   UI_TextF(false, "%-20s %.2f ms", "Average frame time", 1000.0f * Stats->AvgFrameTime);
  }
  UI_EndWindow();
 }
}

internal void
RenderHelpUI(editor *Editor)
{
 temp_arena Temp = TempArena(0);
 if (Editor->HelpWindow)
 {
  if (UI_BeginWindow(&Editor->HelpWindow, UIWindowFlag_AutoResize, StrLit("Help")))
  {
   UI_TextF(false, "Welcome to");
   UI_SameRow();
   UI_Colored(UI_Color_Text, GreenColor)
   {
    UI_TextF(false, "%S", EditorAppName);
   }
   UI_SameRow();
   UI_TextF(false, "- parametric curve editor. Here is the editor's quick overview and tutorial.");
   
   UI_NewRow();
   UI_Text(false,
           StrLit("Controls are made to be as intuitive as possible. Read this tutorial, but\n"
                  "there is no need to understand everything. Note which keys and mouse buttons\n"
                  "might do something interesting and go experiment as soon as possible instead."));
   UI_NewRow();
   UI_Text(false,
           StrLit("NOTE: Most things that apply to curves and curve manipulation (i.e.\n"
                  "move/select/delete) also apply to images, or in general - entities.\n\n"));
   
   // NOTE(hbr): I have no fucking idea why I need to pass true here and not false what I would expect.
   // It's literally the opposite behaviour of what I'm expecting even thgough I checked ImGui expectes
   // "is_open" and not "is_not_open" and I'm passing it correctly. Fuck that.
   UI_SetNextItemOpen(true, UICond_Once);
   if (UI_CollapsingHeader(StrLit("UI tutorial")))
   {
    UI_Text(false, StrLit("ImGui was used to create UI for this application. Those already\n"
                          "familiar with this library can skip this tutorial. Otherwise, let's\n"
                          "go through basic UI widgets and functionalities used throughout the\n"
                          "application:"));
    
    UI_SeparatorText(StrLit("Widgets"));
    
    UI_Text(false, StrLit("HINT: Right click on different widgets and see what happens!"));
    
    //- buttons/checkboxes
    {    
     local rgba Colors[2] = { NiceGreenColor, NiceRedColor };
     local u32 ColorIndex = 0;
     UI_Colored(UI_Color_Item, Colors[ColorIndex])
     {
      if (UI_Button(StrLit("Click me!")))
      {
       ColorIndex = 1-ColorIndex;
      }
      if (ResetCtxMenu(StrLit("ResetButton"))) {ColorIndex = 0;}
     }
     UI_SameRow();
     UI_Disabled(true)
     {
      UI_Button(StrLit("Disabled Button"));
     }
     UI_SameRow();
     UI_Button(StrLit("Hover over me!"));
     if (UI_IsItemHovered())
     {
      UI_Tooltip(StrLit("Hopefully some useful details here!"));
     }
     
     local b32 Checkbox = false;
     string Label = (Checkbox ? StrLit("Click to disable!") : StrLit("Click to enable!"));
     UI_Checkbox(&Checkbox, Label);
     if (ResetCtxMenu(StrLit("ResetCheckbox"))) {Checkbox = false;}
     
     UI_HelpMarkerWithTooltip(StrLit("Hello, World!"));
     UI_SameRow();
     UI_Text(false, StrLit("Hover over the question mark!"));
    }
    
    //- text input
    {
     local char Buffer[128];
     local char_buffer CharBuffer = MakeCharBuffer(Buffer, ArrayCount(Buffer));
     UI_InputText2(&CharBuffer, 0, StrLit("Type something!"));
     if (ResetCtxMenu(StrLit("ResetInput"))) {StructZero(Buffer);}
    }
    
    //- slider
    {
     local i32 Integer = 0;
     UI_SliderInteger(&Integer, 0, 50, StrLit("Slide the slider!"));
     if (ResetCtxMenu(StrLit("ResetSlider"))) {Integer = 0;}
    }
    
    //- drag
    {
     local f32 Scalar = 0;
     local v2 Vector = {};
     UI_DragFloat(&Scalar, 0, 0, 0, StrLit("Drag to adjust scalar value!"));
     if (ResetCtxMenu(StrLit("ResetDrag"))) {StructZero(&Scalar);}
     UI_DragFloat2(&Vector, 0, 0, 0, StrLit("Drag to adjust vector values!"));
     if (ResetCtxMenu(StrLit("ResetDrag2"))) {StructZero(&Vector);}
     UI_Text(false, StrLit("HINT: Double click on the \"drag\" field to input the value directly!"));
    }
    
    //- color picker
    {
     local rgba Color = {};
     UI_ColorPicker(&Color, StrLit("Pick color!"));
     if (ResetCtxMenu(StrLit("ResetColor"))) {StructZero(&Color);}
     UI_Text(false, StrLit("HINT: Click on the small color preview to open color picker!"));
     UI_Text(false, StrLit("HINT: You can also \"drag\" individual RGBA components!"));
    }
    
    //- combo
    {
     local u32 Value = 0;
     local string ValueNames[] = {
      StrLit("Variant A"),
      StrLit("Variant B"),
      StrLit("Variant C"),
      StrLit("Special Variant"),
     };
     local u32 ValueCount = ArrayCount(ValueNames);
     UI_Combo(&Value, ValueCount, ValueNames, StrLit("Click on me to pick variant!"));
     if (ResetCtxMenu(StrLit("ResetCombo"))) {StructZero(&Value);}
    }
    
    //- tree
    {  
     if (UI_BeginTree(StrLit("Click to expand/collapse!##Tree")))
     {
      UI_BulletText(StrLit("Bullet A"));
      local i32 Slider = 0;
      UI_SliderInteger(&Slider, -25, 25, StrLit("Slider"));
      if (ResetCtxMenu(StrLit("ResetSliderTree"))) {StructZero(&Slider);}
      local char Buffer[128];
      local char_buffer Input = MakeCharBuffer(Buffer, ArrayCount(Buffer));
      UI_InputText2(&Input, 0, StrLit("Input"));
      if (ResetCtxMenu(StrLit("ResetInputTree"))) {StructZero(Buffer);}
      UI_BulletText(StrLit("Bullet B"));
      UI_EndTree();
     }
    }
    
    //- selectables
    {
     if (UI_CollapsingHeader(StrLit("Click to expand/collapse!##CollapsingHeader")))
     {
      local b32 Selected[3];
      ForEachElement(Index, Selected)
      {
       UI_PushId(Cast(u32)Index);
       b32 Sel = Selected[Index];
       if (UI_SelectableItem(Sel, StrLit("Click on me to (de)select!")))
       {
        Selected[Index] = !Selected[Index];
       }
       UI_PopId();
      }
      local b32 SpecialSelected = false;
      if (UI_SelectableItem(SpecialSelected,
                            StrLit("Right click on me!")))
      {
       SpecialSelected = !SpecialSelected;
      }
      string CtxMenu = StrLit("EntityContextMenu");
      if (UI_IsItemHovered() && UI_IsMouseClicked(UIMouseButton_Right))
      {
       UI_OpenPopup(CtxMenu);
      }
      local b32 IsMsg = false;
      local string Msg = {};
      if (UI_BeginPopup(CtxMenu, 0))
      {
       if(UI_MenuItem(0, NilStr, StrLit("Action A")))
       {
        IsMsg = true;
        Msg = StrLit("Action A chosen!");
       }
       if(UI_MenuItem(0, NilStr, StrLit("Action B")))
       {
        IsMsg = true;
        Msg = StrLit("Action B chosen!");
       }
       if(UI_MenuItem(0, NilStr, StrLit("Action C")))
       {
        IsMsg = true;
        Msg = StrLit("Action C chosen!");
       }
       UI_EndPopup();
      }
      if (IsMsg)
      {
       UI_Text(false, Msg);
      }
     }
    }
    
    //- windows
    {
     local b32 WindowIsOpen = false;
     if (UI_Button(StrLit("Click me to spawn a new window!")))
     {
      WindowIsOpen = true;
     }
     if (WindowIsOpen)
     {
      if (UI_BeginWindow(&WindowIsOpen, 0, StrLit("UI Tutorial window (drag me around!)")))
      {
       UI_Text(false, StrLit("HINT: Click on the downward-pointing arrow in the top-left\n"
                             "corner to \"collapse/expand\" the window."));
       UI_Text(false, StrLit("HINT: Use the handle in the bottom-right corner to resize the window."));
       UI_Text(false, StrLit("HINT: Use \"X\" button in the top-right corner to close the window."));
       UI_Text(false, StrLit("HINT: Drag the window to move it around."));
      }
      UI_EndWindow();
     }
     
    }
    
    //- tabs
    {    
     UI_BeginTabBar(StrLit("Tabs"));
     if (UI_BeginTabItem(StrLit("Tab A (click on me!)")))
     {
      UI_Text(false, StrLit("This is content of Tab A"));
      UI_EndTabItem();
     }
     
     if (UI_BeginTabItem(StrLit("Tab B (click on me!)")))
     {
      UI_Text(false, StrLit("Entirely different content - Tab B"));
      UI_EndTabItem();
     }
     UI_EndTabBar();
    }
    
    UI_NewRow();
   }
   
   UI_SetNextItemOpen(false, UICond_Once);
   if (UI_CollapsingHeader(StrLit("Keyboard shortcuts")))
   {
    if (UI_BeginTable(2, StrLit("KeyboardShortcuts")))
    {
     ForEachEnumVal(EditorCmd, EditorCommand_Count, editor_command)
     {
      editor_keyboard_shortcut_group *Group = EditorKeyboardShortcuts + EditorCmd;
      if (!Group->DevSpecific)
      {
       string Description = NilStr;
       switch (EditorCmd)
       {
        case EditorCommand_New: {
         Description = StrLit("New project");
        }break;
        case EditorCommand_Open: {
         Description = StrLit("Open file (new project or image)");
        }break;
        case EditorCommand_Save: {
         Description = StrLit("Save project");
        }break;
        case EditorCommand_SaveAs: {
         Description = StrLit("Save project as");
        }break;
        case EditorCommand_Quit: {
         Description = StrLit("Quit");
        }break;
        case EditorCommand_ToggleDevConsole: {
         Description = StrLit("Toggle developer console");
        }break;
        case EditorCommand_Delete: {
         Description = StrLit("Delete currently selected curve control point or currently\nselected entity (depends on what is the current selection)");
        }break;
        case EditorCommand_Duplicate: {
         Description = StrLit("Duplicate selected entity");
        }break;
        case EditorCommand_ToggleProfiler: {
         Description = StrLit("Toggle profiling frames");
        }break;
        case EditorCommand_Undo: {
         Description = StrLit("Undo action");
        }break;
        case EditorCommand_Redo: {
         Description = StrLit("Redo action");
        }break;
        case EditorCommand_ToggleUI: {
         Description = StrLit("Toggle UI");
        }break;
        case EditorCommand_ToggleFullscreen: {
         Description = StrLit("Toggle fullscreen");
        }break;
        
        case EditorCommand_Count: InvalidPath;
       }
       
       UI_TableNextRow();
       UI_TableSetColumnIndex(0);
       UI_Text(false, Description);
       UI_TableSetColumnIndex(1);
       ForEachIndex(ShortcutIndex, Group->Count)
       {
        editor_keyboard_shortcut *Shortcut = Group->Shortcuts + ShortcutIndex;
        string ShortcutString = PlatformKeyCombinationToString(Temp.Arena, Shortcut->Key, Shortcut->Modifiers);
        if (ShortcutIndex > 0)
        {
         UI_SameRow();
         UI_Text(false, StrLit("or"));
         UI_SameRow();
        }
        UI_Text(false, ShortcutString);
       }
      }
     }
     UI_EndTable();
    }
   }
   
   UI_SetNextItemOpen(true, UICond_Once);
   if (UI_CollapsingHeader(StrLit("Mouse controls")))
   {
    if (UI_BeginTree(StrLit("Left button")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Add/move/select control points."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more details:"));
     UI_BulletText(StrLit("clicking on curve control point selects that curve and that control point"));
     UI_BulletText(StrLit("clicking on curve line inserts control point inside that curve"));
     UI_BulletText(StrLit("clicking outside of curve (\"on nothing\"), while curve is selected, appends new control point to that curve"));
     UI_BulletText(StrLit("clicking outside of curve (\"on nothing\"), while no curve is selected, creates a new curve with one control\n"
                          "point in that spot"));
     UI_BulletText(StrLit("notice that left mouse click always adds new control point or selects existing one"));
     UI_BulletText(StrLit("clicking and holding moves that control point until mouse button is released"));
     UI_BulletText(StrLit("while splitting Bezier curve, clicking and holding on split point, moves that point along that curve"));
     UI_BulletText(StrLit("while visualizing De Casteljau's Algorithm, clicking and holding final point, moves that point along that curve"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Left button + Left Ctrl")))
    {
     UI_NewRow();
     UI_Text(false,
             StrLit("Work on curve level, not individual control points level. Move/select\n"
                    "curves themselves."));
     UI_NewRow();
     
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking on curve (either curve line or curve control point) selects it"));
     UI_BulletText(StrLit("clicking and holding anywhere while curve is selected, moves it"));
     UI_BulletText(StrLit("clicking and holding while no curve is selected, results in the same action\n"
                          "as if Left Ctrl was not held - new curve with one control point is added"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Left button + Left Alt")))
    {
     UI_NewRow();
     UI_Text(false,
             StrLit("When creating precise curve shape, one sometimes wants to add new control point\n"
                    "that might be close or directly on the curve line. Normally clicking there, inserts\n"
                    "new control point inside that curve. Use Left Alt to override it and *always*\n"
                    "append instead."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking anywhere while curve is selected, *always* appends a new control point to it"));
     UI_BulletText(StrLit("clicking anywhere while no curve is selected, creates a new curve with one control point in that spot"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Right button")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Remove/deselect control points."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking on control point deletes it and selects that curve"));
     UI_BulletText(StrLit("clicking outside of curve (\"on nothing\") deselects currently selected curve (if one exists)"));
     UI_BulletText(StrLit("while splitting Bezier curve, clicking on split point, actually splits the curve"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Right button + Left Control")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Remove/deselect curves."));
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Middle button (+ Left Ctrl)")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Manipulate the camera."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking and holding moves the camera"));
     UI_BulletText(StrLit("clicking and holding with Left Ctrl rotates the camera around"));
     UI_BulletText(StrLit("scrolling (un)zooms the camera"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
   }
  }
  UI_EndWindow();
 }
 EndTemp(Temp);
}

internal void
UpdateAndRenderNotifications(editor *Editor, platform_input_output *Input, render_group *RenderGroup)
{
 string_store *StrStore = Editor->StrStore;
 b32 ActiveNotificationExists = false;
 
 UI_Label(StrLit("Notifications"))
 {
  v2u WindowDim = RenderGroup->Frame->WindowDim;
  f32 Padding = 0.01f * WindowDim.X;
  f32 TargetPosY = WindowDim.Y - Padding;
  f32 WindowWidth = 0.1f * WindowDim.X;
  
  for (u32 NotificationIndex = 0;
       NotificationIndex < MAX_NOTIFICATION_COUNT;
       ++NotificationIndex)
  {
   notification *Notification = Editor->Notifications + NotificationIndex;
   if (Notification->Type != Notification_None)
   {
    b32 Remove = false;
    
    ActiveNotificationExists = true;
    
    f32 dt = UseAndExtractDeltaTime(Input);
    Notification->LifeTime += dt;
    
    f32 MoveSpeed = 20.0f;
    f32 NextPosY = Lerp(Notification->ScreenPosY,
                        TargetPosY,
                        1.0f - PowF32(2.0f, -MoveSpeed * dt));
    Notification->ScreenPosY = NextPosY;
    
    enum notification_phase
    {
     NotificationPhase_FadeIn,
     NotificationPhase_ProperLife,
     NotificationPhase_FadeOut,
     NotificationPhase_Invisible,
     NotificationPhase_Count,
    };
    local f32 Phases[NotificationPhase_Count] = {
     0.15f, // fade in
     10.0f, // proper lifetime
     0.15f, // fade out
     0.1f,  // invisible
    };
    f32 PhaseFraction = Notification->LifeTime;
    u32 PhaseIndex = 0;
    for (; PhaseIndex < NotificationPhase_Count; ++PhaseIndex)
    {
     if (PhaseFraction <= Phases[PhaseIndex])
     {
      break;
     }
     PhaseFraction -= Phases[PhaseIndex];
    }
    f32 Fade = 0.0f;
    switch (Cast(notification_phase)PhaseIndex)
    {
     case NotificationPhase_FadeIn:     { Fade = Map01(PhaseFraction, 0.0f, Phases[NotificationPhase_FadeIn]); }break;
     case NotificationPhase_ProperLife: { Fade = 1.0f; }break;
     case NotificationPhase_FadeOut:    { Fade = 1.0f - Map01(PhaseFraction, 0.0f, Phases[NotificationPhase_FadeOut]); } break;
     case NotificationPhase_Invisible:  { Fade = 0.0f; } break;
     case NotificationPhase_Count:      { Remove = true; } break;
    }
    // NOTE(hbr): Quadratic interpolation instead of linear.
    Fade = 1.0f - Sqr(1.0f - Fade);
    
    if (!Editor->HideUI)
    {    
     v2 WindowP = V2(WindowDim.X - Padding, NextPosY);
     v2 WindowMinSize = V2(WindowWidth, 0.0f);
     v2 WindowMaxSize = V2(WindowWidth, FLT_MAX);
     UI_SetNextWindowPos(WindowP, UIPlacement_BotRightCorner);
     UI_SetNextWindowSizeConstraints(WindowMinSize, WindowMaxSize);
     
     UI_Alpha(Fade)
     {
      DeferBlock(UI_BeginWindowF(0, UIWindowFlag_AutoResize | UIWindowFlag_NoTitleBar | UIWindowFlag_NoFocusOnAppearing,
                                 "Notification%lu", NotificationIndex),
                 UI_EndWindow())
      {
       UI_BringCurrentWindowToDisplayFront();
       if (UI_IsWindowHovered() && (UI_IsMouseClicked(UIMouseButton_Left) ||
                                    UI_IsMouseClicked(UIMouseButton_Right)))
       {
        Remove = true;
       }
       
       string Title = {};
       rgba TitleColor = {};
       switch (Notification->Type)
       {
        case Notification_Success: { Title = StrLit("Success"); TitleColor = GreenColor; } break;
        case Notification_Error:   { Title = StrLit("Error");   TitleColor = RedColor; } break;
        case Notification_Warning: { Title = StrLit("Warning"); TitleColor = YellowColor; } break;
        case Notification_None: InvalidPath; break;
       }
       
       UI_Colored(UI_Color_Text, TitleColor)
       {
        UI_Text(false, Title);
       }
       
       UI_HorizontalSeparator();
       UI_Text(true, Notification->Content);
       
       f32 WindowHeight = UI_GetWindowHeight();
       TargetPosY -= WindowHeight + Padding;
      }
     }
    }
    
    if (Remove)
    {
     Notification->Type = Notification_None;
    }
   }
  }
 }
 
 if (!ActiveNotificationExists)
 {
  ClearArena(Editor->NotificationsArena);
 }
}

internal void
UpdateAndRenderChoose2CurvesUI(choose_2_curves_state *Choosing, editor *Editor)
{
 entity *Curve0 = EntityFromHandle(Choosing->Curves[0]);
 entity *Curve1 = EntityFromHandle(Choosing->Curves[1]);
 entity *Curves[2] = { Curve0, Curve1 };
 string_store *StrStore = Editor->StrStore;
 
 b32 ChoosingCurve = Choosing->WaitingForChoice;
 
 for (u32 Index = 0;
      Index < 2;
      ++Index)
 {
  b32 Disable = false;
  string Button = {};
  rgba Color = {};
  b32 IsColor = false;
  if (ChoosingCurve && Choosing->ChoosingCurveIndex == Index)
  {
   Disable = true;
   Button = StrLit("Click on curve!");
   IsColor = true;
   Color = RGBA_U8(150, 50, 50);
  }
  else
  {
   if (Curves[Index])
   {
    Button = GetEntityName(Curves[Index]);
   }
   else
   {
    Button = StrLit("...");
    IsColor = true;
    Color = RGBA_Gray(70);
   }
  }
  
  UI_Id(Index)
  {
   UI_TextF(false, "Curve %u: ", Index);
   
   UI_SameRow();
   
   ui_push_color_handle PushHandle = {};
   if (IsColor)
   {
    PushHandle = UI_PushColor(UI_Color_Item, Color);
   }
   if (UI_Button(Button))
   {
    if (Disable)
    {
     Choosing->WaitingForChoice = false;
    }
    else
    {
     Choosing->WaitingForChoice = true;
     Choosing->ChoosingCurveIndex = Index;
    }
   }
   if (IsColor)
   {
    UI_PopColor(PushHandle);
   }
   
   UI_SameRow();
   
   UI_Disabled(Curves[Index] == 0)
   {
    if (UI_Button(StrLit("Select")))
    {
     Assert(Curves[Index]);
     SelectEntity(Editor, Curves[Index]);
    }
   }
  }
 }
 
 UI_Disabled((Curve0 == 0) || (Curve1 == 0))
 {
  b32 Hidden = false;
  if (Curve0 && Curve1)
  {
   Hidden = (!IsEntityVisible(Curve0) && !IsEntityVisible(Curve1));
  }
  
  if (UI_Checkbox(&Hidden, StrLit("Hide Curves")))
  {
   Assert(Curve0);
   Assert(Curve1);
   SetEntityVisibility(Curve0, !Hidden);
   SetEntityVisibility(Curve1, !Hidden);
  }
 }
}

internal void
RenderAnimatingCurvesUI(editor *Editor)
{
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 string_store *StrStore = Editor->StrStore;
 
 if (Animation->Flags & AnimatingCurves_Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindow(&WindowOpen, UIWindowFlag_AutoResize, StrLit("Curve Animation"));
  
  if (WindowOpen)
  {
   UI_SeparatorText(StrLit("Choice"));
   UpdateAndRenderChoose2CurvesUI(&Animation->Choose2Curves, Editor);
   
   entity *Entity0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
   entity *Entity1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
   
   UI_SeparatorText(StrLit("Animation"));
   if (UI_SliderFloat(&Animation->Bouncing.T, 0.0f, 1.0f, StrLit("t")))
   {
    Animation->Flags &= ~AnimatingCurves_Animating;
   }
   UI_SameRow();
   if (Animation->Flags & AnimatingCurves_Animating)
   {
    if (UI_Button(StrLit("Stop")))
    {
     Animation->Flags &= ~AnimatingCurves_Animating;
    }
   }
   else
   {
    UI_Disabled((Entity0 == 0) || (Entity1 == 0))
    {
     if (UI_Button(StrLit("Start")))
     {
      Animation->Flags |= AnimatingCurves_Animating;
     }
    }
   }
   UI_SliderFloat(&Animation->Bouncing.Speed, 0.0f, 4.0f, StrLit("Animation Speed"));
   
   if (!Animation->AlreadySetup)
   {
    UI_SetNextItemOpen(false, UICond_Always);
    Animation->AlreadySetup = true;
   }
   
   UI_HelpMarkerWithTooltip(StrLit("Extract curve by computing Cubic Spline interpolation of animated curve shape"));
   UI_SameRow();
   UI_SeparatorText(StrLit("Extraction"));
   
   if (UI_SliderUnsigned(&Animation->ExtractedCurveDetail, 0,
                         Animation->AnimationCurveSamplePointCount,
                         StrLit("Detail")))
   {
    Animation->ExtractedCurveDetailCustom = true;
   }
   
   if (UI_Button(StrLit("Extract")))
   {
    temp_arena Temp = TempArena(0);
    
    compute_animation_samples_result AnimationSamples = ComputeAnimationSamples(Temp.Arena, Animation);
    InterpolateAnimationCurve(Animation, AnimationSamples.Samples);
    
    entity *Entity = AddEntity(Editor);
    entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
    InitEntityFromEntity(&EntityWitness, Animation->ExtractEntity);
    EndEntityModify(EntityWitness);
    SelectEntity(Editor, Entity);
    
    EndTemp(Temp);
   }
   
   UI_SameRow();
   
   b32 Preview = (Animation->Flags & AnimatingCurves_Preview);
   UI_Checkbox(&Preview, StrLit("Preview"));
   if (Preview)
   {
    Animation->Flags |= AnimatingCurves_Preview;
   }
   else
   {
    Animation->Flags &= ~AnimatingCurves_Preview;
   }
   UI_SameRow();
   UI_HelpMarkerWithTooltip(StrLit("Preview interpolated curve instead of animated one"));
  }
  else
  {
   EndAnimatingCurves(Animation);
  }
  
  UI_EndWindow();
 }
}

internal void
ProcessInputEvents(editor *Editor,
                   platform_input_output *Input,
                   render_group *RenderGroup)
{
 ProfileFunctionBegin();
 
 editor_left_click_state *LeftClick = &Editor->LeftClick;
 editor_right_click_state *RightClick = &Editor->RightClick;
 editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 merging_curves_state *Merging = &Editor->MergingCurves;
 string_store *StrStore = Editor->StrStore;
 
 f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(RenderGroup, Editor->CollisionToleranceClip);
 f32 RotationRadius = ClipSpaceLengthToWorldSpace(RenderGroup, Editor->RotationRadiusClip);
 
 for (u32 EventIndex = 0;
      EventIndex < Input->EventCount;
      ++EventIndex)
 {
  platform_event *Event = Input->Events + EventIndex;
  b32 Eat = false;
  v2 MouseP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
  
  //- left click events processing
  b32 DoesAnimationWantInput = AnimationWantsInput(Animation);
  b32 DoesMergingWantInput = MergingWantsInput(Merging);
  if (!Eat && IsKeyPress(Event, PlatformKey_LeftMouseButton, AnyKeyModifier) &&
      (!LeftClick->Active || DoesAnimationWantInput || DoesMergingWantInput))
  {
   Eat = true;
   
   collision Collision = {};
   if (Event->Modifiers & PlatformKeyModifierFlag_Alt)
   {
    // NOTE(hbr): Force no collision, so that user can add control point wherever they want
   }
   else
   {
    Collision = CheckCollisionWithEntities(Editor, MouseP, CollisionTolerance);
   }
   
   if ((DoesAnimationWantInput || DoesMergingWantInput) &&
       Collision.Entity && Collision.Entity->Type == Entity_Curve)
   {
    if (DoesAnimationWantInput)
    {
     if (SupplyCurve(&Animation->Choose2Curves, Collision.Entity))
     {
      Animation->Flags |= AnimatingCurves_Animating;
     }
    }
    else
    {
     Assert(DoesMergingWantInput);
     SupplyCurve(&Merging->Choose2Curves, Collision.Entity);
    }
   }
   else
   {     
    LeftClick->OriginalVerticesCaptured = false;
    LeftClick->LastMouseP = MouseP;
    
    entity_with_modify_witness CollisionEntityWitness = BeginEntityModify(Collision.Entity);
    curve *CollisionCurve = &Collision.Entity->Curve;
    point_tracking_along_curve_state *CollisionTracking = &CollisionCurve->PointTracking;
    
    entity *SelectedEntity = GetSelectedEntity(Editor);
    curve *SelectedCurve = &SelectedEntity->Curve;
    
    if ((Collision.Entity || SelectedEntity) && (Event->Modifiers & PlatformKeyModifierFlag_Ctrl))
    {
     entity *Entity = (Collision.Entity ? Collision.Entity : SelectedEntity);
     // NOTE(hbr): just move entity if ctrl is pressed
     BeginMovingEntity(LeftClick, Editor, Entity);
    }
    else if ((Collision.Flags & Collision_CurveLine) && CollisionTracking->Type)
    {
     BeginMovingTrackingPoint(LeftClick, Collision.Entity);
     f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveSampleIndex, (CollisionCurve->CurveSampleCount- 1));
     SetTrackingPointFraction(&CollisionEntityWitness, Fraction);
    }
    else if (Collision.Flags & Collision_BSplineKnot)
    {
     BeginMovingBSplineKnot(LeftClick, Collision.Entity, Collision.BSplineKnot);
    }
    else if (Collision.Flags & Collision_CurvePoint)
    {
     BeginMovingCurvePoint(LeftClick, Editor, Collision.Entity, Collision.CurvePoint);
    }
    else if (Collision.Flags & Collision_CurveLine)
    {
     if (UsesControlPoints(CollisionCurve))
     {
      control_point_handle ControlPoint = ControlPointIndexFromCurveSampleIndex(CollisionCurve, Collision.CurveSampleIndex);
      u32 PointIndex = IndexFromControlPointHandle(ControlPoint);
      u32 InsertAt = PointIndex + 1;
      InsertControlPoint(Editor, &CollisionEntityWitness, MouseP, InsertAt);
      curve_point_handle CurvePoint = CurvePointFromControlPoint(ControlPointHandleFromIndex(InsertAt));
      BeginMovingCurvePoint(LeftClick, Editor, Collision.Entity, CurvePoint);
     }
     else
     {
      SelectEntity(Editor, Collision.Entity);
     }
    }
    else if (Collision.Flags & Collision_TrackedPoint)
    {
     // NOTE(hbr): This shouldn't really happen, Collision_CurveLine should be set as well.
     // But if it does, just don't do anything.
    }
    else
    {
     entity *TargetEntity = 0;
     if (SelectedEntity &&
         (SelectedEntity->Type == Entity_Curve) &&
         UsesControlPoints(SelectedCurve))
     {
      TargetEntity = SelectedEntity;
     }
     else
     {
      temp_arena Temp = TempArena(0);
      
      entity *Entity = AddEntity(Editor);
      string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EverIncreasingEntityCounter++);
      InitEntityAsCurve(Entity, Name, Editor->CurveDefaultParams);
      TargetEntity = Entity;
      
      EndTemp(Temp);
     }
     Assert(TargetEntity);
     
     entity_with_modify_witness TargetEntityWitness = BeginEntityModify(TargetEntity);
     control_point_handle Appended = AppendControlPoint(Editor, &TargetEntityWitness, MouseP);
     BeginMovingCurvePoint(LeftClick, Editor, TargetEntity, CurvePointFromControlPoint(Appended));
     EndEntityModify(TargetEntityWitness);
    }
    
    entity *TargetEntity = EntityFromHandle(LeftClick->TargetEntity);
    if (TargetEntity)
    {
     SelectEntity(Editor, TargetEntity);
    }
    
    if (LeftClick->Mode == EditorLeftClick_MovingCurvePoint)
    {
     curve *TargetCurve = SafeGetCurve(TargetEntity);
     SelectControlPointFromCurvePoint(TargetCurve, LeftClick->CurvePoint);
    }
    
    EndEntityModify(CollisionEntityWitness);
   }
  }
  
  if (!Eat && IsKeyRelease(Event, PlatformKey_LeftMouseButton) && LeftClick->Active)
  {
   Eat = true;
   EndLeftClick(Editor, LeftClick);
  }
  
  if (!Eat && Event->Type == PlatformEvent_MouseMove && LeftClick->Active)
  {
   // NOTE(hbr): don't eat mouse move event
   
   entity *Entity = EntityFromHandle(LeftClick->TargetEntity);
   if (Entity)
   {
    entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
    
    v2 Translate = MouseP - LeftClick->LastMouseP;
    v2 TranslateLocal = (WorldToLocalEntityPosition(Entity, MouseP) -
                         WorldToLocalEntityPosition(Entity, LeftClick->LastMouseP));
    
    switch (LeftClick->Mode)
    {
     case EditorLeftClick_MovingTrackingPoint: {
      curve *Curve = SafeGetCurve(Entity);
      point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
      f32 Fraction = Tracking->Fraction;
      MovePointAlongCurve(Curve, &TranslateLocal, &Fraction, true);
      MovePointAlongCurve(Curve, &TranslateLocal, &Fraction, false);
      SetTrackingPointFraction(&EntityWitness, Fraction);
     }break;
     
     case EditorLeftClick_MovingBSplineKnot: {
      curve *Curve = SafeGetCurve(Entity);
      curve_points_static *Points = GetCurvePoints(Curve);
      b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
      f32 *Knots = Points->BSplineKnots;
      b_spline_knot_handle Knot = LeftClick->BSplineKnot;
      u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Knot);
      f32 KnotFraction = Knots[KnotIndex];
      MovePointAlongCurve(Curve, &TranslateLocal, &KnotFraction, true);
      MovePointAlongCurve(Curve, &TranslateLocal, &KnotFraction, false);
      SetBSplineKnotPoint(&EntityWitness, Knot, KnotFraction);
     }break;
     
     case EditorLeftClick_MovingCurvePoint: {
      if (!LeftClick->OriginalVerticesCaptured)
      {
       curve *Curve = SafeGetCurve(Entity);
       arena *Arena = LeftClick->OriginalVerticesArena;
       LeftClick->OriginalCurveVertices = CopyLineVertices(Arena, Curve->CurveVertices);
       LeftClick->OriginalVerticesCaptured = true;
      }
      
      translate_curve_point_flags Flags = (TranslateCurvePoint_MatchBezierTwinDirection |
                                           TranslateCurvePoint_MatchBezierTwinLength);
      if (Event->Modifiers & PlatformKeyModifierFlag_Shift)
      {
       Flags &= ~TranslateCurvePoint_MatchBezierTwinDirection;
      }
      if (Event->Modifiers & PlatformKeyModifierFlag_Ctrl)
      {
       Flags &= ~TranslateCurvePoint_MatchBezierTwinLength;
      }
      TranslateCurvePointTo(&EntityWitness, LeftClick->CurvePoint, MouseP, Flags);
     }break;
     
     case EditorLeftClick_MovingEntity: {
      Entity->XForm.P += Translate;
     }break;
    }
    
    LeftClick->LastMouseP = MouseP;
    LeftClick->Moved = true;
    
    EndEntityModify(EntityWitness);
   }
  }
  
  //- right click events processing
  if (!Eat && IsKeyPress(Event, PlatformKey_RightMouseButton, AnyKeyModifier) && !RightClick->Active)
  {
   Eat = true;
   
   collision Collision = CheckCollisionWithEntities(Editor, MouseP, CollisionTolerance);
   BeginRightClick(RightClick, MouseP, Collision);
   
   entity *Entity = Collision.Entity;
   if (!(Event->Modifiers & KeyModifierFlag(Ctrl)) && Collision.Flags & Collision_CurvePoint)
   {
    curve *Curve = SafeGetCurve(Entity);
    SelectControlPointFromCurvePoint(Curve, Collision.CurvePoint);
   }
   
   if (Entity)
   {
    SelectEntity(Editor, Entity);
   }
  }
  
  if (!Eat && IsKeyRelease(Event, PlatformKey_RightMouseButton) && RightClick->Active)
  {
   Eat = true;
   
   // NOTE(hbr): perform click action only if button was released roughly in the same place
   b32 ReleasedClose = (NormSquared(RightClick->ClickP - MouseP) <= Sqr(CollisionTolerance));
   if (ReleasedClose)
   {
    collision *Collision = &RightClick->CollisionAtP;
    entity *Entity = Collision->Entity;
    entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
    curve *Curve = &Entity->Curve;
    
    if (Event->Modifiers & KeyModifierFlag(Ctrl)) // work on entity level
    {
     if (Entity)
     {
      RemoveEntity(Editor, Entity);
     }
    }
    else // work on control point level
    {
     if (Collision->Flags & Collision_TrackedPoint)
     {
      switch (Curve->PointTracking.Type)
      {
       case PointTrackingAlongCurve_BezierCurveSplit: {
        PerformBezierCurveSplit(Editor, Entity);
       }break;
       
       case PointTrackingAlongCurve_None:
       case PointTrackingAlongCurve_DeCasteljauVisualization: {}break;
      }
     }
     else if (Collision->Flags & Collision_CurvePoint)
     {
      if (Collision->CurvePoint.Type == CurvePoint_ControlPoint)
      {
       RemoveControlPoint(Editor, &EntityWitness, Collision->CurvePoint.Control);
      }
      else
      {
       SelectControlPointFromCurvePoint(Curve, Collision->CurvePoint);
      }
      SelectEntity(Editor, Entity);
     }
     else
     {
      SelectEntity(Editor, Entity);
     }
    }
    
    EndEntityModify(EntityWitness);
   }
   else
   {
    SelectEntity(Editor, 0);
   }
   
   EndRightClick(RightClick);
  }
  
  //- camera control events processing
  if (!Eat && IsKeyPress(Event, PlatformKey_MiddleMouseButton, AnyKeyModifier))
  {
   Eat = true;
   BeginMiddleClick(MiddleClick, Event->Modifiers & PlatformKeyModifierFlag_Ctrl, Event->ClipSpaceMouseP);
  }
  if (!Eat && IsKeyRelease(Event, PlatformKey_MiddleMouseButton))
  {
   Eat = true;
   EndMiddleClick(MiddleClick);
  }
  if (!Eat && Event->Type == PlatformEvent_MouseMove && MiddleClick->Active)
  {
   // NOTE(hbr): don't eat mouse move event
   
   camera *Camera = &Editor->Camera;
   v2 FromP = Unproject(RenderGroup, MiddleClick->ClipSpaceLastMouseP);
   v2 ToP = MouseP;
   if (MiddleClick->Rotate)
   {
    v2 CenterP = Camera->P;
    if (NormSquared(FromP - CenterP) >= Sqr(RotationRadius) &&
        NormSquared(ToP   - CenterP) >= Sqr(RotationRadius))
    {
     rotation2d Rotation = Rotation2DFromMovementAroundPoint(FromP, ToP, CenterP);
     RotateCameraAround(Camera, Rotation2DInverse(Rotation), CenterP);
    }
   }
   else
   {
    v2 Translate = ToP - FromP;
    TranslateCamera(&Editor->Camera, -Translate);
   }
   MiddleClick->ClipSpaceLastMouseP = Event->ClipSpaceMouseP;
  }
  
  if (!Eat && Event->Type == PlatformEvent_Scroll)
  {
   Eat = true;
   ZoomCamera(&Editor->Camera, Event->ScrollDelta);
  }
  
  //- shortcuts
  ForEachEnumVal(Command, EditorCommand_Count, editor_command)
  {
   editor_keyboard_shortcut_group *Group = EditorKeyboardShortcuts + Command;
   
   b32 IsActive = true;
#if !(BUILD_DEBUG)
   IsActive = !Group->DevSpecific;
#endif
   
   if (IsActive)
   {   
    ForEachIndex(ShortcutIndex, Group->Count)
    {
     editor_keyboard_shortcut *Shortcut = Group->Shortcuts + ShortcutIndex;
     
     if (!Eat && IsKeyPress(Event, Shortcut->Key, Shortcut->Modifiers))
     {
      Eat = true;
      PushEditorCmd(Editor, Command);
      break;
     }
    }
   }
  }
  
  //- misc
  if (!Eat && Event->Type == PlatformEvent_FilesDrop)
  {
   Eat = true;
   v2 AtP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
   TryLoadImages(Editor, Event->FileCount, Event->FilePaths, AtP);
  }
  
  if (!Eat && Event->Type == PlatformEvent_WindowClose)
  {
   Eat = true;
   PushEditorCmd(Editor, EditorCommand_Quit);
  }
 }
 
 // NOTE(hbr): these "sanity checks" are only necessary because ImGui captured any input that ends
 // with some possible ImGui interaction - e.g. we start by clicking in our editor but end releasing
 // on some ImGui window
 if (!Input->Pressed[PlatformKey_LeftMouseButton])
 {
  EndLeftClick(Editor, LeftClick);
 }
 if (!Input->Pressed[PlatformKey_RightMouseButton])
 {
  EndRightClick(RightClick);
 }
 if (!Input->Pressed[PlatformKey_MiddleMouseButton])
 {
  EndMiddleClick(MiddleClick);
 }
 
 ProfileEnd();
}

internal void
Merge2Curves(entity_with_modify_witness *MergeWitness,
             entity *Entity0, entity *Entity1,
             curve_merge_method Method)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve0 = SafeGetCurve(Entity0);
 curve *Curve1 = SafeGetCurve(Entity1);
 entity *MergeEntity = MergeWitness->Entity;
 
 string Name = StrF(Temp.Arena, "%S+%S", GetEntityName(Entity0), GetEntityName(Entity1));
 InitEntityFromEntity(MergeWitness, Entity0);
 SetEntityName(MergeEntity, Name);
 
 // TODO(hbr): remove this?
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 curve_points_static *Points0 = GetCurvePoints(Curve0);
 curve_points_static *Points1 = GetCurvePoints(Curve1);
 
 u32 PointCount0 = Points0->ControlPointCount;
 u32 PointCount1 = Points1->ControlPointCount;
 
 v2 *Controls0 = Points0->ControlPoints;
 v2 *Controls1 = Points1->ControlPoints;
 
 f32 *Weights0 = Points0->ControlPointWeights;
 f32 *Weights1 = Points1->ControlPointWeights;
 
 cubic_bezier_point *Beziers0 = Points0->CubicBezierPoints;
 cubic_bezier_point *Beziers1 = Points1->CubicBezierPoints;
 
 u32 DropCount1 = 0;
 v2 Fix1 = {};
 switch (Method)
 {
  case CurveMerge_Concat: {}break;
  
  case CurveMerge_C0:
  case CurveMerge_C1:
  case CurveMerge_C2:
  case CurveMerge_G1: {
   DropCount1 = 1;
   
   if (PointCount0 > 0 && PointCount1 > 0)
   {
    v2 P0 = Controls0[PointCount0 - 1];
    
    v2 P1 = Controls1[0];
    P1 = LocalToWorldEntityPosition(Entity1, P1);
    P1 = WorldToLocalEntityPosition(Entity0, P1);
    
    Fix1 = (P0 - P1);
   }
  }break;
  
  case CurveMerge_Count: InvalidPath;
 }
 u32 PointCount = (PointCount0 + PointCount1);
 if (DropCount1 <= PointCount1)
 {
  PointCount -= DropCount1;
 }
 
 v2 *Points = PushArrayNonZero(Temp.Arena, PointCount, v2);
 f32 *Weights = PushArrayNonZero(Temp.Arena, PointCount, f32);
 cubic_bezier_point *Beziers = PushArrayNonZero(Temp.Arena, PointCount, cubic_bezier_point);
 
 ArrayCopy(Weights,               Weights0,              PointCount0);
 ArrayCopy(Weights + PointCount0, Weights1 + DropCount1, PointCount1 - DropCount1);
 ArrayCopy(Points,                Controls0,               PointCount0);
 ArrayCopy(Beziers,               Beziers0,              PointCount0);
 
 u32 PointIndex = PointCount0;
 for (u32 PointIndex1 = DropCount1;
      PointIndex1 < PointCount1;
      ++PointIndex1)
 {
  v2 Point1 = Controls1[PointIndex1];
  Point1 = LocalToWorldEntityPosition(Entity1, Point1);
  Point1 = WorldToLocalEntityPosition(Entity0, Point1);
  Point1 = Point1 + Fix1;
  Points[PointIndex] = Point1;
  
  cubic_bezier_point Bezier1 = Beziers1[PointIndex1];
  Bezier1.P0 = LocalToWorldEntityPosition(Entity1, Bezier1.P0);
  Bezier1.P0 = WorldToLocalEntityPosition(Entity0, Bezier1.P0);
  Bezier1.P0 = Bezier1.P0 + Fix1;
  
  Bezier1.P1 = LocalToWorldEntityPosition(Entity1, Bezier1.P1);
  Bezier1.P1 = WorldToLocalEntityPosition(Entity0, Bezier1.P1);
  Bezier1.P1 = Bezier1.P1 + Fix1;
  
  Bezier1.P2 = LocalToWorldEntityPosition(Entity1, Bezier1.P2);
  Bezier1.P2 = WorldToLocalEntityPosition(Entity0, Bezier1.P2);
  Bezier1.P2 = Bezier1.P2 + Fix1;
  
  Beziers[PointIndex] = Bezier1;
  
  ++PointIndex;
 }
 
 b32 S_Exists = (SignedCmp(PointCount0 - 3, >=, 0) && SignedCmp(PointCount0 - 3, <, PointCount));
 b32 Q_Exists = (SignedCmp(PointCount0 - 2, >=, 0) && SignedCmp(PointCount0 - 2, <, PointCount));
 b32 P_Exists = (SignedCmp(PointCount0 - 1, >=, 0) && SignedCmp(PointCount0 - 1, <, PointCount));
 b32 R_Exists = (SignedCmp(PointCount0 - 0, >=, 0) && SignedCmp(PointCount0 - 0, <, PointCount));
 b32 T_Exists = (SignedCmp(PointCount0 + 1, >=, 0) && SignedCmp(PointCount0 + 1, <, PointCount));
 
 v2 S = (S_Exists ? Points[PointCount0 - 3] : V2(0,0));
 v2 Q = (Q_Exists ? Points[PointCount0 - 2] : V2(0,0));
 v2 P = (P_Exists ? Points[PointCount0 - 1] : V2(0,0));
 v2 R = (R_Exists ? Points[PointCount0 - 0] : V2(0,0));
 v2 T = (T_Exists ? Points[PointCount0 + 1] : V2(0,0));
 
 f32 s = (S_Exists ? Weights[PointCount0 - 3] : 0);
 f32 q = (Q_Exists ? Weights[PointCount0 - 2] : 0);
 f32 p = (P_Exists ? Weights[PointCount0 - 1] : 0);
 f32 r = (R_Exists ? Weights[PointCount0 - 0] : 0);
 f32 t = (T_Exists ? Weights[PointCount0 + 1] : 0);
 
 v2 R_ = R;
 v2 T_ = T;
 
 f32 n = Cast(f32)PointCount0;
 f32 m = Cast(f32)PointCount1;
 
 switch (Method)
 {
  case CurveMerge_G1: {
   // NOTE(hbr): G1 merge:
   // - C0 and
   // - Q,P,R should be colinear
   // - fix R, but maintain ||R-P|| length
   v2 U = P - Q;
   v2 V = R - P;
   v2 Projected = ProjectOnto(V, U);
   R_ = P + Projected;
  }break;
  
  case CurveMerge_C1:
  case CurveMerge_C2: {
   // NOTE(hbr): C1 merge:
   // - C0 and
   // - n/(b-a) * q/p * (P-Q) = m/(c-b) * r/p * (R-P)
   // - assuming (b-a) = (c-b) = 1
   // - which gives us: n * q/p * (P-Q) = m * r/p * (R-P)
   // - solving for R: R = n/m * q/r * (P-Q) + P
   R_ = n/m * q/r * (P-Q) + P;
   
   if (Method == CurveMerge_C2)
   {
    // NOTE(hbr): C2 merge (version without weights):
    // - C0 and C1 and
    // - n*(n-1)/(b-a)^2 * (P-2Q+S) = m*(m-1)/(c-b)^2 * (T-2R+P)
    // - assuming (b-a) = (c-b) = 1
    // - which gives us: n*(n-1) * (P-2Q+S) = m*(m-1) * (T-2R+P)
    // - solving for T: T = (n*(n-1))/(m*(m-1)) * (P-2Q+S) + (2R-P)
    // T_ = (n*(n-1))/(m*(m-1)) * (P-2*Q+S) + (2*R_-P);
    
    // NOTE(hbr): C2 merge (version with weights):
    // - C0 and C1 and
    // - n*(n-1)/(b-a)^2 * (m_n*P_n+m_n_1*P_n_1*+m_n_2*P_n_2) = m*(m-1)/(c-b)^2 * (m2*P2+m1*P1+m0*P0)
    // - assuming (b-a) = (c-b) = 1
    // - which gives us: n*(n-1) * (m_n*P_n+m_n_1*P_n_1*+m_n_2*P_n_2) = m*(m-1) * (m2*P2+m1*P1+m0*P0)
    // - solving for T: T = (n*(n-1))/(m*(m-1)) * [(m_n*P_n+m_n_1*P_n_1*+m_n_2*P_n_2) - (m1*P1+m0*P0)] / m2
    
    v2 P2 = T;
    v2 P1 = R_;
    v2 P0 = P;
    
    f32 w2 = t;
    f32 w1 = r;
    f32 w0 = p;
    
    f32 m2 = m*(m-1) * w2/w0;
    f32 m1 = -2*m*(m-1) * w1/w0 - 2*Sqr(m) * w1*(w1-w0)/Sqr(w0);
    f32 m0 = 2*m*(m-1) * w1/w0 + 2*Sqr(m) * (w1-w0)/w0 - m*(m-1)* w2/w0 + 2*Sqr(m * (w1-w0)/w0);
    
    v2 P_n = P;
    v2 P_n_1 = Q;
    v2 P_n_2 = S;
    
    f32 w_n = p;
    f32 w_n_1 = q;
    f32 w_n_2 = s;
    
    f32 m_n = 2*n*(n-1) * w_n_1/w_n - 2*Sqr(n) * (w_n - w_n_1)/w_n - n*(n-1) * w_n_2/w_n + 2*Sqr(n * (w_n - w_n_1)/w_n);
    f32 m_n_1 = -2*n*(n-1) * w_n_1/w_n + 2*Sqr(n) * w_n_1*(w_n-w_n_1)/Sqr(w_n);
    f32 m_n_2 = n*(n-1) * w_n_2/w_n;
    
    T_ = (n*(n-1))/(m*(m-1)*m2) * (m_n*P_n + m_n_1*P_n_1 + m_n_2*P_n_2 - m1*P1 - m0*P0);
   }
  }break;
  
  case CurveMerge_C0:
  case CurveMerge_Concat: {}break;
  
  case CurveMerge_Count: InvalidPath;
 }
 
 if (Q_Exists && R_Exists)
 {
  v2 Fix_R = R_ - R;
  Points[PointCount0 - 0] += Fix_R;
  Beziers[PointCount0 - 0].P0 += Fix_R;
  Beziers[PointCount0 - 0].P1 += Fix_R;
  Beziers[PointCount0 - 0].P2 += Fix_R;
 }
 
 if (S_Exists && T_Exists)
 {
  v2 Fix_T = T_ - T;
  Points[PointCount0 + 1] += Fix_T;
  Beziers[PointCount0 + 1].P0 += Fix_T;
  Beziers[PointCount0 + 1].P1 += Fix_T;
  Beziers[PointCount0 + 1].P2 += Fix_T;
 }
 
 curve_points_handle CurvePoints = MakeCurvePointsHandle(PointCount, Points, Weights, Beziers, 0, 0);
 SetCurvePoints(MergeWitness, CurvePoints);
 
 MaybeReverseCurvePoints(MergeEntity);
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 EndTemp(Temp);
}

internal void
RenderMergingCurvesUI(editor *Editor)
{
 merging_curves_state *Merging = &Editor->MergingCurves;
 entity_store *EntityStore = Editor->EntityStore;
 string_store *StrStore = Editor->StrStore;
 
 if (Merging->Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindow(&WindowOpen, UIWindowFlag_AutoResize, StrLit("Merging Curves"));
  
  if (WindowOpen)
  {
   UI_SeparatorText(StrLit("Choice"));
   UpdateAndRenderChoose2CurvesUI(&Merging->Choose2Curves, Editor);
   
   UI_SeparatorText(StrLit("Merging"));
   b32 MergeMethodChanged = UI_Combo(SafeCastToPtr(Merging->Method, u32), CurveMerge_Count, CurveMergeNames, StrLit("Merge Method"));
   
   entity *Entity0 = EntityFromHandle(Merging->Choose2Curves.Curves[0]);
   entity *Entity1 = EntityFromHandle(Merging->Choose2Curves.Curves[1]);
   
   curve *Curve0 = ((Entity0 && Entity0->Type == Entity_Curve) ? &Entity0->Curve : 0);
   curve *Curve1 = ((Entity1 && Entity1->Type == Entity_Curve) ? &Entity1->Curve : 0);
   
   curve_merge_compatibility Compatibility = {};
   if (Curve0 && Curve1)
   {
    Compatibility = AreCurvesCompatibleForMerging(Curve0, Curve1, Merging->Method);
   }
   else
   {
    Compatibility.WhyIncompatible = StrLit("not all curves selected");
   }
   
   b32 Changed0 = EntityModified(Merging->EntityVersioned[0], Entity0);
   b32 Changed1 = EntityModified(Merging->EntityVersioned[1], Entity1);
   
   if (Changed0 || Changed1 || MergeMethodChanged)
   {
    entity_with_modify_witness MergeWitness = BeginEntityModify(Merging->MergeEntity);
    if (Compatibility.Compatible)
    {
     Merge2Curves(&MergeWitness, Entity0, Entity1, Merging->Method);
    }
    else
    {
     SetCurvePoints(&MergeWitness, CurvePointsHandleZero());
    }
    EndEntityModify(MergeWitness);
    
    Merging->EntityVersioned[0] = MakeEntitySnapshotForMerging(Entity0);
    Merging->EntityVersioned[1] = MakeEntitySnapshotForMerging(Entity1);
   }
   
   UI_Disabled(!Compatibility.Compatible)
   {
    if (UI_Button(StrLit("Merge")))
    {
     EndMergingCurves(Editor, true);
    }
   }
   
   UI_SameRow();
   
   UI_Colored(UI_Color_Text, RedColor)
   {
    UI_Text(true, Compatibility.WhyIncompatible);
   }
  }
  else
  {
   EndMergingCurves(Editor, false);
  }
  
  UI_EndWindow();
 }
}

internal void
RenderMergingCurves(merging_curves_state *Merging, render_group *RenderGroup)
{
 if (Merging->Active)
 {
  entity *Entity = Merging->MergeEntity;
  rendering_entity_handle RenderingHandle = BeginRenderingEntity(Entity, RenderGroup);
  curve *Curve = SafeGetCurve(Entity);
  
  curve_draw_params Original = Curve->Params.DrawParams;
  curve_draw_params Fade = Original;
  for (u32 ColorIndex = 0;
       ColorIndex < ArrayCount(Fade.All);
       ++ColorIndex)
  {
   Fade.All[ColorIndex].Color = RGBA_Fade(Fade.All[ColorIndex].Color, 0.15f);
  }
  Curve->Params.DrawParams = Fade;
  RenderEntity(RenderingHandle);
  Curve->Params.DrawParams = Original;
  
  EndRenderingEntity(RenderingHandle);
 }
}

struct sorted_profiler_frame_anchors
{
 u32 Count;
 sort_entry *SortEntries;
 u32 MaxAnchorCount;
};

inline internal f32
MsFromTSC(u64 TSC, f32 Inv_CPU_Freq)
{
 f32 Ms = 1000 * TSC * Inv_CPU_Freq;
 return Ms;
}

internal sorted_profiler_frame_anchors
SortProfilerFrameAnchors(arena *Arena, profiler_frame *Frame, profiler *Profiler)
{
 ProfileFunctionBegin();
 
 u32 MaxAnchorCount = MAX_PROFILER_ANCHOR_COUNT;
 sort_entry_array SortAnchors = AllocSortEntryArray(Arena, MaxAnchorCount + 1, SortOrder_Descending);
 f32 AnchorTotalSelfMsSum = 0;
 f32 MsThreshold = 0.01f;
 
 // omit AnchorIndex=0 because it has "hoax" numbers
 // (not really but for the purposes of this loop it does
 for (u32 AnchorIndex = 1;
      AnchorIndex < MaxAnchorCount;
      ++AnchorIndex)
 {
  profile_anchor *Anchor = Frame->Anchors + AnchorIndex;
  f32 TotalSelfMs = MsFromTSC(Anchor->TotalSelfTSC, Profiler->Inv_CPU_Freq);
  if (TotalSelfMs >= MsThreshold)
  {
   AddSortEntry(&SortAnchors, TotalSelfMs, AnchorIndex);
  }
  AnchorTotalSelfMsSum += TotalSelfMs;
 }
 
 {
  f32 FrameTotalMs = MsFromTSC(Frame->TotalTSC, Profiler->Inv_CPU_Freq);
  //Assert(AnchorTotalSelfMsSum <= FrameTotalMs);
  f32 NotMeasuredMs = FrameTotalMs - AnchorTotalSelfMsSum;
  if (NotMeasuredMs >= MsThreshold)
  {
   AddSortEntry(&SortAnchors, NotMeasuredMs, MaxAnchorCount);
  }
 }
 
 Sort(SortAnchors.Entries, SortAnchors.Count, SortFlag_Stable);
 
 sorted_profiler_frame_anchors Result = {};
 Result.Count = SortAnchors.Count;
 Result.SortEntries = SortAnchors.Entries;
 Result.MaxAnchorCount = MaxAnchorCount;
 
 ProfileEnd();
 
 return Result;
}

struct profile_anchor_info
{
 b32 IsFakeMissingAnchor;
 u32 AnchorIndex;
 f32 AnchorMs;
};
internal profile_anchor_info
GetProfileAnchorInfo(sorted_profiler_frame_anchors *Anchors, u32 SortIndex)
{
 Assert(SortIndex < Anchors->Count);
 
 sort_entry SortEntry = Anchors->SortEntries[SortIndex];
 u32 AnchorIndex = SortEntry.Index;
 f32 AnchorMs = -SortEntry.SortKey;
 
 profile_anchor_info Info = {};
 Info.IsFakeMissingAnchor = (AnchorIndex == Anchors->MaxAnchorCount);
 Info.AnchorIndex = AnchorIndex;
 Info.AnchorMs = AnchorMs;
 
 return Info;
}

internal void
MaybeProfileAnchorSourceCodeLocationTooltip(profile_anchor_info AnchorInfo,
                                            profiler *Profiler)
{
 if (!AnchorInfo.IsFakeMissingAnchor && UI_IsItemHovered())
 {
  profile_anchor_source_code_location Location = Profiler->AnchorLocations[AnchorInfo.AnchorIndex];
  UI_TooltipF("%s:%u", Location.File, Location.Line);
 }
}

struct anchor_filter
{
 string FilterLabel;
 anchor_index AnchorIndex;
};
internal int
AnchorFilterCmp(void *Data, anchor_filter *A, anchor_filter *B)
{
 MarkUnused(Data);
 int Result = StrCmp(A->FilterLabel, B->FilterLabel);
 return Result;
}

enum profiles_layout_mode
{
 ProfilesLayoutMode_Horizontal,
 ProfilesLayoutMode_Vertical,
};
struct profiles_layout
{
 profiles_layout_mode Mode;
 
 f32 Inv_ReferenceMs;
 
 u32 PaletteColorCount;
 rgba *ColorPalette;
 
 rect2 DrawRegion;
 
 f32 AtX;
 f32 AtY;
 
 f32 MainDimensionSize;
 
 u32 RectId;
};
internal profiles_layout
MakeProfilesLayout(profiles_layout_mode Mode,
                   f32 Inv_ReferenceMs,
                   u32 PaletteColorCount,
                   rgba *ColorPalette,
                   rect2 DrawRegion)
{
 profiles_layout Layout = {};
 
 Layout.Mode = Mode;
 Layout.Inv_ReferenceMs = Inv_ReferenceMs;
 Layout.PaletteColorCount = PaletteColorCount;
 Layout.ColorPalette = ColorPalette;
 Layout.DrawRegion = DrawRegion;
 
 f32 DrawWidth = DrawRegion.Max.X - DrawRegion.Min.X;
 f32 DrawHeight = DrawRegion.Max.Y - DrawRegion.Min.Y;
 
 switch (Mode)
 {
  case ProfilesLayoutMode_Horizontal: {
   Layout.AtX = DrawRegion.Min.X;
   Layout.AtY = DrawRegion.Min.Y;
   Layout.MainDimensionSize = DrawWidth;
  }break;
  
  case ProfilesLayoutMode_Vertical: {
   Layout.AtX = DrawRegion.Min.X;
   Layout.AtY = DrawRegion.Max.Y;
   Layout.MainDimensionSize = DrawHeight;
  }break;
 }
 
 return Layout;
}

internal clicked_b32
RenderProfile(profiles_layout *Layout,
              string ProfileLabelOverride,
              f32 ProfileMsOverride,
              u32 ProfileId,
              f32 ProfileSizeInOtherDimension,
              b32 AddStandardTooltip)
{
 b32 Clicked = false;
 
 f32 ProfileMainSize = Layout->MainDimensionSize * ProfileMsOverride * Layout->Inv_ReferenceMs;
 
 v2 ProfileSize = {};
 v2 ProfileTopLeftP = {};
 switch (Layout->Mode)
 {
  case ProfilesLayoutMode_Horizontal: {
   ProfileSize = V2(ProfileMainSize, ProfileSizeInOtherDimension);
   ProfileTopLeftP = V2(Layout->AtX, Layout->AtY);
   Layout->AtX += ProfileSize.X;
  }break;
  
  case ProfilesLayoutMode_Vertical: {
   ProfileSize = V2(ProfileSizeInOtherDimension, ProfileMainSize);
   Layout->AtY -= ProfileSize.Y;
   ProfileTopLeftP = V2(Layout->AtX, Layout->AtY);
  }break;
 }
 
 rgba ProfileColor = Layout->ColorPalette[ProfileId % Layout->PaletteColorCount];
 
 UI_SetNextItemSize(ProfileSize);
 UI_SetNextItemPos(ProfileTopLeftP);
 UI_Colored(UI_Color_Item, ProfileColor)
 {
  if (UI_Rect(Layout->RectId))
  {
   Clicked = true;
  }
 }
 if (AddStandardTooltip)
 {
  if (UI_IsItemHovered())
  {
   UI_TooltipF("[%.2fms] %S", ProfileMsOverride, ProfileLabelOverride);
  }
 }
 
 ++Layout->RectId;
 
 return Clicked;
}

internal void
AdvanceColumn(profiles_layout *Layout, f32 AdvanceBy)
{
 Assert(Layout->Mode == ProfilesLayoutMode_Vertical);
 Layout->AtX += AdvanceBy;
 Layout->AtY = Layout->DrawRegion.Max.Y;
}

internal void
AdvanceRow(profiles_layout *Layout, f32 AdvanceBy)
{
 Assert(Layout->Mode == ProfilesLayoutMode_Horizontal);
 Layout->AtX = Layout->DrawRegion.Min.X;
 Layout->AtY += AdvanceBy;
}

internal void
AdvanceCursor(profiles_layout *Layout, f32 AdvanceBy)
{
 switch (Layout->Mode)
 {
  case ProfilesLayoutMode_Horizontal: {
   Layout->AtX += AdvanceBy;
  }break;
  case ProfilesLayoutMode_Vertical: {
   Layout->AtY -= AdvanceBy;
  }break;
 }
}

internal void
RenderProfilerWindowContents(editor *Editor)
{
 visual_profiler_state *Visual = &Editor->Profiler;
 visual_profiler_mode Mode = Visual->Mode;
 profiler *Profiler = Visual->Profiler;
 
 temp_arena Temp = TempArena(0);
 
 local rgba ColorPalette[] = {
  RGB_Hex(0xffa600),
  RGB_Hex(0xff8531),
  RGB_Hex(0xff6361),
  RGB_Hex(0xde5a79),
  RGB_Hex(0xbc5090),
  RGB_Hex(0x8a508f),
  RGB_Hex(0x58508d),
  RGB_Hex(0x003f5c),
 };
 
 switch (Mode)
 {
  case VisualProfilerMode_AllFrames: {}break;
  
  case VisualProfilerMode_SingleFrame: {
   if (UI_Button(StrLit("Back")))
   {
    ProfilerInspectAllFrames(Visual);
   }
   UI_SameRow();
  }break;
 }
 
 UI_CheckboxF(&Visual->Stopped, "Stop");
 
 UI_SameRow();
 UI_Label(StrLit("Reference"))
 {
  UI_SliderFloat(&Visual->ReferenceMs, 1000.0f/500, 1000.0f/30, StrLit("Reference ms"));
  if (ResetCtxMenu(StrLit("Reset")))
  {
   Visual->ReferenceMs = Visual->DefaultReferenceMs;
  }
 }
 
 //- filter by anchor label
 local u32 FilterIndex = 0;
 u32 SpecialFilterCount = 0;
 u32 FilterNone = 0;
 u32 FilterAll = 0;
 anchor_filter *AnchorFilters = 0;
 {
  u32 FilterCount = 0;
  u32 MaxFilterCount = MAX_PROFILER_ANCHOR_COUNT + 2;
  AnchorFilters = PushArrayNonZero(Temp.Arena, MaxFilterCount, anchor_filter);
  
  AnchorFilters[FilterCount].FilterLabel = StrLit("<none>");
  FilterNone = FilterCount;
  ++FilterCount;
  ++SpecialFilterCount;
  Assert(FilterCount <= MaxFilterCount);
  
  AnchorFilters[FilterCount].FilterLabel = StrLit("<all>");
  FilterAll = FilterCount;
  ++FilterCount;
  ++SpecialFilterCount;
  Assert(FilterCount <= MaxFilterCount);
  
  for (u32 AnchorIndex = 0;
       AnchorIndex < MAX_PROFILER_ANCHOR_COUNT;
       ++AnchorIndex)
  {
   anchor_index Index = MakeAnchorIndex(AnchorIndex);
   if (ProfilerIsAnchorActive(Index))
   {
    anchor_filter Filter = {};
    Filter.FilterLabel = Profiler->AnchorLabels[AnchorIndex];
    Filter.AnchorIndex = Index;
    
    AnchorFilters[FilterCount] = Filter;
    ++FilterCount;
    Assert(FilterCount <= MaxFilterCount);
   }
  }
  
  SortTyped(AnchorFilters + SpecialFilterCount,
            FilterCount - SpecialFilterCount,
            AnchorFilterCmp, 0, SortFlag_None,
            anchor_filter);
  
  string *FilterLabels = PushArrayNonZero(Temp.Arena, FilterCount, string);
  for (u32 FilterIndex = 0;
       FilterIndex < FilterCount;
       ++FilterIndex)
  {
   FilterLabels[FilterIndex] = AnchorFilters[FilterIndex].FilterLabel;
  }
  
  UI_Combo(&FilterIndex, FilterCount, FilterLabels, StrLit("Anchor Filter"));
 }
 
 UI_Colored(UI_Color_Text, YellowColor)
 {
  UI_Text(false, StrLit("WARNING: Some profiles might be missing due short time they took"));
 }
 
 rect2 DrawRegion = UI_GetDrawableRegionBounds();
 f32 DrawWidth = DrawRegion.Max.X - DrawRegion.Min.X;
 f32 DrawHeight = DrawRegion.Max.Y - DrawRegion.Min.Y;
 f32 Inv_ReferenceMs = 1.0f / Visual->ReferenceMs;
 
 //- frame rendering
 switch (Mode)
 {
  case VisualProfilerMode_AllFrames: {
   u32 FrameCount = MAX_PROFILER_FRAME_COUNT;
   f32 FrameWidth = DrawWidth / FrameCount;
   
   profiles_layout Layout = MakeProfilesLayout(ProfilesLayoutMode_Vertical,
                                               Inv_ReferenceMs,
                                               ArrayCount(ColorPalette),
                                               ColorPalette,
                                               DrawRegion);
   
   for (u32 FrameIndex = 0;
        FrameIndex < FrameCount;
        ++FrameIndex)
   {
    profiler_frame *Frame = Profiler->Frames + FrameIndex;
    b32 ProfileClicked = false;
    
    //- render just frames, without splitting into anchors
    if (FilterIndex == FilterNone)
    {
     string FrameLabel = StrF(Temp.Arena, "Frame %u", FrameIndex);
     f32 FrameTotalMs = MsFromTSC(Frame->TotalTSC, Profiler->Inv_CPU_Freq);
     ProfileClicked |= RenderProfile(&Layout, FrameLabel, FrameTotalMs, 0, FrameWidth, true);
    }
    //- render all frame anchors
    else if (FilterIndex == FilterAll)
    {
     temp_arena Temp2 = BeginTemp(Temp.Arena);
     sorted_profiler_frame_anchors SortedAnchors = SortProfilerFrameAnchors(Temp2.Arena, Frame, Profiler);
     
     for (u32 SortIndex = 0;
          SortIndex < SortedAnchors.Count;
          ++SortIndex)
     {
      profile_anchor_info AnchorInfo = GetProfileAnchorInfo(&SortedAnchors, SortIndex);
      string AnchorLabel = (AnchorInfo.IsFakeMissingAnchor ?
                            StrLit("NOT PROFILED") :
                            Profiler->AnchorLabels[AnchorInfo.AnchorIndex]);
      f32 AnchorMs = AnchorInfo.AnchorMs;
      ProfileClicked |= RenderProfile(&Layout, AnchorLabel, AnchorMs, AnchorInfo.AnchorIndex, FrameWidth, true);
     }
     
     EndTemp(Temp2);
    }
    else if (FilterIndex >= SpecialFilterCount)
    {
     Assert(FilterIndex >= SpecialFilterCount);
     anchor_index AnchorIndex = AnchorFilters[FilterIndex].AnchorIndex;
     string AnchorLabel = Profiler->AnchorLabels[AnchorIndex.Index];
     f32 AnchorMs = MsFromTSC(Frame->Anchors[AnchorIndex.Index].TotalSelfTSC, Profiler->Inv_CPU_Freq);
     ProfileClicked |= RenderProfile(&Layout, AnchorLabel, AnchorMs, AnchorIndex.Index, FrameWidth, true);
    }
    else
    {
     InvalidPath;
    }
    
    if (ProfileClicked)
    {
     ProfilerInspectSingleFrame(Visual, FrameIndex);
    }
    
    AdvanceColumn(&Layout, FrameWidth);
   }
  }break;
  
  case VisualProfilerMode_SingleFrame: {
   profiler_frame *Frame = &Visual->FrameSnapshot;
   
   temp_arena Temp2 = BeginTemp(Temp.Arena);
   sorted_profiler_frame_anchors SortedAnchors = SortProfilerFrameAnchors(Temp2.Arena, Frame, Profiler);
   
   f32 PaddingFraction = 0.1f;
   f32 AnchorHeight = DrawHeight / (SortedAnchors.Count + (SortedAnchors.Count + 1) * PaddingFraction);
   f32 PaddingHeight = PaddingFraction * AnchorHeight;
   
   profiles_layout Layout = MakeProfilesLayout(ProfilesLayoutMode_Horizontal,
                                               Inv_ReferenceMs,
                                               ArrayCount(ColorPalette),
                                               ColorPalette,
                                               DrawRegion);
   
   AdvanceRow(&Layout, PaddingHeight);
   
   for (u32 SortIndex = 0;
        SortIndex < SortedAnchors.Count;
        ++SortIndex)
   {
    profile_anchor_info AnchorInfo = GetProfileAnchorInfo(&SortedAnchors, SortIndex);
    string AnchorLabel = (AnchorInfo.IsFakeMissingAnchor ?
                          StrLit("NOT PROFILED") :
                          Profiler->AnchorLabels[AnchorInfo.AnchorIndex]);
    f32 AnchorMs = AnchorInfo.AnchorMs;
    
    RenderProfile(&Layout, AnchorLabel, AnchorMs, AnchorInfo.AnchorIndex, AnchorHeight, false);
    MaybeProfileAnchorSourceCodeLocationTooltip(AnchorInfo, Profiler);
    UI_SameRow();
    UI_TextF(false, "[%.2fms] %S", AnchorMs, AnchorLabel);
    MaybeProfileAnchorSourceCodeLocationTooltip(AnchorInfo, Profiler);
    
    AdvanceRow(&Layout, PaddingHeight + AnchorHeight);
   }
   
   EndTemp(Temp2);
  }break;
 }
 
 EndTemp(Temp);
}
internal void
RenderProfilerUI(editor *Editor)
{
 ProfileFunctionBegin();
 if (Editor->ProfilerWindow)
 {
  if (UI_BeginWindow(&Editor->ProfilerWindow, 0, StrLit("Profiler")))
  {
   RenderProfilerWindowContents(Editor);
  }
  UI_EndWindow();
 }
 ProfileEnd();
}

internal void
RenderDevConsoleUI(editor *Editor)
{
 camera *Camera = &Editor->Camera;
 
 if (Editor->DevConsole)
 {
  if (UI_BeginWindow(&Editor->DevConsole, 0, StrLit("Dev Console")))
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
  }
  UI_EndWindow();
 }
}

internal void
InitGlobalsOnInitOrCodeReload(editor *Editor)
{
 if (Editor)
 {
  NilExpr = &Editor->NilParametricExpr;
  DEBUG_Vars = &Editor->DEBUG_Vars;
  InitEditorCtx(Editor->ArenaStore,
                Editor->RendererQueue,
                Editor->EntityStore,
                Editor->ThreadTaskMemoryStore,
                Editor->ImageLoadingStore,
                Editor->StrStore,
                Editor->CurvePointsStore,
                Editor->LowPriorityQueue,
                Editor->HighPriorityQueue);
 }
}

internal u32
RenderGridLinesAlongAxis(render_group *RenderGroup, axis2 Axis)
{
 i32 LogBase = 5;
 f32 LineZOffset = 0.0f;
 rgba LineColor = RGBA_Gray(122, 50);
 f32 MajorLineWidthClip = 0.003f;
 f32 MinorLineWidthClip = 0.001f;
 
 v2 Corner0 = Unproject(RenderGroup, V2(-1.0f, -1.0f));
 v2 Corner1 = Unproject(RenderGroup, V2(-1.0f, +1.0f));
 v2 Corner2 = Unproject(RenderGroup, V2(+1.0f, -1.0f));
 v2 Corner3 = Unproject(RenderGroup, V2(+1.0f, +1.0f));
 
 rect2 ViewportWorld = {};
 AddPointAABB(&ViewportWorld, Corner0);
 AddPointAABB(&ViewportWorld, Corner1);
 AddPointAABB(&ViewportWorld, Corner2);
 AddPointAABB(&ViewportWorld, Corner3);
 
 f32 MajorLineWidth = ClipSpaceLengthToWorldSpace(RenderGroup, MajorLineWidthClip);
 f32 MinorLineWidth = ClipSpaceLengthToWorldSpace(RenderGroup, MinorLineWidthClip);
 f32 LogBaseF = Cast(f32)LogBase;
 f32 Span = ClipSpaceLengthToWorldSpace(RenderGroup, 2.0f / 5);
 f32 Spacing = PowF32(LogBaseF, FloorF32(LogBaseF32(LogBaseF, Span)));
 f32 InvWidth = 1.0f / Spacing;
 i32 From = Cast(i32)CeilF32(ViewportWorld.Min.E[Axis] * InvWidth);
 i32 To = Cast(i32)FloorF32(ViewportWorld.Max.E[Axis] * InvWidth);
 
 u32 RenderedLineCount = 0;
 for (i32 I = From;
      I <= To;
      ++I)
 {
  f32 A = I * Spacing;
  
  v2 Begin;
  Begin.E[Axis] = A;
  Begin.E[1-Axis] = ViewportWorld.Min.E[1-Axis];
  
  v2 End;
  End.E[Axis] = A;
  End.E[1-Axis] = ViewportWorld.Max.E[1-Axis];
  
  f32 Width = ((I % LogBase == 0) ? MajorLineWidth : MinorLineWidth);
  PushLine(RenderGroup,
           Begin, End,
           Width, LineColor,
           LineZOffset);
  
  ++RenderedLineCount;
 }
 return RenderedLineCount;
}

internal void
RenderGrid(editor *Editor, render_group *RenderGroup)
{
 ProfileFunctionBegin();
 if (Editor->Grid)
 {
  u32 GridLineCount = 0;
  ForEachEnumVal(Axis, Axis2_Count, axis2)
  {
   GridLineCount += RenderGridLinesAlongAxis(RenderGroup, Axis);
  }
  DEBUG_Vars->DrawnGridLinesCount = GridLineCount;
 }
 ProfileEnd();
}

internal void
RenderRotationIndicator(editor *Editor, render_group *RenderGroup)
{
 editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
 if (MiddleClick->Active && MiddleClick->Rotate)
 {
  camera *Camera = &Editor->Camera;
  f32 Radius = ClipSpaceLengthToWorldSpace(RenderGroup, Editor->RotationRadiusClip);
  rgba Color = RGBA_U8(30, 56, 87, 80);
  f32 OutlineThickness = 0.1f * Radius;
  rgba OutlineColor = RGBA_U8(255, 255, 255, 24);
  // TODO(hbr): ZOffset here is possibly wrong, it should be something on top of everything
  // instead
  PushCircle(RenderGroup,
             Camera->P,
             Radius - OutlineThickness,
             Color, 0.0f,
             OutlineThickness, OutlineColor);
 }
}

internal void
RenderRemoveIndicator(editor *Editor, render_group *RenderGroup)
{
 editor_right_click_state *RightClick = &Editor->RightClick;
 if (RightClick->Active)
 {
  rgba Color = RGBA(0.5f, 0.5f, 0.5f, 0.3f);
  f32 CollisionTolerance = ClipSpaceLengthToWorldSpace(RenderGroup, Editor->CollisionToleranceClip);
  // TODO(hbr): Again, zoffset of this thing is wrong
  PushCircle(RenderGroup, RightClick->ClickP, CollisionTolerance, Color, 0.0f);
 }
}

internal void
RenderCurveShadowWhenMoving(editor *Editor, render_group *RenderGroup)
{
 // TODO(hbr): This looks like a mess, probably use functions that already exist
 editor_left_click_state *LeftClick = &Editor->LeftClick;
 entity *Entity = EntityFromHandle(LeftClick->TargetEntity);
 if (LeftClick->Active && Entity && LeftClick->OriginalVerticesCaptured && Entity->Curve.Params.DrawParams.Line.Enabled)
 {
  rgba ShadowColor = RGBA_Fade(Entity->Curve.Params.DrawParams.Line.Color, 0.15f);
  rendering_entity_handle RenderingHandle = BeginRenderingEntity(Entity, RenderGroup);
  
  PushVertexArray(RenderGroup,
                  LeftClick->OriginalCurveVertices.Vertices,
                  LeftClick->OriginalCurveVertices.VertexCount,
                  LeftClick->OriginalCurveVertices.Primitive,
                  ShadowColor,
                  GetCurvePartVisibilityZOffset(CurvePartVisibility_LineShadow));
  
  EndRenderingEntity(RenderingHandle);
 }
}

internal string
ProjectFileDisplayName(arena *Arena)
{
 string Result = StrF(Arena, "%S session files", EditorAppName);
 return Result;
}

internal b32
TryChangeProject(editor *Editor,
                 change_project_method ChangeHow,
                 b32 ForceLoad)
{
 b32 RequestQuit = false;
 if (!ForceLoad && Editor->ProjectModified)
 {
  project_change_request_state *ProjectChange = &Editor->ProjectChange;
  ClearArena(ProjectChange->Arena);
  ProjectChange->ChangeHow = CopyProjectChangeMethod(ProjectChange->Arena, ChangeHow);
  ProjectChange->CurrentProjectSaveModalIsOpen = true;
  switch (ChangeHow.Type)
  {
   case ChangeProjectMethod_LoadEmptyProject: {ProjectChange->CurrentProjectSaveModalLabel = StrLit("New Project");}break;
   case ChangeProjectMethod_LoadProjectFromFile: {ProjectChange->CurrentProjectSaveModalLabel = StrLit("Open Project");}break;
   case ChangeProjectMethod_Quit: {ProjectChange->CurrentProjectSaveModalLabel = StrLit("Quit Project");}break;
  }
  UI_OpenPopup(ProjectChange->CurrentProjectSaveModalLabel);
 }
 else
 {
  switch (ChangeHow.Type)
  {
   case ChangeProjectMethod_LoadEmptyProject: {LoadEmptyProject(Editor);}break;
   case ChangeProjectMethod_Quit: {RequestQuit = true;}break;
   
   case ChangeProjectMethod_LoadProjectFromFile: {
    b32 Success = LoadProjectFromFile(Editor, ChangeHow.FilePath);
    if (!Success)
    {
     AddNotificationF(Editor, Notification_Error,
                      "failed to load project from \"%S\"",
                      ChangeHow.FilePath);
    }
   }break;
   
  }
 }
 return RequestQuit;
}

internal b32
SaveProjectWithNotifications(editor *Editor, string FilePath)
{
 b32 Saved = false;
 if (SaveProjectIntoFile(Editor, FilePath))
 {
  Saved = true;
 }
 else
 {
  AddNotificationF(Editor, Notification_Error,
                   "failed to save project into \"%S\"",
                   FilePath);
 }
 return Saved;
}

internal b32
TrySaveProject(editor *Editor, b32 SaveAs)
{
 b32 Saved = false;
 temp_arena Temp = TempArena(0);
 
 if (!SaveAs && Editor->IsProjectFileBacked)
 {
  if (SaveProjectWithNotifications(Editor, Editor->ProjectFilePath))
  {
   Saved = true;
  }
 }
 else
 {
  platform_file_dialog_filter Project = {};
  Project.DisplayName = ProjectFileDisplayName(Temp.Arena);
  string Project_Extensions[] = { EditorSessionFileExtension };
  Project.ExtensionCount = ArrayCount(Project_Extensions);
  Project.Extensions = Project_Extensions;
  
  platform_file_dialog_filter AllFilters[] = { Project };
  
  platform_file_dialog_filters Filters = {};
  Filters.FilterCount = ArrayCount(AllFilters);
  Filters.Filters = AllFilters;
  
  platform_file_dialog_result SaveDialog = Platform.SaveFileDialog(Temp.Arena, Filters);
  if (SaveDialog.FileSelected)
  {
   if (SaveProjectWithNotifications(Editor, SaveDialog.FilePath))
   {
    Saved = true;
   }
  }
 }
 
 EndTemp(Temp);
 
 return Saved;
}

internal void
UpdateAndRenderChangingProject(editor *Editor, platform_input_output *Input)
{
 temp_arena Temp = TempArena(0);
 project_change_request_state *ProjectChange = &Editor->ProjectChange;
 b32 QuitRequested = false;
 
 switch (ProjectChange->Request)
 {
  case ProjectChangeRequest_OpenFile: {
   platform_file_dialog_filter PNG = {};
   PNG.DisplayName = StrLit("PNG");
   string PNG_Extensions[] = { StrLit("png") };
   PNG.ExtensionCount = ArrayCount(PNG_Extensions);
   PNG.Extensions = PNG_Extensions;
   
   platform_file_dialog_filter JPEG = {};
   JPEG.DisplayName = StrLit("JPEG");
   string JPEG_Extensions[] = { StrLit("jpg"), StrLit("jpeg"), StrLit("jpe") };
   JPEG.ExtensionCount = ArrayCount(JPEG_Extensions);
   JPEG.Extensions = JPEG_Extensions;
   
   platform_file_dialog_filter BMP = {};
   BMP.DisplayName = StrLit("Windows BMP File");
   string BMP_Extensions[] = { StrLit("bmp") };
   BMP.ExtensionCount = ArrayCount(BMP_Extensions);
   BMP.Extensions = BMP_Extensions;
   
   platform_file_dialog_filter Project = {};
   Project.DisplayName = ProjectFileDisplayName(Temp.Arena);
   string Project_Extensions[] = { EditorSessionFileExtension };
   Project.ExtensionCount = ArrayCount(Project_Extensions);
   Project.Extensions = Project_Extensions;
   
   platform_file_dialog_filter AllFilters[] = { PNG, JPEG, BMP, Project };
   
   platform_file_dialog_filters Filters = {};
   Filters.AnyFileFilter = true;
   Filters.FilterCount = ArrayCount(AllFilters);
   Filters.Filters = AllFilters;
   
   platform_file_dialog_result OpenDialog = Platform.OpenFileDialog(Temp.Arena, Filters);
   if (OpenDialog.FileSelected)
   {
    string FilePath = OpenDialog.FilePath;
    string Extension = StrAfterLastDot(FilePath);
    if (StrEqual(Extension, EditorSessionFileExtension))
    {
     change_project_method Change = {};
     Change.Type = ChangeProjectMethod_LoadProjectFromFile;
     Change.FilePath = FilePath;
     QuitRequested = TryChangeProject(Editor, Change, false);
    }
    else
    {
     camera *Camera = &Editor->Camera;
     TryLoadImages(Editor, 1, &FilePath, Camera->P);
    }
   }
  }break;
  
  case ProjectChangeRequest_NewProject: {
   change_project_method Change = {};
   Change.Type = ChangeProjectMethod_LoadEmptyProject;
   QuitRequested = TryChangeProject(Editor, Change, false);
  }break;
  
  case ProjectChangeRequest_SaveProject: {
   TrySaveProject(Editor, false);
  }break;
  
  case ProjectChangeRequest_SaveProjectAs: {
   TrySaveProject(Editor, true);
  }break;
  
  case ProjectChangeRequest_Quit: {
   change_project_method Change = {};
   Change.Type = ChangeProjectMethod_Quit;
   QuitRequested = TryChangeProject(Editor, Change, false);
  }break;
  
  case ProjectChangeRequest_None: {}break;
 }
 
 if (UI_BeginPopupModal(&ProjectChange->CurrentProjectSaveModalIsOpen,
                        ProjectChange->CurrentProjectSaveModalLabel))
 {
  UI_Text(false, StrLit("Do you want to save your work?"));
  UI_TextF(false, "There are unsaved changes in \"%S\".", GetBaseProjectTitle(Editor));
  
  if (UI_Button(StrLit("Yes")))
  {
   if (TrySaveProject(Editor, false))
   {
    QuitRequested = TryChangeProject(Editor, ProjectChange->ChangeHow, true);
   }
   UI_CloseCurrentPopup();
  }
  UI_SameRow();
  if (UI_Button(StrLit("No")))
  {
   QuitRequested = TryChangeProject(Editor, ProjectChange->ChangeHow, true);
   UI_CloseCurrentPopup();
  }
  
  UI_EndPopup();
 }
 
 ProjectChange->Request = ProjectChangeRequest_None; 
 if (QuitRequested)
 {
  Input->QuitRequested = true;
 }
 
 EndTemp(Temp);
}

internal void
EditorUpdateAndRenderImpl(editor_memory *Memory, platform_input_output *Input, struct render_frame *Frame)
{
 editor *Editor = Memory->Editor;
 if (!Editor)
 {
  arena *PermamentArena = Memory->PermamentArena;
  Editor = Memory->Editor = PushStruct(PermamentArena, editor);
  Editor->Persistent.Memory = Memory;
  
  // TODO(hbr): Quick fix
  NilExpr = &Editor->NilParametricExpr;
  DEBUG_Vars = &Editor->DEBUG_Vars;
  LoadLastSessionOrEmptyProject(Editor);
  InitGlobalsOnInitOrCodeReload(Editor);
  
  // NOTE(hbr): Sanity check that I didn't mess up keyboard shortcut definitions
  ForEachEnumVal(EditorCmd, EditorCommand_Count, editor_command)
  {
   Assert(EditorKeyboardShortcuts[EditorCmd].Command == EditorCmd);
  }
 }
 
 BeginEditorFrame(Editor);
 
 render_group RenderGroup_ = {};
 render_group *RenderGroup = &RenderGroup_;
 {
  camera *Camera = &Editor->Camera;
  RenderGroup_ = BeginRenderGroup(Frame,
                                  Camera->P, Camera->Rotation, Camera->Zoom,
                                  Editor->BackgroundColor);
 }
 
 ProcessImageLoadingTasks(Editor);
 ProcessInputEvents(Editor, Input, RenderGroup);
 
 if (!Editor->HideUI)
 {
  ProfileBegin("UI Update");
  
  RenderSelectedEntityUI(Editor, RenderGroup);
  RenderMenuBarUI(Editor);
  RenderEntityListUI(Editor, RenderGroup);
  RenderDiagnosticsUI(Editor, Input);
  RenderHelpUI(Editor);
  RenderAnimatingCurvesUI(Editor);
  RenderMergingCurvesUI(Editor);
  RenderDevConsoleUI(Editor);
  RenderProfilerUI(Editor);
  
#if BUILD_DEBUG
  UI_RenderDemoWindow();
#endif
  
  ProfileEnd();
 }
 UpdateAndRenderChangingProject(Editor, Input);
 
 UpdateCamera(&Editor->Camera, Input);
 UpdateFrameStats(&Editor->FrameStats, Input);
 RenderGrid(Editor, RenderGroup);
 UpdateAndRenderEntities(Editor, RenderGroup);
 UpdateAndRenderAnimatingCurves(Editor, Input, RenderGroup);
 UpdateAndRenderNotifications(Editor, Input, RenderGroup);
 RenderMergingCurves(&Editor->MergingCurves, RenderGroup);
 RenderRotationIndicator(Editor, RenderGroup);
 RenderRemoveIndicator(Editor, RenderGroup);
 RenderCurveShadowWhenMoving(Editor, RenderGroup);
 
 Input->ProfilingStopped = Editor->Profiler.Stopped;
#if BUILD_DEBUG
 Input->RefreshRequested = true;
#endif
 
 EndEditorFrame(Editor);
}

internal void
EditorOnCodeReloadImpl(editor_memory *Memory)
{
 Platform = Memory->PlatformAPI;
 ProfilerEquip(Memory->Profiler);
 InitGlobalsOnInitOrCodeReload(Memory->Editor);
}

#if BUILD_HOT_RELOAD
DLL_EXPORT
#endif
EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender)
{
 ProfileFunctionBegin();
 EditorUpdateAndRenderImpl(Memory, Input, Frame);
 ProfileEnd();
}

#if BUILD_HOT_RELOAD
DLL_EXPORT
#endif
EDITOR_ON_CODE_RELOAD(EditorOnCodeReload)
{
 EditorOnCodeReloadImpl(Memory);
}