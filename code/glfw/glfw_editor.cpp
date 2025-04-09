#include "platform_shared.h"
#include "editor_glfw.h"
#include "editor_math.h"
#include "glfw/glfw_editor.h"
#include "glfw/glfw_editor_renderer_opengl.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"

#include "platform_shared.cpp"
#include "editor_math.cpp"
#include "glfw/glfw_editor_renderer_opengl.cpp"

global glfw_state GlobalGLFWState;

IMGUI_NEW_FRAME(GLFWOpenGLImGuiNewFrame)
{
 ImGui_ImplOpenGL3_NewFrame();
 ImGui_ImplGlfw_NewFrame();
 ImGui::NewFrame();
}

IMGUI_RENDER(GLFWOpenGLImGuiRender)
{
 ImGui::Render();
 ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

internal glfw_imgui_maybe_capture_input_result
GLFWImGuiMaybeCaptureInput(void)
{
 glfw_imgui_maybe_capture_input_result ImGuiCapture = {};
 ImGuiCapture.ImGuiWantCaptureKeyboard = Cast(b32)ImGui::GetIO().WantCaptureKeyboard;
 ImGuiCapture.ImGuiWantCaptureMouse = Cast(b32)ImGui::GetIO().WantCaptureMouse;
 
 return ImGuiCapture;
}

internal platform_event *
GLFWPushPlatformEvent(platform_event_type Type)
{
 glfw_state *GLFWState = &GlobalGLFWState;
 platform_event *Result = 0;
 glfw_platform_input *Input = &GLFWState->GLFWInput;
 if (Input->EventCount < GLFW_MAX_EVENT_COUNT)
 {
  Result = Input->Events + Input->EventCount++;
  Result->Type = Type;
 }
 
 return Result;
}

internal platform_key
GLFWMouseButtonToPlatformKey(int Button)
{
 glfw_state *GLFWState = &GlobalGLFWState;
 Assert(Button >= 0 && Button < ArrayCount(GLFWState->GLFWToPlatformKeyTable));
 platform_key Result = GLFWState->GLFWToPlatformKeyTable[Button];
 Assert(Result != 0);
 
 return Result;
}

internal platform_event_flags
GLFWModsToFlags(int Mods)
{
 platform_event_flags Flags = 0;
 if (Mods & GLFW_MOD_SHIFT) Flags |= PlatformEventFlag_Shift;
 if (Mods & GLFW_MOD_CONTROL) Flags |= PlatformEventFlag_Ctrl;
 if (Mods & GLFW_MOD_ALT) Flags |= PlatformEventFlag_Alt;
 
 return Flags;
}

internal void
GLFWKeyCallback(GLFWwindow *Window, int Key, int ScanCode, int Action, int Mods)
{
 glfw_state *GLFWState = &GlobalGLFWState;
 
 glfw_imgui_maybe_capture_input_result ImGuiCapture =  GLFWImGuiMaybeCaptureInput();
 if (!ImGuiCapture.ImGuiWantCaptureKeyboard)
 {
  switch (Action)
  {
   case GLFW_PRESS:
   case GLFW_RELEASE: {
    platform_event_type Type = (Action == GLFW_PRESS ? PlatformEvent_Press : PlatformEvent_Release);
    platform_event *Event = GLFWPushPlatformEvent(Type);
    if (Event)
    {
     Event->Key = GLFWState->GLFWToPlatformKeyTable[Key];
     Event->Flags = GLFWModsToFlags(Mods);
    }
   }break;
   
   case GLFW_REPEAT: {}break;
   
   default: InvalidPath;
  }
 }
}

internal v2u
GLFWWindowSize(GLFWwindow *Window)
{
 int Width;
 int Height;
 glfwGetWindowSize(Window, &Width, &Height);
 Assert(Width > 0);
 Assert(Height > 0);
 
 v2u Size = V2U(Width, Height);
 return Size;
}

internal v2
GLFWCursorPosToClipSpace(GLFWwindow *Window, double X_Pos, double Y_Pos)
{
 v2u WindowSize = GLFWWindowSize(Window);
 
 f32 X = +(2.0f * (Cast(f32)X_Pos / WindowSize.X) - 1.0f);
 f32 Y = -(2.0f * (Cast(f32)Y_Pos / WindowSize.Y) - 1.0f);
 
 v2 Result = V2(X, Y);
 return Result;
}

internal v2
GLFWGetCursorPosInClipSpace(GLFWwindow *Window)
{
 double X_Pos;
 double Y_Pos;
 glfwGetCursorPos(Window, &X_Pos, &Y_Pos);
 
 v2 Result = GLFWCursorPosToClipSpace(Window, X_Pos, Y_Pos);
 
 return Result;
}

internal void
GLFWMouseButtonCallback(GLFWwindow *Window, int Button, int Action, int Mods)
{
 glfw_imgui_maybe_capture_input_result ImGuiCapture = GLFWImGuiMaybeCaptureInput();
 if (!ImGuiCapture.ImGuiWantCaptureMouse)
 {
  switch (Action)
  {
   case GLFW_PRESS:
   case GLFW_RELEASE: {
    platform_event_type Type = (Action == GLFW_PRESS ? PlatformEvent_Press : PlatformEvent_Release);
    platform_event *Event = GLFWPushPlatformEvent(Type);
    if (Event)
    {
     Event->Key = GLFWMouseButtonToPlatformKey(Button);
     Event->ClipSpaceMouseP = GLFWGetCursorPosInClipSpace(Window);
     Event->Flags = GLFWModsToFlags(Mods);
    }
   }break;
   
   case GLFW_REPEAT: {}break;
   
   default: InvalidPath;
  }
 }
}

internal void
GLFWMouseMoveCallback(GLFWwindow *Window, double X, double Y)
{
 platform_event *Event = GLFWPushPlatformEvent(PlatformEvent_MouseMove);
 if (Event)
 {
  Event->ClipSpaceMouseP = GLFWCursorPosToClipSpace(Window, X, Y);
 }
}

internal void
GLFWMouseScrollCallback(GLFWwindow *Window, double XOffset, double YOffset)
{
 glfw_imgui_maybe_capture_input_result ImGuiCapture = GLFWImGuiMaybeCaptureInput();
 if (!ImGuiCapture.ImGuiWantCaptureMouse)
 {
  platform_event *Event = GLFWPushPlatformEvent(PlatformEvent_Scroll);
  if (Event)
  {
   Event->ScrollDelta = Cast(f32)YOffset;
  }
 }
}

internal void
GLFWDropCallback(GLFWwindow *Window, int PathCount, char const *Paths[])
{
 glfw_state *GLFWState = &GlobalGLFWState;
 
 platform_event *Event = GLFWPushPlatformEvent(PlatformEvent_FilesDrop);
 if (Event)
 {
  arena *Arena = GLFWState->InputArena;
  
  u32 FileCount = SafeCastU32(PathCount);
  string *FilePaths = PushArray(Arena, FileCount, string);
  for (u32 FileIndex = 0;
       FileIndex < FileCount;
       ++FileIndex)
  {
   FilePaths[FileIndex] = StrFromCStr(Paths[FileIndex]);
  }
  
  Event->FilePaths = FilePaths;
  Event->FileCount = FileCount;
  
  Event->ClipSpaceMouseP = GLFWGetCursorPosInClipSpace(Window);
 }
}

internal void
EntryPoint(int ArgCount, char **Args)
{
 OS_Init(ArgCount, Args);
 ThreadCtxInit();
 
 profiler *Profiler = Cast(profiler *)OS_Reserve(SizeOf(profiler), true);
 ProfilerInit(Profiler);
 ProfilerEquip(Profiler);
 
 arena *PermamentArena = AllocArena(Gigabytes(64));
 
 glfw_state *GLFWState = &GlobalGLFWState;
 GLFWState->InputArena = AllocArena(Kilobytes(4));
 
 {
  platform_key *Table = GLFWState->GLFWToPlatformKeyTable;
  
#define DefineGLFWMapping(GLFWButton, PlatformButton) \
Assert(GLFWButton < ArrayCount(GLFWState->GLFWToPlatformKeyTable)); \
Assert(Table[GLFWButton] == 0); \
Table[GLFWButton] = PlatformButton
  
  DefineGLFWMapping(GLFW_KEY_F1, PlatformKey_F1);
  DefineGLFWMapping(GLFW_KEY_F2, PlatformKey_F2);
  DefineGLFWMapping(GLFW_KEY_F3, PlatformKey_F3);
  DefineGLFWMapping(GLFW_KEY_F4, PlatformKey_F4);
  DefineGLFWMapping(GLFW_KEY_F5, PlatformKey_F5);
  DefineGLFWMapping(GLFW_KEY_F6, PlatformKey_F6);
  DefineGLFWMapping(GLFW_KEY_F7, PlatformKey_F7);
  DefineGLFWMapping(GLFW_KEY_F8, PlatformKey_F8);
  DefineGLFWMapping(GLFW_KEY_F9, PlatformKey_F9);
  DefineGLFWMapping(GLFW_KEY_F10, PlatformKey_F10);
  DefineGLFWMapping(GLFW_KEY_F11, PlatformKey_F11);
  DefineGLFWMapping(GLFW_KEY_F12, PlatformKey_F12);
  
  DefineGLFWMapping(GLFW_KEY_A, PlatformKey_A);
  DefineGLFWMapping(GLFW_KEY_B, PlatformKey_B);
  DefineGLFWMapping(GLFW_KEY_C, PlatformKey_C);
  DefineGLFWMapping(GLFW_KEY_D, PlatformKey_D);
  DefineGLFWMapping(GLFW_KEY_E, PlatformKey_E);
  DefineGLFWMapping(GLFW_KEY_F, PlatformKey_F);
  DefineGLFWMapping(GLFW_KEY_G, PlatformKey_G);
  DefineGLFWMapping(GLFW_KEY_H, PlatformKey_H);
  DefineGLFWMapping(GLFW_KEY_I, PlatformKey_I);
  DefineGLFWMapping(GLFW_KEY_J, PlatformKey_J);
  DefineGLFWMapping(GLFW_KEY_K, PlatformKey_K);
  DefineGLFWMapping(GLFW_KEY_L, PlatformKey_L);
  DefineGLFWMapping(GLFW_KEY_M, PlatformKey_M);
  DefineGLFWMapping(GLFW_KEY_N, PlatformKey_N);
  DefineGLFWMapping(GLFW_KEY_O, PlatformKey_O);
  DefineGLFWMapping(GLFW_KEY_P, PlatformKey_P);
  DefineGLFWMapping(GLFW_KEY_Q, PlatformKey_Q);
  DefineGLFWMapping(GLFW_KEY_R, PlatformKey_R);
  DefineGLFWMapping(GLFW_KEY_S, PlatformKey_S);
  DefineGLFWMapping(GLFW_KEY_T, PlatformKey_T);
  DefineGLFWMapping(GLFW_KEY_U, PlatformKey_U);
  DefineGLFWMapping(GLFW_KEY_V, PlatformKey_V);
  DefineGLFWMapping(GLFW_KEY_W, PlatformKey_W);
  DefineGLFWMapping(GLFW_KEY_X, PlatformKey_X);
  DefineGLFWMapping(GLFW_KEY_Y, PlatformKey_Y);
  DefineGLFWMapping(GLFW_KEY_Z, PlatformKey_Z);
  
  DefineGLFWMapping(GLFW_KEY_ESCAPE, PlatformKey_Escape);
  DefineGLFWMapping(GLFW_KEY_LEFT_SHIFT, PlatformKey_Shift);
  DefineGLFWMapping(GLFW_KEY_LEFT_CONTROL, PlatformKey_Ctrl);
  DefineGLFWMapping(GLFW_KEY_LEFT_ALT, PlatformKey_Alt);
  DefineGLFWMapping(GLFW_KEY_SPACE, PlatformKey_Space);
  DefineGLFWMapping(GLFW_KEY_TAB, PlatformKey_Tab);
  DefineGLFWMapping(GLFW_KEY_DELETE, PlatformKey_Delete);
  DefineGLFWMapping(GLFW_KEY_GRAVE_ACCENT, PlatformKey_Backtick);
  
  DefineGLFWMapping(GLFW_MOUSE_BUTTON_LEFT, PlatformKey_LeftMouseButton);
  DefineGLFWMapping(GLFW_MOUSE_BUTTON_RIGHT, PlatformKey_RightMouseButton);
  DefineGLFWMapping(GLFW_MOUSE_BUTTON_MIDDLE, PlatformKey_MiddleMouseButton);
  
#undef DefineGLFWMapping
 }
 
 if (glfwInit())
 {
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
  GLFWvidmode const *VideoMode = glfwGetVideoMode(Monitor);
  main_window_params WindowParams = Platform_GetMainWindowInitialParams(VideoMode->width, VideoMode->height);
  
  int WindowX = GLFW_ANY_POSITION;
  int WindowY = GLFW_ANY_POSITION;
  int WindowWidth = 0;
  int WindowHeight = 0;
  
  if (!WindowParams.UseDefault)
  {
   WindowX = SafeCastInt(WindowParams.LeftCornerP.X);
   WindowY = SafeCastInt(WindowParams.LeftCornerP.Y);
   
   WindowWidth = SafeCastInt(WindowParams.Dims.X);
   WindowHeight = SafeCastInt(WindowParams.Dims.Y);
  }
  
  glfwWindowHint(GLFW_POSITION_X, WindowX);
  glfwWindowHint(GLFW_POSITION_Y, WindowY);
  
  GLFWwindow *Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowParams.Title, 0, 0);
  
  if (Window)
  {
   glfwSetKeyCallback(Window, GLFWKeyCallback);
   glfwSetCursorPosCallback(Window, GLFWMouseMoveCallback);
   glfwSetMouseButtonCallback(Window, GLFWMouseButtonCallback);
   glfwSetScrollCallback(Window, GLFWMouseScrollCallback);
   glfwSetDropCallback(Window, GLFWDropCallback);
   
   //- renderer
   renderer_memory RendererMemory = Platform_MakeRendererMemory(PermamentArena, Profiler,
                                                                GLFWOpenGLImGuiNewFrame,
                                                                GLFWOpenGLImGuiRender);
   
   work_queue LowPriorityQueue = {};
   work_queue HighPriorityQueue = {};
   Platform_MakeWorkQueues(&LowPriorityQueue, &HighPriorityQueue);
   
   renderer *Renderer = GLFWRendererInit(PermamentArena, &RendererMemory, Window);
   
   //- imgui init
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(Window, true);
   ImGui_ImplOpenGL3_Init();
   
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
   
   b32 Running = true;
   b32 ProfilingStopped = false;
   b32 RefreshRequested = true;
   platform_clock Clock = Platform_MakeClock();
   
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
    }
    
    //- process input events
    // TODO(hbr): get this from GLFW
    v2u WindowDim = {};
    platform_input Input = {};
    {
     StructZero(&GLFWState->GLFWInput);
     ClearArena(GLFWState->InputArena);
     
     if (RefreshRequested)
     {
      glfwPollEvents();
     }
     else
     {
      glfwWaitEvents();
     }
     
     if (glfwWindowShouldClose(Window))
     {
      platform_event *Event = GLFWPushPlatformEvent(PlatformEvent_WindowClose);
      MarkUnused(Event);
     }
     
     Input.EventCount = GLFWState->GLFWInput.EventCount;
     Input.Events = GLFWState->GLFWInput.Events;
     
     for (u32 KeyIndex = 0;
          KeyIndex < ArrayCount(GLFWState->GLFWToPlatformKeyTable);
          ++KeyIndex)
     {
      platform_key Key = GLFWState->GLFWToPlatformKeyTable[KeyIndex];
      if (Key)
      {
       int StateKey = glfwGetKey(Window, KeyIndex);
       int StateButton = glfwGetMouseButton(Window, KeyIndex);
       b32 IsDown = (StateKey == GLFW_PRESS || StateButton == GLFW_PRESS);
       Input.Pressed[Key] = IsDown;
      }
     }
     
     int Width = 0;
     int Height = 0;
     glfwGetWindowSize(Window, &Width, &Height);
     Assert(Width >= 0 && Height >= 0);
     WindowDim = V2U(Width, Height);
    }
    
    Platform_PrintDebugInputEvents(&Input);
    
    render_frame *Frame = GLFWRendererBeginFrame(Renderer, &RendererMemory, WindowDim);
    
    Input.dtForFrame = Platform_ClockFrame(&Clock);
    if (EditorCode.IsValid && Frame)
    {
     EditorFunctions.UpdateAndRender(&EditorMemory, &Input, Frame);
    }
    if (Input.QuitRequested)
    {
     Running = false;
    }
    
    GLFWRendererEndFrame(Renderer, &RendererMemory, Frame);
    
    if (!ProfilingStopped)
    {
     ProfilerBeginFrame(Profiler);
    }
    
    RefreshRequested = Input.RefreshRequested;
    ProfilingStopped = Input.ProfilingStopped;
   }
  }
 }
 else
 {
  OS_MessageBox(StrLit("failed to initialize GLFW"));
 }
}

#include "editor_main.cpp"