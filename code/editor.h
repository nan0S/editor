#ifndef EDITOR_H
#define EDITOR_H

#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"

#include "editor_memory.h"
#include "editor_third_party_inc.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_math.h"
#include "editor_sort.h"
#include "editor_parametric_equation.h"
#include "editor_entity.h"
#include "editor_ui.h"
#include "editor_stb.h"
#include "editor_camera.h"

/* TODO(hbr):
Ideas:
- probably add optimization to evaluate multiple points at once, maybe even don't store Ti, just iterator through iterator
- consider using restrict keyword
- remove that ugly Gaussian Elimination, maybe add optimized version of periodic cubic spline M calculation
- some kind of locking system - when I want to edit only one curve without
obstacles, I don't want to unwittingly click on some other curve/image and select it
- confirm window when removing points
- dodawanie punktow z dwoch stron
- loop curve with checkbox - it seems useful
- czesto jak rysuje to jestem mega zzoomwany na image - chce dodac nowa curve ale nie mam jak latwo odselectowac aktualnej bo wszedzie wokolo jest image
- ogolnie caly czas sie boje ze cos usune - undo/redo to troche zmityguje ale chyba to nie powinno byc takie proste - szczegolnie images - byc moze lewy przycisk nie powinien usuwac entities - tylko punkty na przyklad
- zmienianie koloru krzywej "inline" a nie w osobnym okienku
- hide all control points in ALL curves using some button or something
- resize image "inline", not in separate window
- persistent openGL mapping for texture uploads, mesh uploads

TODOs:
- image paths relative to the project
- add docking
- maybe change SomeParameterChanged to be more granular in order to avoid recomputing the whole data for curve everytime anything changes (flags approach)
- investigate why drawing takes so much time
- investigate why this application on Windows is ~10x slower than on Linux
- add computing lines on GPU
- Checbyshev polynomial bezier degree lowering method
- multi-threaded computaiton (GPU?) of internsive stuff
- remove todos
- fix adding cubic bezier points when zero control points - now its hardcoded (0.5f, 0.5f)
- add SIMD
- maybe splitting, deCasteljau visualization, degree lowering should be per curve, not global (but maybe not)
- better profiler, snapshots, shouldn't update so frequently
- change curve v4s when combining or choosing to transform
- splitting and splitting on point curves should have either the same name or something better than (left), (right)
- change the way SetCurveControlPoints works to optimize a little - avoid unnecessary memcpys and allocs
- take more things by pointer in general
- maybe don't be so precise when checking curvepoints collisions - jump every 50 points or something like that
- don't use constructors, more use designated struct initialization, or nothing?
- don't use other type for saving entity, use current type and override pointers - why? simplify by default, don't complicate by default, and more types is more complicated than less types
- consider not using malloced strings in notifications and in images, just static string will do?
- maybe rename EntityDestroy, in general consider using Allocate/Deallocate maybe or Make/Destroy, or something consistent more, just think about it
- in general do a pass on variable names, make them less descriptive, like [EditorParams], [EditorState], [ChckecCollisionWithFlags], ...
- when returning named type from internal like [added_point_index], include 64 in the name, and don't use this name at call sites, just variable name will do
- is [MarkCurveForRecomputation] really needed after every operation to curve like [Append] or [Insert], can't just set the flag instead
- when passing [curve] to a internal but as [entity], maybe write a macro that gets [curve] from that [entity] but asserts that it really is a curve
- get rid of defer and templates
- get rid of [MemoryZero] in all initialization internals?
- replace imgui
- [splitting_curve_point] and similar shouldn't have [IsSplitting] maybe, just some pointer to identify whether this is happening
 - when initializing entities, image and curve, I use [MemoryZero] a lot, maybe just assume that memory is zeroed already
- be careful when calling [SetCurveControlPoints] when it comes to whether points are in world or local space
- probably allocate things like arena in [editor_state], don't pass them into [Make] internal, because they are probably not used outside of this internal, probably do it to other internals as well
- go pass through the whole source code and shorten variable names dramatically
- rename "Copy" button into "Duplicate"
- do a pass over internals that are called just 1 time and maybe inline them
- Focus on curve is weird for larger curves - investigate that
- Delete key should do the same as right click but just for selected, not positioned
- placeholder when loading images asynchronously
- improve error msg when failed to load images - include name or something
- improve error msg when failed to start to load image - write some vague msg (because why user would care about the details here) and make sure to include "try again" or sth like that

Bugs:
- clicking on control point doesn't do anything, but should select this curve and control point
- splitting bezier curve doesnt fully recompute one of the curves
- removing curve point unselects curve, dont do that
- klikanie na image kiedy jest wysuniety na przod za pomoca sorting layer i tak dodaje punkt zamiast robic select tego image
- when i'm moving control point and go out of the win32 window, renderer curve line is crazy
- when picking up control point, don't set it's position to the mouse position, but instead to a delta
- ZOffset are fucked - if multiple images have the same ZOffset, make sure to check collisions in the reverse order they are renderer
- when I open image to load with an image dialog, app then doesnt detect code recompilation and doesnt hot reload
- adding points to periodic curve doesn't work again
- instead of marking curve when it should be recomputed, just have a structure that holds curve "settings", and every time, check if it changed, it yes then recompute, otherwise don't
- there might be something wrong with right-click - I think (but not sure) that sometimes releasing doesnt remove images for example (but curves also)
- loading a lot of files at once asynchronously doesn't work, only some images are loaded
- I need to test what happens when queue texture memory is emptied - because I think there is a bug in there
- scaling is not implemented correctly - specifically lines vs control points are not in the same place in relation to each other when I scale the curve
- dragging tracked point along curve is broken - specifically for "pętelka" ale może dla innych przypadków też
- disabling control points is done globally currently - not per entity as it should be
- watch out for hot-reaoading  - I use NilExpr in editor_parametric_equation.h and it is global - because of that hot-realoding might crash

Stack:
 - add sorting to rendering - this then will allow to draw things out of order and will simplify the code a lot
- don't always require to specify model matrix - do something like SetModel(transform_inv)
- check why editor type is so fucking big - probably compress it a little
- try to remove multiple transform_invs from render_group
- usunac artefakty ktore sie pojawiaja gdy linia sie zagina w druga storne bardzo ostro
- okienka ui powinny byc customizowalne jak duze
- inserting control point in the middle of polynomial curve is sometimes broken
- because clicks are now highly independent, we might now delete control point while moving the same control point which could lead to some weird behaviours, investigate that
 - try to highlight entity that is about to be removed - maybe even in general - highlight stuff upon sliding mouse into its collider
- curves should have mesh_id/model_id in the same way images have texture_id
- load files texture asynchronously
- do accumulated input pass after procsesing all the input - instead of accumulating by overwriting Pressed inside input handling function
that way we can be sure what is the final state of Platform_Key/Button


Testing:
- check what happens when texture queue is very small
- async queue
- task with memory queue

*/

