#include <windows.h>

#include "base_ctx_crack.h"
#include "base_core.h"
#include "base_string.h"

#include "imgui_bindings.h"

#include "editor_memory.h"
#include "editor_thread_ctx.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "os_core.h"

#include "base_hot_reload.h"

#include "win32_editor.h"
#include "win32_editor_renderer.h"
#include "win32_imgui_bindings.h"

#include "base_core.cpp"
#include "base_string.cpp"
#include "base_hot_reload.cpp"
#include "editor_memory.cpp"
#include "editor_thread_ctx.cpp"
#include "os_core.cpp"

#ifndef EDITOR_DLL
# error EDITOR_DLL with path to editor DLL code is not defined
#endif
#ifndef EDITOR_RENDERER_DLL
# error EDITOR_RENDERER_DLL with path to editor renderer DLL code is not defined
#endif

#define EDITOR_DLL_PATH ConvertNameToString(EDITOR_DLL)
#define EDITOR_RENDERER_DLL_PATH ConvertNameToString(EDITOR_RENDERER_DLL)

global u64 GlobalProcCount;
global u64 GlobalPageSize;
global win32_platform_input *GlobalWin32Input;
global u64 GlobalWin32ClockFrequency;
global platform_input *GlobalInput;

IMGUI_INIT(Win32ImGuiInitStub) {}
IMGUI_NEW_FRAME(Win32ImGuiNewFrameStub) {}
IMGUI_RENDER(Win32ImGuiRenderStub) {}
IMGUI_MAYBE_CAPTURE_INPUT(Win32ImGuiMaybeCaptureInputStub) { return {}; }

global imgui_bindings GlobalImGuiBindings = {
 Win32ImGuiInitStub,
 Win32ImGuiNewFrameStub,
 Win32ImGuiRenderStub,
 Win32ImGuiMaybeCaptureInputStub,
};

PLATFORM_ALLOC_VIRTUAL_MEMORY(Win32AllocMemory)
{
 DWORD AllocationType = (MEM_RESERVE | (Commit ? MEM_COMMIT : 0));
 DWORD Protect = (Commit ? PAGE_READWRITE : PAGE_NOACCESS);
 void *Result = VirtualAlloc(0, Size, AllocationType, Protect);
 
 return Result;
}

PLATFORM_COMMIT_VIRTUAL_MEMORY(Win32CommitMemory)
{
 u64 PageSnappedSize = AlignForwardPow2(Size, GlobalPageSize);
 VirtualAlloc(Memory, PageSnappedSize, MEM_COMMIT, PAGE_READWRITE);
}

PLATFORM_DEALLOC_VIRTUAL_MEMORY(Win32DeallocMemory)
{
 VirtualFree(Memory, 0, MEM_RELEASE);
}

PLATFORM_OPEN_FILE_DIALOG(Win32OpenFileDialog)
{
 platform_file_dialog_result Result = {};
 
 local char Buffer[256];
 OPENFILENAME Open = {};
 Open.lStructSize = SizeOf(Open);
 Open.lpstrFile = Buffer;
 Open.lpstrFile[0] = '\0';
 Open.nMaxFile = ArrayCount(Buffer);
 Open.lpstrFilter =
  "All Files (*.*)\0" "*.*\0"
  "PNG (*.png)\0" "*.png\0"
  "JPEG (*.jpg *.jpeg *jpe)\0" "*.jpg;*.jpeg;*.jpe\0"
  "Windows BMP File (*.bmp)\0" "*.bmp\0"
  ;
 Open.nFilterIndex = 1;
 Open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
 
 if (GetOpenFileName(&Open) == TRUE)
 {
  Result.Success = true;
  Result.FilePath = StrFromCStr(Buffer);
 }
 
 return Result;
}

PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
 temp_arena Temp = TempArena(Arena);
 
 string CFilePath = CStrFromStr(Temp.Arena, FilePath);
 
 DWORD DesiredAccess = GENERIC_READ;
 DWORD CreationDisposition = OPEN_EXISTING;
 HANDLE File = CreateFileA(CFilePath.Data, DesiredAccess, FILE_SHARE_READ, 0,
                           CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
 
 u64 FileSize = 0;
 {
  LARGE_INTEGER Win32FileSize = {};
  if (GetFileSizeEx(File, &Win32FileSize))
  {
   FileSize = Win32FileSize.QuadPart;
  }
 }
 
 char *Buffer = PushArrayNonZero(Arena, FileSize, char);
 
 char *At = Buffer;
 u64 Left = FileSize;
 while (Left > 0)
 {
  DWORD ToRead = Cast(DWORD)Min(U32_MAX, Left);
  DWORD Read = 0;
  ReadFile(File, Buffer, ToRead, &Read, 0);
  
  Left -= Read;
  At += Read;
  
  if (Read != ToRead)
  {
   break;
  }
 }
 string Result = MakeStr(Buffer, FileSize - Left);
 
 EndTemp(Temp);
 
 return Result;
}

