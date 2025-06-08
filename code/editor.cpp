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
#include "editor_entity.cpp"
#include "editor_ui.cpp"
#include "editor_stb.cpp"
#include "editor_debug.cpp"
#include "editor_camera.cpp"
#include "editor_sort.cpp"
#include "editor_collision.cpp"
#include "editor_core.cpp"
#include "editor_editor.cpp"
#include "editor_platform.cpp"

platform_api Platform;

internal f32
GetCurvePartZOffset(curve_part Part)
{
 Assert(Part < CurvePart_Count);
 f32 Result = Cast(f32)(CurvePart_Count-1 - Part) / CurvePart_Count;
 return Result;
}

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
  Reset = UI_MenuItem(0, 0, StrLit("Reset"));
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
 
 mat3 Model = ModelTransform(Entity->P, Entity->Rotation, Entity->Scale);
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
 
 if (Tracking->Active)
 {
  switch (Tracking->Type)
  {
   case PointTrackingAlongCurve_BezierCurveSplit: {
    f32 Radius = GetCurveTrackedPointRadius(Curve);
    v4 Color = V4(0.0f, 1.0f, 0.0f, 0.5f);
    f32 OutlineThickness = 0.3f * Radius;
    v4 OutlineColor = DarkenColor(Color, 0.5f);
    
    PushCircle(RenderGroup,
               Tracking->LocalSpaceTrackedPoint,
               Radius, Color,
               GetCurvePartZOffset(CurvePart_BezierSplitPoint),
               OutlineThickness, OutlineColor);
   }break;
   
   case PointTrackingAlongCurve_DeCasteljauVisualization: {
    u32 IterationCount = Tracking->Intermediate.IterationCount;
    f32 P = 0.0f;
    f32 Delta_P = 1.0f / (IterationCount - 1);
    v4 GradientA = RGBA_Color(255, 0, 144);
    v4 GradientB = RGBA_Color(155, 200, 0);
    // TODO(hbr): Shouldn't this use some of the functions that are already used for drwaing curve points to be consistent?
    f32 PointSize = CurveParams->PointRadius;
    
    u32 PointIndex = 0;
    for (u32 Iteration = 0;
         Iteration < IterationCount;
         ++Iteration)
    {
     v4 IterationColor = Lerp(GradientA, GradientB, P);
     
     PushVertexArray(RenderGroup,
                     Tracking->LineVerticesPerIteration[Iteration].Vertices,
                     Tracking->LineVerticesPerIteration[Iteration].VertexCount,
                     Tracking->LineVerticesPerIteration[Iteration].Primitive,
                     IterationColor, GetCurvePartZOffset(CurvePart_DeCasteljauAlgorithmLines));
     
     for (u32 I = 0; I < IterationCount - Iteration; ++I)
     {
      v2 Point = Tracking->Intermediate.P[PointIndex];
      PushCircle(RenderGroup,
                 Point, PointSize, IterationColor,
                 GetCurvePartZOffset(CurvePart_DeCasteljauAlgorithmPoints));
      ++PointIndex;
     }
     
     P += Delta_P;
    }
   }break;
  }
 }
}

internal void
UpdateAndRenderDegreeLowering(rendering_entity_handle Handle)
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
   Assert(Lowering->LowerDegree.MiddlePointIndex < Curve->ControlPointCount);
   
   if (MixChanged)
   {
    v2 NewControlPoint = Lerp(Lowering->LowerDegree.P_I, Lowering->LowerDegree.P_II, Lowering->MixParameter);
    f32 NewControlPointWeight = Lerp(Lowering->LowerDegree.W_I, Lowering->LowerDegree.W_II, Lowering->MixParameter);
    
    control_point_handle MiddlePoint = ControlPointHandleFromIndex(Lowering->LowerDegree.MiddlePointIndex);
    SetCurveControlPoint(&EntityWitness,
                         MiddlePoint,
                         NewControlPoint,
                         NewControlPointWeight);
   }
   
   if (Revert)
   {
    SetCurveControlPoints(&EntityWitness,
                          Curve->ControlPointCount + 1,
                          Lowering->SavedControlPoints,
                          Lowering->SavedControlPointWeights,
                          Lowering->SavedCubicBezierPoints);
   }
   
   if (Ok || Revert || !IsDegreeLoweringWindowOpen)
   {
    Lowering->Active = false;
   }
  }
  
  if (Lowering->Active)
  {
   v4 Color = Curve->Params.LineColor;
   Color.A *= 0.5f;
   
   PushVertexArray(RenderGroup,
                   Lowering->SavedLineVertices.Vertices,
                   Lowering->SavedLineVertices.VertexCount,
                   Lowering->SavedLineVertices.Primitive,
                   Color,
                   GetCurvePartZOffset(CurvePart_LineShadow));
  }
  
  EndEntityModify(EntityWitness);
 }
}

internal void
LoadImageWork(void *UserData)
{
 load_image_work *Work = Cast(load_image_work *)UserData;
 
 thread_task_memory_store *Store = Work->Store;
 thread_task_memory *Task = Work->TaskMemory;
 renderer_transfer_op *TextureOp = Work->TextureOp;
 string ImagePath = Work->ImagePath;
 image_loading_task *ImageLoading = Work->ImageLoading;
 arena *Arena = Task->Arena;
 
 string ImageData = Platform.ReadEntireFile(Arena, Work->ImagePath);
 loaded_image LoadedImage = LoadImageFromMemory(Arena, ImageData.Data, ImageData.Count);
 
 image_loading_state AsyncTaskState;
 renderer_transfer_op_state OpState;
 if (LoadedImage.Success)
 {
  // TODO(hbr): Don't do memory copy, just read into that memory directly
  MemoryCopy(TextureOp->Pixels, LoadedImage.Pixels, LoadedImage.Info.SizeInBytes);
  OpState = RendererOp_ReadyToTransfer;
  AsyncTaskState = Image_Loaded;
 }
 else
 {
  OpState = RendererOp_Empty;
  AsyncTaskState = Image_Failed;
 }
 
 CompilerWriteBarrier;
 TextureOp->State = OpState;
 ImageLoading->State = AsyncTaskState;
 
 EndThreadTaskMemory(Store, Task);
}

