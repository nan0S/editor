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

#include "editor_file.h"
#include "editor_adapt.h"
#include "editor_entity.h"
#include "editor_draw.h"
#include "editor_debug.h"
#include "editor_input.h"
#include "editor_editor.h"
#include "editor_project.h"

#if defined(BUILD_DEBUG)
#define LOG(...) Log(__VA_ARGS__)
#else
#define LOG(...)
#endif

internal void Log(char const *Fmt, ...);
internal void ReportError(char const *Fmt, ...);

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
- change the way CurveSetControlPoints works to optimize a little - avoid unnecessary memcpys and allocs
- take more things by pointer in general
- maybe don't be so precise when checking curvepoints collisions - jump every 50 points or something like that
- use u32 instead of u64 in most places
- don't use constructors, more use designated struct initialization, or nothing?
- remove defer?
- make os layer
- maybe make arenas infinitely growable?
- use Count rather that Num
- don't use other type for saving entity, use current type and override pointers - why? simplify by default, don't complicate by default, and more types is more complicated than less types
- consider not using malloced strings in notifications and in images, just static string will do?
- rename [EditorState]
- rename SizeOf, Cast
- replace string with ptr and size struct
- maybe rename EntityDestroy, in general consider using Allocate/Deallocate maybe or Make/Destroy, or something consistent more, just think about it
- in general do a pass on variable names, make them less descriptive, like [EditorParams], [EditorState], [ChckecCollisionWithFlags], ...
- when returning named type from internal like [added_point_index], include 64 in the name, and don't use this name at call sites, just variable name will do
- is [CurveRecompute] really needed after every operation to curve like [Append] or [Insert], can't just set the flag instead
- replace [Assert(false]] with InvalidPath or something like that, and add [InvalidPath] to [base.h]
- when passing [curve] to a internal but as [entity], maybe write a macro that gets [curve] from that [entity] but asserts that it really is a curve
- get rid of defer and templates
- get rid of [MemoryZero] in all initialization internals?
- replace printf
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

DONE:
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
- fix CurvePointIndex calculation in case of collisions, etc., dividing by NumCurvePointsPerSegment doesnt't work
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

Ideas:
- some kind of locking system - when I want to edit only one curve without
obstacles, I don't want to unwittingly click on some other curve/image and select it
- confirm window when removing points
*/

#endif //EDITOR_H
