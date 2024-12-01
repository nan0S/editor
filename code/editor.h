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
#include "editor_entity2.h"
#include "editor_entity.h"
#include "editor_ui.h"

/* TODO(hbr):
Refactors:
- get rid of SFML? (f32).
- probably add optimization to evaluate multiple points at once, maybe even don't store Ti, just iterator through iterator
- consider using restrict keyword
- remove that ugly Gaussian Elimination, maybe add optimized version of periodic cubic spline M calculation

Functionality:
- fix camera rotating
- consider changing left mouse button internalality - pressing adds new point which is automatically moved until release
- consider changing the way removing points work - if press and release on the same point then delete, otherwise nothing,
this would remove necessity of using Are PressPoint and ReleasePoints close

TODOs:
- image paths relative to the project
- add docking
- maybe change SomeParameterChanged to be more granular in order to avoid recomputing the whole data
 for curve everytime anything changes (flags approach)
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
- is [RecomputeCurve] really needed after every operation to curve like [Append] or [Insert], can't just set the flag instead
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
- hide UI with Tab

DONE:
- replace printf
- fix convex hull line width problem
- fix that clicking on image will move the image
- image rotation
- image scaling
- selection system - select images or curves
- fix image collision with scale and rotation
- pickup images true to their layer
- image layer sorting
- list of entitires with selection capabilities and folding
- disabling list of entities
- naming of entities - curves and images
- fix saving project
- fix saving project as PNG or JPG
- rotating camera
- semi-transparent circle when rotating with mouse
- rotation indicator is too big when zooming in camera
- move internals to their corresponding struct definitions
- notification system
- better notifications and error messages
- update TODO comments
- world_position, screen_position, camera_position
- when creating images and curves in editor_project.cpp, first gather the data, and then make the curve and image
- replace sf::Vector2f with v2
- replace float with f32
- replace sf::Vector2i with v2i
- replace sf::Vector2u with v2u32
- measure and improve performance
- add point selection
- selected curve points should have outline instead of different v4
- add different v4 selected point
- control point weights should have nicer ui for changing weights
- add possibilty to change default editor parameters
- add possibility to change default curve parameters
- add parameters to project file format saving
- fit nice default v4s, especially selected curve outline v4
- add possibility for curve v4 to have different alpha than 255
- collision tolerance should not be in world space, it should be in camera space
- fix GlobalImageScaleFactor
- rename some variables from Scale to ClipSpace
- key input handling
- weighted bezier curve degree elevation
- weighted bezier curve degree lowering
- O(n) calculation of bezier curve
- window popping out to choose Alpha when inversed-elevation lowering degree method of bezier curve fails
- remove ;; in different places
- add reset position and rotation to curve
- dragging Cubic Bezier helper points
- correct rendering of cubic spline helper lines - they should be below curve
- remove pragma
- adding points to Cubic Spline with in-between point selection when dragged
- saving curve should include CubicBezierPoints
- when moving control point, still show the previous curve, only when button is released actually change the curve
- fix CurvePointIndex calculation in case of collisions, etc., dividing by LinePointCountPerSegment doesnt't work
 in case of CubicSpline Periodic
- focus on images
- animating camera focus
- focus on curve/image
- change layout of action buttons
- disabled buttons instead of no buttons
- split on control point
- fix action buttons placement, tidy them up
- animating top speed should not be 1.0f
- add arrow to indicate which curve is combine TO which
- merge 
- there is a lot of places where I use %s to indicate sprintf format, but I pass string instead of char * - fix that
- because we are going away from returning strings when some os function fails, rather returning boolean, refactor
 all those places that convert boolean into string artificially, instead just pass boolean further
- maybe make things that are only one just global - like input
- change function names to DoSomethingEntity, rather than EntityDoSomething
- get rid off vertex_array_allocation_type
- remove [RecomputeCurve] from every function that modifies curve
- rename [CurveEntity] to just [Entity] - keep things simple
- write [CurveAppendPoint] in terms of [CurveInsertPoint]
 - replace strcmp, memcpy, memset, memmove
- maybe move all the things related to rendering into separate struct - things like Window, Projection, ...
- specify sizes of enums
- there is a lot of compressing opportunities in [entity] struct, try to compress
- think about supporting merging curve of bezier type with cubic bezier subtype
- optimization idea: sometimes we create a temporary buffer of control points and then call CurveSetControlPoints,
   instead we could create those control points already in place - in the curve we would want to call CurveSetControlPoints,
the API could look like CurveBeginControlPoints(), CurveEndControlPoints()

Ideas:
- some kind of locking system - when I want to edit only one curve without
obstacles, I don't want to unwittingly click on some other curve/image and select it
- confirm window when removing points

Ideas:
- dodawanie punktow z dwoch stron
- loop curve with checkbox - it seems useful
- czesto jak rysuje to jestem mega zzoomwany na image - chce dodac nowa curve ale nie mam jak latwo odselectowac aktualnej bo wszedzie wokolo jest image
- ogolnie caly czas sie boje ze cos usune - undo/redo to troche zmityguje ale chyba to nie powinno byc takie proste - szczegolnie images - byc moze lewy przycisk nie powinien usuwac entities - tylko punkty na przyklad
- zmienianie koloru krzywej "inline" a nie w osobnym okienku
- hide all control points in ALL curves using some button or something
- resize image "inline", not in separate window
- persistent openGL mapping for texture uploads, mesh uploads

Bugs:
- clicking on control point doesn't do anything, but should select this curve and control point
- splitting bezier curve doesnt fully recompute one of the curves
- removing curve point unselects curve, dont do that
- klikanie na image kiedy jest wysuniety na przod za pomoca sorting layer i tak dodaje punkt zamiast robic select tego image
- when i'm moving control point and go out of the win32 window, renderer curve line is crazy
- when picking up control point, don't set it's position to the mouse position, but instead to a delta
- ZOffset are fucked - if multiple images have the same ZOffset, make sure to check collisions in the reverse order they are renderer

Stack:
 - add sorting to rendering - this then will allow to draw things out of order and will simplify the code a lot
- don't always require to specify model matrix - do something like SetModel(transform_inv)
- check why editor type is so fucking big - probably compress it a little
 - remove world_position, screen_position, clip_space and camera_position
- try to remove multiple transform_invs from render_group
- usunac artefakty ktore sie pojawiaja gdy linia sie zagina w druga storne bardzo ostro
- okienka ui powinny byc customizowalne jak duze
- inserting control point in the middle of polynomial curve is sometimes broken
- because clicks are now highly independent, we might now delete control point while moving the same control point which could lead to some weird behaviours, investigate that
 - try to highlight entity that is about to be removed - maybe even in general - highlight stuff upon sliding mouse into its collider
- curves should have mesh_id/model_id in the same way images have texture_id
- load files texture asynchronously
*/

