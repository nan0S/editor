#include <windows.h>

#include "editor_ctx_crack.h"
#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
#include "imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "win32_editor.h"
#include "win32_editor_renderer.h"
#include "win32_imgui_bindings.h"

#include "editor_memory.cpp"
#include "editor_string.cpp"

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
 u64 PageSnappedSize = AlignPow2(Size, GlobalPageSize);
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

internal LARGE_INTEGER
Win32GetClock()
{
 LARGE_INTEGER Result;
 QueryPerformanceCounter(&Result);
 return Result;
}

internal f32
Win32SecondsElapsed(LARGE_INTEGER Begin, LARGE_INTEGER End)
{
 f32 Result = Cast(f32)(End.QuadPart - Begin.QuadPart) / GlobalWin32ClockFrequency;
 return Result;
}

int
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd)
{
 b32 InitSuccess = false;
 
 //- init platform API and some global variables
 SYSTEM_INFO Info = {};
 GetSystemInfo(&Info);
 GlobalProcCount = Info.dwNumberOfProcessors;
 GlobalPageSize = Info.dwPageSize;
 
 LARGE_INTEGER PerfCounterFrequency;
 QueryPerformanceFrequency(&PerfCounterFrequency);
 GlobalWin32ClockFrequency = PerfCounterFrequency.QuadPart;
 
 InitThreadCtx();
#if 0
 ImGui::CreateContext();
#endif
 // TODO(hbr): Maybe set ImGui IO flags
 
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
  int WindowWidth =  ScreenWidth * 9/10;
  int WindowHeight = ScreenHeight * 9/10;
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
   
   //- load editor code
   // TODO(hbr): Don't hardcore "debug" suffix here as well
   HMODULE EditorCodeLibrary = LoadLibrary("editor_debug.dll");
   editor_update_and_render *EditorUpdateAndRender = 0;
   editor_get_imgui_bindings *EditorGetImGuiBindings = 0;
   if (EditorCodeLibrary)
   {
    // TODO(hbr): And this function name - don't hardcode it
    EditorUpdateAndRender = Cast(editor_update_and_render *)GetProcAddress(EditorCodeLibrary, "EditorUpdateAndRender");
    EditorGetImGuiBindings = Cast(editor_get_imgui_bindings *)GetProcAddress(EditorCodeLibrary, "EditorGetImGuiBindings");
   }
   GlobalImGuiBindings = EditorGetImGuiBindings();
   
   //- load renderer code
   // TODO(hbr): Don't hardcode the fact that it has to be suffixed with _debug - either remove suffixes from the
   // build system or #if BUILD_DEBUG or maybe input these things through build system
   HMODULE RendererCodeLibrary = LoadLibrary("win32_editor_renderer_opengl_debug.dll");
   win32_renderer_init *Win32RendererInit = 0;
   renderer_begin_frame *Win32RendererBeginFrame = 0;
   renderer_end_frame *Win32RendererEndFrame = 0;
   if (RendererCodeLibrary)
   {
    // TODO(hbr): Don't hardcode these strings here, instead define them in one place
    Win32RendererInit = Cast(win32_renderer_init *)GetProcAddress(RendererCodeLibrary, "Win32RendererInit");
    Win32RendererBeginFrame = Cast(renderer_begin_frame *)GetProcAddress(RendererCodeLibrary, "Win32RendererBeginFrame");
    Win32RendererEndFrame = Cast(renderer_end_frame *)GetProcAddress(RendererCodeLibrary, "Win32RendererEndFrame");
   }
   
   //- init renderer
   platform_renderer_limits Limits = {};
   Limits.MaxCommandCount = 4096;
   Limits.MaxTextureQueueMemorySize = Megabytes(100);
   Limits.MaxTextureCount = 256;
   arena *RendererArena = AllocArena();
   HDC WindowDC = GetDC(Window);
   platform_renderer *Renderer = Win32RendererInit(RendererArena, &Limits, Platform, GlobalImGuiBindings, Window, WindowDC);
   
   editor_memory Memory = {};
   Memory.PermamentArena = AllocArena();
   Memory.MaxTextureCount = Limits.MaxTextureCount;
   Memory.TextureQueue = &Renderer->TextureQueue;
   Memory.PlatformAPI = Platform;
   
   LARGE_INTEGER LastClock = Win32GetClock();
   
   platform_input Input = {};
   GlobalInput = &Input;
   
   //- main loop
   b32 Running = true;
   while (Running)
   {
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
    
    render_frame *Frame = Win32RendererBeginFrame(Renderer, WindowDim);
    
    LARGE_INTEGER EndClock = Win32GetClock();
    Input.dtForFrame = Win32SecondsElapsed(LastClock, EndClock);
    EditorUpdateAndRender(&Memory, &Input, Frame);
    
    Win32RendererEndFrame(Renderer, Frame);
    
    if (Input.QuitRequested)
    {
     Running = false;
    }
    
    LastClock = EndClock;
   }
  }
 }
 
 if (!InitSuccess)
 {
  Win32DisplayErrorBox("Failed to initialize window");
 }
 
 return 0;
}