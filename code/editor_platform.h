#ifndef EDITOR_PLATFORM_H
#define EDITOR_PLATFORM_H

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
global char const *PlatformEventTypeNames[] = { "None", "Press", "Release", "MouseMove", "Scroll", "WindowClose", "FilesDrop" };
StaticAssert(ArrayCount(PlatformEventTypeNames) == PlatformEvent_Count, PlatformEventNamesDefined);

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
 
 PlatformKey_LeftMouseButton,
 PlatformKey_RightMouseButton,
 PlatformKey_MiddleMouseButton,
 
 PlatformKey_Count,
};
global char const *PlatformKeyNames[] =
{
 "Unknown",
 
 "F1",
 "F2",
 "F3",
 "F4",
 "F5",
 "F6",
 "F7",
 "F8",
 "F9",
 "F10",
 "F11",
 "F12",
 
 "A",
 "B",
 "C",
 "D",
 "E",
 "F",
 "G",
 "H",
 "I",
 "J",
 "K",
 "L",
 "M",
 "N",
 "O",
 "P",
 "Q",
 "R",
 "S",
 "T",
 "U",
 "V",
 "W",
 "X",
 "Y",
 "Z",
 
 "Escape",
 "Shift",
 "Ctrl",
 "Alt",
 "Space",
 "Tab",
 "Delete",
 
 "LeftMouseButton",
 "RightMouseButton",
 "MiddleMouseButton",
};
StaticAssert(ArrayCount(PlatformKeyNames) == PlatformKey_Count, PlatformKeyNamesDefined);

typedef u32 platform_event_flags;
enum
{
 PlatformEventFlag_Eaten    = (1<<0),
 PlatformEventFlag_Alt      = (1<<1),
 PlatformEventFlag_Shift    = (1<<2),
 PlatformEventFlag_Ctrl     = (1<<3),
};

struct platform_event
{
 platform_event_type Type;
 platform_key Key;
 platform_event_flags Flags;
 v2 ClipSpaceMouseP;
 f32 ScrollDelta;
 
 u32 FileCount;
 string *FilePaths; // absolute paths
};

struct platform_input
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
};

struct platform_file_dialog_result
{
 u32 FileCount;
 string *FilePaths;
};

#define PLATFORM_ALLOC_VIRTUAL_MEMORY(Name) void *Name(u64 Size, b32 Commit)
typedef PLATFORM_ALLOC_VIRTUAL_MEMORY(platform_alloc_virtual_memory);

#define PLATFORM_DEALLOC_VIRTUAL_MEMORY(Name) void Name(void *Memory, u64 Size)
typedef PLATFORM_DEALLOC_VIRTUAL_MEMORY(platform_dealloc_virtual_memory);

#define PLATFORM_GET_SCRATCH_ARENA(Name) temp_arena Name(arena *Conflict)
typedef PLATFORM_GET_SCRATCH_ARENA(platform_get_scratch_arena);

#define PLATFORM_OPEN_FILE_DIALOG(Name) platform_file_dialog_result Name(arena *Arena)
typedef PLATFORM_OPEN_FILE_DIALOG(platform_open_file_dialog);

#define PLATFORM_READ_ENTIRE_FILE(Name) string Name(arena *Arena, string FilePath)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

typedef void work_queue_func(void *UserData);
#define PLATFORM_WORK_QUEUE_ADD_ENTRY(Name) void Name(struct work_queue *Queue, work_queue_func *Func, void *UserData)
typedef PLATFORM_WORK_QUEUE_ADD_ENTRY(platform_work_queue_add_entry);

#define PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(Name) void Name(work_queue *Queue)
typedef PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(platform_work_queue_complete_all_work);

struct platform_api
{
 platform_alloc_virtual_memory *AllocVirtualMemory;
 platform_dealloc_virtual_memory *DeallocVirtualMemory;
 platform_get_scratch_arena *GetScratchArena;
 
 platform_open_file_dialog *OpenFileDialog;
 
 platform_read_entire_file *ReadEntireFile;
 
 platform_work_queue_add_entry *WorkQueueAddEntry;
 platform_work_queue_complete_all_work *WorkQueueCompleteAllWork;
};
extern platform_api Platform;

struct editor_memory
{
 struct arena *PermamentArena;
 
 struct editor *Editor;
 
 u32 MaxTextureCount;
 u32 MaxBufferCount;
 struct renderer_transfer_queue *RendererQueue;
 
 b32 EditorCodeReloaded;
 
 platform_api PlatformAPI;
 
 struct work_queue *LowPriorityQueue;
 struct work_queue *HighPriorityQueue;
};

#define EDITOR_UPDATE_AND_RENDER(Name) void Name(editor_memory *Memory, platform_input *Input, struct render_frame *Frame)
typedef EDITOR_UPDATE_AND_RENDER(editor_update_and_render);

#define EDITOR_GET_IMGUI_BINDINGS(Name) imgui_bindings Name(void)
typedef EDITOR_GET_IMGUI_BINDINGS(editor_get_imgui_bindings);

union editor_function_table
{
 struct {
  editor_update_and_render *UpdateAndRender;
  editor_get_imgui_bindings *GetImGuiBindings;
 };
 void *Functions[2];
};
global char const *EditorFunctionTableNames[] = {
 "EditorUpdateAndRender",
 "EditorGetImGuiBindings",
};
StaticAssert(SizeOf(editor_function_table) == SizeOf(editor_function_table::Functions), EditorFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(editor_function_table::Functions) == ArrayCount(EditorFunctionTableNames), EditorFunctionTableNamesDefined);

#endif //EDITOR_PLATFORM_H
