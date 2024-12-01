#include <windows.h>

#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "base/base_hot_reload.h"

#include "editor_memory.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_thread_ctx.h"
#include "editor_work_queue.h"

#include "win32/win32_editor.h"
#include "win32/win32_editor_renderer.h"
#include "win32/win32_editor_imgui_bindings.h"
#include "win32/win32_shared.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_hot_reload.cpp"
#include "base/base_os.cpp"

#include "editor_memory.cpp"
#include "editor_thread_ctx.cpp"
#include "editor_work_queue.cpp"

#ifndef EDITOR_DLL
# error EDITOR_DLL with path to editor DLL code is not defined
#endif
#ifndef EDITOR_RENDERER_DLL
# error EDITOR_RENDERER_DLL with path to editor renderer DLL code is not defined
#endif
#define EDITOR_DLL_PATH ConvertNameToString(EDITOR_DLL)
#define EDITOR_RENDERER_DLL_PATH ConvertNameToString(EDITOR_RENDERER_DLL)

global win32_platform_input GlobalWin32Input;
global platform_input *GlobalInput;
global arena *InputArena;

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
 void *Memory = OS_Reserve(Size);
 OS_Commit(Memory, Size);
 return Memory;
}

PLATFORM_DEALLOC_VIRTUAL_MEMORY(Win32DeallocMemory)
{
 OS_Release(Memory, Size);
}

PLATFORM_GET_SCRATCH_ARENA(Win32GetScratchArena)
{
 temp_arena Result = ThreadCtxTempArena(Conflict);
 return Result;
}

