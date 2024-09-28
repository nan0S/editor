#ifndef EDITOR_H
#define EDITOR_H

// TODO(hbr): what the fuck is this
#pragma warning(1 : 4062)

#include "third_party/sfml/include/SFML/Graphics.hpp"
#include "imgui_inc.h"

#define EDITOR_PROFILER 1

#include "editor_base.h"
#include "editor_os.h"
#include "editor_profiler.h"
#include "editor_math.h"
#include "editor_ui.h"

#include "editor_adapt.h"
#include "editor_entity.h"
#include "editor_draw.h"
#include "editor_debug.h"
#include "editor_input.h"

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
- add computing lines on GPU, remove SFML, add graphic acceleration
- Checbyshev polynomial bezier degree lowering method
- multi-threaded computaiton (GPU?) of internsive stuff
- remove todos
- do kinda major refactor
- fix adding cubic bezier points when zero control points - now its hardcoded (0.5f, 0.5f)
- add hot code reloading
- add SIMD
- maybe splitting, deCasteljau visualization, degree lowering should be per curve, not global (but maybe not)
- better profiler, snapshots, shouldn't update so frequently
- change curve colors when combining or choosing to transform
- splitting and splitting on point curves should have either the same name or something better than (left), (right)
- change the way SetCurveControlPoints works to optimize a little - avoid unnecessary memcpys and allocs
- take more things by pointer in general
- maybe don't be so precise when checking curvepoints collisions - jump every 50 points or something like that
- use u32 instead of u64 in most places
- don't use constructors, more use designated struct initialization, or nothing?
- make os layer
- don't use other type for saving entity, use current type and override pointers - why? simplify by default, don't complicate by default, and more types is more complicated than less types
- consider not using malloced strings in notifications and in images, just static string will do?
- rename [EditorState]
- rename SizeOf, Cast
- replace string with ptr and size struct
- maybe rename EntityDestroy, in general consider using Allocate/Deallocate maybe or Make/Destroy, or something consistent more, just think about it
- in general do a pass on variable names, make them less descriptive, like [EditorParams], [EditorState], [ChckecCollisionWithFlags], ...
- when returning named type from internal like [added_point_index], include 64 in the name, and don't use this name at call sites, just variable name will do
- is [RecomputeCurve] really needed after every operation to curve like [Append] or [Insert], can't just set the flag instead
- replace [Assert(false]] with InvalidPath or something like that, and add [InvalidPath] to [base.h]
- when passing [curve] to a internal but as [entity], maybe write a macro that gets [curve] from that [entity] but asserts that it really is a curve
- get rid of defer and templates
- get rid of [MemoryZero] in all initialization internals?
- replace imgui
- replace sfml
- rename internals to more natural language, don't follow those stupid conventions
- [splitting_curve_point] and similar shouldn't have [IsSplitting] maybe, just some pointer to identify whether this is happening
- implement [NotImplemented]
- make sure window resizing works correctly
 - when initializing entities, image and curve, I use [MemoryZero] a lot, maybe just assume that memory is zeroed already
- use "Count" instead of "Num"
- be careful when calling [SetCurveControlPoints] when it comes to whether points are in world or local space
- probably allocate things like arena in [editor_state], don't pass them into [Make] internal, because they are probably not used outside of this internal, probably do it to other internals as well
- go pass through the whole source code and shorten variable names dramatically
- rename "Copy" button into "Duplicate"
- pressing Tab hides all the ui
- do a pass over internals that are called just 1 time and maybe inline them
- remove Bezier_Normal - just replace with Weighted
- Focus on curve is weird for larger curves - investigate that

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
- replace sf::Vector2f with v2f32
- replace float with f32
- replace sf::Vector2i with v2s32
- replace sf::Vector2u with v2u32
- measure and improve performance
- add point selection
- selected curve points should have outline instead of different color
- add different color selected point
- control point weights should have nicer ui for changing weights
- add possibilty to change default editor parameters
- add possibility to change default curve parameters
- add parameters to project file format saving
- fit nice default colors, especially selected curve outline color
- add possibility for curve color to have different alpha than 255
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
- fix CurvePointIndex calculation in case of collisions, etc., dividing by CurvePointCountPerSegment doesnt't work
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
- get rid off line_vertices_allocation_type
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

