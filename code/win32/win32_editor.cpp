#include <windows.h>

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "base/base_arena.h"
#include "base/base_thread_ctx.h"
#include "base/base_hot_reload.h"

#include "editor_profiler.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_work_queue.h"

#include "win32/win32_editor.h"
#include "win32/win32_editor_renderer.h"
#include "win32/win32_editor_imgui_bindings.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_hot_reload.cpp"

#include "editor_work_queue.cpp"
#include "editor_profiler.cpp"

#include "platform_shared.h"
#include "platform_shared.cpp"

global win32_platform_input GlobalWin32Input;
global platform_input *GlobalInput;
global arena *InputArena;

#define WIN32_KEY_COUNT 0xFF
global platform_key Win32KeyTable[WIN32_KEY_COUNT];

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

internal void *
Win32AllocMemory(u64 Size, b32 Commit)
{
 void *Memory = OS_Reserve(Size, Commit);
 return Memory;
}

internal void
Win32DeallocMemory(void *Memory, u64 Size)
{
 OS_Release(Memory, Size);
}

internal void
Win32CommitMemory(void *Memory, u64 Size)
{
 OS_Commit(Memory, Size);
}

internal temp_arena
Win32GetScratchArena(arena *Conflict)
{
 temp_arena Result = ThreadCtxGetScratch(Conflict);
 return Result;
}