internal void
TryLoadImages(editor *Editor, u32 FileCount, string *FilePaths, v2 AtP)
{
 work_queue *WorkQueue = Editor->LowPriorityQueue;
 entity_store *EntityStore = &Editor->EntityStore;
 thread_task_memory_store *ThreadTaskMemoryStore = &Editor->ThreadTaskMemoryStore;
 image_loading_store *ImageLoadingStore = &Editor->ImageLoadingStore;
 
 ForEachIndex(FileIndex, FileCount)
 {
  string FilePath = FilePaths[FileIndex];
  
  image_info ImageInfo = LoadImageInfo(FilePath);
  render_texture_handle TextureHandle = AllocTextureHandle(EntityStore);
  renderer_transfer_op *TextureOp = PushTextureTransfer(Editor->RendererQueue, ImageInfo.Width, ImageInfo.Height, ImageInfo.Channels, TextureHandle);
  image_loading_task *ImageLoading = BeginAsyncImageLoadingTask(ImageLoadingStore);
  thread_task_memory *TaskMemory = BeginThreadTaskMemory(ThreadTaskMemoryStore);
  
  string FileName = PathLastPart(FilePath);
  string FileNameNoExt = StrChopLastDot(FileName);
  entity *Entity = AllocEntity(EntityStore, false);
  InitEntityAsImage(Entity, AtP, FileNameNoExt, ImageInfo.Width, ImageInfo.Height, TextureHandle);
  
  ImageLoading->Entity = Entity;
  ImageLoading->ImageFilePath = StrCopy(ImageLoading->Arena, FilePath);
  
  load_image_work *Work = PushStruct(TaskMemory->Arena, load_image_work);
  Work->Store = ThreadTaskMemoryStore;
  Work->TaskMemory = TaskMemory;
  Work->TextureOp = TextureOp;
  Work->ImagePath = StrCopy(TaskMemory->Arena, FilePath);
  Work->ImageLoading = ImageLoading;
  
  Platform.WorkQueueAddEntry(WorkQueue, LoadImageWork, Work);
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
UpdateAndRenderAnimatingCurves(animating_curves_state *Animation, platform_input_output *Input, render_group *RenderGroup)
{
 entity *Entity0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
 entity *Entity1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
 
 if ((Animation->Flags & AnimatingCurves_Active) && Entity0 && Entity1)
 {
  if (Animation->Flags & AnimatingCurves_Animating)
  {
   f32 dt = UseAndExtractDeltaTime(Input);
   UpdateBouncingParam(&Animation->Bouncing, dt);
  }
  
  v2 Points[4] = {
   V2(0.0f, 0.0f),
   V2(1.0f, 0.0f),
   V2(0.0f, 1.0f),
   V2(1.0f, 1.0f),
  };
  f32 MappedT = BezierCurveEvaluate(Animation->Bouncing.T, Points, 4).Y;
  
  curve *Curve0 = SafeGetCurve(Entity0);
  curve *Curve1 = SafeGetCurve(Entity1);
  
  v2 *CurveSamples0 = Curve0->CurveSamples;
  v2 *CurveSamples1 = Curve1->CurveSamples;
  
  u32 CurveSampleCount0 = Curve0->CurveSampleCount;
  u32 CurveSampleCount1 = Curve1->CurveSampleCount;
  
  // TODO(hbr): Multithread this
  arena *Arena = Animation->Arena;
  ClearArena(Arena);
  
  // TODO(hbr): Use max instead
  u32 CurveSampleCount = Min(CurveSampleCount0, CurveSampleCount1);
  v2 *CurveSamples = PushArrayNonZero(Arena, CurveSampleCount, v2);
  for (u32 LinePointIndex = 0;
       LinePointIndex < CurveSampleCount;
       ++LinePointIndex)
  {
   u32 LinePointIndex0 = ClampTop(CurveSampleCount0 * LinePointIndex / (CurveSampleCount - 1), CurveSampleCount0 - 1);
   u32 LinePointIndex1 = ClampTop(CurveSampleCount1 * LinePointIndex / (CurveSampleCount - 1), CurveSampleCount1 - 1);
   
   if (IsCurveReversed(Entity0)) LinePointIndex0 = (CurveSampleCount0-1 - LinePointIndex0);
   if (IsCurveReversed(Entity1)) LinePointIndex1 = (CurveSampleCount1-1 - LinePointIndex1);
   
   v2 PointLocal0 = CurveSamples0[LinePointIndex0];
   v2 PointLocal1 = CurveSamples1[LinePointIndex1];
   
   v2 Point0 = LocalEntityPositionToWorld(Entity0, PointLocal0);
   v2 Point1 = LocalEntityPositionToWorld(Entity1, PointLocal1);
   
   v2 Point  = Lerp(Point0, Point1, MappedT);
   CurveSamples[LinePointIndex] = Point;
  }
  
  f32 LineWidth0 = Curve0->Params.LineWidth;
  f32 LineWidth1 = Curve1->Params.LineWidth;
  f32 LineWidth  = Lerp(LineWidth0, LineWidth1, MappedT);
  
  v4 LineColor0 = Curve0->Params.LineColor;
  v4 LineColor1 = Curve1->Params.LineColor;
  v4 LineColor  = Lerp(LineColor0, LineColor1, MappedT);
  
  f32 ZOffset0 = Cast(f32)Entity0->SortingLayer;
  f32 ZOffset1 = Cast(f32)Entity1->SortingLayer;
  f32 ZOffset  = Lerp(ZOffset0, ZOffset1, MappedT);
  
  vertex_array LineVertices = ComputeVerticesOfThickLine(Arena, CurveSampleCount, CurveSamples, LineWidth, false);
  PushVertexArray(RenderGroup,
                  LineVertices.Vertices, LineVertices.VertexCount, LineVertices.Primitive,
                  LineColor,
                  ZOffset);
 }
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
   
   if (!CurveParams->LineDisabled)
   {
    PushVertexArray(RenderGroup,
                    Curve->CurveVertices.Vertices,
                    Curve->CurveVertices.VertexCount,
                    Curve->CurveVertices.Primitive,
                    Curve->Params.LineColor,
                    GetCurvePartZOffset(CurvePart_CurveLine));
   }
   
   if (IsPolylineVisible(Curve))
   {
    PushVertexArray(RenderGroup,
                    Curve->PolylineVertices.Vertices,
                    Curve->PolylineVertices.VertexCount,
                    Curve->PolylineVertices.Primitive,
                    Curve->Params.PolylineColor,
                    GetCurvePartZOffset(CurvePart_CurvePolyline));
   }
   
   if (IsConvexHullVisible(Curve))
   {
    PushVertexArray(RenderGroup,
                    Curve->ConvexHullVertices.Vertices,
                    Curve->ConvexHullVertices.VertexCount,
                    Curve->ConvexHullVertices.Primitive,
                    Curve->Params.ConvexHullColor,
                    GetCurvePartZOffset(CurvePart_CurveConvexHull));
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
     f32 HelperLineWidth = 0.2f * CurveParams->LineWidth;
     
     // TODO(hbr): It is really fucked up I have to add Entity->P when I do [SetTransform] above.
     // clean up this mess
     PushLine(RenderGroup,
              Entity->P + BezierPoint,
              Entity->P + CenterPoint,
              HelperLineWidth,
              CurveParams->LineColor,
              GetCurvePartZOffset(CurvePart_CubicBezierHelperLines));
     PushCircle(RenderGroup,
                BezierPoint,
                BezierPointRadius, CurveParams->PointColor,
                GetCurvePartZOffset(CurvePart_CubicBezierHelperPoints));
    }
    
    u32 ControlPointCount = Curve->ControlPointCount;
    v2 *ControlPoints = Curve->ControlPoints;
    for (u32 PointIndex = 0;
         PointIndex < ControlPointCount;
         ++PointIndex)
    {
     point_info PointInfo = GetCurveControlPointInfo(Entity, ControlPointHandleFromIndex(PointIndex));
     PushCircle(RenderGroup,
                ControlPoints[PointIndex],
                PointInfo.Radius,
                PointInfo.Color,
                GetCurvePartZOffset(CurvePart_CurveControlPoint),
                PointInfo.OutlineThickness,
                PointInfo.OutlineColor);
    }
   }
   
   if (Are_B_SplineKnotsVisible(Curve))
   {
    u32 PartitionSize = Curve->B_SplineKnotParams.PartitionSize;
    v2 *PartitionKnotPoints = Curve->B_SplinePartitionKnotPoints;
    b_spline_params B_Spline = CurveParams->B_Spline;
    point_info KnotPointInfo = Get_B_SplineKnotPointInfo(Entity);
    
    for (u32 KnotIndex = 0;
         KnotIndex < PartitionSize;
         ++KnotIndex)
    {
     v2 Knot = PartitionKnotPoints[KnotIndex];
     PushCircle(RenderGroup,
                Knot,
                KnotPointInfo.Radius,
                KnotPointInfo.Color,
                GetCurvePartZOffset(CurvePart_B_SplineKnot),
                KnotPointInfo.OutlineThickness,
                KnotPointInfo.OutlineColor);
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
UpdateAndRenderEntities(editor *Editor, render_group *RenderGroup)
{
 entity_array Entities = AllEntityArrayFromStore(&Editor->EntityStore);
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  if (IsEntityVisible(Entity))
  {
   rendering_entity_handle Handle = BeginRenderingEntity(Entity, RenderGroup);
   
   RenderEntity(Handle);
   UpdateAndRenderDegreeLowering(Handle);
   UpdateAndRenderPointTracking(Handle);
   
   EndRenderingEntity(Handle);
  }
 }
}

struct update_parametric_curve_var
{
 b32 Changed;
 b32 Remove;
 f32 Value;
};
enum update_parametric_curve_var_mode
{
 UpdateParametricCurveVar_Static,
 UpdateParametricCurveVar_Dynamic,
};
internal update_parametric_curve_var
UpdateAndRenderParametricCurveVar(arena *Arena,
                                  parametric_curve_var *Var,
                                  b32 ForceRecomputeEquation,
                                  string *VarNames,
                                  f32 *VarValues,
                                  u32 VarCount,
                                  update_parametric_curve_var_mode Mode)
{
 b32 VarChanged = false;
 
 b32 VarEquationChanged = false;
 if (Var->EquationMode)
 {
  VarEquationChanged = UI_InputText(Var->VarEquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, StrLit("##Equation")).Changed;
 }
 else
 {
  VarChanged |= UI_DragFloat(&Var->DragValue, 0, 0, 0, StrLit("##Drag"));
 }
 
 if (ForceRecomputeEquation || VarEquationChanged)
 {
  VarChanged = true;
  
  string VarEquationInput = StrFromCStr(Var->VarEquationBuffer);
  
  parametric_equation_eval_result VarEval = ParametricEquationEval(Arena, VarEquationInput, VarCount, VarNames, VarValues);
  Var->EquationFail = VarEval.Fail;
  Var->EquationErrorMessage = VarEval.ErrorMessage;
  Var->EquationValue = VarEval.Value;
 }
 
 if (Var->EquationMode)
 {
  Var->DragValue = Var->EquationValue;
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
  if (Var->EquationMode)
  {
   ButtonLabel = StrLit("S");
   TooltipContents = StrLit("Switch to slider");
  }
  else
  {
   ButtonLabel = StrLit("E");
   TooltipContents = StrLit("Switch to equation");
  }
  
  UI_SameRow();
  SwapMode = UI_Button(ButtonLabel);
  if (UI_IsItemHovered())
  {
   UI_Tooltip(TooltipContents);
  }
 }
 
 if (Var->EquationMode && Var->EquationFail)
 {
  UI_Colored(UI_Color_Text, RedColor)
  {
   UI_SameRow();
   UI_Text(false, Var->EquationErrorMessage);
  }
 }
 
 if (SwapMode)
 {
  VarChanged = true;
  Var->EquationMode = !Var->EquationMode;
 }
 
 if (Remove)
 {
  VarChanged = true;
 }
 
 update_parametric_curve_var Result = {};
 Result.Changed = VarChanged;
 Result.Remove = Remove;
 Result.Value = (Var->EquationMode ? Var->EquationValue : Var->DragValue);
 
 return Result;
}

internal void
FocusCameraOnEntity(camera *Camera, entity *Entity, render_group *RenderGroup)
{
 rect2 AABB = EmptyAABB();
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   u32 SampleCount = Curve->CurveSampleCount;
   v2 *Samples = Curve->CurveSamples;
   u32 SampleIndex = 0;
   while (SampleIndex + 8 < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex + 0]);
    AddPointAABB(&AABB, Samples[SampleIndex + 1]);
    AddPointAABB(&AABB, Samples[SampleIndex + 2]);
    AddPointAABB(&AABB, Samples[SampleIndex + 3]);
    AddPointAABB(&AABB, Samples[SampleIndex + 4]);
    AddPointAABB(&AABB, Samples[SampleIndex + 5]);
    AddPointAABB(&AABB, Samples[SampleIndex + 6]);
    AddPointAABB(&AABB, Samples[SampleIndex + 7]);
    SampleIndex += 8;
   }
   while (SampleIndex < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex]);
    ++SampleIndex;
   }
  } break;
  
  case Entity_Image: {
   image *Image = SafeGetImage(Entity);
   v2 Dim = Image->Dim;
   AddPointAABB(&AABB, V2( Dim.X,  Dim.Y));
   AddPointAABB(&AABB, V2( Dim.X, -Dim.Y));
   AddPointAABB(&AABB, V2(-Dim.X,  Dim.Y));
   AddPointAABB(&AABB, V2(-Dim.X, -Dim.Y));
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
 
 if (IsNonEmpty(&AABB))
 {
  rect2_corners AABB_Corners = AABBCorners(AABB);
  rect2 AABB_Transformed = EmptyAABB();
  ForEachEnumVal(Corner, Corner_Count, corner)
  {
   v2 P = LocalEntityPositionToWorld(Entity, AABB_Corners.Corners[Corner]);
   AddPointAABB(&AABB_Transformed, P);
  }
  
  v2 TargetP = 0.5f * (AABB_Transformed.Min + AABB_Transformed.Max);
  v2 Size = AABB_Transformed.Max - AABB_Transformed.Min;
  v2 Expand = 0.6f * Size;
  AABB_Transformed.Min += Expand;
  AABB_Transformed.Max += Expand;
  
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
  
  rect2_corners Corners = AABBCorners(AABB_Transformed);
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

internal void
RenderSelectedEntityUI(editor *Editor, render_group *RenderGroup)
{
 entity *Entity = GetSelectedEntity(Editor);
 entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
 b32 DeleteEntity = false;
 b32 CrucialEntityParamChanged = false;
 
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
  
  if (UI_BeginWindow(&Editor->SelectedEntityWindow, UIWindowFlag_AutoResize, StrLit("Selected")))
  {
   UI_PushLabelF("SelectedEntity %u", Entity->Id);
   
   if (UI_BeginTabBar(StrLit("SelectedEntityTabBar")))
   {
    if (UI_BeginTabItem(StrLit("General")))
    {
     UI_InputText2(&Entity->NameBuffer, 0, StrLit("Name"));
     
     UI_DragFloat2(Entity->P.E, 0.0f, 0.0f, 0, StrLit("Position"));
     if (ResetCtxMenu(StrLit("PositionReset")))
     {
      Entity->P = V2(0.0f, 0.0f);
     }
     
     UI_AngleSlider(&Entity->Rotation, StrLit("Rotation"));
     if (ResetCtxMenu(StrLit("RotationReset")))
     {
      Entity->Rotation = Rotation2DZero();
     }
     
     UI_DragFloat2(Entity->Scale.E, 0.0f, 0.0f, 0, StrLit("Scale"));
     if (ResetCtxMenu(StrLit("ScaleReset")))
     {
      Entity->Scale = V2(1.0f, 1.0f);
     }
     
     {
      UI_PushLabel(StrLit("DragMe"));
      f32 UniformScale = 0.0f;
      UI_DragFloat(&UniformScale, 0.0f, 0.0f, "Drag Me!", StrLit("Uniform Scale"));
      f32 WidthOverHeight = Entity->Scale.X / Entity->Scale.Y;
      Entity->Scale = Entity->Scale + V2(WidthOverHeight * UniformScale, UniformScale);
      if (ResetCtxMenu(StrLit("DragMeReset")))
      {
       Entity->Scale = V2(1.0f, 1.0f);
      }
      UI_PopLabel();
     }
     
     {     
      b32 Visible = IsEntityVisible(Entity);
      UI_Checkbox(&Visible, StrLit("Visible"));
      SetEntityVisibility(Entity, Visible);
     }
     
     if (Curve)
     {
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
        
        case Curve_Parametric: {
         parametric_curve_params *Parametric = &CurveParams->Parametric;
         parametric_curve_resources *Resources = &Curve->ParametricResources;
         arena *EquationArena = Resources->Arena;
         temp_arena Temp = TempArena(EquationArena);
         b32 EquationChanged = false;
         
         b32 ArenaCleared = false;
         if (Resources->ShouldClearArena)
         {
          ArenaCleared = true;
          Resources->ShouldClearArena = false;
          ClearArena(EquationArena);
         }
         
         //- additional vars
         UI_Text(false, StrLit("Additional Vars"));
         UI_SameRow();
         
         UI_Disabled(!CanAddAdditionalVar(Resources))
         {
          if (UI_Button(StrLit("+")))
          {
           AddNewAdditionalVar(Resources);
          }
         }
         
         u32 VarCount = 0;
         string *VarNames = PushArrayNonZero(Temp.Arena, MAX_ADDITIONAL_VAR_COUNT, string);
         f32 *VarValues = PushArrayNonZero(Temp.Arena, MAX_ADDITIONAL_VAR_COUNT, f32);
         b32 VarChanged = false;
         
         ListIter(Var, Resources->AdditionalVarsHead, parametric_curve_var)
         {
          UI_PushId(Var->Id);
          
          ui_input_result VarName = UI_InputText(Var->VarNameBuffer, MAX_VAR_NAME_BUFFER_LENGTH, 4, StrLit("##Var"));
          if (VarName.Changed)
          {
           VarChanged = true;
          }
          UI_SameRow();
          UI_Text(false, StrLit(" := "));
          UI_SameRow();
          update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena, Var,
                                                                                 ArenaCleared || VarChanged,
                                                                                 VarNames, VarValues, VarCount,
                                                                                 UpdateParametricCurveVar_Dynamic);
          VarChanged |= Update.Changed;
          if (Update.Remove)
          {
           RemoveAdditionalVar(Resources, Var);
          }
          else
          {
           Assert(VarCount < MAX_ADDITIONAL_VAR_COUNT);
           VarValues[VarCount] = Update.Value;
           VarNames[VarCount] = VarName.Input;
           ++VarCount;
          }
          
          UI_PopId();
         }
         
         //- min/max bounds
         UI_Label(StrLit("t_min"))
         {
          UI_Disabled(true)
          {
           UI_InputText("t_min", 0, 4, StrLit("##t_min_label"));
          }
          UI_SameRow();
          UI_Text(false, StrLit(" := "));
          UI_SameRow();
          update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena,
                                                                                 &Resources->MinT_Var,
                                                                                 ArenaCleared || VarChanged,
                                                                                 VarNames, VarValues, VarCount,
                                                                                 UpdateParametricCurveVar_Static);
          EquationChanged |= Update.Changed;
          Parametric->MinT = Update.Value;
         }
         
         UI_Label(StrLit("t_max"))
         {
          UI_Disabled(true)
          {
           UI_InputText("t_max", 0, 4, StrLit("##t_max_label"));
          }
          UI_SameRow();
          UI_Text(false, StrLit(" := "));
          UI_SameRow();
          update_parametric_curve_var Update = UpdateAndRenderParametricCurveVar(EquationArena,
                                                                                 &Resources->MaxT_Var,
                                                                                 ArenaCleared || VarChanged,
                                                                                 VarNames, VarValues, VarCount,
                                                                                 UpdateParametricCurveVar_Static);
          EquationChanged |= Update.Changed;
          Parametric->MaxT = Update.Value;
         }
         
         //- (x,y) equations
         UI_Label(StrLit("x(t)"))
         {
          UI_Disabled(true)
          {
           UI_InputText("x(t)", 0, 4, StrLit("##x(t)_label"));
          }
          UI_SameRow();
          UI_Text(false, StrLit(" := "));
          UI_SameRow();
          ui_input_result X_Equation = UI_InputText(Resources->X_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, StrLit("##x(t)"));
          if (ArenaCleared || VarChanged || X_Equation.Changed)
          {
           EquationChanged = true;
           
           parametric_equation_parse_result X_Parse = ParametricEquationParse(EquationArena, X_Equation.Input, VarCount, VarNames, VarValues);
           Parametric->X_Equation = X_Parse.ParsedExpr;
           Resources->X_Fail = X_Parse.Fail;
           Resources->X_ErrorMessage = X_Parse.ErrorMessage;
          }
          if (Resources->X_Fail)
          {
           UI_SameRow();
           UI_Colored(UI_Color_Text, RedColor)
           {
            UI_Text(false, Resources->X_ErrorMessage);
           }
          }
          if (DEBUG_Settings->ParametricEquationDebugMode)
          {
           UI_ParametricEquationExpr(Parametric->X_Equation, StrLit("x(t) expr"));
          }
         }
         
         UI_Label(StrLit("y(t)"))
         {
          UI_Disabled(true)
          {
           UI_InputText("y(t)", 0, 4, StrLit("##y(t)_label"));
          }
          UI_SameRow();
          UI_Text(false, StrLit(" := "));
          UI_SameRow();
          ui_input_result Y_Equation = UI_InputText(Resources->Y_EquationBuffer, MAX_EQUATION_BUFFER_LENGTH, 0, StrLit("##y(t)"));
          if (ArenaCleared || VarChanged || Y_Equation.Changed)
          {
           EquationChanged = true;
           
           parametric_equation_parse_result Y_Parse = ParametricEquationParse(EquationArena, Y_Equation.Input, VarCount, VarNames, VarValues);
           Parametric->Y_Equation = Y_Parse.ParsedExpr;
           Resources->Y_Fail = Y_Parse.Fail;
           Resources->Y_ErrorMessage = Y_Parse.ErrorMessage;
          }
          if (Resources->Y_Fail)
          {
           UI_SameRow();
           UI_Colored(UI_Color_Text, RedColor)
           {
            UI_Text(false, Resources->Y_ErrorMessage);
           }
          }
          if (DEBUG_Settings->ParametricEquationDebugMode)
          {
           UI_ParametricEquationExpr(Parametric->Y_Equation, StrLit("y(t) expr"));
          }
         }
         
         if (EquationChanged)
         {
          CrucialEntityParamChanged = true;
         }
         
         if (EquationChanged && !ArenaCleared)
         {
          Resources->ShouldClearArena = true;
         }
         
         EndTemp(Temp);
        }break;
        
        case Curve_B_Spline: {
         b_spline_params *B_Spline = &CurveParams->B_Spline;
         b_spline_degree_bounds Bounds = B_SplineDegreeBounds(Curve->ControlPointCount);
         
         // TODO(hbr): I shouldn't just directly modify degree value
         CrucialEntityParamChanged |= UI_SliderInteger(SafeCastToPtr(Curve->B_SplineKnotParams.Degree, i32), Bounds.MinDegree, Bounds.MaxDegree, StrLit("Degree"));
         if (ResetCtxMenu(StrLit("Degree")))
         {
          Curve->B_SplineKnotParams.Degree = 1;
          CrucialEntityParamChanged = true;
         }
         
         CrucialEntityParamChanged |= UI_Combo(SafeCastToPtr(B_Spline->Partition, u32), B_SplinePartition_Count, B_SplinePartitionNames, StrLit("Partition"));
         if (ResetCtxMenu(StrLit("Partition")))
         {
          B_Spline->Partition = DefaultParams->B_Spline.Partition;
          CrucialEntityParamChanged = true;
         }
         
         UI_Checkbox(&B_Spline->ShowPartitionKnotPoints, StrLit("Partition Knots Visible"));
        }break;
        
        case Curve_Count: InvalidPath;
       }
       
       if (CurveHasWeights(Curve))
       {
        UI_SeparatorText(StrLit("Control Points"));
        
        b32 WeightChanged = false;
        u32 PointCount = Curve->ControlPointCount;
        f32 *Weights = Curve->ControlPointWeights;
        
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
        
        if (UI_BeginTree(StrLit("List of all weights")))
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
           WeightChanged |= UI_DragFloatF(&Weights[PointIndex], 0.0f, FLT_MAX, 0, "Point (%u)", PointIndex);
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
      if (UI_Button(StrLit("Copy")))
      {
       DuplicateEntity(Editor, Entity);
      }
      if (UI_IsItemHovered())
      {
       UI_Tooltip(StrLit("Duplicate entity, self explanatory"));
      }
      
      UI_SameRow();
      
      DeleteEntity = UI_Button(StrLit("Delete"));
      if (UI_IsItemHovered())
      {
       UI_Tooltip(StrLit("Delete entity, self explanatory"));
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
       UI_Disabled(!IsControlPointSelected(Curve))
       {
        if (UI_Button(StrLit("Split")))
        {
         SplitCurveOnControlPoint(Editor, &EntityWitness);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Split curve into two parts, based on the currently selected control point"));
        }
       }
       
       UI_SameRow();
       
       if (UI_ButtonF("Swap Side"))
       {
        Entity->Flags ^= EntityFlag_CurveAppendFront;
       }
       if (UI_IsItemHovered())
       {
        UI_Tooltip(StrLit("Swap the side to which append new control points"));
       }
       
       UI_Disabled(!IsRegularBezierCurve(Curve))
       {
        if (UI_Button(StrLit("Elevate Degree")))
        {
         ElevateBezierCurveDegree(Entity);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Elevate Bezier curve degree, while maintaining its shape"));
        }
        
        UI_SameRow();
        
        if (UI_Button(StrLit("Lower Degree")))
        {
         LowerBezierCurveDegree(Entity);
        }
        if (UI_IsItemHovered())
        {
         UI_Tooltip(StrLit("Lower Bezier curve degree, while maintaining its shape (if possible)"));
        }
       }
      }
     }
     
     if (Curve && IsCurveEligibleForPointTracking(Curve))
     {
      point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
      f32 Fraction = Tracking->Fraction;
      b32 Changed = false;
      
      b32 BezierTrackingActive = false;
      b32 SplittingTrackingActive = false;
      switch (Tracking->Type)
      {
       case PointTrackingAlongCurve_DeCasteljauVisualization: {BezierTrackingActive = Tracking->Active;}break;
       case PointTrackingAlongCurve_BezierCurveSplit: {SplittingTrackingActive = Tracking->Active;}break;
      }
      
      UI_Label(StrLit("DeCasteljauVisualization"))
      {
       if (UI_Checkbox(&BezierTrackingActive, StrLit("##DeCasteljauEnabled")))
       {
        Changed = true;
        Tracking->Active = BezierTrackingActive;
        if (Tracking->Active)
        {
         Tracking->Type = PointTrackingAlongCurve_DeCasteljauVisualization;
        }
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
        Tracking->Active = SplittingTrackingActive;
        if (Tracking->Active)
        {
         Tracking->Type = PointTrackingAlongCurve_BezierCurveSplit;
        }
       }
       UI_SameRow();
       UI_SeparatorText(StrLit("Split Bezier Curve"));
       
       if (SplittingTrackingActive)
       {
        Changed |= UI_SliderFloat(&Fraction, 0.0f, 1.0f, StrLit("t"));
        UI_SameRow();
        if (UI_Button(StrLit("Split!")))
        {
         PerformBezierCurveSplit(Editor, &EntityWitness);
        }
       }
      }
      
      if (Tracking->Active && Changed)
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
      UI_SeparatorText(StrLit("Line"));
      {
       b32 LineEnabled = !CurveParams->LineDisabled;
       UI_Checkbox(&LineEnabled, StrLit("Visible"));
       CurveParams->LineDisabled = !LineEnabled;
       
       UI_ColorPickerF(&CurveParams->LineColor, "Color");
       if (ResetCtxMenu(StrLit("ColorReset")))
       {
        CurveParams->LineColor = DefaultParams->LineColor;
       }
       
       CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->LineWidth, 0.0f, FLT_MAX, 0, "Width");
       if (ResetCtxMenu(StrLit("WidthReset")))
       {
        CurveParams->LineWidth = DefaultParams->LineWidth;
        CrucialEntityParamChanged = true;
       }
       
       if (IsCurveTotalSamplesMode(Curve))
       {
        CrucialEntityParamChanged |= UI_SliderIntegerF(SafeCastToPtr(CurveParams->TotalSamples, i32), 1, 5000, "Total Samples");
        if (ResetCtxMenu(StrLit("Samples")))
        {
         CurveParams->TotalSamples = DefaultParams->TotalSamples;
         CrucialEntityParamChanged = true;
        }
       }
       else
       {
        CrucialEntityParamChanged |= UI_SliderIntegerF(SafeCastToPtr(CurveParams->SamplesPerControlPoint, i32), 1, 500, "Samples per Control Point");
        if (ResetCtxMenu(StrLit("Samples")))
        {
         CurveParams->SamplesPerControlPoint = DefaultParams->SamplesPerControlPoint;
         CrucialEntityParamChanged = true;
        }
       }
      }
      
      if (UsesControlPoints(Curve))
      {
       UI_SeparatorText(StrLit("Control Points"));
       UI_Label(StrLit("Control Points"))
       {
        b32 PointsEnabled = !CurveParams->PointsDisabled;
        UI_Checkbox(&PointsEnabled, StrLit("Visible"));
        CurveParams->PointsDisabled = !PointsEnabled;
        
        UI_ColorPickerF(&CurveParams->PointColor, "Color");
        if (ResetCtxMenu(StrLit("PointColorReset")))
        {
         CurveParams->PointColor = DefaultParams->PointColor;
        }
        
        UI_DragFloatF(&CurveParams->PointRadius, 0.0f, FLT_MAX, 0, "Radius");
        if (ResetCtxMenu(StrLit("PointRadiusReset")))
        {
         CurveParams->PointRadius = DefaultParams->PointRadius;
        }
       }
      }
      
      if (CurveParams->Type == Curve_B_Spline)
      {
       b_spline_params *B_Spline = &CurveParams->B_Spline;
       
       UI_SeparatorText(StrLit("B-Spline Knots"));
       UI_Label(StrLit("B-Spline Knots"))
       {
        UI_Checkbox(&B_Spline->ShowPartitionKnotPoints, StrLit("Visible"));
        
        UI_ColorPicker(&B_Spline->KnotPointColor, StrLit("Color"));
        if (ResetCtxMenu(StrLit("ColorReset")))
        {
         B_Spline->KnotPointColor= DefaultParams->B_Spline.KnotPointColor;
        }
        
        UI_DragFloat(&B_Spline->KnotPointRadius, 0.0f, FLT_MAX, 0, StrLit("Radius"));
        if (ResetCtxMenu(StrLit("RadiusReset")))
        {
         B_Spline->KnotPointRadius = DefaultParams->B_Spline.KnotPointRadius;
        }
       }
      }
      
      if (UsesControlPoints(Curve))
      {
       UI_SeparatorText(StrLit("Polyline"));
       UI_Label(StrLit("Polyline"))
       {
        UI_Checkbox(&CurveParams->PolylineEnabled, StrLit("Visible"));
        
        UI_ColorPickerF(&CurveParams->PolylineColor, "Color");
        if (ResetCtxMenu(StrLit("PolylineColorReset")))
        {
         CurveParams->PolylineColor = DefaultParams->PolylineColor;
        }
        
        CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->PolylineWidth, 0.0f, FLT_MAX, 0, "Width");
        if (ResetCtxMenu(StrLit("PolylineWidthReset")))
        {
         CurveParams->PolylineWidth = DefaultParams->PolylineWidth;
         CrucialEntityParamChanged = true;
        }
       }
       
       UI_SeparatorText(StrLit("Convex Hull"));
       UI_Label(StrLit("Convex Hull"))
       {
        UI_Checkbox(&CurveParams->ConvexHullEnabled, StrLit("Visible"));
        
        UI_ColorPickerF(&CurveParams->ConvexHullColor, "Color");
        if (ResetCtxMenu(StrLit("ConvexHullColorReset")))
        {
         CurveParams->ConvexHullColor = DefaultParams->ConvexHullColor;
        }
        
        CrucialEntityParamChanged |= UI_DragFloatF(&CurveParams->ConvexHullWidth, 0.0f, FLT_MAX, 0, "Width");
        if (ResetCtxMenu(StrLit("ConvexHullWidthReset")))
        {
         CurveParams->ConvexHullWidth = DefaultParams->ConvexHullWidth;
         CrucialEntityParamChanged = true;
        }
       }
      }
      
      UI_EndTabItem();
     }
    }
    
    if (UI_BeginTabItem(StrLit("Info")))
    {
     if (Curve)
     {
      UI_Label(StrLit("Info"))
      {
       UI_TextF(false, "Number of control points  %u", Curve->ControlPointCount);
       UI_TextF(false, "Number of samples         %u", Curve->CurveSampleCount);
      }
     }
     
     if (Image)
     {
      // TODO(hbr): print some info
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
  DeallocEntity(&Editor->EntityStore, Entity);
 }
}

internal void
RenderMenuBarUI(editor *Editor,
                b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                b32 *SaveProjectAs, b32 *QuitProject)
{
 if (UI_BeginMainMenuBar())
 {
  if (UI_BeginMenu(StrLit("File")))
  {
   *NewProject     = UI_MenuItem(0, "Ctrl+N",       StrLit("New"));
   *OpenFileDialog = UI_MenuItem(0, "Ctrl+O",       StrLit("Open"));
   *SaveProject    = UI_MenuItem(0, "Ctrl+S",       StrLit("Save"));
   *SaveProjectAs  = UI_MenuItem(0, "Ctrl+Shift+S", StrLit("Save As"));
   *QuitProject    = UI_MenuItem(0, "Escape",       StrLit("Quit"));
   UI_EndMenu();
  }
  
  if(UI_BeginMenu(StrLit("Actions")))
  {
   if (UI_MenuItem(0, 0, StrLit("Animate Curves")))
   {
    BeginAnimatingCurves(&Editor->AnimatingCurves);
   }
   if (UI_MenuItem(0, 0, StrLit("Merge Curves")))
   {
    BeginMergingCurves(&Editor->MergingCurves);
   }
   UI_EndMenu();
  }
  
  if (UI_BeginMenu(StrLit("View")))
  {
   UI_MenuItem(&Editor->EntityListWindow, 0, StrLit("Entity List"));
   UI_MenuItem(&Editor->SelectedEntityWindow, 0, StrLit("Selected Entity"));
   UI_MenuItem(&Editor->ProfilerWindow, 0, StrLit("Profiler"));
   UI_EndMenu();
  }
  
  if(UI_BeginMenu(StrLit("Settings")))
  {
   UI_SeparatorText(StrLit("Camera"));
   {
    camera *Camera = &Editor->Camera;
    UI_DragFloat2(Camera->P.E, 0.0f, 0.0f, 0, StrLit("Position"));
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
  
  UI_MenuItem(&Editor->HelpWindow, 0, StrLit("Help"));
  
  UI_EndMainMenuBar();
 }
}

internal void
RenderEntityListWindowContents(editor *Editor, render_group *RenderGroup)
{
 ForEachEnumVal(EntityType, Entity_Count, entity_type)
 {
  entity_array Entities = EntityArrayFromType(&Editor->EntityStore, EntityType);
  
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
     if(UI_MenuItem(0, 0, StrLit("Delete")))
     {
      DeallocEntity(&Editor->EntityStore, Entity);
     }
     if(UI_MenuItem(0, 0, StrLit("Copy")))
     {
      DuplicateEntity(Editor, Entity);
     }
     if(UI_MenuItem(0, 0, (Entity->Flags & EntityFlag_Hidden) ? StrLit("Show") : StrLit("Hide")))
     {
      Entity->Flags ^= EntityFlag_Hidden;
     }
     if (UI_MenuItem(0, 0, (Selected ? StrLit("Deselect") : StrLit("Select"))))
     {
      SelectEntity(Editor, (Selected ? 0 : Entity));
     }
     if(UI_MenuItem(0, 0, StrLit("Focus")))
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
 if (Editor->HelpWindow)
 {
  if (UI_BeginWindow(&Editor->HelpWindow, UIWindowFlag_AutoResize, StrLit("Help")))
  {
   UI_Text(false,
           StrLit("Parametric Curve Editor overview and tutorial."));
   UI_NewRow();
   UI_Text(false,
           StrLit("Controls are made to be as intuitive as possible. Read this tutorial, but\n"
                  "there is no need to understand everything. Note which keys and mouse buttons\n"
                  "might do something interesting and go experiment as soon as possible instead."));
   UI_NewRow();
   UI_Text(false,
           StrLit("NOTE: Most things that apply to curves and curve manipulation (i.e.\n"
                  "move/select/delete) also apply to images, or in general - entities.\n\n"
                  
                  ));
   
   if (UI_CollapsingHeader(StrLit("Controls")))
   {
    if (UI_BeginTree(StrLit("Left mouse button")))
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
    
    if (UI_BeginTree(StrLit("Left mouse button + Left Ctrl")))
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
    
    if (UI_BeginTree(StrLit("Left mouse button + Left Alt")))
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
    
    if (UI_BeginTree(StrLit("Right mouse button")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Remove/deselect curves/control points."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking on curve deletes it"));
     UI_BulletText(StrLit("clicking on control point deletes it and selects that curve"));
     UI_BulletText(StrLit("clicking outside of curve (\"on nothing\") deselects currently selected curve (if one exists)"));
     UI_BulletText(StrLit("while splitting Bezier curve, clicking on split point, actually splits the curve"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Middle mouse button")))
    {
     UI_NewRow();
     UI_Text(false, StrLit("Manipulate the camera."));
     UI_NewRow();
     
     UI_Text(false, StrLit("In more detail:"));
     UI_BulletText(StrLit("clicking and holding moves the camera"));
     UI_BulletText(StrLit("scrolling (un)zooms the camera"));
     
     UI_NewRow();
     
     UI_EndTree();
    }
    
    if (UI_BeginTree(StrLit("Middle mouse button (+ Left Ctrl)")))
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
}

internal void
UpdateAndRenderNotifications(editor *Editor, platform_input_output *Input, render_group *RenderGroup)
{
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
    Fade = 1.0f - Square(1.0f - Fade);
    
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
       v4 TitleColor = {};
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
}

internal void
UpdateAndRenderChoose2CurvesUI(choose_2_curves_state *Choosing, editor *Editor)
{
 entity *Curve0 = EntityFromHandle(Choosing->Curves[0]);
 entity *Curve1 = EntityFromHandle(Choosing->Curves[1]);
 
 b32 ChoosingCurve = Choosing->WaitingForChoice;
 
 b32 Disable0 = false;
 string Button0 = {};
 if (ChoosingCurve && Choosing->ChoosingCurveIndex == 0)
 {
  Disable0 = true;
  Button0 = StrLit("Click on curve!");
 }
 else
 {
  if (Curve0)
  {
   Button0 = GetEntityName(Curve0);
  }
  else
  {
   Button0 = StrLit("...");
  }
 }
 
 UI_Id(0)
 {
  UI_TextF(false, "Curve 0: ");
  
  UI_SameRow();
  
  if (UI_Button(Button0))
  {
   if (Disable0)
   {
    Choosing->WaitingForChoice = false;
   }
   else
   {
    Choosing->WaitingForChoice = true;
    Choosing->ChoosingCurveIndex = 0;
   }
  }
  
  UI_SameRow();
  
  UI_Disabled(Curve0 == 0)
  {
   if (UI_Button(StrLit("Select")))
   {
    Assert(Curve0);
    SelectEntity(Editor, Curve0);
   }
  }
 }
 
 string Button1 = {};
 b32 Disable1 = false;
 if (ChoosingCurve && Choosing->ChoosingCurveIndex == 1)
 {
  Disable1 = true;
  Button1 = StrLit("Click on curve");
 }
 else
 {
  if (Curve1)
  {
   Button1 = GetEntityName(Curve1);
  }
  else
  {
   Button1 = StrLit("...");
  }
 }
 
 UI_Id(1)
 {
  UI_Text(false, StrLit("Curve 1: "));
  
  UI_SameRow();
  
  if (UI_Button(Button1))
  {
   if (Disable1)
   {
    Choosing->WaitingForChoice = false;
   }
   else
   {
    Choosing->WaitingForChoice = true;
    Choosing->ChoosingCurveIndex = 1;
   }
  }
  
  UI_SameRow();
  
  UI_Disabled(Curve1 == 0)
  {
   if (UI_Button(StrLit("Select")))
   {
    Assert(Curve1);
    SelectEntity(Editor, Curve1);
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
 if (Animation->Flags & AnimatingCurves_Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindow(&WindowOpen, UIWindowFlag_AutoResize, StrLit("Curve Animation"));
  
  if (WindowOpen)
  {
   UpdateAndRenderChoose2CurvesUI(&Animation->Choose2Curves, Editor);
   
   entity *Curve0 = EntityFromHandle(Animation->Choose2Curves.Curves[0]);
   entity *Curve1 = EntityFromHandle(Animation->Choose2Curves.Curves[1]);
   
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
    UI_Disabled((Curve0 == 0) || (Curve1 == 0))
    {
     if (UI_Button(StrLit("Start")))
     {
      Animation->Flags |= AnimatingCurves_Animating;
     }
    }
   }
   
   if (UI_BeginTree(StrLit("More")))
   {
    UI_SliderFloat(&Animation->Bouncing.Speed, 0.0f, 4.0f, StrLit("Speed"));
    UI_EndTree();
   }
  }
  else
  {
   EndAnimatingCurves(Animation);
  }
  
  UI_EndWindow();
 }
}

internal void
ProcessImageLoadingTasks(editor *Editor)
{
 image_loading_store *ImageLoadingStore = &Editor->ImageLoadingStore;
 ListIter(ImageLoading, ImageLoadingStore->Head, image_loading_task)
 {
  if (ImageLoading->State == Image_Loading)
  {
   // NOTE(hbr): nothing to do
  }
  else
  {
   if (ImageLoading->State == Image_Loaded)
   {
    entity *Entity = ImageLoading->Entity;
    SelectEntity(Editor, Entity);
   }
   else
   {
    Assert(ImageLoading->State == Image_Failed);
    AddNotificationF(Editor, Notification_Error,
                     "failed to load image from %S",
                     ImageLoading->ImageFilePath);
   }
   
   FinishAsyncImageLoadingTask(ImageLoadingStore, ImageLoading);
  }
 }
}

internal void
ProcessInputEvents(editor *Editor,
                   platform_input_output *Input,
                   render_group *RenderGroup,
                   b32 *NewProject, b32 *OpenFileDialog, b32 *SaveProject,
                   b32 *SaveProjectAs, b32 *QuitProject)
{
 ProfileFunctionBegin();
 
 editor_left_click_state *LeftClick = &Editor->LeftClick;
 editor_right_click_state *RightClick = &Editor->RightClick;
 editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
 animating_curves_state *Animation = &Editor->AnimatingCurves;
 merging_curves_state *Merging = &Editor->MergingCurves;
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
   
   entity_array Entities = AllEntityArrayFromStore(&Editor->EntityStore);
   
   collision Collision = {};
   if (Event->Flags & PlatformEventFlag_Alt)
   {
    // NOTE(hbr): Force no collision, so that user can add control point wherever they want
   }
   else
   {
    Collision = CheckCollisionWithEntities(Entities, MouseP, RenderGroup->CollisionTolerance);
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
    
    if ((Collision.Entity || SelectedEntity) && (Event->Flags & PlatformEventFlag_Ctrl))
    {
     entity *Entity = (Collision.Entity ? Collision.Entity : SelectedEntity);
     // NOTE(hbr): just move entity if ctrl is pressed
     BeginMovingEntity(LeftClick, MakeEntityHandle(Entity));
    }
    else if ((Collision.Flags & Collision_CurveLine) && CollisionTracking->Active)
    {
     BeginMovingTrackingPoint(LeftClick, MakeEntityHandle(Collision.Entity));
     
     f32 Fraction = SafeDiv0(Cast(f32)Collision.CurveSampleIndex, (CollisionCurve->CurveSampleCount- 1));
     SetTrackingPointFraction(&CollisionEntityWitness, Fraction);
    }
    else if (Collision.Flags & Collision_B_SplineKnot)
    {
     BeginMovingBSplineKnot(LeftClick, MakeEntityHandle(Collision.Entity), Collision.KnotIndex);
    }
    else if (Collision.Flags & Collision_CurvePoint)
    {
     BeginMovingCurvePoint(LeftClick, MakeEntityHandle(Collision.Entity), Collision.CurvePoint);
    }
    else if (Collision.Flags & Collision_CurveLine)
    {
     if (UsesControlPoints(CollisionCurve))
     {
      control_point_handle ControlPoint = ControlPointIndexFromCurveSampleIndex(CollisionCurve, Collision.CurveSampleIndex);
      u32 PointIndex = IndexFromControlPointHandle(ControlPoint);
      u32 InsertAt = PointIndex + 1;
      InsertControlPoint(&CollisionEntityWitness, MouseP, InsertAt);
      BeginMovingCurvePoint(LeftClick, MakeEntityHandle(Collision.Entity),
                            CurvePointFromControlPoint(ControlPointHandleFromIndex(InsertAt)));
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
     if (SelectedEntity && SelectedEntity->Type == Entity_Curve && UsesControlPoints(SelectedCurve))
     {
      TargetEntity = SelectedEntity;
     }
     else
     {
      temp_arena Temp = TempArena(0);
      
      entity_store *EntityStore = &Editor->EntityStore;
      entity *Entity = AllocEntity(EntityStore, false);
      string Name = StrF(Temp.Arena, "curve(%lu)", Editor->EverIncreasingEntityCounter++);
      InitEntityAsCurve(Entity, Name, Editor->CurveDefaultParams);
      TargetEntity = Entity;
      
      EndTemp(Temp);
     }
     Assert(TargetEntity);
     
     entity_with_modify_witness TargetEntityWitness = BeginEntityModify(TargetEntity);
     control_point_handle Appended = AppendControlPoint(&TargetEntityWitness, MouseP);
     BeginMovingCurvePoint(LeftClick, MakeEntityHandle(TargetEntity), CurvePointFromControlPoint(Appended));
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
   EndLeftClick(LeftClick);
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
      u32 KnotIndex = LeftClick->B_SplineKnotIndex;
      
      curve *Curve = SafeGetCurve(Entity);
      b_spline_knot_params KnotParams = Curve->B_SplineKnotParams;
      f32 *Knots = Curve->B_SplineKnots;
      
      f32 KnotFraction = Knots[KnotParams.Degree + KnotIndex];
      MovePointAlongCurve(Curve, &TranslateLocal, &KnotFraction, true);
      MovePointAlongCurve(Curve, &TranslateLocal, &KnotFraction, false);
      
      // TODO(hbr): Move this out to a function
      OS_PrintDebugF("index: %u, before: %f, after: %f\n", KnotIndex, Knots[KnotIndex], KnotFraction);
      Knots[KnotParams.Degree + KnotIndex] = KnotFraction;
      MarkEntityModified(&EntityWitness);
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
      if (Event->Flags & PlatformEventFlag_Shift)
      {
       Flags &= ~TranslateCurvePoint_MatchBezierTwinDirection;
      }
      if (Event->Flags & PlatformEventFlag_Ctrl)
      {
       Flags &= ~TranslateCurvePoint_MatchBezierTwinLength;
      }
      SetCurvePoint(&EntityWitness, LeftClick->CurvePoint, MouseP, Flags);
     }break;
     
     case EditorLeftClick_MovingEntity: {
      Entity->P += Translate;
     }break;
    }
    
    LeftClick->LastMouseP = MouseP;
    
    EndEntityModify(EntityWitness);
   }
  }
  
  //- right click events processing
  if (!Eat && IsKeyPress(Event, PlatformKey_RightMouseButton, AnyKeyModifier) && !RightClick->Active)
  {
   Eat = true;
   
   entity_array Entities = AllEntityArrayFromStore(&Editor->EntityStore);
   collision Collision = CheckCollisionWithEntities(Entities, MouseP, RenderGroup->CollisionTolerance);
   
   BeginRightClick(RightClick, MouseP, Collision);
   
   entity *Entity = Collision.Entity;
   if (Collision.Flags & Collision_CurvePoint)
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
   b32 ReleasedClose = (NormSquared(RightClick->ClickP - MouseP) <= Square(RenderGroup->CollisionTolerance));
   if (ReleasedClose)
   {
    collision *Collision = &RightClick->CollisionAtP;
    entity *Entity = Collision->Entity;
    entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
    curve *Curve = &Entity->Curve;
    
    if (Collision->Flags & Collision_TrackedPoint)
    {
     switch (Curve->PointTracking.Type)
     {
      case PointTrackingAlongCurve_BezierCurveSplit: {
       PerformBezierCurveSplit(Editor, &EntityWitness);
      }break;
      
      case PointTrackingAlongCurve_DeCasteljauVisualization: {}break;
     }
    }
    else if (Collision->Flags & Collision_CurvePoint)
    {
     if (Collision->CurvePoint.Type == CurvePoint_ControlPoint)
     {
      RemoveControlPoint(&EntityWitness, Collision->CurvePoint.Control);
     }
     else
     {
      SelectControlPointFromCurvePoint(Curve, Collision->CurvePoint);
     }
     SelectEntity(Editor, Entity);
    }
    else if (Entity)
    {
     DeallocEntity(&Editor->EntityStore, Entity);
    }
    else
    {
     SelectEntity(Editor, 0);
    }
    
    EndEntityModify(EntityWitness);
   }
   
   EndRightClick(RightClick);
  }
  
  //- camera control events processing
  if (!Eat && IsKeyPress(Event, PlatformKey_MiddleMouseButton, AnyKeyModifier))
  {
   Eat = true;
   BeginMiddleClick(MiddleClick, Event->Flags & PlatformEventFlag_Ctrl, Event->ClipSpaceMouseP);
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
    if (NormSquared(FromP - CenterP) >= Square(RenderGroup->RotationRadius) &&
        NormSquared(ToP   - CenterP) >= Square(RenderGroup->RotationRadius))
    {
     v2 Rotation = Rotation2DFromMovementAroundPoint(FromP, ToP, CenterP);
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
  
  if (!Eat && IsKeyPress(Event, PlatformKey_Tab, NoKeyModifier))
  {
   Eat = true;
   Editor->HideUI = !Editor->HideUI;
  }
  
  //- shortcuts
  if (!Eat && IsKeyPress(Event, PlatformKey_N, KeyModifierFlag(Ctrl)))
  {
   Eat = true;
   *NewProject = true;
  }
  
  if (!Eat && IsKeyPress(Event, PlatformKey_O, KeyModifierFlag(Ctrl)))
  {
   Eat = true;
   *OpenFileDialog = true;
  }
  
  if (!Eat && IsKeyPress(Event, PlatformKey_S, KeyModifierFlag(Ctrl)))
  {
   Eat = true;
   *SaveProject = true;
  }
  
  if (!Eat && IsKeyPress(Event, PlatformKey_S, KeyModifierFlag(Ctrl) | KeyModifierFlag(Shift)))
  {
   Eat = true;
   *SaveProjectAs = true;
  }
  
  if (!Eat && (IsKeyPress(Event, PlatformKey_Escape, NoKeyModifier) ||
               (Event->Type == PlatformEvent_WindowClose)))
  {
   Eat = true;
   *QuitProject = true;
  }
  
#if BUILD_DEBUG
  if (!Eat && IsKeyPress(Event, PlatformKey_Backtick, NoKeyModifier))
  {
   Eat = true;
   Editor->DevConsole = !Editor->DevConsole;
  }
#endif
  
  if (!Eat &&
      (IsKeyPress(Event, PlatformKey_Delete, PlatformEventFlag_None) ||
       IsKeyPress(Event, PlatformKey_X, KeyModifierFlag(Ctrl))))
  {
   entity *Entity = GetSelectedEntity(Editor);
   entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
   
   if (Entity)
   {
    Eat = true;
    switch (Entity->Type)
    {
     case Entity_Curve: {
      curve *Curve = SafeGetCurve(Entity);
      if (IsControlPointSelected(Curve))
      {
       RemoveControlPoint(&EntityWitness, Curve->SelectedControlPoint);
      }
      else
      {
       DeallocEntity(&Editor->EntityStore, Entity);
      }
     }break;
     
     case Entity_Image: {
      DeallocEntity(&Editor->EntityStore, Entity);
     }break;
     
     case Entity_Count: InvalidPath;
    }
   }
   
   EndEntityModify(EntityWitness);
  }
  
  if (IsKeyPress(Event, PlatformKey_D, KeyModifierFlag(Ctrl)))
  {
   entity *Entity = GetSelectedEntity(Editor);
   if (Entity)
   {
    Eat = true;
    DuplicateEntity(Editor, Entity);
   }
  }
  
  if (!Eat && IsKeyPress(Event, PlatformKey_Q, KeyModifierFlag(Ctrl)))
  {
   Eat = true;
   Editor->Profiler.Stopped = !Editor->Profiler.Stopped;
  }
  
  //- misc
  if (!Eat && Event->Type == PlatformEvent_FilesDrop)
  {
   Eat = true;
   v2 AtP = Unproject(RenderGroup, Event->ClipSpaceMouseP);
   TryLoadImages(Editor, Event->FileCount, Event->FilePaths, AtP);
  }
 }
 
 // NOTE(hbr): these "sanity checks" are only necessary because ImGui captured any input that ends
 // with some possible ImGui interaction - e.g. we start by clicking in our editor but end releasing
 // on some ImGui window
 if (!Input->Pressed[PlatformKey_LeftMouseButton])
 {
  EndLeftClick(LeftClick);
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
             curve_merge_method Method,
             entity_store *EntityStore)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve0 = SafeGetCurve(Entity0);
 curve *Curve1 = SafeGetCurve(Entity1);
 entity *MergeEntity = MergeWitness->Entity;
 
 string Name = StrF(Temp.Arena, "%S+%S", GetEntityName(Entity0), GetEntityName(Entity1));
 InitEntityFromEntity(EntityStore, MergeWitness, Entity0);
 SetEntityName(MergeEntity, Name);
 
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 u32 PointCount0 = Curve0->ControlPointCount;
 u32 PointCount1 = Curve1->ControlPointCount;
 
 v2 *Points0 = Curve0->ControlPoints;
 v2 *Points1 = Curve1->ControlPoints;
 
 f32 *Weights0 = Curve0->ControlPointWeights;
 f32 *Weights1 = Curve1->ControlPointWeights;
 
 cubic_bezier_point *Beziers0 = Curve0->CubicBezierPoints;
 cubic_bezier_point *Beziers1 = Curve1->CubicBezierPoints;
 
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
    v2 P0 = Points0[PointCount0 - 1];
    
    v2 P1 = Points1[0];
    P1 = LocalEntityPositionToWorld(Entity1, P1);
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
 ArrayCopy(Points,                Points0,               PointCount0);
 ArrayCopy(Beziers,               Beziers0,              PointCount0);
 
 u32 PointIndex = PointCount0;
 for (u32 PointIndex1 = DropCount1;
      PointIndex1 < PointCount1;
      ++PointIndex1)
 {
  v2 Point1 = Points1[PointIndex1];
  Point1 = LocalEntityPositionToWorld(Entity1, Point1);
  Point1 = WorldToLocalEntityPosition(Entity0, Point1);
  Point1 = Point1 + Fix1;
  Points[PointIndex] = Point1;
  
  cubic_bezier_point Bezier1 = Beziers1[PointIndex1];
  Bezier1.P0 = LocalEntityPositionToWorld(Entity1, Bezier1.P0);
  Bezier1.P0 = WorldToLocalEntityPosition(Entity0, Bezier1.P0);
  Bezier1.P0 = Bezier1.P0 + Fix1;
  
  Bezier1.P1 = LocalEntityPositionToWorld(Entity1, Bezier1.P1);
  Bezier1.P1 = WorldToLocalEntityPosition(Entity0, Bezier1.P1);
  Bezier1.P1 = Bezier1.P1 + Fix1;
  
  Bezier1.P2 = LocalEntityPositionToWorld(Entity1, Bezier1.P2);
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
   // - n/(b-a) * (P-Q) = m/(c-b) * (R-P)
   // - assuming (b-a) = (c-b) = 1
   // - which gives us: n * (P-Q) = m * (R-P)
   // - solving for R: R = n/m * (P-Q) + P
   R_ = n/m * (P-Q) + P;
   
   if (Method == CurveMerge_C2)
   {
    // NOTE(hbr): C2 merge:
    // - C0 and C1 and
    // - n*(n-1)/(b-a)^2 * (P-2Q+S) = m*(m-1)/(c-b)^2 * (T-2R+P)
    // - assuming (b-a) = (c-b) = 1
    // - which gives us: n*(n-1) * (P-2Q+S) = m*(m-1) * (T-2R+P)
    // - solving for T: T = (n*(n-1))/(m*(m-1)) * (P-2Q+S) + (2R-P)
    T_ = (n*(n-1))/(m*(m-1)) * (P-2*Q+S) + (2*R_-P);
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
 
 SetCurveControlPoints(MergeWitness, PointCount, Points, Weights, Beziers);
 
 MaybeReverseCurvePoints(MergeEntity);
 MaybeReverseCurvePoints(Entity0);
 MaybeReverseCurvePoints(Entity1);
 
 EndTemp(Temp);
}

internal void
RenderMergingCurvesUI(editor *Editor)
{
 merging_curves_state *Merging = &Editor->MergingCurves;
 entity_store *EntityStore = &Editor->EntityStore;
 
 if (Merging->Active)
 {
  b32 WindowOpen = true;
  UI_BeginWindow(&WindowOpen, UIWindowFlag_AutoResize, StrLit("Merging Curves"));
  
  if (WindowOpen)
  {
   UpdateAndRenderChoose2CurvesUI(&Merging->Choose2Curves, Editor);
   
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
     Merge2Curves(&MergeWitness, Entity0, Entity1, Merging->Method, EntityStore);
    }
    else
    {
     SetCurveControlPoints(&MergeWitness, 0, 0, 0, 0);
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
  
  entity_colors Colors = ExtractEntityColors(Entity);
  entity_colors Fade = Colors;
  for (u32 ColorIndex = 0;
       ColorIndex < ArrayCount(Fade.AllColors);
       ++ColorIndex)
  {
   Fade.AllColors[ColorIndex] = FadeColor(Fade.AllColors[ColorIndex], 0.15f);
  }
  ApplyColorsToEntity(Entity, Fade);
  RenderEntity(RenderingHandle);
  ApplyColorsToEntity(Entity, Colors);
  
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

internal int Gowno(void);

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
 v4 *ColorPalette;
 
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
                   v4 *ColorPalette,
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
 
 v4 ProfileColor = Layout->ColorPalette[ProfileId % Layout->PaletteColorCount];
 
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
 
 local v4 ColorPalette[] = {
  RGB_FromHex(0xffa600),
  RGB_FromHex(0xff8531),
  RGB_FromHex(0xff6361),
  RGB_FromHex(0xde5a79),
  RGB_FromHex(0xbc5090),
  RGB_FromHex(0x8a508f),
  RGB_FromHex(0x58508d),
  RGB_FromHex(0x003f5c),
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
   UI_Checkbox(&DEBUG_Settings->ParametricEquationDebugMode, StrLit("Parametric Equation Debug Mode Enabled"));
   UI_ExponentialAnimation(&Camera->Animation);
  }
  UI_EndWindow();
 }
}

internal void
InitGlobalsOnInitOrCodeReload(editor *Editor)
{
 NilExpr = &Editor->NilParametricExpr;
 DEBUG_Settings = &Editor->DEBUG_Settings;
}

internal void
EditorUpdateAndRenderImpl(editor_memory *Memory, platform_input_output *Input, struct render_frame *Frame)
{
 editor *Editor = Memory->Editor;
 if (!Editor)
 {
  Editor = Memory->Editor = PushStruct(Memory->PermamentArena, editor);
  InitGlobalsOnInitOrCodeReload(Editor);
  InitEditor(Editor, Memory);
 }
 
 render_group RenderGroup_ = {};
 render_group *RenderGroup = &RenderGroup_;
 {
  camera *Camera = &Editor->Camera;
  RenderGroup_ = BeginRenderGroup(Frame,
                                  Camera->P, Camera->Rotation, Camera->Zoom,
                                  Editor->BackgroundColor,
                                  Editor->CollisionToleranceClip,
                                  Editor->RotationRadiusClip);
 }
 
 ProcessImageLoadingTasks(Editor);
 
 //- process events and update click events
 b32 NewProject = false;
 b32 OpenFileDialog = false;
 b32 SaveProject = false;
 b32 SaveProjectAs = false;
 b32 QuitProject = false;
 
 ProcessInputEvents(Editor, Input, RenderGroup,
                    &NewProject, &OpenFileDialog, &SaveProject,
                    &SaveProjectAs, &QuitProject);
 
 //- render line shadow when moving
 {
  // TODO(hbr): This looks like a mess, probably use functions that already exist
  editor_left_click_state *LeftClick = &Editor->LeftClick;
  entity *Entity = EntityFromHandle(LeftClick->TargetEntity);
  if (LeftClick->Active && Entity && LeftClick->OriginalVerticesCaptured && !Entity->Curve.Params.LineDisabled)
  {
   v4 ShadowColor = FadeColor(Entity->Curve.Params.LineColor, 0.15f);
   rendering_entity_handle RenderingHandle = BeginRenderingEntity(Entity, RenderGroup);
   
   PushVertexArray(RenderGroup,
                   LeftClick->OriginalCurveVertices.Vertices,
                   LeftClick->OriginalCurveVertices.VertexCount,
                   LeftClick->OriginalCurveVertices.Primitive,
                   ShadowColor,
                   GetCurvePartZOffset(CurvePart_LineShadow));
   
   EndRenderingEntity(RenderingHandle);
  }
 }
 
 //- render "remove" indicator
 {
  editor_right_click_state *RightClick = &Editor->RightClick;
  if (RightClick->Active)
  {
   v4 Color = V4(0.5f, 0.5f, 0.5f, 0.3f);
   // TODO(hbr): Again, zoffset of this thing is wrong
   PushCircle(RenderGroup, RightClick->ClickP, RenderGroup->CollisionTolerance, Color, 0.0f);
  }
 }
 
 //- render rotation indicator
 {
  editor_middle_click_state *MiddleClick = &Editor->MiddleClick;
  if (MiddleClick->Active && MiddleClick->Rotate)
  {
   camera *Camera = &Editor->Camera;
   f32 Radius = RenderGroup->RotationRadius;
   v4 Color = RGBA_Color(30, 56, 87, 80);
   f32 OutlineThickness = 0.1f * Radius;
   v4 OutlineColor = RGBA_Color(255, 255, 255, 24);
   // TODO(hbr): ZOffset here is possibly wrong, it should be something on top of everything
   // instead
   PushCircle(RenderGroup,
              Camera->P,
              Radius - OutlineThickness,
              Color, 0.0f,
              OutlineThickness, OutlineColor);
  }
 }
 
 if (!Editor->HideUI)
 {
  ProfileBegin("UI Update");
  
  RenderSelectedEntityUI(Editor, RenderGroup);
  RenderMenuBarUI(Editor, &NewProject, &OpenFileDialog, &SaveProject, &SaveProjectAs, &QuitProject);
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
 
 UpdateCamera(&Editor->Camera, Input);
 UpdateFrameStats(&Editor->FrameStats, Input);
 UpdateAndRenderEntities(Editor, RenderGroup);
 UpdateAndRenderAnimatingCurves(&Editor->AnimatingCurves, Input, RenderGroup);
 UpdateAndRenderNotifications(Editor, Input, RenderGroup);
 RenderMergingCurves(&Editor->MergingCurves, RenderGroup);
 
 if (OpenFileDialog)
 {
  temp_arena Temp = TempArena(0);
  
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
  
  platform_file_dialog_filter AllFilters[] = { PNG, JPEG, BMP };
  
  platform_file_dialog_filters Filters = {};
  Filters.AnyFileFilter = true;
  Filters.FilterCount = ArrayCount(AllFilters);
  Filters.Filters = AllFilters;
  
  platform_file_dialog_result OpenDialog = Platform.OpenFileDialog(Temp.Arena, Filters);
  
  camera *Camera = &Editor->Camera;
  TryLoadImages(Editor, OpenDialog.FileCount, OpenDialog.FilePaths, Camera->P);
  
  EndTemp(Temp);
 }
 
 if (QuitProject)
 {
  Input->QuitRequested = true;
 }
 
 Input->ProfilingStopped = Editor->Profiler.Stopped;
 
#if BUILD_DEBUG
 Input->RefreshRequested = true;
#endif
}

internal void
EditorOnCodeReloadImpl(editor_memory *Memory)
{
 Platform = Memory->PlatformAPI;
 ProfilerEquip(Memory->Profiler);
 InitGlobalsOnInitOrCodeReload(Memory->Editor);
}

DLL_EXPORT
EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender)
{
 ProfileFunctionBegin();
 EditorUpdateAndRenderImpl(Memory, Input, Frame);
 ProfileEnd();
}

DLL_EXPORT
EDITOR_ON_CODE_RELOAD(EditorOnCodeReload)
{
 EditorOnCodeReloadImpl(Memory);
}