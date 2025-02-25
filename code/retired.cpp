#if 0
if (Collision.Entity)
{
 curve *Curve = SafeGetCurve(Collision.Entity);
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
    curve *SourceCurve = SafeGetCurve(Combining->SourceEntity);
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
    curve *WithCurve = SafeGetCurve(Combining->WithEntity);
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

#define TEST_WORK_QUEUE 1
#if !defined(TEST_WORK_QUEUE)
# define TEST_WORK_QUEUE 0
#endif
struct work_queue_entry
{
 void (*Func)(void *Data);
 void *Data;
#if TEST_WORK_QUEUE
 b32 Completed;
#endif
};
struct work_queue
{
 work_queue_entry Entries[1024];
 u64 volatile NextEntryToWrite;
 u64 volatile NextEntryToRead;
 u64 volatile CompletionCount;
 u64 EntryCount;
 semaphore Semaphore;
};
internal void InitWorkQueue(work_queue *Queue, u32 ThreadCount);
internal void PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data);
internal void CompleteAllWork(work_queue *Queue);

internal void
PutWork(work_queue *Queue, void (*Func)(void *Data), void *Data)
{
 ++Queue->EntryCount;
 Assert(Queue->EntryCount < ArrayCount(Queue->Entries));
 
 u64 EntryIndex = Queue->NextEntryToWrite;
 work_queue_entry *Entry = Queue->Entries + EntryIndex;
 Entry->Func = Func;
 Entry->Data = Data;
#if TEST_WORK_QUEUE
 Entry->Completed = false;
#endif
 
 CompilerWriteBarrier;
 
 Queue->NextEntryToWrite = (EntryIndex + 1) % ArrayCount(Queue->Entries);
 PostSemaphore(&Queue->Semaphore);
}

internal b32
DoWork(work_queue *Queue)
{
 b32 ShouldSleep = false;
 
 u64 EntryIndex = Queue->NextEntryToRead;
 if (EntryIndex != Queue->NextEntryToWrite)
 {
  u64 Exchange = (EntryIndex + 1) % ArrayCount(Queue->Entries);
  if (InterlockedCmpExch(&Queue->NextEntryToRead, EntryIndex, Exchange) == EntryIndex)
  {
   work_queue_entry *Entry = Queue->Entries + EntryIndex;
   Entry->Func(Entry->Data);
#if TEST_WORK_QUEUE
   Entry->Completed = true;
#endif
   InterlockedIncr(&Queue->CompletionCount);
  }
 }
 else
 {
  ShouldSleep = true;
 }
 
 return ShouldSleep;
}

internal OS_THREAD_FUNC(WorkerThreadProc)
{
 work_queue *Queue = Cast(work_queue *)Data;
 InitThreadCtx(Gigabytes(64));
 for (;;)
 {
  if (DoWork(Queue))
  {
   WaitSemaphore(&Queue->Semaphore);
  }
 }
}

internal void
CompleteAllWork(work_queue *Queue)
{
 while (Queue->CompletionCount != Queue->EntryCount)
 {
  DoWork(Queue);
 }
 
#if TEST_WORK_QUEUE
 for (u64 WorkIndex = 0;
      WorkIndex < Queue->EntryCount;
      ++WorkIndex)
 {
  work_queue_entry *Entry = Queue->Entries + WorkIndex;
  Assert(Entry->Completed);
 }
#endif
 
 Queue->CompletionCount = 0;
 Queue->EntryCount = 0;
}

internal void
InitWorkQueue(work_queue *Queue, u32 ThreadCount)
{
 InitSemaphore(&Queue->Semaphore, 0, ThreadCount);
 for (u64 ThreadIndex = 0;
      ThreadIndex < ThreadCount;
      ++ThreadIndex)
 {
  OS_LaunchThread(WorkerThreadProc, Queue);
 }
}
internal u64
EstimateCPUFreq(u64 GuessSampleTimeMs)
{
 u64 OSFreq = GetOSTimerFreq();
 u64 OSWaitCount = OSFreq * GuessSampleTimeMs / 1000;
 u64 OSElapsed = 0;
 
 u64 CPUBegin = ReadCPUTimer();
 u64 OSBegin = ReadOSTimer();
 while (OSElapsed < OSWaitCount)
 {
  u64 OSEnd = ReadOSTimer();
  OSElapsed = OSEnd - OSBegin;
 }
 u64 CPUEnd = ReadCPUTimer();
 
 u64 CPUElapsed = CPUEnd - CPUBegin;
 u64 CPUFreq = OSFreq * CPUElapsed / OSElapsed;
 
 return CPUFreq;
}

