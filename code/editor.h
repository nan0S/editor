/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef EDITOR_H
#define EDITOR_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"
#include "base/base_memory.h"

#include "editor_dev.h"
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

/*
 TODO(hbr): 

- predefined loading doesn't work for flower petal curve
- make button for making bezier curve polynomial in one click
- transforming curves doesn't really work very very well

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
 b32 ShowToTheUser;
};

global read_only editor_keyboard_shortcut_group EditorKeyboardShortcuts[] =
{
 {EditorCommand_New, {{PlatformKey_N, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_Open, {{PlatformKey_O, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_Save, {{PlatformKey_S, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_SaveAs, {{PlatformKey_S, KeyModifierFlag(Ctrl) | KeyModifierFlag(Shift)}}, 1, false, true},
 {EditorCommand_Quit, {{PlatformKey_Escape, NoKeyModifier}}, 1, false, true},
 {EditorCommand_ToggleDevConsole, {{PlatformKey_Backtick, NoKeyModifier}}, 1, true, false},
 {EditorCommand_Delete, {{PlatformKey_X, KeyModifierFlag(Ctrl)}, {PlatformKey_Delete, NoKeyModifier}}, 2, false, true},
 {EditorCommand_Duplicate, {{PlatformKey_D, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_ToggleProfiler, {{PlatformKey_Q, KeyModifierFlag(Ctrl)}}, 1, true, false},
 {EditorCommand_Undo, {{PlatformKey_Z, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_Redo, {{PlatformKey_R, KeyModifierFlag(Ctrl)}}, 1, false, true},
 {EditorCommand_ToggleUI, {{PlatformKey_Tab, NoKeyModifier}}, 1, false, true},
 {EditorCommand_ToggleFullscreen, {{PlatformKey_F11, NoKeyModifier}}, 1, false, true},
 {EditorCommand_ToggleDiagnostics, {{PlatformKey_F3, NoKeyModifier}}, 1, false, false},
};
StaticAssert(ArrayCount(EditorKeyboardShortcuts) == EditorCommand_Count, EditorKeyboardShortcutsDefined);

#endif //EDITOR_H
