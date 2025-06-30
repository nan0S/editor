#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

struct frame_stats
{
 struct {
  u64 FrameCount;
  f32 MinFrameTime;
  f32 MaxFrameTime;
  f32 SumFrameTime;
 } Calculation;
 
 f32 FPS;
 f32 MinFrameTime;
 f32 MaxFrameTime;
 f32 AvgFrameTime;
};

// TODO(hbr): Consider moving out to editor_notification.h
enum notification_type
{
 Notification_None,
 Notification_Success,
 Notification_Error,
 Notification_Warning,
};
struct notification
{
 notification_type Type;
 char ContentBuffer[256];
 string Content;
 f32 LifeTime;
 f32 ScreenPosY;
};

enum tracked_action_type
{
 TrackedAction_AddEntity,
 TrackedAction_RemoveEntity,
 TrackedAction_TransformEntity,
 TrackedAction_SelectEntity,
 TrackedAction_AddControlPoint,
 TrackedAction_RemoveControlPoint,
 TrackedAction_MoveControlPoint,
 TrackedAction_ModifyCurvePoints,
};
struct tracked_action
{
 tracked_action *Next;
 tracked_action *Prev;
 tracked_action_type Type;
 entity_handle Entity;
 entity_handle PreviouslySelectedEntity;
 control_point ControlPoint;
 control_point MovedToControlPoint;
 control_point_handle ControlPointHandle;
 entity_xform OriginalEntityXForm;
 entity_xform XFormedToEntityXForm;
 b32 IsPending;
 curve_points_static *CurvePoints;
 curve_points_static *FinalCurvePoints;
};
global tracked_action NilTrackedAction;
struct action_tracking_group
{
 tracked_action *ActionsHead;
 tracked_action *ActionsTail;
};
global action_tracking_group NilActionTrackingGroup;

enum editor_left_click_mode
{
 EditorLeftClick_MovingTrackingPoint,
 EditorLeftClick_MovingBSplineKnot,
 EditorLeftClick_MovingCurvePoint,
 EditorLeftClick_MovingEntity,
};
struct editor_left_click_state
{
 b32 Active;
 
 entity_handle TargetEntity;
 editor_left_click_mode Mode;
 curve_point_handle CurvePoint;
 v2 LastMouseP;
 u32 B_SplineKnotIndex;
 control_point_handle ControlPointHandle;
 control_point InitialControlPoint;
 b32 Moved;
 v2 InitialEntityP;
 tracked_action *PendingTrackedAction;
 
 arena *OriginalVerticesArena;
 b32 OriginalVerticesCaptured;
 vertex_array OriginalCurveVertices;
};

struct editor_right_click_state
{
 b32 Active;
 v2 ClickP;
 collision CollisionAtP;
};

struct editor_middle_click_state
{
 b32 Active;
 b32 Rotate;
 v2 ClipSpaceLastMouseP;
};

struct choose_2_curves_state
{
 b32 WaitingForChoice;
 entity_handle Curves[2];
 u32 ChoosingCurveIndex;
};

struct bouncing_parameter
{
 f32 T;
 f32 Sign;
 f32 Speed;
};
enum
{
 AnimatingCurves_Active = (1<<0),
 AnimatingCurves_Animating = (1<<1),
};
typedef u32 animating_curves_flags;
struct animating_curves_state
{
 animating_curves_flags Flags;
 choose_2_curves_state Choose2Curves;
 bouncing_parameter Bouncing;
 arena *Arena;
};

struct merging_curves_state
{
 b32 Active;
 choose_2_curves_state Choose2Curves;
 curve_merge_method Method;
 entity *MergeEntity;
 entity_snapshot_for_merging EntityVersioned[2];
};

enum visual_profiler_mode
{
 VisualProfilerMode_AllFrames,
 VisualProfilerMode_SingleFrame,
};
struct visual_profiler_state
{
 b32 Stopped;
 profiler *Profiler;
 visual_profiler_mode Mode;
 f32 DefaultReferenceMs;
 f32 ReferenceMs;
 u32 FrameIndex;
 profiler_frame FrameSnapshot;
};