{
 
}

#endif


internal void
UpdateAndRenderCurveCombining(render_group *Group, editor *Editor)
{
#if 0
 temp_arena Temp = TempArena(0);
 
 curve_combining_state *State = &Editor->CurveCombining;
 if (State->SourceEntity)
 {
  b32 Combine      = false;
  b32 Cancel       = false;
  b32 IsWindowOpen = true;
  if (UI_BeginWindowF(&IsWindowOpen, 0, "Combine Curves"))
  {                 
   if (IsWindowOpen)
   {
    RenderEntityCombo(MAX_ENTITY_COUNT, Editor->Entities, &State->SourceEntity, StrLit("Curve 1"));
    RenderEntityCombo(MAX_ENTITY_COUNT, Editor->Entities, &State->WithEntity,   StrLit("Curve 2"));
    UI_ComboF(Cast(u32 *)&State->CombinationType, CurveCombination_Count, CurveCombinationNames, "Method");
   }
   
   b32 CanCombine = (State->CombinationType != CurveCombination_None);
   if (State->WithEntity)
   {
    curve_params *SourceParams = &SafeGetCurve(State->SourceEntity)->Params;
    curve_params *WithParams = &SafeGetCurve(State->WithEntity)->Params;
    
    if (SourceParams != WithParams &&
        SourceParams->Interpolation == WithParams->Interpolation)
    {
     switch (SourceParams->Interpolation)
     {
      case Curve_Polynomial:  {
       CanCombine = (State->CombinationType == CurveCombination_Merge);
      } break;
      case Curve_CubicSpline: {
       CanCombine = ((SourceParams->CubicSpline == WithParams->CubicSpline) &&
                     (State->CombinationType == CurveCombination_Merge));
      } break;
      case Curve_Bezier: {
       CanCombine = ((SourceParams->Bezier == WithParams->Bezier) &&
                     (SourceParams->Bezier != Bezier_Cubic));
      } break;
      
      case Curve_Count: InvalidPath;
     }
    }
   }
   UI_Disabled(!CanCombine)
   {
    Combine = UI_ButtonF("Combine");
    UI_SameRow();
   }
   Cancel = UI_ButtonF("Cancel");
  }
  UI_EndWindow();
  
  if (Combine)
  {
   entity *FromEntity = State->SourceEntity;
   entity *ToEntity   = State->WithEntity;
   curve  *From       = SafeGetCurve(FromEntity);
   curve  *To         = SafeGetCurve(ToEntity);
   u32     FromCount  = From->ControlPointCount;
   u32     ToCount    = To->ControlPointCount;
   
   entity_with_modify_witness ToEntityWitness = BeginEntityModify(ToEntity);
   
   if (State->SourceCurveLastControlPoint)
   {
    ArrayReverse(From->ControlPoints,       FromCount, v2);
    ArrayReverse(From->ControlPointWeights, FromCount, f32);
    ArrayReverse(From->CubicBezierPoints,   FromCount, cubic_bezier_point);
   }
   
   if (State->WithCurveFirstControlPoint)
   {
    ArrayReverse(To->ControlPoints,       ToCount, v2);
    ArrayReverse(To->ControlPointWeights, ToCount, f32);
    ArrayReverse(To->CubicBezierPoints,   ToCount, cubic_bezier_point);
   }
   
   u32 CombinedPointCount = ToCount;
   u32 StartIndex = 0;
   v2 Translation = {};
   switch (State->CombinationType)
   {
    case CurveCombination_Merge: {
     CombinedPointCount += FromCount;
    } break;
    
    case CurveCombination_C0:
    case CurveCombination_C1:
    case CurveCombination_C2:
    case CurveCombination_G1: {
     if (FromCount > 0)
     {
      CombinedPointCount += FromCount - 1;
      StartIndex = 1;
      if (ToCount > 0)
      {
       Translation =
        LocalEntityPositionToWorld(ToEntity, To->ControlPoints[ToCount - 1]) -
        LocalEntityPositionToWorld(FromEntity, From->ControlPoints[0]);
      }
     }
    } break;
    
    case CurveCombination_None:
    case CurveCombination_Count: InvalidPath;
   }
   
   // NOTE(hbr): Allocate buffers and copy control points into them
   v2 *CombinedPoints  = PushArrayNonZero(Temp.Arena, CombinedPointCount, v2);
   f32 *CombinedWeights = PushArrayNonZero(Temp.Arena, CombinedPointCount, f32);
   cubic_bezier_point *CombinedBeziers = PushArrayNonZero(Temp.Arena, CombinedPointCount, cubic_bezier_point);
   ArrayCopy(CombinedPoints, To->ControlPoints, ToCount);
   ArrayCopy(CombinedWeights, To->ControlPointWeights, ToCount);
   ArrayCopy(CombinedBeziers, To->CubicBezierPoints, ToCount);
   
   // TODO(hbr): SIMD?
   for (u32 I = StartIndex; I < FromCount; ++I)
   {
    v2 FromPoint = LocalEntityPositionToWorld(FromEntity, From->ControlPoints[I]);
    v2 ToPoint   = WorldToLocalEntityPosition(ToEntity, FromPoint + Translation);
    CombinedPoints[ToCount - StartIndex + I] = ToPoint;
   }
   for (u32 I = StartIndex; I < FromCount; ++I)
   {
    for (u32 J = 0; J < 3; ++J)
    {
     v2 FromBezier = LocalEntityPositionToWorld(FromEntity, From->CubicBezierPoints[I].Ps[J]);
     v2 ToBezier   = WorldToLocalEntityPosition(ToEntity, FromBezier + Translation); 
     CombinedBeziers[(ToCount - StartIndex) + I].Ps[J] = ToBezier;
    }
   }
   ArrayCopy(CombinedWeights + ToCount,
             From->ControlPointWeights + StartIndex,
             (FromCount - StartIndex));
   
   // NOTE(hbr): Combine control points properly on the border
   switch (State->CombinationType)
   {
    // NOTE(hbr): Nothing to do
    case CurveCombination_Merge:
    case CurveCombination_C0: {} break;
    
    case CurveCombination_C1: {
     if (FromCount >= 2 && ToCount >= 2)
     {
      v2 P = CombinedPoints[ToCount - 1];
      v2 Q = CombinedPoints[ToCount - 2];
      
      // NOTE(hbr): First derivative equal
      v2 FixedControlPoint = Cast(f32)ToCount/FromCount * (P - Q) + P;
      v2 Fix = FixedControlPoint - CombinedPoints[ToCount];
      CombinedPoints[ToCount] = FixedControlPoint;
      
      CombinedBeziers[ToCount].P0 += Fix;
      CombinedBeziers[ToCount].P1 += Fix;
      CombinedBeziers[ToCount].P2 += Fix;
     }
    } break;
    
    case CurveCombination_C2: {
     if (FromCount >= 3 && ToCount >= 3)
     {
      // TODO(hbr): Merge C1 with C2, maybe G1.
      v2 R = CombinedPoints[ToCount - 3];
      v2 Q = CombinedPoints[ToCount - 2];
      v2 P = CombinedPoints[ToCount - 1];
      
      // NOTE(hbr): First derivative equal
      v2 Fixed_T = Cast(f32)ToCount/FromCount * (P - Q) + P;
      // NOTE(hbr): Second derivative equal
      v2 Fixed_U = Cast(f32)(FromCount * (FromCount-1))/(ToCount * (ToCount-1)) * (P - 2.0f * Q + R) + 2.0f * Fixed_T - P;
      v2 Fix_T = Fixed_T - CombinedPoints[ToCount];
      v2 Fix_U = Fixed_U - CombinedPoints[ToCount + 1];
      CombinedPoints[ToCount] = Fixed_T;
      CombinedPoints[ToCount + 1] = Fixed_U;
      
      CombinedBeziers[ToCount].P0 += Fix_T;
      CombinedBeziers[ToCount].P1 += Fix_T;
      CombinedBeziers[ToCount].P2 += Fix_T;
      
      CombinedBeziers[ToCount + 1].P0 += Fix_U;
      CombinedBeziers[ToCount + 1].P1 += Fix_U;
      CombinedBeziers[ToCount + 1].P2 += Fix_U;
     }
    } break;
    
    case CurveCombination_G1: {
     if (FromCount >= 2 && ToCount >= 2)
     {
      f32 PreserveLength = Norm(From->ControlPoints[1] - From->ControlPoints[0]);
      
      v2 P = CombinedPoints[ToCount - 2];
      v2 Q = CombinedPoints[ToCount - 1];
      v2 Direction = P - Q;
      Normalize(&Direction);
      
      v2 FixedControlPoint = Q - PreserveLength * Direction;
      v2 Fix = FixedControlPoint - CombinedPoints[ToCount];
      CombinedPoints[ToCount] = FixedControlPoint;
      
      CombinedBeziers[ToCount].P0 += Fix;
      CombinedBeziers[ToCount].P1 += Fix;
      CombinedBeziers[ToCount].P2 += Fix;
     }
    } break;
    
    case CurveCombination_None:
    case CurveCombination_Count: InvalidPath;
   }
   
   SetCurveControlPoints(&ToEntityWitness,
                         CombinedPointCount, CombinedPoints,
                         CombinedWeights, CombinedBeziers);
   
   DeallocEntity(Editor, FromEntity);
   SelectEntity(Editor, ToEntity);
   
   EndEntityModify(ToEntityWitness);
  }
  
  if (Combine || !IsWindowOpen || Cancel)
  {
   EndCurveCombining(State);
  }
 }
 
 curve *SourceCurve = SafeGetCurve(State->SourceEntity);
 curve *WithCurve = SafeGetCurve(State->WithEntity);
 if (SourceCurve && WithCurve && (SourceCurve != WithCurve) &&
     SourceCurve->ControlPointCount > 0 &&
     WithCurve->ControlPointCount > 0)
 {
  v2 SourcePointLocal = (State->SourceCurveLastControlPoint ?
                         SourceCurve->ControlPoints[SourceCurve->ControlPointCount - 1] :
                         SourceCurve->ControlPoints[0]);
  v2 WithPointLocal = (State->WithCurveFirstControlPoint ?
                       WithCurve->ControlPoints[0] :
                       WithCurve->ControlPoints[WithCurve->ControlPointCount - 1]);
  
  v2 SourcePoint = LocalEntityPositionToWorld(State->SourceEntity, SourcePointLocal);
  v2 WithPoint = LocalEntityPositionToWorld(State->WithEntity, WithPointLocal);
  
  f32 LineWidth = 0.5f * SourceCurve->Params.LineWidth;
  v4 Color = SourceCurve->Params.LineColor;
  Color.A = Cast(u8)(0.5f * Color.A);
  
  v2 LineDirection = WithPoint - SourcePoint;
  Normalize(&LineDirection);
  
  f32 TriangleSide = 10.0f * LineWidth;
  f32 TriangleHeight = TriangleSide * SqrtF32(3.0f) / 2.0f;
  v2 BaseVertex = WithPoint - TriangleHeight * LineDirection;
  PushLine(Group, SourcePoint, BaseVertex, LineWidth, Color, GetCurvePartZOffset(CurvePart_Count));
  
  v2 LinePerpendicular = Rotate90DegreesAntiClockwise(LineDirection);
  v2 LeftVertex = BaseVertex + 0.5f * TriangleSide * LinePerpendicular;
  v2 RightVertex = BaseVertex - 0.5f * TriangleSide * LinePerpendicular;
  PushTriangle(Group, LeftVertex, RightVertex, WithPoint, Color, GetCurvePartZOffset(CurvePart_Count));
 }
 
 EndTemp(Temp);
 
#endif
}

