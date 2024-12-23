#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "editor_memory.h"
#include "base/base_thread_ctx.h"

#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_work_queue.h"
#include "editor_renderer.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_thread_ctx.cpp"

#include "editor_memory.cpp"
#include "editor_work_queue.cpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "x11/x11_editor.h"
#include "x11/x11_editor_renderer.h"

internal void *
LinuxAllocMemory(u64 Size, b32 Commit)
{
 void *Memory = OS_Reserve(Size, Commit);
 return Memory;
}

internal void
LinuxDeallocMemory(void *Memory, u64 Size)
{
 OS_Release(Memory, Size);
}

internal temp_arena
LinuxGetScratchArena(arena *Conflict)
{
 temp_arena Result = ThreadCtxGetScratch(Conflict);
 return Result;
}

internal platform_file_dialog_result
LinuxOpenFileDialog(arena *Arena)
{
 NotImplemented;
 return {};
}

internal string
LinuxReadEntireFile(arena *Arena, string FilePath)
{
 string Result = OS_ReadEntireFile(Arena, FilePath);
 return Result;
}

platform_api Platform = {
 LinuxAllocMemory,
 LinuxDeallocMemory,
 LinuxGetScratchArena,
 LinuxOpenFileDialog,
 LinuxReadEntireFile,
 WorkQueueAddEntry,
 WorkQueueCompleteAllWork,
};

internal platform_key
X11KeyCodeToPlatformKey(Display *Display, unsigned int KeyCode)
{
 local b32 Calculated = false;
 local platform_key Mapping[256] = {};
 if (!Calculated)
 {
  unsigned int Code = 0;
#define AddMapping(KeySym, Key) Code = XKeysymToKeycode(Display, KeySym); Assert(Code < ArrayCount(Mapping)); Mapping[Code] = Key

  AddMapping(XK_F1, PlatformKey_F1);
  AddMapping(XK_F2, PlatformKey_F2);
  AddMapping(XK_F3, PlatformKey_F3);
  AddMapping(XK_F4, PlatformKey_F4);
  AddMapping(XK_F5, PlatformKey_F5);
  AddMapping(XK_F6, PlatformKey_F6);
  AddMapping(XK_F7, PlatformKey_F7);
  AddMapping(XK_F8, PlatformKey_F8);
  AddMapping(XK_F9, PlatformKey_F9);
  AddMapping(XK_F10, PlatformKey_F10);
  AddMapping(XK_F11, PlatformKey_F11);
  AddMapping(XK_F12, PlatformKey_F12);
  
  AddMapping(XK_A, PlatformKey_A);
  AddMapping(XK_B, PlatformKey_B);
  AddMapping(XK_C, PlatformKey_C);
  AddMapping(XK_D, PlatformKey_D);
  AddMapping(XK_E, PlatformKey_E);
  AddMapping(XK_F, PlatformKey_F);
  AddMapping(XK_G, PlatformKey_G);
  AddMapping(XK_H, PlatformKey_H);
  AddMapping(XK_I, PlatformKey_I);
  AddMapping(XK_J, PlatformKey_J);
  AddMapping(XK_K, PlatformKey_K);
  AddMapping(XK_L, PlatformKey_L);
  AddMapping(XK_M, PlatformKey_M);
  AddMapping(XK_N, PlatformKey_N);
  AddMapping(XK_O, PlatformKey_O);
  AddMapping(XK_P, PlatformKey_P);
  AddMapping(XK_Q, PlatformKey_Q);
  AddMapping(XK_R, PlatformKey_R);
  AddMapping(XK_S, PlatformKey_S);
  AddMapping(XK_T, PlatformKey_T);
  AddMapping(XK_U, PlatformKey_U);
  AddMapping(XK_V, PlatformKey_V);
  AddMapping(XK_W, PlatformKey_W);
  AddMapping(XK_X, PlatformKey_X);
  AddMapping(XK_Y, PlatformKey_Y);
  AddMapping(XK_Z, PlatformKey_Z);
  
  AddMapping(XK_Escape, PlatformKey_Escape);
  AddMapping(XK_Shift_L, PlatformKey_Shift);
  AddMapping(XK_Control_L, PlatformKey_Ctrl);
  AddMapping(XK_Alt_L, PlatformKey_Alt);
  AddMapping(XK_space, PlatformKey_Space);
  AddMapping(XK_Tab, PlatformKey_Tab);
  AddMapping(XK_Delete, PlatformKey_Delete);

  #undef AddMapping

  Calculated = true;
 }

 platform_key Result = Mapping[KeyCode];
 return Result;
}

internal platform_key
X11MouseButtonToPlatformKey(Display *Display, unsigned int Button)
{
 local b32 Calculated = false;
 local platform_key Mapping[16] = {};
 if (!Calculated)
 {
#define AddMapping(Button, Key) Assert(Button < ArrayCount(Mapping)); Mapping[Button] = Key
  AddMapping(Button1, PlatformKey_LeftMouseButton);
  AddMapping(Button2, PlatformKey_MiddleMouseButton);
  AddMapping(Button3, PlatformKey_RightMouseButton);
#undef AddMapping
  Calculated = true;
 }

 platform_key Result = Mapping[Button];
 return Result;
}

internal platform_event *
LinuxPushPlatformEvent(linux_platform_input *Input, platform_event_type Type)
{
 platform_event *Result = 0;
 if (Input->EventCount < LINUX_MAX_PLATFORM_EVENT_COUNT)
 {
  Result = Input->Events + Input->EventCount++;
  Result->Type = Type;
 }

 return Result;
}