typedef u32 collision_flags;
enum
{
 Collision_CurvePoint   = (1<<0),
 Collision_CurveLine    = (1<<1),
 Collision_TrackedPoint = (1<<2),
};
struct collision
{
 entity *Entity;
 collision_flags Flags;
 curve_point_index CurvePointIndex;
 u32 CurveLinePointIndex;
};

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

// defines visibility/draw order for different curve parts
enum curve_part
{
 // this is at the bottom
 CurvePart_LineShadow,
 
 CurvePart_CurveLine,
 CurvePart_CurveControlPoint,
 
 CurvePart_CurvePolyline,
 CurvePart_CurveConvexHull,
 
 CurvePart_CubicBezierHelperLines,
 CurvePart_CubicBezierHelperPoints,
 
 CurvePart_DeCasteljauAlgorithmLines,
 CurvePart_DeCasteljauAlgorithmPoints,
 
 // this is at the very top
 CurvePart_BezierSplitPoint,
 
 CurvePart_Count,
};
internal f32
GetCurvePartZOffset(curve_part Part)
{
 Assert(Part < CurvePart_Count);
 f32 Result = Cast(f32)(CurvePart_Count-1 - Part) / CurvePart_Count;
 return Result;
}

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

enum editor_left_click_mode
{
 EditorLeftClick_MovingTrackingPoint,
 EditorLeftClick_MovingCurvePoint,
 EditorLeftClick_MovingEntity,
};
struct editor_left_click_state
{
 b32 Active;
 
 entity_id TargetEntity;
 editor_left_click_mode Mode;
 curve_point_index CurvePointIndex;
 v2 LastMouseP;
 
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

struct renderer_index
{
 renderer_index *Next;
 u32 Index;
};

struct editor_assets
{
 renderer_transfer_queue *RendererQueue;
 renderer_index *FirstFreeTextureIndex;
 renderer_index *FirstFreeBufferIndex;
};

struct task_with_memory
{
 b32 Allocated;
 arena *Arena;
};

enum image_loading_state
{
 Image_Loading,
 Image_Loaded,
 Image_Failed,
};
// NOTE(hbr): Maybe async tasks (that general name) is not the best here.
// It is currently just used for loading images asynchronously (using threads).
// I'm happy to generalize this (for example store function pointers in here),
// but for now I only have images use case and don't expect new stuff. I didn't
// bother renaming this.
struct async_task
{
 b32 Active;
 arena *Arena;
 image_loading_state State;
 u32 ImageWidth;
 u32 ImageHeight;
 string ImageFilePath;
 v2 AtP;
 renderer_index *TextureIndex;
};

struct choose_2_curves_state
{
 b32 WaitingForChoice;
 entity_id Curves[2];
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
 entity MergeEntity;
};

struct editor
{
 camera Camera;
 frame_stats FrameStats;
 
 entity_id SelectedEntityId;
 
#define MAX_ENTITY_COUNT 1024
 entity Entities[MAX_ENTITY_COUNT];
 u64 EverIncreasingEntityCounter;
 
#define MAX_NOTIFICATION_COUNT 16
 u32 NotificationCount;
 notification Notifications[MAX_NOTIFICATION_COUNT];
 
 b32 HideUI;
 b32 EntityListWindow;
 b32 DiagnosticsWindow;
 b32 SelectedEntityWindow;
 b32 HelpWindow;
 
 editor_left_click_state LeftClick;
 editor_right_click_state RightClick;
 editor_middle_click_state MiddleClick;
 
 editor_assets Assets;
 
#define MAX_TASK_COUNT 128
 task_with_memory Tasks[MAX_TASK_COUNT];
 async_task AsyncTasks[MAX_TASK_COUNT];
 
 struct work_queue *LowPriorityQueue;
 struct work_queue *HighPriorityQueue;
 
 animating_curves_state AnimatingCurves;
 merging_curves_state MergingCurves;
 
 //////////////////////////////
 
 // TODO(hbr): Remove this from the state
 render_group *RenderGroup;
 
 arena *MovingPointArena;
 
 v4 DefaultBackgroundColor;
 v4 BackgroundColor;
 f32 CollisionToleranceClip;
 f32 RotationRadiusClip;
 curve_params CurveDefaultParams;
};

struct load_image_work
{
 task_with_memory *Task;
 renderer_transfer_op *TextureOp;
 string ImagePath;
 async_task *AsyncTask;
};

#endif //EDITOR_H
