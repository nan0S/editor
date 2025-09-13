#ifndef EDITOR_PLATFORM_H
#define EDITOR_PLATFORM_H

struct arena;
struct editor;
struct renderer_transfer_queue;
struct work_queue;
struct profiler;
struct render_frame;

enum platform_event_type
{
 PlatformEvent_None,
 PlatformEvent_Press,
 PlatformEvent_Release,
 PlatformEvent_MouseMove,
 PlatformEvent_Scroll,
 PlatformEvent_WindowClose,
 PlatformEvent_FilesDrop,
 PlatformEvent_Count,
};
read_only global string PlatformEventTypeNames[] = {
 StrLit("None"),
 StrLit("Press"),
 StrLit("Release"),
 StrLit("MouseMove"),
 StrLit("Scroll"),
 StrLit("WindowClose"),
 StrLit("FilesDrop"),
};
StaticAssert(ArrayCount(PlatformEventTypeNames) == PlatformEvent_Count,
             PlatformEventNamesDefined);

enum platform_key
{
 PlatformKey_Unknown,
 
 PlatformKey_F1,
 PlatformKey_F2,
 PlatformKey_F3,
 PlatformKey_F4,
 PlatformKey_F5,
 PlatformKey_F6,
 PlatformKey_F7,
 PlatformKey_F8,
 PlatformKey_F9,
 PlatformKey_F10,
 PlatformKey_F11,
 PlatformKey_F12,
 
 PlatformKey_A,
 PlatformKey_B,
 PlatformKey_C,
 PlatformKey_D,
 PlatformKey_E,
 PlatformKey_F,
 PlatformKey_G,
 PlatformKey_H,
 PlatformKey_I,
 PlatformKey_J,
 PlatformKey_K,
 PlatformKey_L,
 PlatformKey_M,
 PlatformKey_N,
 PlatformKey_O,
 PlatformKey_P,
 PlatformKey_Q,
 PlatformKey_R,
 PlatformKey_S,
 PlatformKey_T,
 PlatformKey_U,
 PlatformKey_V,
 PlatformKey_W,
 PlatformKey_X,
 PlatformKey_Y,
 PlatformKey_Z,
 
 PlatformKey_Escape,
 PlatformKey_Shift,
 PlatformKey_Ctrl,
 PlatformKey_Alt,
 PlatformKey_Space,
 PlatformKey_Tab,
 PlatformKey_Delete,
 PlatformKey_Backtick,
 
 PlatformKey_LeftMouseButton,
 PlatformKey_RightMouseButton,
 PlatformKey_MiddleMouseButton,
 
 PlatformKey_Count,
};
read_only global string PlatformKeyNames[] =
{
 StrLit("Unknown"),
 
 StrLit("F1"),
 StrLit("F2"),
 StrLit("F3"),
 StrLit("F4"),
 StrLit("F5"),
 StrLit("F6"),
 StrLit("F7"),
 StrLit("F8"),
 StrLit("F9"),
 StrLit("F10"),
 StrLit("F11"),
 StrLit("F12"),
 
 StrLit("A"),
 StrLit("B"),
 StrLit("C"),
 StrLit("D"),
 StrLit("E"),
 StrLit("F"),
 StrLit("G"),
 StrLit("H"),
 StrLit("I"),
 StrLit("J"),
 StrLit("K"),
 StrLit("L"),
 StrLit("M"),
 StrLit("N"),
 StrLit("O"),
 StrLit("P"),
 StrLit("Q"),
 StrLit("R"),
 StrLit("S"),
 StrLit("T"),
 StrLit("U"),
 StrLit("V"),
 StrLit("W"),
 StrLit("X"),
 StrLit("Y"),
 StrLit("Z"),
 
 StrLit("Escape"),
 StrLit("Shift"),
 StrLit("Ctrl"),
 StrLit("Alt"),
 StrLit("Space"),
 StrLit("Tab"),
 StrLit("Delete"),
 StrLit("Backtick"),
 
 StrLit("LeftMouseButton"),
 StrLit("RightMouseButton"),
 StrLit("MiddleMouseButton"),
};
StaticAssert(ArrayCount(PlatformKeyNames) == PlatformKey_Count,
             PlatformKeyNamesDefined);