enum editor_command
{
 EditorCommand_New,
 EditorCommand_Open,
 EditorCommand_Save,
 EditorCommand_SaveAs,
 EditorCommand_Quit,
 EditorCommand_ToggleDevConsole,
 EditorCommand_Delete,
 EditorCommand_Duplicate,
 EditorCommand_ToggleProfiler,
 EditorCommand_Undo,
 EditorCommand_Redo,
 EditorCommand_ToggleUI,
 
 EditorCommand_Count,
};
global read_only string EditorCommandNames[] = {
 StrLitComp("New"),
 StrLitComp("Open"),
 StrLitComp("Save"),
 StrLitComp("Save As"),
 StrLitComp("Quit"),
 StrLitComp("Toggle Dev Console"),
 StrLitComp("Delete"),
 StrLitComp("Duplicate"),
 StrLitComp("Toggle Profiler"),
 StrLitComp("Undo"),
 StrLitComp("Redo"),
 StrLitComp("Toggle UI"),
};
struct editor_command_node
{
 editor_command_node *Next;
 editor_command Command;
};

struct curve_points_static_node
{
 curve_points_static_node *Next;
 curve_points_static Points;
};

struct selected_entity_transform_state
{
 tracked_action *TransformAction;
};

struct editor
{
 arena *Arena;
 
 renderer_transfer_queue *RendererQueue;
 string_cache StrCache;
 entity_store EntityStore;
 thread_task_memory_store ThreadTaskMemoryStore;
 image_loading_store ImageLoadingStore;
 
 struct work_queue *LowPriorityQueue;
 struct work_queue *HighPriorityQueue;
 
 camera Camera;
 frame_stats FrameStats;
 
 selected_entity_transform_state SelectedEntityTransformState;
 entity_handle SelectedEntity;
 u64 EverIncreasingEntityCounter;
 
 u32 ActionTrackingGroupCount;
 u32 ActionTrackingGroupIndex;
 action_tracking_group ActionTrackingGroups[1024];
 b32 IsPendingActionTrackingGroup;
 action_tracking_group PendingActionTrackingGroup;
 tracked_action *FreeTrackedAction;
 curve_points_static_node *FreeCurvePointsNode;
 
 editor_command_node *EditorCommandsHead;
 editor_command_node *EditorCommandsTail;
 editor_command_node *FreeEditorCommandNode;
 
#define MAX_NOTIFICATION_COUNT 16
 u32 NotificationCount;
 notification Notifications[MAX_NOTIFICATION_COUNT];
 
 b32 HideUI;
 b32 EntityListWindow;
 b32 DiagnosticsWindow;
 b32 SelectedEntityWindow;
 b32 HelpWindow;
 b32 ProfilerWindow;
 b32 DevConsole;
 b32 Grid;
 
 editor_left_click_state LeftClick;
 editor_right_click_state RightClick;
 editor_middle_click_state MiddleClick;
 
 parametric_equation_expr NilParametricExpr;
 
 animating_curves_state AnimatingCurves;
 merging_curves_state MergingCurves;
 visual_profiler_state Profiler;
 
 debug_vars DEBUG_Vars;
 
 //////////////////////////////
 
 v4 DefaultBackgroundColor;
 v4 BackgroundColor;
 f32 CollisionToleranceClip;
 f32 RotationRadiusClip;
 curve_params CurveDefaultParams;
};

struct begin_modify_curve_points_static_tracked_result
{
 curve_points_static_modify_handle ModifyPoints;
 tracked_action *ModifyAction;
};

//- editor
internal void InitEditor(editor *Editor, editor_memory *Memory);
internal void DuplicateEntity(editor *Editor, entity *Entity);
internal void SplitCurveOnControlPoint(editor *Editor, entity *Entity);
internal void PerformBezierCurveSplit(editor *Editor, entity *Entity);
internal void ElevateBezierCurveDegree(editor *Editor, entity *Entity);
internal entity *GetSelectedEntity(editor *Editor);
internal void Undo(editor *Editor);
internal void Redo(editor *Editor);