Bugs:
- clicking on control point doesn't do anything, but should select this curve and control point
*/

#define WINDOW_TITLE "Parametric Curves Editor"
#define SAVED_PROJECT_FILE_EXTENSION ".editor"
// NOTE(hbr): This is a little too complex for such a simple thing.
#define PROJECT_FILE_EXTENSION_SELECTION "Project File (*" SAVED_PROJECT_FILE_EXTENSION ")"
#define JPG_FILE_EXTENSION_SELECTION "JPG Image (*.jpg)"
#define PNG_FILE_EXTENSION_SELECTION "PNG Image (*.png)"

#define SAVE_AS_MODAL_EXTENSION_SELECTION \
PROJECT_FILE_EXTENSION_SELECTION "{" SAVED_PROJECT_FILE_EXTENSION "}" \
"," JPG_FILE_EXTENSION_SELECTION "{.jpg}" \
"," PNG_FILE_EXTENSION_SELECTION "{.png}"

// TODO(hbr): Do something with this variable. Don't want to load project on different monitor
// and images to be different size.
global f32 GlobalImageScaleFactor         = 1.0f / 1920.0f;

struct render_point_data
{
   f32 RadiusClipSpace;
   f32 OutlineThicknessFraction;
   color FillColor;
   color OutlineColor;
};

struct editor_params
{
   render_point_data RotationIndicator;
   render_point_data BezierSplitPoint;
   
   color BackgroundColor;
   f32   CollisionToleranceClipSpace;
   f32   LastControlPointSizeMultiplier;
   
   f32   SelectedCurveControlPointOutlineThicknessScale;
   color SelectedCurveControlPointOutlineColor;
   color SelectedControlPointOutlineColor;
   f32   CubicBezierHelperLineWidthClipSpace;
   curve_params CurveDefaultParams;
};

struct camera
{
   world_position Position;
   rotation_2d Rotation;
   
   f32 Zoom;
   f32 ZoomSensitivity;
   
   b32 ReachingTarget;
   f32 ReachingTargetSpeed;
   world_position PositionTarget;
   f32 ZoomTarget;
};

typedef u64 curve_collision_flags;
enum
{
   CurveCollision_ControlPoint     = (1<<0),
   CurveCollision_CurveLine        = (1<<1),
   CurveCollision_CubicBezierPoint = (1<<2),
   CurveCollision_SplitPoint       = (1<<3),
   CurveCollision_DeCasteljauPoint = (1<<4),
};

struct collision
{
   entity *Entity;
   curve_collision_flags Flags;
   u64 ControlPointIndex;
   u64 CubicBezierPointIndex;
   u64 CurveLinePointIndex;
};

typedef u64 check_collision_with_flags;
enum
{
   CheckCollisionWith_CurveControlPoints = (1<<0),
   CheckCollisionWith_CurveLines         = (1<<1),
   CheckCollisionWith_CurveSplitPoints   = (1<<2),
   CheckCollisionWith_DeCastelajauPoints = (1<<3),
   CheckCollisionWith_Images             = (1<<4),
};

enum user_action_type
{
   UserAction_None,
   UserAction_ButtonClicked,
   UserAction_ButtonDrag,
   UserAction_ButtonReleased,
   UserAction_MouseMove,
};

struct user_action
{
   user_action_type Type;
   union
   {
      struct {
         button Button;
         screen_position ClickPosition;
      } Click;
      
      struct {
         button Button;
         screen_position DragFromPosition;
      } Drag;
      
      struct {
         button Button;
         screen_position ReleasePosition;
      } Release;
      
      struct {
         screen_position FromPosition;
         screen_position ToPosition;
      } MouseMove;
   };
   user_input *Input;
};

