#ifndef EDITOR_H
#define EDITOR_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"
#include "base/base_memory.h"

#include "editor_debug.h"
#include "editor_profiler.h"
#include "editor_imgui.h"
#include "editor_platform.h"
#include "editor_math.h"
#include "editor_renderer.h"
#include "editor_sort.h"
#include "editor_parametric_equation.h"
#include "editor_ui.h"
#include "editor_stb.h"
#include "editor_camera.h"
#include "editor_editor.h"
#include "editor_const.h"
#include "editor_ctx.h"

/* TODO(hbr):

@ stack:

TODO features:

crucial:
- Checbyshev polynomial bezier degree lowering method
- incluce imgui tutorial to get familiar with the ui widgets used throughout the project
- icon
- maybe different title bar

important:
- extracing animated bezier curve perfectly
 - undo/redo should actually support running out of space

maybe:
- better error msgs when curves fail to merge
- parallelize curve recomputation
- GPU buffer_index in the same way we have texture_index
- section about moving cubic bezier helpers in help
- section about undo/redo in help
- section about parametric equations in help
- add resource monitor - memory/texture handles - to investigate if there are any leaks
- holding Ctrl-Z should do a bunch of undos in a row - aka. sticky keys or something
- "sample"? tool - from point that lets us sample color from the application and use it to color curve for example
- multiselect curves to change it's parameters at once - for example change line width/color, also maybe to move them at once
  - resize "handles" for images
 - don't recompute animation every frame, clean up the code, while it's not messy per-se, there is a lot of unnecessary paths that result in something very fragile

 TODO bugs:

crucial:
- jak robie undo/redo wiele razy z rzedu to czasami na chwilke pojawiaja sie pewne artefakty rysowania linii - tak jakbym rysowal wielki trojkat zamiast malych ktore skalaja sie na linie, wyglada to jakby jakis vertex linii zostal przesuniety (do (0,0)?
- this is not working properly for parametric expression: unexpected error but this should evaluate: StrLit("sin(t) * (exp(sin(t)) - 2*cos(4t))"),
- undo/redo doesn't seem to work well with entity list and what is selected - I tend to have multiple things that are selected
- removing control points when animating curves cause "division by zero", check the same for merging curves
- undo/redo doesn't track if we changed the curve shape - thus we can remove control point, change the curve shape to shape that doesn't have control points, and then undo - it will try to add control point to a curve that doesn't use control points - error

maybe:
- usunac artefakty ktore sie pojawiaja gdy linia sie zagina w druga storne bardzo ostro
- ZOffset is fucked - if multiple images have the same ZOffset, make sure to check collisions in the reverse order they are renderer
- I need to test what happens when queue texture memory is emptied - because I think there is a bug in there
 - because clicks are now highly independent, we might now delete control point while moving the same control point which could lead to some weird behaviours, investigate that
 - focusing on entity is not 100% correct mathematically - if camera is rotated and entity is rotated, it zooms out a little too much compared to what it "should", although probably noone will notice anyway, I can't figure out the math behing this
 - imgui modal background color fades in in two steps and because in release mode we don't update the window if [dt] is not requested, it will enter phase 2 only when mouse is moved or something like that

fixed:
- adding points to periodic curve doesn't work again
- inserting control point in the middle of polynomial curve is sometimes broken
- mergeing linii z sama soba sprawia ze cos sie dziwnego dzieje z undo/redo

  ideas:
 - confirm window when removing points
 - loop curve with checkbox - it seems useful
 - czesto jak rysuje to jestem mega zzoomwany na image - chce dodac nowa curve ale nie mam jak latwo odselectowac aktualnej bo wszedzie wokolo jest image
 - ogolnie caly czas sie boje ze cos usune - undo/redo to troche zmityguje ale chyba to nie powinno byc takie proste - szczegolnie images - byc moze lewy przycisk nie powinien usuwac entities - tylko punkty na przyklad
 - hide all control points in ALL curves using some button or something
 - resize image "inline", not in separate window
 - persistent openGL mapping for texture uploads, mesh uploads
 - image paths relative to the project
 - add docking
 - investigate why drawing takes so much time
 - investigate why this application on Windows is ~10x slower than on Linux
 - add computing lines on GPU
 - multi-threaded computaiton (GPU?) of internsive stuff
 - add SIMD
 - change curve v4s when combining or choosing to transform
 - maybe don't be so precise when checking curvepoints collisions - jump every 50 points or something like that
 - don't use other type for saving entity, use current type and override pointers - why? simplify by default, don't complicate by default, and more types is more complicated than less types
 - improve error msg when failed to load images - include name or something
 - improve error msg when failed to start to load image - write some vague msg (because why user would care about the details here) and make sure to include "try again" or sth like that
 - try to highlight entity that is about to be removed - maybe even in general - highlight stuff upon sliding mouse into its collider
 - curves should have mesh_id/model_id in the same way images have texture_id
 
new things added:
- undo/redo
 - fixed inactive buttons
- improved help
- parametric curves predefined examples
- grid
- multiple bug fixes
- moving b-spline point (partial)
- merging weighted bezier curves
- moving partition knot along curve
- convex hull around pieces of b-spline segments
- extract animated curve - bezier can be 100% matching, other can do with interpolation
- saving/loading project from/to file
- remove DLL loading in release mode

 */

struct rendering_entity_handle
{
 entity *Entity;
 render_group *RenderGroup;
};

struct editor_keyboard_shortcut
{
 platform_key Key;
 platform_key_modifier_flags Modifiers;
};
struct editor_keyboard_shortcut_group
{
 editor_command Command;
 editor_keyboard_shortcut Shortcuts[2];
 u32 Count;
 b32 DevSpecific;
};

global read_only editor_keyboard_shortcut_group EditorKeyboardShortcuts[] =
{
 {EditorCommand_New, {{PlatformKey_N, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_Open, {{PlatformKey_O, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_Save, {{PlatformKey_S, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_SaveAs, {{PlatformKey_S, KeyModifierFlag(Ctrl) | KeyModifierFlag(Shift)}}, 1, false},
 {EditorCommand_Quit, {{PlatformKey_Escape, NoKeyModifier}}, 1, false},
 {EditorCommand_ToggleDevConsole, {{PlatformKey_Backtick, NoKeyModifier}}, 1, true},
 {EditorCommand_Delete, {{PlatformKey_X, KeyModifierFlag(Ctrl)}, {PlatformKey_Delete, NoKeyModifier}}, 2, false},
 {EditorCommand_Duplicate, {{PlatformKey_D, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_ToggleProfiler, {{PlatformKey_Q, KeyModifierFlag(Ctrl)}}, 1, true},
 {EditorCommand_Undo, {{PlatformKey_Z, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_Redo, {{PlatformKey_R, KeyModifierFlag(Ctrl)}}, 1, false},
 {EditorCommand_ToggleUI, {{PlatformKey_Tab, NoKeyModifier}}, 1, false},
};
StaticAssert(ArrayCount(EditorKeyboardShortcuts) == EditorCommand_Count, EditorKeyboardShortcutsDefined);

#endif //EDITOR_H