platform_api Platform = {
 Win32AllocMemory,
 Win32DeallocMemory,
 Win32CommitMemory,
 Win32OpenFileDialog,
 Win32ReadEntireFile,
};

internal platform_event *
Win32PushPlatformEvent(platform_event_type Type)
{
 platform_event *Result = 0;
 if (GlobalWin32Input->EventCount < WIN32_MAX_EVENT_COUNT)
 {
  Result = GlobalWin32Input->Events + GlobalWin32Input->EventCount++;
  Result->Type = Type;
 }
 return Result;
}

internal v2u
Win32GetWindowDim(HWND Window)
{
 v2u WindowDim = {};
 {
  RECT Rect;
  GetClientRect(Window, &Rect);
  WindowDim.X = Rect.right - Rect.left;
  WindowDim.Y = Rect.bottom - Rect.top;
 }
 return WindowDim;
}

internal v2
Win32ScreenToClip(int X, int Y, v2u WindowDim)
{
 v2 Result = V2(2.0f * X / WindowDim.X - 1.0f,
                -(2.0f * Y / WindowDim.Y - 1.0f));
 return Result;
}

internal v2
Win32ClipSpaceMousePFromLParam(HWND Window, LPARAM lParam)
{
 int MouseX = LOWORD(lParam);
 int MouseY = HIWORD(lParam);
 v2u WindowDim = Win32GetWindowDim(Window);
 v2 Result = Win32ScreenToClip(MouseX, MouseY, WindowDim);
 
 return Result;
}

internal platform_key
Win32KeyToPlatformKey(WPARAM Key)
{
 local b32 Calculated = false;
 local platform_key KeyTable[0xFF] = {};
 if (!Calculated)
 {
  KeyTable[VK_F1] = PlatformKey_F1;
  KeyTable[VK_F2] = PlatformKey_F2;
  KeyTable[VK_F3] = PlatformKey_F3;
  KeyTable[VK_F4] = PlatformKey_F4;
  KeyTable[VK_F5] = PlatformKey_F5;
  KeyTable[VK_F6] = PlatformKey_F6;
  KeyTable[VK_F7] = PlatformKey_F7;
  KeyTable[VK_F8] = PlatformKey_F8;
  KeyTable[VK_F9] = PlatformKey_F9;
  KeyTable[VK_F10] = PlatformKey_F10;
  KeyTable[VK_F11] = PlatformKey_F11;
  KeyTable[VK_F12] = PlatformKey_F12;
  
  KeyTable['A'] = PlatformKey_A;
  KeyTable['B'] = PlatformKey_B;
  KeyTable['C'] = PlatformKey_C;
  KeyTable['D'] = PlatformKey_D;
  KeyTable['E'] = PlatformKey_E;
  KeyTable['F'] = PlatformKey_F;
  KeyTable['G'] = PlatformKey_G;
  KeyTable['H'] = PlatformKey_H;
  KeyTable['I'] = PlatformKey_I;
  KeyTable['J'] = PlatformKey_J;
  KeyTable['K'] = PlatformKey_K;
  KeyTable['L'] = PlatformKey_L;
  KeyTable['M'] = PlatformKey_M;
  KeyTable['N'] = PlatformKey_N;
  KeyTable['O'] = PlatformKey_O;
  KeyTable['P'] = PlatformKey_P;
  KeyTable['Q'] = PlatformKey_Q;
  KeyTable['R'] = PlatformKey_R;
  KeyTable['S'] = PlatformKey_S;
  KeyTable['T'] = PlatformKey_T;
  KeyTable['U'] = PlatformKey_U;
  KeyTable['V'] = PlatformKey_V;
  KeyTable['W'] = PlatformKey_W;
  KeyTable['X'] = PlatformKey_X;
  KeyTable['Y'] = PlatformKey_Y;
  KeyTable['Z'] = PlatformKey_Z;
  
  KeyTable[VK_ESCAPE] = PlatformKey_Escape;
  
  KeyTable[VK_LSHIFT] = PlatformKey_LeftShift;
  KeyTable[VK_RSHIFT] = PlatformKey_RightShift;
  KeyTable[VK_LCONTROL] = PlatformKey_LeftCtrl;
  KeyTable[VK_RCONTROL] = PlatformKey_RightCtrl;
  KeyTable[VK_LMENU] = PlatformKey_LeftAlt;
  KeyTable[VK_RMENU] = PlatformKey_RightAlt;
  KeyTable[VK_SPACE] = PlatformKey_Space;
  KeyTable[VK_TAB] = PlatformKey_Tab;
  
  KeyTable[VK_LBUTTON] = PlatformKey_LeftMouseButton;
  KeyTable[VK_RBUTTON] = PlatformKey_RightMouseButton;
  KeyTable[VK_MBUTTON] = PlatformKey_MiddleMouseButton;
  
  Calculated = true;
 }
 
 platform_key Result = KeyTable[Key];
 return Result;
}

