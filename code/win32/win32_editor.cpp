#include <windows.h>

#include "platform_shared.h"

#include "win32/win32_editor.h"
#include "win32/win32_editor_renderer.h"
#include "win32/win32_editor_imgui_bindings.h"

#include "platform_shared.cpp"

global win32_state GlobalWin32State;
global imgui_bindings GlobalImGuiBindings;

internal string
Win32FileDialogFiltersToWin32Filter(arena *Arena, platform_file_dialog_filters Filters)
{
 string_list FilterList = {};
 
 if (Filters.AnyFileFilter)
 {
  StrListPush(Arena, &FilterList, StrLit("All Files (*.*)"));
  StrListPush(Arena, &FilterList, StrLit("*.*"));
 }
 
 for (u32 FilterIndex = 0;
      FilterIndex < Filters.FilterCount;
      ++FilterIndex)
 {
  platform_file_dialog_filter Filter = Filters.Filters[FilterIndex];
  
  string_list ExtensionRegexes = {};
  for (u32 ExtensionIndex = 0;
       ExtensionIndex < Filter.ExtensionCount;
       ++ExtensionIndex)
  {
   string Extension = Filter.Extensions[ExtensionIndex];
   StrListPushF(Arena, &ExtensionRegexes, "*.%S", Extension);
  }
  
  string_list_join_options SpaceOpts = {};
  SpaceOpts.Sep = StrLit(" ");
  string SpaceJoinedExtensionRegexes = StrListJoin(Arena, &ExtensionRegexes, SpaceOpts);
  StrListPushF(Arena, &FilterList, "%S (%S)", Filter.DisplayName, SpaceJoinedExtensionRegexes);
  
  string_list_join_options SemicolonOpts = {};
  SemicolonOpts.Sep = StrLit(";");
  string SemicolonJoinedExtensionRegexes = StrListJoin(Arena, &ExtensionRegexes, SemicolonOpts);
  StrListPush(Arena, &FilterList, SemicolonJoinedExtensionRegexes);
 }
 
 string_list_join_options NilOpts = {};
 NilOpts.Sep = StrLit("\0");
 NilOpts.Post = StrLit("\0");
 string Win32Filter = StrListJoin(Arena, &FilterList, NilOpts);
 
 return Win32Filter;
}

internal platform_file_dialog_result
Win32OpenFileDialog(arena *Arena, platform_file_dialog_filters Filters)
{
 platform_file_dialog_result Result = {};
 
 temp_arena Temp = TempArena(Arena);
 
 string Win32Filter = Win32FileDialogFiltersToWin32Filter(Temp.Arena, Filters);
 
 char Buffer[2048];
 OPENFILENAME Open = {};
 Open.lStructSize = SizeOf(Open);
 Open.lpstrFile = Buffer;
 Open.lpstrFile[0] = '\0';
 Open.nMaxFile = ArrayCount(Buffer);
 Open.lpstrFilter = Win32Filter.Data;
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
 
 EndTemp(Temp);
 
 return Result;
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
 win32_state *State = &GlobalWin32State;
 platform_event *Result = 0;
 
 if (State->Win32Input.EventCount < WIN32_MAX_EVENT_COUNT)
 {
  Result = State->Win32Input.Events + State->Win32Input.EventCount++;
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
      platform_key Key = GlobalWin32State.Win32ToPlatformKeyTable[wParam];
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
     string *Files = PushArray(GlobalWin32State.InputArena, FileCount, string);
     for (UINT FileIndex = 0;
          FileIndex < FileCount;
          ++FileIndex)
     {
      UINT RequiredCount = DragQueryFileA(DropHandle, FileIndex, 0, 0);
      string File = PushString(GlobalWin32State.InputArena, RequiredCount + 1);
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
Win32PrintDebugInputEvents(platform_input *Input)
{
 for (u32 EventIndex = 0;
      EventIndex < Input->EventCount;
      ++EventIndex)
 {
  platform_event *Event = Input->Events + EventIndex;
  char const *Name = PlatformEventTypeNames[Event->Type];
  char const *KeyName = PlatformKeyNames[Event->Key];
  if (Event->Type != PlatformEvent_MouseMove)
  {
   OS_PrintDebugF("[%lu] %s %s\n", Name, KeyName);
  }
 }
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
 
 Platform.OpenFileDialog = Win32OpenFileDialog;
 GlobalImGuiBindings = ImGuiStubBindings();
 
 //- key mappings
 {
  platform_key *Table = GlobalWin32State.Win32ToPlatformKeyTable;
  
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
  
  Table[VK_LBUTTON] = PlatformKey_LeftMouseButton;
  Table[VK_RBUTTON] = PlatformKey_RightMouseButton;
  Table[VK_MBUTTON] = PlatformKey_MiddleMouseButton;
 }
 
 GlobalWin32State.InputArena = AllocArena(Gigabytes(1));
 
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
   
   
   //- misc
   u64 LastTSC = OS_ReadCPUTimer();
   u64 CPU_TimerFreq = OS_CPUTimerFreq();
   
   b32 Running = true;
   b32 RefreshRequested = true;
   b32 ProfilingStopped = false;
   
   //- main loop
   while (Running)
   {
    if (!ProfilingStopped)
    {
     ProfilerBeginFrame(Profiler);
    }
    
    //- hot reload
    b32 EditorCodeReloaded = false;
    b32 RendererCodeReloaded = false;
    ProfileBlock("Hot Reload")
    {
     EditorCodeReloaded = HotReloadIfOutOfSync(&EditorCode);
     RendererCodeReloaded = HotReloadIfOutOfSync(&RendererCode);
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
    platform_input Input = {};
    v2u WindowDim = {};
    ProfileBlock("Profile Input Events")
    {
     StructZero(&GlobalWin32State.Win32Input);
     ClearArena(GlobalWin32State.InputArena);
     
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
      platform_key Key = GlobalWin32State.Win32ToPlatformKeyTable[KeyIndex];
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
     
     Input.EventCount = GlobalWin32State.Win32Input.EventCount;
     Input.Events = GlobalWin32State.Win32Input.Events;
     
     // NOTE(hbr): get window dim after processing events
     WindowDim = Win32GetWindowDim(Window);
     
     POINT CursorP;
     GetCursorPos(&CursorP);
     ScreenToClient(Window, &CursorP);
     Input.ClipSpaceMouseP = Win32ScreenToClip(CursorP.x, CursorP.y, WindowDim);
     
     //Win32PrintDebugInputEvents(&Input);
    }
    
    //- update and render
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
    
    RefreshRequested = Input.RefreshRequested;
    ProfilingStopped = Input.ProflingStopped;
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