enum editor_mode_type
{
   EditorMode_Normal,
   EditorMode_Moving,
   EditorMode_Rotating,
};

// TODO(hbr): Try to get rid of this type, if this is necessary, just remove this TODO
enum moving_mode_type
{
   MovingMode_Entity,
   MovingMode_Camera,
   MovingMode_CurvePoint,
   MovingMode_SplitPoint,
   MovingMode_DeCasteljauPoint,
};

// TODO(hbr): Be consistent in naming Rotating mode or Rotate mode - be consistent
struct editor_mode
{
   editor_mode_type Type;
   union
   {
      struct {
         moving_mode_type Type;
         entity *Entity;
         
         u64 PointIndex;
         b32 IsBezierPoint;
         
         sf::Vertex *SavedCurveVertices;
         u64 SavedNumCurveVertices;
         sf::PrimitiveType SavedPrimitiveType;
      } Moving;
      
      struct {
         entity *Entity;
         screen_position RotationCenter;
         button Button;
      } Rotating;
   };
};

enum animate_curve_animation_stage
{
   AnimateCurveAnimation_None,
   AnimateCurveAnimation_PickingTarget,
   AnimateCurveAnimation_Animating,
};

enum animation_type : u8
{
   Animation_Smooth,
   Animation_Linear,
   
   Animation_Count,
};
read_only global char const *AnimationNames[] = { "Smooth", "Linear" };
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
   u64 CurvePointCount;
   v2f32 *ToCurvePoints;
   line_vertices AnimatedCurveVertices;
   
   b32 IsAnimationPlaying;
   animation_type Animation;
   f32 AnimationSpeed;
   f32 AnimationMultiplier;
   f32 Blend;
};

enum curve_combination_type : u8
{
   CurveCombination_None,
   
   CurveCombination_Merge,
   CurveCombination_C0,
   CurveCombination_C1,
   CurveCombination_C2,
   CurveCombination_G1,
   
   CurveCombination_Count
};
read_only global char const *CurveCombinationNames[] = { "None", "Merge", "C0", "C1", "C2", "G1" };
StaticAssert(ArrayCount(CurveCombinationNames) == CurveCombination_Count, CurveCombinationNamesDefined);

struct curve_combining_state
{
   entity *SourceEntity;
   entity *WithEntity;
   b32 SourceCurveLastControlPoint;
   b32 WithCurveFirstControlPoint;
   curve_combination_type CombinationType;
};

enum action_to_do
{
   ActionToDo_Nothing,
   ActionToDo_NewProject,
   ActionToDo_OpenProject,
   ActionToDo_Quit,
};

enum notification_type
{
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
   f32 BoxHeight;
};

// NOTE(hbr): Not sure if this is the best name
struct ui_config
{
   b32 ViewSelectedEntityWindow;
   b32 ViewListOfEntitiesWindow;
   b32 ViewParametersWindow;
   b32 ViewDiagnosticsWindow;
   b32 ViewProfilerWindow;
   b32 ViewDebugWindow;
   
   b32 DefaultCurveHeaderCollapsed;
   b32 RotationIndicatorHeaderCollapsed;
   b32 BezierSplitPointHeaderCollapsed;
   b32 OtherSettingsHeaderCollapsed;
};

struct render_data
{
   sf::RenderWindow *Window;
   camera Camera;
   f32 AspectRatio;
   f32 FrustumSize;
};

struct editor
{
   sf::Clock DeltaClock;
   frame_stats FrameStats;
   render_data RenderData;
   
   entities Entities;
   u64 EntityCounter;
   entity *SelectedEntity;
   
   arena *MovingPointArena;
   editor_mode Mode;
   editor_params Params;
   ui_config UI_Config;
   
   curve_animation_state       CurveAnimation;
   curve_combining_state       CurveCombining;
   
   b32 Empty;
   arena *ProjectPathArena;
   string ProjectPath;
   action_to_do ActionWaitingToBeDone;
   
   u64 NotificationCount;
   notification Notifications[32];
   
   b32 HideUI;
};

#endif //EDITOR_H