internal LRESULT 
Win32WindowProc(HWND Window, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT Result = 0;
 
 imgui_maybe_capture_input_data ImGuiData = {};
 ImGuiData.Window = Window;
 ImGuiData.Msg = Msg;
 ImGuiData.wParam = wParam;
 ImGuiData.lParam = lParam;
 imgui_maybe_capture_input_result ImGuiResult = GlobalImGuiBindings.MaybeCaptureInput(&ImGuiData);
 
 if (ImGuiResult.CapturedInput)
 {
  Result = 1;
 }
 else
 {
  switch (Msg)
  {
   case WM_CLOSE: {
    Win32PushPlatformEvent(PlatformEvent_WindowClose);
   }break;
   
   case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DC = BeginPaint(Window, &Paint);
    EndPaint(Window, &Paint);
   }break;
   
   case WM_LBUTTONDOWN:
   case WM_LBUTTONUP:
   case WM_MBUTTONDOWN:
   case WM_MBUTTONUP:
   case WM_RBUTTONDOWN:
   case WM_RBUTTONUP:
   {
    b32 Pressed = false;
    switch (Msg)
    {
     case WM_LBUTTONDOWN:
     case WM_MBUTTONDOWN:
     case WM_RBUTTONDOWN: {Pressed = true;}break;
    }
    
    platform_key Key = PlatformKey_Unknown;
    switch (Msg)
    {
     case WM_LBUTTONDOWN:
     case WM_LBUTTONUP: {Key = PlatformKey_LeftMouseButton;}break;
     case WM_MBUTTONDOWN:
     case WM_MBUTTONUP: {Key = PlatformKey_MiddleMouseButton;}break;
     case WM_RBUTTONDOWN:
     case WM_RBUTTONUP: {Key = PlatformKey_RightMouseButton;}break;
    }
    
    if (!ImGuiResult.ImGuiWantCaptureMouse)
    {
     platform_event *Event = Win32PushPlatformEvent(Pressed ? PlatformEvent_Press : PlatformEvent_Release);
     if (Event)
     {
      Event->Key = Key;
      Event->ClipSpaceMouseP = Win32ClipSpaceMousePFromLParam(Window, lParam);
     }
    }
    
    GlobalInput->Pressed[Key] = Pressed;
   }break;
   
   case WM_KEYUP:
   case WM_KEYDOWN: {
    b32 Pressed = (Msg == WM_KEYDOWN);
    platform_key Key = Win32KeyToPlatformKey(wParam);
    platform_event *Event = Win32PushPlatformEvent(Pressed ? PlatformEvent_Press : PlatformEvent_Release);
    if (Event)
    {
     Event->Key = Key;
     // TODO(hbr): Flags
    }
    
    GlobalInput->Pressed[Key] = Pressed;
   }break;
   
   case WM_MOUSEMOVE: {
    platform_event *Event = Win32PushPlatformEvent(PlatformEvent_MouseMove);
    if (Event)
    {
     Event->ClipSpaceMouseP = Win32ClipSpaceMousePFromLParam(Window, lParam);
    }
   }break;
   
   case WM_MOUSEWHEEL: {
    platform_event *Event = Win32PushPlatformEvent(PlatformEvent_Scroll);
    if (Event)
    {
     s32 MouseDelta = Cast(s16)HIWORD(wParam);
     Event->ScrollDelta = Cast(f32)(MouseDelta / WHEEL_DELTA);
     Event->ClipSpaceMouseP = Win32ClipSpaceMousePFromLParam(Window, lParam);
    }
   }break;
   
   default: { Result = DefWindowProc(Window, Msg, wParam, lParam); }break;
  }
 }
 
 return Result;
}