internal entity *AddEntity(editor *Editor);
internal void RemoveEntity(editor *Editor, entity *Entity);
internal void SelectEntity(editor *Editor, entity *Entity);
internal control_point_handle AppendControlPoint(editor *Editor, entity_with_modify_witness *Entity, v2 P);
internal control_point_handle InsertControlPoint(editor *Editor, entity_with_modify_witness *Entity, v2 P, u32 At);
internal void RemoveControlPoint(editor *Editor, entity_with_modify_witness *Entity, control_point_handle Point);
internal tracked_action *BeginEntityTransform(editor *Editor, entity *Entity);
internal void EndEntityTransform(editor *Editor, tracked_action *MoveAction);
internal tracked_action *BeginControlPointMove(editor *Editor, entity *Entity, control_point_handle Point);
internal void EndControlPointMove(editor *Editor, tracked_action *MoveAction);
internal begin_modify_curve_points_static_tracked_result BeginModifyCurvePointsTracked(editor *Editor, entity_with_modify_witness *Entity, u32 RequestedPointCount, modify_curve_points_static_which_points Which);
internal void EndModifyCurvePointsTracked(editor *Editor, tracked_action *ModifyAction, curve_points_static_modify_handle ModifyPoints);
internal void SetCurvePointsTracked(editor *Editor, entity_with_modify_witness *Curve, curve_points_handle Points);

internal void BeginEditorFrame(editor *Editor);
internal void EndEditorFrame(editor *Editor, platform_input_output *Input);
internal void PushEditorCmd(editor *Editor, editor_command Command);

//- merging curves
internal void BeginMergingCurves(merging_curves_state *Merging);
internal void EndMergingCurves(editor *Editor, b32 Merged);
internal b32 MergingWantsInput(merging_curves_state *Merging);

//- animating curves
internal bouncing_parameter MakeBouncingParam(void);
internal void BeginAnimatingCurves(animating_curves_state *Animation);
internal void EndAnimatingCurves(animating_curves_state *Animation);
internal b32 AnimationWantsInput(animating_curves_state *Animation);

//- chosing curves
internal void BeginChoosing2Curves(choose_2_curves_state *Choosing);
internal b32 SupplyCurve(choose_2_curves_state *Choosing, entity *Curve);

//- lowering bezier curve degree
internal void BeginLoweringBezierCurveDegree(editor *Editor, entity *Entity);
internal void EndLoweringBezierCurveDegree(editor *Editor, curve_degree_lowering_state *Lowering);

//- click states
internal void BeginMovingEntity(editor_left_click_state *Left, editor *Editor, entity *Entity);
internal void BeginMovingCurvePoint(editor_left_click_state *Left, editor *Editor, action_tracking_group *TrackingGroup, entity *Target, curve_point_handle CurvePoint);
internal void BeginMovingTrackingPoint(editor_left_click_state *Left, entity_handle Target);
internal void BeginMovingBSplineKnot(editor_left_click_state *Left, entity_handle Target, u32 B_SplineKnotIndex);
internal void EndLeftClick(editor *Editor, editor_left_click_state *Left);

internal void BeginMiddleClick(editor_middle_click_state *Middle, b32 Rotate, v2 ClipSpaceLastMouseP);
internal void EndMiddleClick(editor_middle_click_state *Middle);

internal void BeginRightClick(editor_right_click_state *Right, v2 ClickP, collision CollisionAtP);
internal void EndRightClick(editor_right_click_state *Right);

//- misc
internal void FocusCameraOnEntity(camera *Camera, entity *Entity);
internal void AddNotificationF(editor *Editor, notification_type Type, char const *Format, ...);

internal void ProfilerInspectAllFrames(visual_profiler_state *Visual);
internal void ProfilerInspectSingleFrame(visual_profiler_state *Visual, u32 FrameIndex);

#endif //EDITOR_EDITOR_H
