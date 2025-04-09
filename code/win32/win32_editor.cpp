#include <windows.h>

#include "platform_shared.h"
#include "win32/win32_editor.h"
#include "win32/win32_editor_renderer.h"
#include "win32/win32_editor_imgui_bindings.h"
#include "third_party/imgui/imgui_impl_win32.h"
#include "third_party/imgui/imgui_impl_opengl3.h"

#include "platform_shared.cpp"

global win32_state GlobalWin32State;

IMGUI_NEW_FRAME(Win32OpenGLImGuiNewFrame)
{
 ImGui_ImplOpenGL3_NewFrame();
 ImGui_ImplWin32_NewFrame();
 ImGui::NewFrame();
}

IMGUI_RENDER(Win32OpenGLImGuiRender)
{
 ImGui::Render();
 ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

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
 win32_state *Win32State = &GlobalWin32State;
 platform_event *Result = 0;
 
 if (Win32State->Win32Input.EventCount < WIN32_MAX_EVENT_COUNT)
 {
  Result = Win32State->Win32Input.Events + Win32State->Win32Input.EventCount++;
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
 v2 Result = V2( (2.0f * X / WindowDim.X - 1.0f),
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// TODO(hbr): remove
global b32 GlobalImGuiInit;
internal LRESULT 
Win32WindowProc(HWND Window, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT Result = 0;
 win32_state *Win32State = &GlobalWin32State;
 
 b32 ImGuiCapturedInput = false;
 b32 ImGuiWantCaptureKeyboard = false;
 b32 ImGuiWantCaptureMouse = false;
 if (GlobalImGuiInit)
 {
  ImGuiCapturedInput = Cast(b32)ImGui_ImplWin32_WndProcHandler(Window, Msg, wParam, lParam);
  ImGuiWantCaptureKeyboard = Cast(b32)ImGui::GetIO().WantCaptureKeyboard;
  ImGuiWantCaptureMouse = Cast(b32)ImGui::GetIO().WantCaptureMouse;
 }
 
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
      platform_key Key = Win32State->Win32ToPlatformKeyTable[wParam];
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
     string *Files = PushArray(Win32State->InputArena, FileCount, string);
     for (UINT FileIndex = 0;
          FileIndex < FileCount;
          ++FileIndex)
     {
      UINT RequiredCount = DragQueryFileA(DropHandle, FileIndex, 0, 0);
      string File = PushString(Win32State->InputArena, RequiredCount + 1);
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
EntryPoint(int ArgCount, char **Args)
{
 HINSTANCE Instance = GetModuleHandle(0);
 b32 InitSuccess = false;
 
 Platform = Platform_MakePlatformAPI();
 
 OS_Init(ArgCount, Args);
 ThreadCtxInit();
 
 profiler *Profiler = Cast(profiler *)OS_Reserve(SizeOf(profiler), true);
 ProfilerInit(Profiler);
 ProfilerEquip(Profiler);
 
 arena *PermamentArena = AllocArena(Gigabytes(64));
 
 win32_state *Win32State = &GlobalWin32State;
 Win32State->InputArena = AllocArena(Gigabytes(1));
 
 //- key mappings
 {
  platform_key *Table = Win32State->Win32ToPlatformKeyTable;
  
  Table[VK_F1] = PlatformKey_F1;
  Table[VK_F2] = PlatformKey_F2;
  Table[VK_F3] = PlatformKey_F3;
  Table[VK_F4] = PlatformKey_F4;
  Table[VK_F5] = PlatformKey_F5;
  Table[VK_F6] = PlatformKey_F6;
  Table[VK_F7] = PlatformKey_F7;
  Table[VK_F8] = PlatformKey_F8;
  Table[VK_F9] = PlatformKey_F9;
  Table[VK_F10] = PlatformKey_F10;
  Table[VK_F11] = PlatformKey_F11;
  Table[VK_F12] = PlatformKey_F12;
  
  Table['A'] = PlatformKey_A;
  Table['B'] = PlatformKey_B;
  Table['C'] = PlatformKey_C;
  Table['D'] = PlatformKey_D;
  Table['E'] = PlatformKey_E;
  Table['F'] = PlatformKey_F;
  Table['G'] = PlatformKey_G;
  Table['H'] = PlatformKey_H;
  Table['I'] = PlatformKey_I;
  Table['J'] = PlatformKey_J;
  Table['K'] = PlatformKey_K;
  Table['L'] = PlatformKey_L;
  Table['M'] = PlatformKey_M;
  Table['N'] = PlatformKey_N;
  Table['O'] = PlatformKey_O;
  Table['P'] = PlatformKey_P;
  Table['Q'] = PlatformKey_Q;
  Table['R'] = PlatformKey_R;
  Table['S'] = PlatformKey_S;
  Table['T'] = PlatformKey_T;
  Table['U'] = PlatformKey_U;
  Table['V'] = PlatformKey_V;
  Table['W'] = PlatformKey_W;
  Table['X'] = PlatformKey_X;
  Table['Y'] = PlatformKey_Y;
  Table['Z'] = PlatformKey_Z;
  
  Table[VK_ESCAPE] = PlatformKey_Escape;
  
  Table[VK_SHIFT] = PlatformKey_Shift;
  Table[VK_CONTROL] = PlatformKey_Ctrl;
  Table[VK_MENU] = PlatformKey_Alt;
  Table[VK_SPACE] = PlatformKey_Space;
  Table[VK_TAB] = PlatformKey_Tab;
  Table[VK_DELETE] = PlatformKey_Delete;
  Table[VK_OEM_3] = PlatformKey_Backtick;
  
  Table[VK_LBUTTON] = PlatformKey_LeftMouseButton;
  Table[VK_RBUTTON] = PlatformKey_RightMouseButton;
  Table[VK_MBUTTON] = PlatformKey_MiddleMouseButton;
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
  
  main_window_params WindowParams = Platform_GetMainWindowInitialPlacement(ScreenWidth, ScreenHeight);
  
  int WindowX = CW_USEDEFAULT;
  int WindowY = CW_USEDEFAULT;
  int WindowWidth = CW_USEDEFAULT;
  int WindowHeight = CW_USEDEFAULT;
  
  if (!WindowParams.UseDefault)
  {
   WindowX = SafeCastInt(WindowParams.LeftCornerP.X);
   WindowY = SafeCastInt(WindowParams.LeftCornerP.Y);
   WindowWidth = SafeCastInt(WindowParams.Dims.X);
   WindowHeight = SafeCastInt(WindowParams.Dims.Y);
  }
  
  HWND Window = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
                                WindowClass.lpszClassName,
                                WindowParams.Title,
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
   
   renderer_memory RendererMemory = Platform_MakeRendererMemory(PermamentArena, Profiler,
                                                                Win32OpenGLImGuiNewFrame,
                                                                Win32OpenGLImGuiRender);
   
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
   
   
   //- misc
   b32 Running = true;
   b32 RefreshRequested = true;
   b32 ProfilingStopped = false;
   platform_clock Clock = Platform_MakeClock();
   
   //- main loop
   while (Running)
   {
    if (!ProfilingStopped)
    {
     ProfilerBeginFrame(Profiler);
    }
    
    //- hot reload
    ProfileBlock("Hot Reload")
    {
     if (HotReloadIfOutOfSync(&EditorCode))
     {
      EditorFunctions.OnCodeReload(&EditorMemory);
     }
     
     if (HotReloadIfOutOfSync(&RendererCode))
     {
      RendererFunctions.OnCodeReload(&RendererMemory);
     }
    }
    
    if (!Renderer && RendererCode.IsValid)
    {
     Renderer = RendererFunctions.Init(PermamentArena, &RendererMemory, Window, WindowDC);
     
     win32_imgui_init_data *Win32Init = Cast(win32_imgui_init_data *)Init;
     ImGui::CreateContext();
     ImGui_ImplWin32_Init(Win32Init->Window);
     ImGui_ImplOpenGL3_Init();
     
     GlobalImGuiInit = true;
    }
    
    //- process input events
    platform_input Input = {};
    v2u WindowDim = {};
    ProfileBlock("Profile Input Events")
    {
     StructZero(&Win32State->Win32Input);
     ClearArena(Win32State->InputArena);
     
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
      platform_key Key = Win32State->Win32ToPlatformKeyTable[KeyIndex];
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
     
     Input.EventCount = Win32State->Win32Input.EventCount;
     Input.Events = Win32State->Win32Input.Events;
     
     // NOTE(hbr): get window dim after processing events
     WindowDim = Win32GetWindowDim(Window);
     
     POINT CursorP;
     GetCursorPos(&CursorP);
     ScreenToClient(Window, &CursorP);
     Input.ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
     //Platform_PrintDebugInputEvents(&Input);
    }
    
    //- update and render
    render_frame *Frame = 0;
    if (Renderer && RendererCode.IsValid)
    {
     Frame = RendererFunctions.BeginFrame(Renderer, &RendererMemory, WindowDim);
    }
    
    Input.dtForFrame = Platform_ClockFrame(&Clock);
    if (EditorCode.IsValid)
    {
     EditorFunctions.UpdateAndRender(&EditorMemory, &Input, Frame);
    }
    if (Input.QuitRequested)
    {
     Running = false;
    }
    
    if (Renderer && RendererCode.IsValid)
    {
     RendererFunctions.EndFrame(Renderer, &RendererMemory, Frame);
    }
    
    if (!ProfilingStopped)
    {
     ProfilerEndFrame(Profiler);
    }
    
    RefreshRequested = Input.RefreshRequested;
    ProfilingStopped = Input.ProfilingStopped;
   }
  }
 }
 
 if (!InitSuccess)
 {
  OS_MessageBox(StrLit("Error initializing window!"));
 }
}

#include "editor_main.cpp"