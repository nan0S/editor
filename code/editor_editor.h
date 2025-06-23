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

enum
{
 TrackedActionFlag_Active = (1<<0),
 TrackedActionFlag_Pending = (1<<1),
};
typedef u32 tracked_action_flags;
enum tracked_action_type
{
 TrackedAction_AddEntity,
 TrackedAction_RemoveEntity,
 TrackedAction_MoveEntity,
 TrackedAction_SelectEntity,
 TrackedAction_AddControlPoint,
 TrackedAction_RemoveControlPoint,
 TrackedAction_MoveControlPoint,
};
struct tracked_action
{
 tracked_action_type Type;
 entity_handle Entity;
 entity_handle PreviouslySelectedEntity;
 control_point ControlPoint;
 control_point MovedToControlPoint;
 control_point_handle ControlPointHandle;
 v2 OriginalEntityP;
 v2 MovedToEntityP;
 tracked_action_flags Flags;
};
global tracked_action NilTrackedAction;
struct action_tracking_group
{
 u32 Count;
 tracked_action Actions[4];
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

struct editor
{
 camera Camera;
 frame_stats FrameStats;
 
 renderer_transfer_queue *RendererQueue;
 
 string_cache StrCache;
 entity_store EntityStore;
 arena *Arena;
 
 entity_handle SelectedEntity;
 u64 EverIncreasingEntityCounter;
 
 u32 ActionTrackingGroupCount;
 u32 ActionTrackingGroupIndex;
 action_tracking_group ActionTrackingGroups[1024];
 b32 IsPendingActionTrackingGroup;
 action_tracking_group PendingActionTrackingGroup;
 
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
 
 editor_left_click_state LeftClick;
 editor_right_click_state RightClick;
 editor_middle_click_state MiddleClick;
 
 thread_task_memory_store ThreadTaskMemoryStore;
 image_loading_store ImageLoadingStore;
 
 parametric_equation_expr NilParametricExpr;
 
 struct work_queue *LowPriorityQueue;
 struct work_queue *HighPriorityQueue;
 
 animating_curves_state AnimatingCurves;
 merging_curves_state MergingCurves;
 visual_profiler_state Profiler;
 
 debug_settings DEBUG_Settings;
 
 //////////////////////////////
 
 v4 DefaultBackgroundColor;
 v4 BackgroundColor;
 f32 CollisionToleranceClip;
 f32 RotationRadiusClip;
 curve_params CurveDefaultParams;
};

//- editor
internal void InitEditor(editor *Editor, editor_memory *Memory);
internal void DuplicateEntity(editor *Editor, entity *Entity);
internal void SplitCurveOnControlPoint(editor *Editor, entity *Entity);
internal void PerformBezierCurveSplit(editor *Editor, entity *Entity);
internal void ElevateBezierCurveDegree(entity *Entity);
internal void LowerBezierCurveDegree(entity *Entity);
internal entity *GetSelectedEntity(editor *Editor);
internal void Undo(editor *Editor);
internal void Redo(editor *Editor);

internal entity *AddEntity(editor *Editor);
internal void RemoveEntity(editor *Editor, entity *Entity);
internal void SelectEntity(editor *Editor, entity *Entity);
internal control_point_handle AppendControlPoint(editor *Editor, entity_with_modify_witness *Entity, v2 P);
internal control_point_handle InsertControlPoint(editor *Editor, entity_with_modify_witness *Entity, v2 P, u32 At);
internal void RemoveControlPoint(editor *Editor, entity_with_modify_witness *Entity, control_point_handle Point);
internal tracked_action *BeginEntityMove(editor *Editor, entity *Entity);
internal void EndEntityMove(editor *Editor, tracked_action *MoveAction);
internal tracked_action *BeginControlPointMove(editor *Editor, entity *Entity, control_point_handle Point);
internal void EndControlPointMove(editor *Editor, tracked_action *MoveAction);

internal void BeginEditorFrame(editor *Editor);
internal void EndEditorFrame(editor *Editor);

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
