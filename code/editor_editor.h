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
 curve_point_index CurvePointIndex;
 v2 LastMouseP;
 u32 B_SplineKnotIndex;
 
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
 
 entity_handle SelectedEntityHandle;
 entity_store EntityStore;
 u64 EverIncreasingEntityCounter;
 
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
 
 //////////////////////////////
 
 v4 DefaultBackgroundColor;
 v4 BackgroundColor;
 f32 CollisionToleranceClip;
 f32 RotationRadiusClip;
 curve_params CurveDefaultParams;
};

//- editor
internal void InitEditor(editor *Editor, editor_memory *Memory);
internal void SelectEntity(editor *Editor, entity *Entity);
internal void DuplicateEntity(editor *Editor, entity *Entity);
internal void FocusCameraOnEntity(editor *Editor, entity *Entity, f32 AspectRatio);
internal void SplitCurveOnControlPoint(editor *Editor, entity *Entity);
internal void PerformBezierCurveSplit(editor *Editor, entity *Entity);
internal void ElevateBezierCurveDegree(entity *Entity);
internal void LowerBezierCurveDegree(entity *Entity);

//- merging curves
internal void BeginMergingCurves(merging_curves_state *Merging);
internal void EndMergingCurves(editor *Editor, b32 Merged);
internal b32 MergingWantsInput(merging_curves_state *Merging);
internal void Merge2Curves(entity_with_modify_witness *MergeWitness, entity *Entity0, entity *Entity1, curve_merge_method Method);

//- animating curves
internal bouncing_parameter MakeBouncingParam(void);
internal void BeginAnimatingCurves(animating_curves_state *Animation);
internal void EndAnimatingCurves(animating_curves_state *Animation);
internal b32 AnimationWantsInput(animating_curves_state *Animation);

//- chosing curves
internal void BeginChoosing2Curves(choose_2_curves_state *Choosing);
internal b32 SupplyCurve(choose_2_curves_state *Choosing, entity *Curve);

//- left click
internal void BeginMovingEntity(editor_left_click_state *Left, entity_handle Target);
internal void BeginMovingCurvePoint(editor_left_click_state *Left, entity_handle Target, curve_point_index CurvePoint);
internal void BeginMovingTrackingPoint(editor_left_click_state *Left, entity_handle Target);
internal void BeginMovingBSplineKnot(editor_left_click_state *Left, entity_handle Target, u32 B_SplineKnotIndex);
internal void EndLeftClick(editor_left_click_state *Left);

//- middle click
internal void BeginMiddleClick(editor_middle_click_state *Middle, b32 Rotate, v2 ClipSpaceLastMouseP);
internal void EndMiddleClick(editor_middle_click_state *Middle);

//- right click
internal void BeginRightClick(editor_right_click_state *Right, v2 ClickP, collision CollisionAtP);
internal void EndRightClick(editor_right_click_state *Right);

//- visual profiler
internal void ProfilerInspectAllFrames(visual_profiler_state *Visual);
internal void ProfilerInspectSingleFrame(visual_profiler_state *Visual, u32 FrameIndex);

//- notifications
internal void AddNotificationF(editor *Editor, notification_type Type, char const *Format, ...);

#endif //EDITOR_EDITOR_H