internal void
Win32DisplayErrorBox(char const *Msg)
{
 MessageBoxA(0, Msg, 0, MB_OK);
}

internal f32
Win32SecondsElapsed(u64 BeginTSC, u64 EndTSC)
{
 u64 ElapsedTSC = EndTSC - BeginTSC;
 f32 Result = Cast(f32)ElapsedTSC / OS_CPUTimerFreq();
 return Result;
}

#define WIN32_BEGIN_DEBUG_BLOCK(Name) u64 Name##BeginTSC = OS_ReadCPUTimer()
#define WIN32_END_DEBUG_BLOCK(Name) do { \
u64 EndTSC = OS_ReadCPUTimer(); \
f32 Elapsed = Win32SecondsElapsed(Name##BeginTSC, EndTSC); \
OS_PrintDebugF("%s: %fms\n", #Name, Elapsed * 1000.0f); \
} while(0)

int
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd)
{
 b32 InitSuccess = false;
 
 WIN32_BEGIN_DEBUG_BLOCK(FromInit);
 
 //- init platform API and some global variables
 {
  SYSTEM_INFO Info = {};
  GetSystemInfo(&Info);
  GlobalProcCount = Info.dwNumberOfProcessors;
  GlobalPageSize = Info.dwPageSize;
  
  LARGE_INTEGER PerfCounterFrequency;
  QueryPerformanceFrequency(&PerfCounterFrequency);
  GlobalWin32ClockFrequency = PerfCounterFrequency.QuadPart;
 }
 
 InitThreadCtx();
 
 //- create window
 WNDCLASSEXA WindowClass = {};
 { 
  WindowClass.cbSize = SizeOf(WindowClass);
  WindowClass.style = (CS_HREDRAW | CS_VREDRAW);
  WindowClass.lpfnWndProc = Win32WindowProc;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "Win32WindowClass";
  WindowClass.hCursor = LoadCursorA(0, IDC_ARROW);
 }
 
 if (RegisterClassExA(&WindowClass))
 {
  int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
  int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
  // TODO(hbr): restore window sizes
  int WindowWidth =  ScreenWidth * 1/2;
  //int WindowWidth =  ScreenWidth * 9/10;
  int WindowHeight = ScreenHeight * 1/2;
  //int WindowHeight = ScreenHeight * 9/10;
  int WindowX = (ScreenWidth - WindowWidth) / 2;
  int WindowY = (ScreenHeight - WindowHeight) / 2;
  if (WindowWidth == 0 || WindowHeight == 0)
  {
   WindowX = CW_USEDEFAULT;
   WindowY = CW_USEDEFAULT;
   WindowWidth = CW_USEDEFAULT;
   WindowHeight = CW_USEDEFAULT;
  }
  
  HWND Window = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
                                WindowClass.lpszClassName,
                                "Parametric Curves Editor",
                                WS_OVERLAPPEDWINDOW,
                                WindowX, WindowY,
                                WindowWidth, WindowHeight,
                                0, 0, Instance, 0);
  if (Window)
  {
   ShowWindow(Window, nShowCmd);
   InitSuccess = true;
   
   WIN32_END_DEBUG_BLOCK(FromInit);
   
   arena *PermamentArena = AllocArena();
   
   //- init renderer stuff
   WIN32_BEGIN_DEBUG_BLOCK(LoadRendererCode);
   win32_renderer_function_table RendererFunctionTable = {};
   win32_renderer_function_table TempRendererFunctionTable = {};
   hot_reload_library RendererCode = InitHotReloadableLibrary(PermamentArena,
                                                              EDITOR_RENDERER_DLL_PATH,
                                                              Win32RendererFunctionTableNames,
                                                              RendererFunctionTable.Functions,
                                                              TempRendererFunctionTable.Functions,
                                                              ArrayCount(RendererFunctionTable.Functions));
   HotReloadIfRecompiled(&RendererCode);
   WIN32_END_DEBUG_BLOCK(LoadRendererCode);
   
   renderer_memory RendererMemory = {};
   {
    RendererMemory.PlatformAPI = Platform;
    
    platform_renderer_limits *Limits = &RendererMemory.Limits;
    Limits->MaxTextureCount = 256;
    
    texture_transfer_queue *Queue = &RendererMemory.TextureQueue;
    Queue->TransferMemorySize = Megabytes(100);
    Queue->TransferMemory = PushArrayNonZero(PermamentArena, Queue->TransferMemorySize, char);
    
    RendererMemory.MaxCommandCount = 4096;
    RendererMemory.CommandBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxCommandCount, render_command);
   }
   
   HDC WindowDC = GetDC(Window);
   arena *RendererArena = AllocArena();
   
   WIN32_BEGIN_DEBUG_BLOCK(RendererInit);
   renderer *Renderer = RendererFunctionTable.Init(RendererArena, &RendererMemory, Window, WindowDC);
   WIN32_END_DEBUG_BLOCK(RendererInit);
   
   //- init editor stuff
   WIN32_BEGIN_DEBUG_BLOCK(LoadEditorCode);
   editor_function_table EditorFunctionTable = {};
   editor_function_table TempEditorFunctionTable = {};
   hot_reload_library EditorCode = InitHotReloadableLibrary(PermamentArena,
                                                            EDITOR_DLL_PATH,
                                                            EditorFunctionTableNames,
                                                            EditorFunctionTable.Functions,
                                                            TempEditorFunctionTable.Functions,
                                                            ArrayCount(EditorFunctionTable.Functions));
   WIN32_END_DEBUG_BLOCK(LoadEditorCode);
   
   editor_memory EditorMemory = {};
   EditorMemory.PermamentArena = PermamentArena;
   EditorMemory.MaxTextureCount = RendererMemory.Limits.MaxTextureCount;
   EditorMemory.TextureQueue = &RendererMemory.TextureQueue;
   EditorMemory.PlatformAPI = Platform;
   
   u64 LastTSC = OS_ReadCPUTimer();
   
   platform_input Input = {};
   GlobalInput = &Input;
   
   //- main loop
   b32 Running = true;
   while (Running)
   {
    //- hot reload
    EditorMemory.EditorCodeReloaded = HotReloadIfRecompiled(&EditorCode);
    if (EditorMemory.EditorCodeReloaded)
    {
     imgui_bindings Bindings = EditorFunctionTable.GetImGuiBindings();
     GlobalImGuiBindings = Bindings;
     RendererMemory.ImGuiBindings = Bindings;
     
     imgui_init_data Init = {};
     Init.Window = Window;
     Bindings.Init(&Init);
    }
    
    RendererMemory.RendererCodeReloaded = HotReloadIfRecompiled(&RendererCode);
    
    //- process input events
    win32_platform_input Win32Input = {};
    GlobalWin32Input = &Win32Input;
    v2u WindowDim = {};
    {
     MSG Msg;
     while (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE))
     {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
     }
     if (Msg.message == WM_QUIT)
     {
      Win32PushPlatformEvent(PlatformEvent_WindowClose);
     }
     
     Input.EventCount = Win32Input.EventCount;
     Input.Events = Win32Input.Events;
     
     // NOTE(hbr): get window dim after processing events
     WindowDim = Win32GetWindowDim(Window);
     
     POINT CursorP;
     GetCursorPos(&CursorP);
     ScreenToClient(Window, &CursorP);
     Input.ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
    }
    
    //- update and render
    // TODO(hbr): Temporary
    local b32 FirstFrame = true;
    
    WIN32_BEGIN_DEBUG_BLOCK(RendererBeginFrame);
    render_frame *Frame = RendererFunctionTable.BeginFrame(Renderer, &RendererMemory, WindowDim);
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(RendererBeginFrame);}
    
    {
     u64 NowTSC = OS_ReadCPUTimer();
     Input.dtForFrame = Win32SecondsElapsed(LastTSC, NowTSC);
     LastTSC = NowTSC;
    }
    
    WIN32_BEGIN_DEBUG_BLOCK(EditorUpdateAndRender);
    EditorFunctionTable.UpdateAndRender(&EditorMemory, &Input, Frame);
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(EditorUpdateAndRender);}
    
    WIN32_BEGIN_DEBUG_BLOCK(RendererEndFrame);
    RendererFunctionTable.EndFrame(Renderer, &RendererMemory, Frame);
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(RendererEndFrame);}
    
    
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(FromInit);}
    FirstFrame = false;
    
    if (Input.QuitRequested)
    {
     Running = false;
    }
   }
  }
 }
 
 if (!InitSuccess)
 {
  Win32DisplayErrorBox("Failed to initialize window");
 }
 
 return 0;
}