internal v2u
X11GetWindowDims(Display *Display, Window Window)
{
 XWindowAttributes Attrs = {};
 XGetWindowAttributes(Display, Window, &Attrs);
 v2u Result = {};
 Result.X = Attrs.width;
 Result.Y = Attrs.height;
 
 return Result;
}

internal void
X11ProcessEvent(linux_platform_input *LinuxInput, Display *Display, Window Window, Atom AtomWMDeleteWindow, XEvent *Event)
{
 switch (Event->type)
 {
  case KeyPress:
  case KeyRelease: {
   platform_event_type Type = (Event->type == KeyPress ? PlatformEvent_Press : PlatformEvent_Release);
   platform_event *PlatformEvent = LinuxPushPlatformEvent(LinuxInput, Type);
   if (PlatformEvent)
   {
    PlatformEvent->Key = X11KeyCodeToPlatformKey(Display, Event->xkey.keycode);
   }
  }break;

  case ButtonPress:
  case ButtonRelease: {
   platform_event_type Type = (Event->type == ButtonPress ? PlatformEvent_Press : PlatformEvent_Release);
   platform_event *PlatformEvent = LinuxPushPlatformEvent(LinuxInput, Type);
   if (PlatformEvent)
   {
    PlatformEvent->Key = X11MouseButtonToPlatformKey(Display, Event->xbutton.button);
   }
  }break;

  case MotionNotify: {
   platform_event *PlatformEvent = LinuxPushPlatformEvent(LinuxInput, PlatformEvent_MouseMove);
   if (PlatformEvent)
   {
    int X = Event->xmotion.x;
    int Y = Event->xmotion.y;
    v2u WindowDim = X11GetWindowDims(Display, Window);
    f32 ClipX = +(2.0f * X / WindowDim.X - 1.0f);
    f32 ClipY = -(2.0f * Y / WindowDim.Y - 1.0f);
    PlatformEvent->ClipSpaceMouseP = V2(ClipX, ClipY);
   }
  }break;

  case KeymapNotify: {
   XRefreshKeyboardMapping(&Event->xmapping);
  }break;
  
  case DestroyNotify: {
   LinuxPushPlatformEvent(LinuxInput, PlatformEvent_WindowClose);
  }break;

  case ClientMessage: {
			if (Cast(unsigned long int)Event->xclient.data.l[0] == AtomWMDeleteWindow) {
    LinuxPushPlatformEvent(LinuxInput, PlatformEvent_WindowClose);
			}
  }break;
 }
}

int
main(int ArgCount, char *Argv[])
{
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0; ArenaIndex < ArrayCount(Arenas); ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena();
 } 
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));
 arena *Arena = Arenas[0];

 Display *Display = XOpenDisplay(0);
 if (Display)
 {
  x11_renderer_function_table RendererFunctions = {};

  Window Window = 0;
  renderer *Renderer = 0;
  {
   int X = 0;
   int Y = 0;
   uint WindowWidth = 300;
   uint WindowHeight = 300;
   uint BorderWidth = 0;
   x11_renderer_init_result RendererInit = RendererFunctions.CreateWindowAndInitRenderer(Arena, Display, X, Y, WindowWidth, WindowHeight, BorderWidth);
   Window = RendererInit.X11Window;
   Renderer = RendererInit.Renderer;
  }
  XStoreName(Display, Window, "Parametrics Curves Editor");

  renderer_memory _RendererMemory = {};
  renderer_memory *RendererMemory = &_RendererMemory;
  // TODO(hbr): initialize renderer memory

  long EventMask = (KeyPressMask | KeyReleaseMask | KeymapStateMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XSelectInput(Display, Window, EventMask);

  Atom AtomWMDeleteWindow = XInternAtom(Display, "WM_DELETE_WINDOW", False);
	 XSetWMProtocols(Display, Window, &AtomWMDeleteWindow, 1);
  
  XMapWindow(Display, Window);

  if (Window)
  {
   b32 Running = true;
   linux_platform_input LinuxInput = {};

   while (Running)
   {
    //- process input
    LinuxInput.EventCount = 0;
    for (;;)
    {
     int PendingCount = XPending(Display);
     if (PendingCount > 0)
     {
      XEvent Event;
      XNextEvent(Display, &Event);
      X11ProcessEvent(&LinuxInput, Display, Window, AtomWMDeleteWindow, &Event);
     }
     else
     {
      break;
     }
    }
    for (u32 EventIndex = 0;
         EventIndex < LinuxInput.EventCount;
         ++EventIndex)
    {
     platform_event *Event = LinuxInput.Events + EventIndex;
     char const *EventName = PlatformEventTypeNames[Event->Type];
     char const *KeyName = PlatformKeyNames[Event->Key];
     OS_PrintDebugF("%s %s\n", EventName, KeyName);
    }

    platform_input Input = {};
    Input.EventCount = LinuxInput.EventCount;
    Input.Events = LinuxInput.Events;

    for (u32 EventIndex = 0;
         EventIndex < Input.EventCount;
         ++EventIndex)
    {
     platform_event *Event = Input.Events + EventIndex;
     if (Event->Type == PlatformEvent_Press && Event->Key == PlatformKey_Escape)
     {
      Running = false;
     }
     if (Event->Type == PlatformEvent_WindowClose)
     {
      Running = false;
     }
    }

    v2u WindowDim = X11GetWindowDims(Display, Window);
    render_frame *Frame = RendererFunctions.BeginFrame(Renderer, RendererMemory, WindowDim);
    RendererFunctions.EndFrame(Renderer, RendererMemory, Frame);
   }
  }
 }
 else
 {
  // TODO(hbr): error handling
 }

 return 0;
}