struct camera
{
 v2 P;
 v2 Rotation;
 f32 Zoom;
 f32 ZoomSensitivity;
 
 b32 ReachingTarget;
 f32 ReachingTargetSpeed;
 v2 TargetP;
 f32 TargetZoom;
};

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

enum animate_curve_animation_stage
{
 AnimateCurveAnimation_None,
 AnimateCurveAnimation_PickingTarget,
 AnimateCurveAnimation_Animating,
};

enum animation_type
{
 Animation_Smooth,
 Animation_Linear,
 Animation_Count,
};
global char const *AnimationNames[] = { "Smooth", "Linear" };
StaticAssert(ArrayCount(AnimationNames) == Animation_Count, AllAnimationNamesDefined);

struct curve_animation_state
{
 animate_curve_animation_stage Stage;
 
 entity *FromCurveEntity;
 entity *ToCurveEntity;
 
 arena *Arena;
 u64 SavedToCurveVersion;
 // NOTE(hbr): Number of points in ToCurve has to match number
 // of points in FromCurve in order to avoid animation artifacts.
 u32 LinePointCount;
 v2 *ToLinePoints;
 vertex_array AnimatedLineVertices;
 
 b32 IsAnimationPlaying;
 animation_type Animation;
 f32 AnimationSpeed;
 f32 AnimationMultiplier;
 f32 Blend;
};

enum curve_combination_type : u32
{
 CurveCombination_None,
 CurveCombination_Merge,
 CurveCombination_C0,
 CurveCombination_C1,
 CurveCombination_C2,
 CurveCombination_G1,
 CurveCombination_Count,
};
global char const *CurveCombinationNames[] = { "None", "Merge", "C0", "C1", "C2", "G1" };
StaticAssert(ArrayCount(CurveCombinationNames) == CurveCombination_Count, CurveCombinationNamesDefined);

struct curve_combining_state
{
 entity *SourceEntity;
 entity *WithEntity;
 b32 SourceCurveLastControlPoint;
 b32 WithCurveFirstControlPoint;
 curve_combination_type CombinationType;
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

enum curve_part
{
 CurvePart_LineShadow,
 CurvePart_Line,
 CurvePart_ControlPoint,
 CurvePart_Special,
 CurvePart_Count,
};
internal f32
GetCurvePartZOffset(curve_part Part)
{
 f32 Result = Cast(f32)Part / Cast(f32)CurvePart_Count;
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
 
 entity *TargetEntity;
 editor_left_click_mode Mode;
 curve_point_index CurvePointIndex;
 v2 LastMouseP;
 
 arena *OriginalVerticesArena;
 b32 OriginalVerticesCaptured;
 vertex_array OriginalLineVertices;
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

struct editor
{
 camera Camera;
 frame_stats FrameStats;
 
 entity *SelectedEntity;
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
 
 editor_left_click_state LeftClick;
 editor_right_click_state RightClick;
 editor_middle_click_state MiddleClick;
 
 editor_assets Assets;
 
 //////////////////////////////
 
 // TODO(hbr): Remove this from the state
 render_group *RenderGroup;
 
 arena *MovingPointArena;
 
 v4 DefaultBackgroundColor;
 v4 BackgroundColor;
 f32 CollisionToleranceClip;
 f32 RotationRadiusClip;
 curve_params CurveDefaultParams;
 
 curve_animation_state CurveAnimation;
 curve_combining_state CurveCombining;
};

#endif //EDITOR_H