enum platform_key_modifier
{
 PlatformKeyModifier_Ctrl,
 PlatformKeyModifier_Alt,
 PlatformKeyModifier_Shift,
 
 PlatformKeyModifier_Count,
};
#define NoKeyModifier PlatformKeyModifierFlag_None
#define KeyModifierFlag(Key) (1<<PlatformKeyModifier_##Key)
#define AnyKeyModifier (KeyModifierFlag(Count) - 1)
typedef u32 platform_key_modifier_flags;
enum
{
 PlatformKeyModifierFlag_None,
 PlatformKeyModifierFlag_Alt   = KeyModifierFlag(Alt),
 PlatformKeyModifierFlag_Shift = KeyModifierFlag(Shift),
 PlatformKeyModifierFlag_Ctrl  = KeyModifierFlag(Ctrl),
};
global read_only string PlatformKeyModifierNames[] = {
 StrLitComp("Ctrl"),
 StrLitComp("Alt"),
 StrLitComp("Shift"),
};

struct platform_event
{
 platform_event_type Type;
 platform_key Key;
 platform_key_modifier_flags Modifiers;
 v2 ClipSpaceMouseP;
 f32 ScrollDelta;
 u32 FileCount;
 string *FilePaths; // absolute paths
};
global platform_event NilPlatformEvent;

struct platform_input_output
{
 u32 EventCount;
 platform_event *Events;
 b32 Pressed[PlatformKey_Count];
 f32 dtForFrame;
 v2 ClipSpaceMouseP;
 
 // NOTE(hbr): Set by application
 b32 QuitRequested;
 // NOTE(hbr): If set, then platform event processing will be non-blocking. Otherwise it
 // will be blocking thus EditorUpdateAndRender will be called only after the first user
 // event (MouseMove, KeyPressed, ...) has been received.
 b32 RefreshRequested;
 b32 ProfilingStopped;
};

typedef os_file_dialog_result platform_file_dialog_result;
typedef os_file_dialog_filter platform_file_dialog_filter;
typedef os_file_dialog_filters platform_file_dialog_filters;
typedef os_info platform_info;

#define PLATFORM_ALLOC_VIRTUAL_MEMORY(Name) void *Name(u64 Size, b32 Commit)
typedef PLATFORM_ALLOC_VIRTUAL_MEMORY(platform_alloc_virtual_memory);

#define PLATFORM_DEALLOC_VIRTUAL_MEMORY(Name) void Name(void *Memory, u64 Size)
typedef PLATFORM_DEALLOC_VIRTUAL_MEMORY(platform_dealloc_virtual_memory);

#define PLATFORM_COMMIT_VIRTUAL_MEMORY(Name) void Name(void *Memory, u64 Size)
typedef PLATFORM_COMMIT_VIRTUAL_MEMORY(platform_commit_virtual_memory);

#define PLATFORM_GET_SCRATCH_ARENA(Name) temp_arena Name(arena *Conflict)
typedef PLATFORM_GET_SCRATCH_ARENA(platform_get_scratch_arena);

#define PLATFORM_OPEN_FILE_DIALOG(Name) platform_file_dialog_result Name(arena *Arena, platform_file_dialog_filters Filters)
typedef PLATFORM_OPEN_FILE_DIALOG(platform_open_file_dialog);

#define PLATFORM_SAVE_FILE_DIALOG(Name) platform_file_dialog_result Name(arena *Arena, platform_file_dialog_filters Filters)
typedef PLATFORM_SAVE_FILE_DIALOG(platform_save_file_dialog);

#define PLATFORM_READ_ENTIRE_FILE(Name) string Name(arena *Arena, string FilePath)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_SET_WINDOW_TITLE(Name) void Name(string Title)
typedef PLATFORM_SET_WINDOW_TITLE(platform_set_window_title);