#if 0
// TODO(hbr): Do a pass oveer this internal to simplify the logic maybe (and simplify how the UI looks like in real life)
// TODO(hbr): Maybe also shorten some labels used to pass to ImGUI
internal void
RenderSelectedEntityUI(editor *Editor)
{
 entity *Entity = Editor->SelectedEntity;
 ui_config *UI_Config = &Editor->UI_Config;
 if (UI_Config->ViewSelectedEntityWindow && Entity)
 {
  UI_LabelF("SelectedEntity")
  {
   if (UI_BeginWindowF(&UI_Config->ViewSelectedEntityWindow, "Selected Entity"))
   {
    b32 SomeCurveParamChanged = false;
    if (Curve)
    {
     curve_params DefaultParams = Editor->CurveDefaultParams;
     curve_params *CurveParams = &Curve->Params;
     
     b32 Delete                        = false;
     b32 Copy                          = false;
     b32 SwitchVisibility              = false;
     b32 Deselect                      = false;
     b32 FocusOn                       = false;
     b32 ElevateBezierCurve            = false;
     b32 LowerBezierCurve              = false;
     b32 SplitBezierCurve              = false;
     b32 AnimateCurve                  = false;
     b32 CombineCurve                  = false;
     b32 SplitOnControlPoint           = false;
     b32 VisualizeDeCasteljau          = false;
     
     UI_NewRow();
     UI_SeparatorTextF("Actions");
     {
      Delete = UI_ButtonF("Delete");
      UI_SameRow();
      Copy = UI_ButtonF("Copy");
      UI_SameRow();
      SwitchVisibility = UI_ButtonF((Entity->Flags & EntityFlag_Hidden) ? "Show" : "Hide");
      UI_SameRow();
      Deselect = UI_ButtonF("Deselect");
      UI_SameRow();
      FocusOn = UI_ButtonF("Focus");
      
      if (Curve)
      {
       curve_params *CurveParams = &Curve->Params;
       
       // TODO(hbr): Maybe pick better name than transform
       AnimateCurve = UI_ButtonF("Animate");
       UI_SameRow();
       // TODO(hbr): Maybe pick better name than "Combine"
       CombineCurve = UI_ButtonF("Combine");
       
       UI_Disabled(!IsControlPointSelected(Curve))
       {
        UI_SameRow();
        SplitOnControlPoint = UI_ButtonF("Split on Control Point");
       }
       
       b32 IsBezierRegular = (CurveParams->Interpolation == Curve_Bezier &&
                              CurveParams->Bezier == Bezier_Regular);
       UI_Disabled(!IsBezierRegular)
       {
        UI_Disabled(Curve->ControlPointCount < 2)
        {
         UI_SameRow();
         SplitBezierCurve = UI_ButtonF("Split");
        }
        ElevateBezierCurve = UI_ButtonF("Elevate Degree");
        UI_Disabled(Curve->ControlPointCount == 0)
        {
         UI_SameRow();
         LowerBezierCurve = UI_ButtonF("Lower Degree");
        }
        VisualizeDeCasteljau = UI_ButtonF("Visualize De Casteljau");
       }
       
      }
     }
     
     if (Delete)
     {
      DeallocEntity(Editor, Entity);
     }
     
     if (Copy)
     {
      DuplicateEntity(Entity, Editor);
     }
     
     if (SwitchVisibility)
     {
      Entity->Flags ^= EntityFlag_Hidden;
     }
     
     if (Deselect)
     {
      DeselectCurrentEntity(Editor);
     }
     
     if (FocusOn)
     {
      FocusCameraOnEntity(Editor, Entity);
     }
     
     if (ElevateBezierCurve)
     {
      ElevateBezierCurveDegree(Entity);
     }
     
     if (LowerBezierCurve)
     {
      LowerBezierCurveDegree(Entity);
     }
     
     if (SplitBezierCurve)
     {
      BeginCurvePointTracking(Curve, true);
     }
     
     if (AnimateCurve)
     {
      BeginAnimatingCurve(&Editor->CurveAnimation, Entity);
     }
     
     if (CombineCurve)
     {
      BeginCurveCombining(&Editor->CurveCombining, Entity);
     }
     
     if (SplitOnControlPoint)
     {
      SplitCurveOnControlPoint(Entity, Editor);
     }
     
     if (VisualizeDeCasteljau)
     {
      BeginCurvePointTracking(Curve, false);
     }
     
     if (SomeCurveParamChanged)
     {
      MarkCurveForRecomputation(Entity);
     }
    }
    UI_EndWindow();
   }
  }
 }
}
#endif

