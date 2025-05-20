#ifndef EDITOR_H
#define EDITOR_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"
#include "base/base_memory.h"

#include "editor_profiler.h"
#include "editor_imgui.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_math.h"
#include "editor_sort.h"
#include "editor_parametric_equation.h"
#include "editor_entity.h"
#include "editor_ui.h"
#include "editor_stb.h"
#include "editor_camera.h"
#include "editor_collision.h"
#include "editor_core.h"
#include "editor_editor.h"

/* TODO(hbr):
Ideas:
- consider using restrict keyword
- some kind of locking system - when I want to edit only one curve without obstacles, I don't want to unwittingly click on some other curve/image and select it
- confirm window when removing points
- loop curve with checkbox - it seems useful
- czesto jak rysuje to jestem mega zzoomwany na image - chce dodac nowa curve ale nie mam jak latwo odselectowac aktualnej bo wszedzie wokolo jest image
- ogolnie caly czas sie boje ze cos usune - undo/redo to troche zmityguje ale chyba to nie powinno byc takie proste - szczegolnie images - byc moze lewy przycisk nie powinien usuwac entities - tylko punkty na przyklad
- zmienianie koloru krzywej "inline" a nie w osobnym okienku
- hide all control points in ALL curves using some button or something
- resize image "inline", not in separate window
- persistent openGL mapping for texture uploads, mesh uploads
- image paths relative to the project
- add docking
- investigate why drawing takes so much time
- investigate why this application on Windows is ~10x slower than on Linux
- add computing lines on GPU
- Checbyshev polynomial bezier degree lowering method
- multi-threaded computaiton (GPU?) of internsive stuff
- fix adding cubic bezier points when zero control points - now its hardcoded (0.5f, 0.5f)
- add SIMD
- better profiler, snapshots, shouldn't update so frequently
- change curve v4s when combining or choosing to transform
- change the way SetCurveControlPoints works to optimize a little - avoid unnecessary memcpys and allocs
- maybe don't be so precise when checking curvepoints collisions - jump every 50 points or something like that
- don't use other type for saving entity, use current type and override pointers - why? simplify by default, don't complicate by default, and more types is more complicated than less types
- maybe rename EntityDestroy, in general consider using Allocate/Deallocate maybe or Make/Destroy, or something consistent more, just think about it
- rename "Copy" button into "Duplicate"
- Focus on curve is weird for larger curves - investigate that
- Delete key should do the same as right click but just for selected, not positioned
- placeholder when loading images asynchronously
- improve error msg when failed to load images - include name or something
- improve error msg when failed to start to load image - write some vague msg (because why user would care about the details here) and make sure to include "try again" or sth like that

Bugs:
- klikanie na image kiedy jest wysuniety na przod za pomoca sorting layer i tak dodaje punkt zamiast robic select tego image
- ZOffset are fucked - if multiple images have the same ZOffset, make sure to check collisions in the reverse order they are renderer
- adding points to periodic curve doesn't work again
- loading a lot of files at once asynchronously doesn't work, only some images are loaded
- I need to test what happens when queue texture memory is emptied - because I think there is a bug in there
- scaling is not implemented correctly - specifically lines vs control points are not in the same place in relation to each other when I scale the curve

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

Testing:
- check what happens when texture queue is very small
- async queue
- task with memory queue

*/

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
 
 CurvePart_BezierSplitPoint,
 
 CurvePart_B_SplineKnot,
 // this is at the very top
 
 CurvePart_Count,
};

struct rendering_entity_handle
{
 entity *Entity;
 render_group *RenderGroup;
};

struct load_image_work
{
 thread_task_memory_store *Store;
 thread_task_memory *TaskMemory;
 renderer_transfer_op *TextureOp;
 string ImagePath;
 image_loading_task *ImageLoading;
};

internal f32 GetCurvePartZOffset(curve_part Part);

internal rendering_entity_handle BeginRenderingEntity(entity *Entity, render_group *RenderGroup);
internal void EndRenderingEntity(rendering_entity_handle Handle);

#endif //EDITOR_H