internal platform_file_dialog_result
Win32OpenFileDialog(arena *Arena)
{
 platform_file_dialog_result Result = {};
 
 char Buffer[2048];
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
  string BasePath = StrCopy(Arena, StrFromCStr(At));
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
    // NOTE(hbr): No need to StrCopy here because we PathConcat anyway
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

internal string
Win32ReadEntireFile(arena *Arena, string FilePath)
{
 // TODO(hbr): probably just use OS_ReadEntireFile
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
 Win32GetScratchArena,
 Win32OpenFileDialog,
 Win32ReadEntireFile,
 WorkQueueAddEntry,
 WorkQueueCompleteAllWork,
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
Win32ScreenToClip(i32 X, i32 Y, v2u WindowDim)
{
 v2 Result = V2(2.0f * X / WindowDim.X - 1.0f,
                -(2.0f * Y / WindowDim.Y - 1.0f));
 return Result;
}

internal v2
Win32ClipSpaceMousePFromLParam(HWND Window, LPARAM lParam)
{
 i32 MouseX = Cast(i16)(lParam >> 0  & 0xFFFF);
 i32 MouseY = Cast(i16)(lParam >> 16 & 0xFFFF);
 
 v2u WindowDim = Win32GetWindowDim(Window);
 v2 Result = Win32ScreenToClip(MouseX, MouseY, WindowDim);
 
 return Result;
}

internal LRESULT 
Win32WindowProc(HWND Window, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT Result = 0;
 
 win32_imgui_maybe_capture_input_data ImGuiData = {};
 ImGuiData.Window = Window;
 ImGuiData.Msg = Msg;
 ImGuiData.wParam = wParam;
 ImGuiData.lParam = lParam;
 imgui_maybe_capture_input_result ImGuiResult = GlobalImGuiBindings.MaybeCaptureInput(Cast(imgui_maybe_capture_input_data *)&ImGuiData);
 
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
    if (!ImGuiResult.ImGuiWantCaptureKeyboard)
    {
     b32 WasDown = ((lParam & 1<<30) != 0);
     b32 IsDown =  ((lParam & 1<<31) == 0);
     // NOTE(hbr): dont't repeat PlatformEvent_Press msgs when holding a key
     if (WasDown != IsDown)
     {
      b32 Pressed = (Msg == WM_KEYDOWN);
      platform_key Key = Win32KeyTable[wParam];
      platform_event *Event = Win32PushPlatformEvent(Pressed ? PlatformEvent_Press : PlatformEvent_Release);
      if (Event)
      {
       Event->Key = Key;
       if (Key == PlatformKey_Ctrl && (Event->Flags & PlatformEventFlag_Ctrl)) Event->Flags &= ~PlatformEventFlag_Ctrl;
       if (Key == PlatformKey_Shift && (Event->Flags & PlatformEventFlag_Shift)) Event->Flags &= ~PlatformEventFlag_Shift;
       if (Key == PlatformKey_Alt && (Event->Flags & PlatformEventFlag_Alt)) Event->Flags &= ~PlatformEventFlag_Alt;
      }
     }
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
    if (!ImGuiResult.ImGuiWantCaptureMouse)
    {
     platform_event *Event = Win32PushPlatformEvent(PlatformEvent_Scroll);
     if (Event)
     {
      i32 MouseDelta = Cast(i16)HIWORD(wParam);
      Event->ScrollDelta = Cast(f32)(MouseDelta / WHEEL_DELTA);
      Event->ClipSpaceMouseP = Win32ClipSpaceMousePFromLParam(Window, lParam);
     }
    }
   }break;
   
   case WM_DROPFILES: {
    platform_event *Event = Win32PushPlatformEvent(PlatformEvent_FilesDrop);
    if (Event)
    {
     HDROP DropHandle = Cast(HDROP)wParam;
     
     POINT CursorP;
     BOOL ClientAreaOfWindow = DragQueryPoint(DropHandle, &CursorP);
     v2u WindowDim = Win32GetWindowDim(Window);
     Event->ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
     // NOTE(hbr): 0xFFFFFFFF queries file count
     UINT FileCount = DragQueryFileA(DropHandle, 0xFFFFFFFF, 0, 0);
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
     
     DragFinish(DropHandle);
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

internal void
EntryPoint(void)
{
 int ArgCount = __argc;
 char **Args = __argv;
 HINSTANCE Instance = GetModuleHandle(0);
 b32 InitSuccess = false;
 
 OS_Init(ArgCount, Args);
 ThreadCtxInit();
 
 profiler *Profiler = Cast(profiler *)OS_Reserve(SizeOf(profiler), true);
 ProfilerInit(Profiler);
 ProfilerEquip(Profiler);
 
 arena *PermamentArena = AllocArena(Gigabytes(64));
 
 //- key mappings
 {
  Win32KeyTable[VK_F1] = PlatformKey_F1;
  Win32KeyTable[VK_F2] = PlatformKey_F2;
  Win32KeyTable[VK_F3] = PlatformKey_F3;
  Win32KeyTable[VK_F4] = PlatformKey_F4;
  Win32KeyTable[VK_F5] = PlatformKey_F5;
  Win32KeyTable[VK_F6] = PlatformKey_F6;
  Win32KeyTable[VK_F7] = PlatformKey_F7;
  Win32KeyTable[VK_F8] = PlatformKey_F8;
  Win32KeyTable[VK_F9] = PlatformKey_F9;
  Win32KeyTable[VK_F10] = PlatformKey_F10;
  Win32KeyTable[VK_F11] = PlatformKey_F11;
  Win32KeyTable[VK_F12] = PlatformKey_F12;
  
  Win32KeyTable['A'] = PlatformKey_A;
  Win32KeyTable['B'] = PlatformKey_B;
  Win32KeyTable['C'] = PlatformKey_C;
  Win32KeyTable['D'] = PlatformKey_D;
  Win32KeyTable['E'] = PlatformKey_E;
  Win32KeyTable['F'] = PlatformKey_F;
  Win32KeyTable['G'] = PlatformKey_G;
  Win32KeyTable['H'] = PlatformKey_H;
  Win32KeyTable['I'] = PlatformKey_I;
  Win32KeyTable['J'] = PlatformKey_J;
  Win32KeyTable['K'] = PlatformKey_K;
  Win32KeyTable['L'] = PlatformKey_L;
  Win32KeyTable['M'] = PlatformKey_M;
  Win32KeyTable['N'] = PlatformKey_N;
  Win32KeyTable['O'] = PlatformKey_O;
  Win32KeyTable['P'] = PlatformKey_P;
  Win32KeyTable['Q'] = PlatformKey_Q;
  Win32KeyTable['R'] = PlatformKey_R;
  Win32KeyTable['S'] = PlatformKey_S;
  Win32KeyTable['T'] = PlatformKey_T;
  Win32KeyTable['U'] = PlatformKey_U;
  Win32KeyTable['V'] = PlatformKey_V;
  Win32KeyTable['W'] = PlatformKey_W;
  Win32KeyTable['X'] = PlatformKey_X;
  Win32KeyTable['Y'] = PlatformKey_Y;
  Win32KeyTable['Z'] = PlatformKey_Z;
  
  Win32KeyTable[VK_ESCAPE] = PlatformKey_Escape;
  
  Win32KeyTable[VK_SHIFT] = PlatformKey_Shift;
  Win32KeyTable[VK_CONTROL] = PlatformKey_Ctrl;
  Win32KeyTable[VK_MENU] = PlatformKey_Alt;
  Win32KeyTable[VK_SPACE] = PlatformKey_Space;
  Win32KeyTable[VK_TAB] = PlatformKey_Tab;
  Win32KeyTable[VK_DELETE] = PlatformKey_Delete;
  
  Win32KeyTable[VK_LBUTTON] = PlatformKey_LeftMouseButton;
  Win32KeyTable[VK_RBUTTON] = PlatformKey_RightMouseButton;
  Win32KeyTable[VK_MBUTTON] = PlatformKey_MiddleMouseButton;
 }
 
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
   ShowWindow(Window, SW_SHOWDEFAULT);
   InitSuccess = true;
   
   //- init renderer stuff
   renderer *Renderer = 0;
   HDC WindowDC = GetDC(Window);
   
   renderer_memory RendererMemory = Platform_MakeRendererMemory(PermamentArena, Profiler);
   
   win32_renderer_function_table RendererFunctions = {};
   win32_renderer_function_table TempRendererFunctions = {};
   string RendererDLL = OS_ExecutableRelativeToFullPath(PermamentArena, StrFromCStr(EDITOR_RENDERER_DLL_FILE_NAME));
   hot_reload_library RendererCode = MakeHotReloadableLibrary(PermamentArena,
                                                              RendererDLL,
                                                              Win32RendererFunctionTableNames,
                                                              RendererFunctions.Functions,
                                                              TempRendererFunctions.Functions,
                                                              ArrayCount(RendererFunctions.Functions));
   
   work_queue LowPriorityQueue = {};
   work_queue HighPriorityQueue = {};
   Platform_MakeWorkQueues(&LowPriorityQueue, &HighPriorityQueue);
   
   //- init editor stuff
   editor_function_table EditorFunctions = {};
   editor_function_table TempEditorFunctions = {};
   string EditorDLL = OS_ExecutableRelativeToFullPath(PermamentArena, StrFromCStr(EDITOR_DLL_FILE_NAME));
   hot_reload_library EditorCode = MakeHotReloadableLibrary(PermamentArena,
                                                            EditorDLL,
                                                            EditorFunctionTableNames,
                                                            EditorFunctions.Functions,
                                                            TempEditorFunctions.Functions,
                                                            ArrayCount(EditorFunctions.Functions));
   
   editor_memory EditorMemory = Platform_MakeEditorMemory(PermamentArena, &RendererMemory,
                                                          &LowPriorityQueue, &HighPriorityQueue,
                                                          Platform, Profiler);
   
   u64 LastTSC = OS_ReadCPUTimer();
   u64 CPU_TimerFreq = OS_CPUTimerFreq();
   
   platform_input Input = {};
   GlobalInput = &Input;
   InputArena = AllocArena(Gigabytes(1));
   
   //- main loop
   b32 Running = true;
   b32 RefreshRequested = true;
   // TODO(hbr): Temporary
   u64 FrameCount = 0;
   b32 ProfilingStopped = false;
   
   while (Running)
   {
    //- hot reload
    b32 EditorCodeReloaded = false;
    b32 RendererCodeReloaded = false;
    ProfileBlock("Hot Reload")
    {
     win32_hot_reload_task EditorHotReload = {};
     win32_hot_reload_task RendererHotReload = {};
     
     EditorHotReload.Code = &EditorCode;
     RendererHotReload.Code = &RendererCode;
#if 0
     // TODO(hbr): Unfortunately multithreaded path doesn't improve performance. It seems as
     // OS_LoadLibrary (LoadLibrary from Win32 API) grabs mutex or something, making the calls
     // serial in practice.
     WorkQueueAddEntry(HighPriorityQueue, Win32HotReloadTask, &EditorHotReload);
     WorkQueueAddEntry(HighPriorityQueue, Win32HotReloadTask, &RendererHotReload);
     
     WorkQueueCompleteAllWork(HighPriorityQueue);
#else
     Win32HotReloadTask(&EditorHotReload);
     Win32HotReloadTask(&RendererHotReload);
#endif
     
     EditorCodeReloaded = EditorHotReload.CodeReloaded;
     RendererCodeReloaded = RendererHotReload.CodeReloaded;
    }
    
    if (RendererCodeReloaded && RendererCode.IsValid)
    {
     RendererFunctions.OnCodeReload(&RendererMemory);
    }
    
    if (!Renderer && RendererCode.IsValid)
    {
     Renderer = RendererFunctions.Init(PermamentArena, &RendererMemory, Window, WindowDC);
    }
    
    if (EditorCodeReloaded && EditorCode.IsValid)
    {
     imgui_bindings Bindings = EditorFunctions.OnCodeReload(&EditorMemory);
     GlobalImGuiBindings = Bindings;
     RendererMemory.ImGuiBindings = Bindings;
     
     win32_imgui_init_data Init = {};
     Init.Window = Window;
     Bindings.Init(Cast(imgui_init_data *)&Init);
    }
    
    //- process input events
    v2u WindowDim = {};
    ProfileBlock("Profile Input Events")
    {
     GlobalWin32Input = {};
     Input = {};
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
     
     for (u32 KeyIndex = 0;
          KeyIndex < WIN32_KEY_COUNT;
          ++KeyIndex)
     {
      platform_key Key = Win32KeyTable[KeyIndex];
      if (Key)
      {
       // NOTE(hbr): I tried to use GetAsyncKeyState but run into issues -
       // there was a situation where I updated Pressed, before I processed input
       // event - which was still incoming - thus ending up in unexpected state.
       SHORT State = GetKeyState(KeyIndex);
       b32 IsDown = (State & 0x8000);
       Input.Pressed[Key] = IsDown;
      }
     }
     
     Input.EventCount = GlobalWin32Input.EventCount;
     Input.Events = GlobalWin32Input.Events;
     
     // NOTE(hbr): get window dim after processing events
     WindowDim = Win32GetWindowDim(Window);
     
     POINT CursorP;
     GetCursorPos(&CursorP);
     ScreenToClient(Window, &CursorP);
     Input.ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
#if 0
     for (u32 EventIndex = 0;
          EventIndex < Input.EventCount;
          ++EventIndex)
     {
      platform_event *Event = Input.Events + EventIndex;
      char const *Name = PlatformEventTypeNames[Event->Type];
      char const *KeyName = PlatformKeyNames[Event->Key];
      if (Event->Type != PlatformEvent_MouseMove)
      {
       OS_PrintDebugF("[%lu] %s %s\n", FrameCount, Name, KeyName);
      }
     }
#endif
    }
    
    //- update and render
    if (!ProfilingStopped)
    {
     ProfilerBeginFrame(Profiler);
    }
    
    render_frame *Frame = 0;
    if (RendererCode.IsValid)
    {
     Frame = RendererFunctions.BeginFrame(Renderer, &RendererMemory, WindowDim);
    }
    
    {
     u64 NowTSC = OS_ReadCPUTimer();
     Input.dtForFrame = Cast(f32)(NowTSC - LastTSC) / CPU_TimerFreq;
     LastTSC = NowTSC;
    }
    
    if (EditorCode.IsValid)
    {
     EditorFunctions.UpdateAndRender(&EditorMemory, &Input, Frame);
    }
    RefreshRequested = Input.RefreshRequested;
    ProfilingStopped = Input.ProflingStopped;
    
    if (RendererCode.IsValid)
    {
     RendererFunctions.EndFrame(Renderer, &RendererMemory, Frame);
    }
    
    if (Input.QuitRequested)
    {
     Running = false;
    }
    
    if (!ProfilingStopped)
    {
     ProfilerEndFrame(Profiler);
    }
    
    ++FrameCount;
   }
  }
 }
 
 if (!InitSuccess)
 {
  Win32DisplayErrorBox("Failed to initialize window");
 }
}

#ifndef CONSOLE_APPLICATION
# define CONSOLE_APPLICATION 0
#endif
int
#if CONSOLE_APPLICATION
main(int ArgCount, char **Argv)
#else
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd)
#endif
{
 EntryPoint();
 return 0;
}