PLATFORM_OPEN_FILE_DIALOG(Win32OpenFileDialog)
{
 platform_file_dialog_result Result = {};
 
 local char Buffer[2048];
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
 Open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
 
 if (GetOpenFileName(&Open) == TRUE)
 {
  char *At = Buffer;
  u32 FileCount = 0;
  while (At[0])
  {
   At += CStrLen(At) + 1;
   ++FileCount;
  }
  
  At = Buffer;
  string BasePath = StrFromCStr(At);
  At += BasePath.Count + 1;
  
  string *FilePaths = PushArrayNonZero(Arena, FileCount, string);
  if (FileCount == 1)
  {
   FilePaths[0] = BasePath;
  }
  else
  {
   Assert(FileCount > 0);
   --FileCount;
   for (u32 FileIndex = 0;
        FileIndex < FileCount;
        ++FileIndex)
   {
    string FileName = StrFromCStr(At);
    At += FileName.Count + 1;
    FilePaths[FileIndex] = PathConcat(Arena, BasePath, FileName);
   }
  }
  
  Result.FileCount = FileCount;
  Result.FilePaths = FilePaths;
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
 Win32GetScratchArena,
 Win32OpenFileDialog,
 Win32ReadEntireFile,
};

internal platform_event_flags
Win32GetEventFlags(void)
{
 platform_event_flags Flags = 0;
 if (GetKeyState(VK_CONTROL) & 0x8000)
 {
  Flags |= PlatformEventFlag_Ctrl;
 }
 if (GetKeyState(VK_SHIFT) & 0x8000)
 {
  Flags |= PlatformEventFlag_Shift;
 }
 if (GetKeyState(VK_MENU) & 0x8000)
 {
  Flags |= PlatformEventFlag_Alt;
 }
 
 return Flags;
}

internal platform_event *
Win32PushPlatformEvent(platform_event_type Type)
{
 platform_event *Result = 0;
 if (GlobalWin32Input.EventCount < WIN32_MAX_EVENT_COUNT)
 {
  Result = GlobalWin32Input.Events + GlobalWin32Input.EventCount++;
  Result->Type = Type;
  Result->Flags = Win32GetEventFlags();
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
  
  KeyTable[VK_SHIFT] = PlatformKey_Shift;
  KeyTable[VK_CONTROL] = PlatformKey_Ctrl;
  KeyTable[VK_MENU] = PlatformKey_Alt;
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
   
   case WM_SYSKEYUP:
   case WM_SYSKEYDOWN: {
    // NOTE(hbr): took it from Ryan Fleury's Radbg, didn't really dive into it.
    // I know that it makes Alt+F4 work. Otherwise we would capture it entirely.
    if(wParam != VK_MENU && (wParam < VK_F1 || VK_F24 < wParam || wParam == VK_F4))
    {
     Result = DefWindowProc(Window, Msg, wParam, lParam);
    }
   } // fallthrough
   case WM_KEYUP:
   case WM_KEYDOWN: {
    b32 WasDown = ((lParam & 1<<30) != 0);
    b32 IsDown =  ((lParam & 1<<31) == 0);
    // NOTE(hbr): dont't repeat PlatformEvent_Press msgs when holding a key
    if (WasDown != IsDown)
    {
     b32 Pressed = (Msg == WM_KEYDOWN);
     platform_key Key = Win32KeyToPlatformKey(wParam);
     platform_event *Event = Win32PushPlatformEvent(Pressed ? PlatformEvent_Press : PlatformEvent_Release);
     if (Event)
     {
      Event->Key = Key;
      if (Key == PlatformKey_Ctrl && (Event->Flags & PlatformEventFlag_Ctrl)) Event->Flags &= ~PlatformEventFlag_Ctrl;
      if (Key == PlatformKey_Shift && (Event->Flags & PlatformEventFlag_Shift)) Event->Flags &= ~PlatformEventFlag_Shift;
      if (Key == PlatformKey_Alt && (Event->Flags & PlatformEventFlag_Alt)) Event->Flags &= ~PlatformEventFlag_Alt;
     }
     
     GlobalInput->Pressed[Key] = Pressed;
    }
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
     i32 MouseDelta = Cast(i16)HIWORD(wParam);
     Event->ScrollDelta = Cast(f32)(MouseDelta / WHEEL_DELTA);
     Event->ClipSpaceMouseP = Win32ClipSpaceMousePFromLParam(Window, lParam);
    }
   }break;
   
   case WM_DROPFILES: {
    platform_event *Event = Win32PushPlatformEvent(PlatformEvent_FilesDrop);
    if (Event)
    {
     HDROP DropHandle = Cast(HDROP)wParam;
     
     // NOTE(hbr): 0xFFFFFFFF queries file count
     UINT FileCount = DragQueryFileA(DropHandle, 0xFFFFFFFF, 0, 0);
     
     POINT CursorP;
     BOOL ClientAreaOfWindow = DragQueryPoint(DropHandle, &CursorP);
     v2u WindowDim = Win32GetWindowDim(Window);
     Event->ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
     string *Files = PushArray(InputArena, FileCount, string);
     for (UINT FileIndex = 0;
          FileIndex < FileCount;
          ++FileIndex)
     {
      UINT RequiredCount = DragQueryFileA(DropHandle, FileIndex, 0, 0);
      string File = PushString(InputArena, RequiredCount + 1);
      UINT CopiedCount = DragQueryFileA(DropHandle, FileIndex, File.Data, Cast(UINT)File.Count);
      Assert(RequiredCount == CopiedCount);
      Files[FileIndex] = File;
     }
     
     Event->FilePaths = Files;
     Event->FileCount = FileCount;
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

internal void
Win32HotReloadTask(void *UserData)
{
 win32_hot_reload_task *Task = Cast(win32_hot_reload_task *)UserData;
 Task->CodeReloaded = HotReloadIfRecompiled(Task->Code);
}

int
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd)
{
 b32 InitSuccess = false;
 WIN32_BEGIN_DEBUG_BLOCK(FromBegin);
 ThreadCtxAlloc();
 
 //- create window
 WIN32_BEGIN_DEBUG_BLOCK(Win32WindowInit);
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
  //int WindowWidth =  ScreenWidth;
  //int WindowWidth =  ScreenWidth * 9/10;
  
  int WindowHeight = ScreenHeight * 1/2;
  //int WindowHeight = ScreenHeight;
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
   
   WIN32_END_DEBUG_BLOCK(Win32WindowInit);
   
   WIN32_BEGIN_DEBUG_BLOCK(LoopInit);
   arena *PermamentArena = AllocArena();
   
   //- init renderer stuff
   renderer *Renderer = 0;
   HDC WindowDC = GetDC(Window);
   arena *RendererArena = AllocArena();
   
   renderer_memory RendererMemory = {};
   {
    RendererMemory.PlatformAPI = Platform;
    
    platform_renderer_limits *Limits = &RendererMemory.Limits;
    Limits->MaxTextureCount = 256;
    
    renderer_transfer_queue *Queue = &RendererMemory.RendererQueue;
    Queue->TransferMemorySize = Megabytes(100);
    Queue->TransferMemory = PushArrayNonZero(PermamentArena, Queue->TransferMemorySize, char);
    
    RendererMemory.MaxCommandCount = 4096;
    RendererMemory.CommandBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxCommandCount, render_command);
    
    // TODO(hbr): Tweak these parameters
    RendererMemory.MaxCircleCount = 4096;
    RendererMemory.CircleBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxCircleCount, render_circle);
   }
   
   win32_renderer_function_table RendererFunctionTable = {};
   win32_renderer_function_table TempRendererFunctionTable = {};
   hot_reload_library RendererCode = InitHotReloadableLibrary(PermamentArena,
                                                              EDITOR_RENDERER_DLL_PATH,
                                                              Win32RendererFunctionTableNames,
                                                              RendererFunctionTable.Functions,
                                                              TempRendererFunctionTable.Functions,
                                                              ArrayCount(RendererFunctionTable.Functions));
   
   //- init editor stuff
   editor_function_table EditorFunctionTable = {};
   editor_function_table TempEditorFunctionTable = {};
   hot_reload_library EditorCode = InitHotReloadableLibrary(PermamentArena,
                                                            EDITOR_DLL_PATH,
                                                            EditorFunctionTableNames,
                                                            EditorFunctionTable.Functions,
                                                            TempEditorFunctionTable.Functions,
                                                            ArrayCount(EditorFunctionTable.Functions));
   
   editor_memory EditorMemory = {};
   EditorMemory.PermamentArena = PermamentArena;
   EditorMemory.MaxTextureCount = RendererMemory.Limits.MaxTextureCount;
   EditorMemory.RendererQueue = &RendererMemory.RendererQueue;
   EditorMemory.PlatformAPI = Platform;
   
   u64 LastTSC = OS_ReadCPUTimer();
   
   platform_input Input = {};
   GlobalInput = &Input;
   InputArena = AllocArena();
   
   work_queue _Queue = {};
   work_queue *Queue = &_Queue;
   {
    u32 ThreadCount = Cast(u32)OS_ProcCount() - 1;
    Assert(ThreadCount > 0);
    WorkQueueInit(Queue, ThreadCount);
   }
   
   WIN32_END_DEBUG_BLOCK(LoopInit);
   
   //- main loop
   b32 Running = true;
   b32 RefreshRequested = true;
   // TODO(hbr): Temporary
   b32 FirstFrame = true;
   while (Running)
   {
    //- hot reload
    WIN32_BEGIN_DEBUG_BLOCK(HotReload);
    {
     win32_hot_reload_task RendererTask = {};
     RendererTask.Code = &RendererCode;
     
     win32_hot_reload_task EditorTask = {};
     EditorTask.Code = &EditorCode;
     
#if 1
     // TODO(hbr): Unfortunately multithreaded path doesn't improve performance. It seems as
     // OS_LoadLibrary (LoadLibrary from Win32 API) grabs mutex or something, making the calls
     // serial in practice.
     WorkQueueAddEntry(Queue, Win32HotReloadTask, &RendererTask);
     WorkQueueAddEntry(Queue, Win32HotReloadTask, &EditorTask);
     
     WorkQueueCompleteAllWork(Queue);
#else
     Win32HotReloadTask(&RendererTask);
     Win32HotReloadTask(&EditorTask);
#endif
     
     RendererMemory.RendererCodeReloaded = RendererTask.CodeReloaded;
     EditorMemory.EditorCodeReloaded = EditorTask.CodeReloaded;
    }
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(HotReload);}
    
    WIN32_BEGIN_DEBUG_BLOCK(RendererInit);
    if (RendererMemory.RendererCodeReloaded && !Renderer)
    {
     Renderer = RendererFunctionTable.Init(RendererArena, &RendererMemory, Window, WindowDC);
    }
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(RendererInit);}
    
    WIN32_BEGIN_DEBUG_BLOCK(ImGuiInit);
    if (EditorMemory.EditorCodeReloaded)
    {
     imgui_bindings Bindings = EditorFunctionTable.GetImGuiBindings();
     GlobalImGuiBindings = Bindings;
     RendererMemory.ImGuiBindings = Bindings;
     
     imgui_init_data Init = {};
     Init.Window = Window;
     Bindings.Init(&Init);
    }
    if (FirstFrame) {WIN32_BEGIN_DEBUG_BLOCK(ImGuiInit);}
    
    //- process input events
    WIN32_BEGIN_DEBUG_BLOCK(InputHandling);
    v2u WindowDim = {};
    {
     GlobalWin32Input = {};
     ClearArena(InputArena);
     
     MSG Msg;
     BOOL MsgReceived = 0;
     if (!RefreshRequested)
     {
      MsgReceived = GetMessage(&Msg, 0, 0, 0);
     }
     do
     {
      if (MsgReceived)
      {
       TranslateMessage(&Msg);
       DispatchMessage(&Msg);
       if (Msg.message == WM_QUIT)
       {
        Win32PushPlatformEvent(PlatformEvent_WindowClose);
       }
      }
     } while((MsgReceived = PeekMessage(&Msg, 0, 0, 0, PM_REMOVE)));
     
     Input.EventCount = GlobalWin32Input.EventCount;
     Input.Events = GlobalWin32Input.Events;
     
     // NOTE(hbr): get window dim after processing events
     WindowDim = Win32GetWindowDim(Window);
     
     POINT CursorP;
     GetCursorPos(&CursorP);
     ScreenToClient(Window, &CursorP);
     Input.ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
     for (u32 EventIndex = 0;
          EventIndex < Input.EventCount;
          ++EventIndex)
     {
      platform_event *Event = Input.Events + EventIndex;
      char const *Name = PlatformEventTypeNames[Event->Type];
      OS_PrintDebugF("%s\n", Name);
     }
     
    }
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(InputHandling);}
    
    //- update and render
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
    RefreshRequested = Input.RefreshRequested;
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(EditorUpdateAndRender);}
    
    WIN32_BEGIN_DEBUG_BLOCK(RendererEndFrame);
    RendererFunctionTable.EndFrame(Renderer, &RendererMemory, Frame);
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(RendererEndFrame);}
    
    if (FirstFrame) {WIN32_END_DEBUG_BLOCK(FromBegin);}
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