#define PLATFORM_GET_PLATFORM_INFO(Name) platform_info Name(void)
typedef PLATFORM_GET_PLATFORM_INFO(platform_get_platform_info);

#define PLATFORM_TOGGLE_FULLSCREEN(Name) void Name(void)
typedef PLATFORM_TOGGLE_FULLSCREEN(platform_toggle_fullscreen);

typedef void work_queue_func(void *UserData);
#define PLATFORM_WORK_QUEUE_ADD_ENTRY(Name) void Name(work_queue *Queue, work_queue_func *Func, void *UserData)
typedef PLATFORM_WORK_QUEUE_ADD_ENTRY(platform_work_queue_add_entry);

#define PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(Name) void Name(work_queue *Queue)
typedef PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(platform_work_queue_complete_all_work);

#define PLATFORM_WORK_QUEUE_FREE_ENTRY_COUNT(Name) u32 Name(work_queue *Queue)
typedef PLATFORM_WORK_QUEUE_FREE_ENTRY_COUNT(platform_work_queue_free_entry_count);

struct platform_api
{
 platform_alloc_virtual_memory *AllocVirtualMemory;
 platform_dealloc_virtual_memory *DeallocVirtualMemory;
 platform_commit_virtual_memory *CommitVirtualMemory;
 platform_get_scratch_arena *GetScratchArena;
 
 platform_open_file_dialog *OpenFileDialog;
 platform_save_file_dialog *SaveFileDialog;
 platform_read_entire_file *ReadEntireFile;
 platform_set_window_title *SetWindowTitle;
 platform_get_platform_info *GetPlatformInfo;
 platform_toggle_fullscreen *ToggleFullscreen;
 
 platform_work_queue_add_entry *WorkQueueAddEntry;
 platform_work_queue_complete_all_work *WorkQueueCompleteAllWork;
 platform_work_queue_free_entry_count *WorkQueueFreeEntryCount;
 
 imgui_bindings ImGui;
};
extern platform_api Platform;
#define AllocVirtualMemory Platform.AllocVirtualMemory
#define DeallocVirtualMemory Platform.DeallocVirtualMemory
#define CommitVirtualMemory Platform.CommitVirtualMemory
#define TempArena Platform.GetScratchArena

struct editor_memory
{
 arena *PermamentArena;
 
 editor *Editor;
 
 u32 MaxTextureCount;
 u32 MaxBufferCount; 
 renderer_transfer_queue *RendererQueue;
 
 platform_api PlatformAPI;
 
 work_queue *LowPriorityQueue;
 work_queue *HighPriorityQueue;
 
 profiler *Profiler;
};

#include "editor_platform_functions.h"

union editor_function_table
{
 struct {
  editor_update_and_render *UpdateAndRender;
  editor_on_code_reload *OnCodeReload;
 };
 void *Functions[2];
};
global char const *EditorFunctionTableNames[] = {
 "EditorUpdateAndRender",
 "EditorOnCodeReload",
};
StaticAssert(SizeOf(MemberOf(editor_function_table, Functions)) ==
             SizeOf(editor_function_table),
             EditorFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(MemberOf(editor_function_table, Functions)) ==
             ArrayCount(EditorFunctionTableNames),
             EditorFunctionTableNamesDefined);

//- platform input/output
internal f32 UseAndExtractDeltaTime(platform_input_output *Input);
internal f32 ExtractDeltaTimeOnly(platform_input_output *Input);

//- event helpers
internal b32 IsKeyPress(platform_event *Event, platform_key Key, platform_key_modifier_flags Flags = NoKeyModifier);
internal b32 IsKeyRelease(platform_event *Event, platform_key Key);

//- misc
internal string PlatformKeyCombinationToString(arena *Arena, platform_key Key, platform_key_modifier_flags Modifiers);

#endif //EDITOR_PLATFORM_H