internal void
UpdateAndRenderMenuBar(editor *Editor, platform_input *Input, render_group *RenderGroup)
{
 local char const *SaveAsLabel = "SaveAsWindow";
 local char const *SaveAsTitle = "Save As";
 local char const *OpenNewProjectLabel = "OpenNewProject";
 local char const *OpenNewProjectTitle = "Open";
 
 local ImGuiWindowFlags FileDialogWindowFlags = ImGuiWindowFlags_NoCollapse;
 
 v2u WindowDim = RenderGroup->Frame->WindowDim;
 ImVec2 HalfWindowDim = ImVec2(0.5f * WindowDim.X, 0.5f * WindowDim.Y);
 ImVec2 FileDialogMinSize = HalfWindowDim;
 ImVec2 FileDialogMaxSize = ImVec2(Cast(f32)WindowDim.X, Cast(f32)WindowDim.Y);
 
#if 0
 auto FileDialog = ImGuiFileDialog::Instance();
 
 string ConfirmCloseProject = StrLit("ConfirmCloseCurrentProject");
 if (NewProject || KeyPressed(Input, Key_N, Modifier_Ctrl))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_NewProject;
 }
 if (OpenProject || KeyPressed(Input, Key_O, Modifier_Ctrl))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_OpenProject;
 }
 if (Quit || KeyPressed(Input, Key_Q, 0) || KeyPressed(Input, Key_Esc, 0))
 {
  UI_OpenPopup(ConfirmCloseProject);
  Editor->ActionWaitingToBeDone = ActionToDo_Quit;
 }
 
 if (SaveProject || KeyPressed(Input, Key_S, Modifier_Ctrl))
 {
  // TODO(hbr): Implement
  
  if (IsValid(Editor->ProjectSavePath))
  {
   temp_arena Temp = TempArena(0);
   
   error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                               Editor->SaveProjectFormat,
                                                               Editor->ProjectSavePath,
                                                               Editor);
   
   if (IsError(SaveProjectInFormatError))
   {
    AddNotificationF(Editor, Notification_Error, SaveProjectInFormatError.Data);
   }
   else
   {
    AddNotificationF(Editor, Notification_Success, "project successfully saved in %s",
                     Editor->ProjectSavePath.Data);
   }
   
   EndTemp(Temp);
  }
  else
  {
   FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                         SAVE_AS_MODAL_EXTENSION_SELECTION,
                         ".");
  }
 }
 
 if (SaveProjectAs || KeyPressed(Input, Key_S, Modifier_Ctrl|Modifier_Shift))
 {
  FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                        SAVE_AS_MODAL_EXTENSION_SELECTION,
                        ".");
 }
 
 if (LoadImage)
 {
  FileDialog->OpenModal(LoadImageLabel, LoadImageTitle,
                        "Image Files (*.jpg *.png){.jpg,.png}",
                        ".");
 }
 
 
 action_to_do ActionToDo = ActionToDo_Nothing;
 // NOTE(hbr): Open "Are you sure you want to exit?" popup
 {   
  // NOTE(hbr): Center window.
  ImGui::SetNextWindowPos(HalfWindowDim, ImGuiCond_Always, ImVec2(0.5f,0.5f));
  if (UI_BeginPopupModal(ConfirmCloseProject))
  {
   UI_TextF("You are about to discard current project. Save it?");
   ImGui::Separator();
   b32 Yes    = UI_ButtonF("Yes"); UI_SameRow();
   b32 No     = UI_ButtonF("No"); UI_SameRow();
   b32 Cancel = UI_ButtonF("Cancel");
   
   if (Yes || No)
   {
    if (Yes)
    {
     // TODO(hbr): Implement
     if (IsValid(Editor->ProjectSavePath))
     {
      temp_arena Temp = TempArena(0);
      
      string SaveProjectFilePath = Editor->ProjectSavePath;
      save_project_format SaveProjectFormat = Editor->SaveProjectFormat;
      
      error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                                  SaveProjectFormat,
                                                                  SaveProjectFilePath,
                                                                  Editor);
      
      if (IsError(SaveProjectInFormatError))
      {
       AddNotificationF(Editor, Notification_Error, "failed to discard current project: %s",
                        SaveProjectInFormatError.Data);
       
       Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
      }
      else
      {
       AddNotificationF(Editor, Notification_Success, "project sucessfully saved in %s",
                        SaveProjectFilePath.Data);
       
       ActionToDo = Editor->ActionWaitingToBeDone;
       Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
      }
      
      EndTemp(Temp);
     }
     else
     {
      FileDialog->OpenModal(SaveAsLabel, SaveAsTitle,
                            SAVE_AS_MODAL_EXTENSION_SELECTION,
                            ".");
      // NOTE(hbr): Action is still waiting to be done as soon as save path is chosen.
     }
    }
    else if (No)
    {
     // NOTE(hbr): Do action now.
     ActionToDo = Editor->ActionWaitingToBeDone;
     Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
    }
   }
   
   if (Yes || No || Cancel)
   {
    UI_CloseCurrentPopup();
   }
   
   UI_EndPopup();
  }
 }
 
 // NOTE(hbr): Open dialog to Save Project As 
 if (FileDialog->Display(SaveAsLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   std::string const &SelectedFilter = FileDialog->GetCurrentFilter();
   string SaveProjectFilePath = Str(Temp.Arena,
                                    SelectedPath.c_str(),
                                    SelectedPath.size());
   string SaveProjectExtension = Str(Temp.Arena,
                                     SelectedFilter.c_str(),
                                     SelectedFilter.size());
   
   string Project = StrLitArena(Temp.Arena, PROJECT_FILE_EXTENSION_SELECTION);
   string JPG     = StrLitArena(Temp.Arena, JPG_FILE_EXTENSION_SELECTION);
   string PNG     = StrLitArena(Temp.Arena, PNG_FILE_EXTENSION_SELECTION);
   
   string AddExtension = {};
   save_project_format SaveProjectFormat = SaveProjectFormat_None;
   if (AreStringsEqual(SaveProjectExtension, Project))
   {
    AddExtension = StrLitArena(Temp.Arena, SAVED_PROJECT_FILE_EXTENSION);
    SaveProjectFormat = SaveProjectFormat_ProjectFile;
   }
   else if (AreStringsEqual(SaveProjectExtension, JPG))
   {
    AddExtension = StrLitArena(Temp.Arena, ".jpg");
    SaveProjectFormat = SaveProjectFormat_ImageFile;
   }
   else if (AreStringsEqual(SaveProjectExtension, PNG))
   {
    AddExtension = StrLitArena(Temp.Arena, ".png");
    SaveProjectFormat = SaveProjectFormat_ImageFile;
   }
   
   string SaveProjectFilePathWithExtension = SaveProjectFilePath;
   if (IsValid(AddExtension))
   {
    if (!HasSuffix(SaveProjectFilePathWithExtension, AddExtension))
    {
     SaveProjectFilePathWithExtension = StrF(Temp.Arena,
                                             "%s%s",
                                             SaveProjectFilePathWithExtension,
                                             AddExtension);
    }
   }
   
   error_string SaveProjectInFormatError = SaveProjectInFormat(Temp.Arena,
                                                               SaveProjectFormat,
                                                               SaveProjectFilePathWithExtension,
                                                               Editor);
   
   if (IsError(SaveProjectInFormatError))
   {
    AddNotificationF(Editor, Notification_Error, SaveProjectInFormatError.Data);
    
    Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
   }
   else
   {
    EditorSetSaveProjectPath(Editor, SaveProjectFormat, SaveProjectFilePathWithExtension);
    
    AddNotificationF(Editor, Notification_Success, "project sucessfully saved in %s",
                     SaveProjectFilePathWithExtension.Data);
    
    ActionToDo = Editor->ActionWaitingToBeDone;
    Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
   }
   
   EndTemp(Temp);
  }
  else
  {
   Editor->ActionWaitingToBeDone = ActionToDo_Nothing;
  }
  
  FileDialog->Close();
 }
 
 switch (ActionToDo)
 {
  case ActionToDo_Nothing: {} break;
  
  case ActionToDo_NewProject: {
   
   // TODO(hbr): Load new project
   
   editor_state NewEditor =
    CreateDefaultEditor(Editor->EntityPool,
                        Editor->DeCasteljauVisual.Arena,
                        Editor->DegreeLowering.Arena,
                        Editor->MovingPointArena,
                        Editor->CurveAnimation.Arena);
   
   DeallocEditor(&Editor);
   Editor = NewEditor;
   
   EditorSetSaveProjectPath(Editor, SaveProjectFormat_None, {});
  } break;
  
  case ActionToDo_OpenProject: {
   Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
   FileDialog->OpenModal(OpenNewProjectLabel, OpenNewProjectTitle,
                         SAVED_PROJECT_FILE_EXTENSION, ".");
  } break;
  
  case ActionToDo_Quit: {
   Assert(Editor->ActionWaitingToBeDone == ActionToDo_Nothing);
   Input->QuitRequested = true;
  } break;
 }
 
 // NOTE(hbr): Open dialog to Open New Project
 if (FileDialog->Display(OpenNewProjectLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   // TODO(hbr): Implement this
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   string OpenProjectFilePath = Str(Temp.Arena,
                                    SelectedPath.c_str(),
                                    SelectedPath.size());
   
   
   load_project_result LoadResult = LoadProjectFromFile(Temp.Arena,
                                                        OpenProjectFilePath,
                                                        Editor->EntityPool,
                                                        Editor->DeCasteljauVisual.Arena,
                                                        Editor->DegreeLowering.Arena,
                                                        Editor->MovingPointArena,
                                                        Editor->CurveAnimation.Arena);
   if (IsError(LoadResult.Error))
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Error,
                     "failed to open new project: %s",
                     LoadResult.Error.Data);
   }
   else
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Success,
                     "project successfully loaded from %s",
                     OpenProjectFilePath.Data);
    
    DeallocEditor(&Editor);
    Editor = LoadResult.Editor;
    Editor->Params = LoadResult.EditorParameters;
    Editor->UI_Config = LoadResult.UI_Config;
    
    EditorSetSaveProjectPath(Editor,
                             SaveProjectFormat_ProjectFile,
                             OpenProjectFilePath);
   }
   
   ListIter(Warning, LoadResult.Warnings.Head, string_list_node)
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Warning,
                     Warning->String.Data);
   }
   
   EndTemp(Temp);
   
   
  }
  
  FileDialog->Close();
 }
 
 // NOTE(hbr): Open dialog to Load Image
 if (FileDialog->Display(LoadImageLabel,
                         FileDialogWindowFlags,
                         FileDialogMinSize,
                         FileDialogMaxSize))
 {
  if (FileDialog->IsOk())
  {
   temp_arena Temp = TempArena(0);
   
   std::string const &SelectedPath = FileDialog->GetFilePathName();
   string NewImageFilePath = Str(Temp.Arena,
                                 SelectedPath.c_str(),
                                 SelectedPath.size());
   
   string LoadTextureError = {};
   sf::Texture LoadedTexture = LoadTextureFromFile(Temp.Arena,
                                                   NewImageFilePath,
                                                   &LoadTextureError);
   
   if (IsError(LoadTextureError))
   {
    AddNotificationF(&Editor->Notifications,
                     Notification_Error,
                     "failed to load image: %s",
                     LoadTextureError.Data);
   }
   else
   {
    std::string const &SelectedFileName = FileDialog->GetCurrentFileName();
    string ImageFileName = Str(Temp.Arena,
                               SelectedFileName.c_str(),
                               SelectedFileName.size());
    RemoveExtension(&ImageFileName);
    
    entity *Entity = AllocEntity(&Editor);
    InitImageEntity(Entity,
                    V2(0.0f, 0.0f),
                    V2(1.0f, 1.0f),
                    Rotation2DZero(),
                    StrToNameStr(ImageFileName),
                    NewImageFilePath,
                    &LoadedTexture);
    
    SelectEntity(&Editor, Entity);
    
    AddNotificationF(&Editor->Notifications,
                     Notification_Success,
                     "successfully loaded image from %s",
                     NewImageFilePath.Data);
   }
   
   EndTemp(Temp);
  }
  
  FileDialog->Close();
 }
#endif
}
