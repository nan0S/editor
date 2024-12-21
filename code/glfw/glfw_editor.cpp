#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "editor_memory.h"
#include "base/base_thread_ctx.h"

#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_work_queue.h"

#include "glfw/glfw_editor.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_thread_ctx.cpp"

#include "editor_memory.cpp"
#include "editor_work_queue.cpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
  unsigned int Code = {};
  #define AddMapping(KeySym, Key) Code = XKeysymToKeycode(Display, KeySym); Assert(Code < ArrayCount(Mapping)); Mapping[Code] = PlatformKey_##Key
  AddMapping(XK_Escape, Escape);
  // TODO(hbr): add other mappings
  #undef AddMapping

  Calculated = true;
 }

 platform_key Result = Mapping[KeyCode];
 return Result;
}

internal void
LinuxPushPlatformEvent(linux_platform_input *Input, platform_event_type Type)
{
}

int main(int ArgCount, char *Argv[])
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
  Screen *Screen = DefaultScreenOfDisplay(Display);
  int ScreenID = DefaultScreen(Display);
  Window Parent = RootWindowOfScreen(Screen);
  int X = 0, Y = 0;
  uint WindowWidth = 300;
  uint WindowHeight = 300;
  uint BorderWidth = 1;
  ulong BorderColor = BlackPixel(Display, ScreenID);
  ulong BackgroundColor = WhitePixel(Display, ScreenID);
  Window Window = XCreateSimpleWindow(Display, Parent, X, Y, WindowWidth, WindowHeight, BorderWidth, BorderColor, BackgroundColor);

  long EventMask = (KeyPressMask | KeyReleaseMask | KeymapStateMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XSelectInput(Display, Window, EventMask);

  XClearWindow(Display, Window);
  XMapWindow(Display, Window);

  if (Window)
  {
   b32 Running = true;
   linux_platform_input LinuxInput = {};

   while (Running)
   {
    XEvent Event;
    XNextEvent(Display, &Event);

    switch (Event.type)
    {
     case KeyPress:
     case KeyRelease: {
      b32 Pressed = (Event.type == KeyPress);
      platform_event *PlatformEvent = LinuxPushPlatformEvent(Pressed ? PlatformEvent_Press : PlatformEvent_Release);
      if (PlatformEvent)
      {
       PlatformEvent.Key = X11KeyCodeToPlatformKey(Display, Event.xkey.keycode);
      }    
     }break;

     case ButtonPress:
     case ButtonRelease: {
     }break;

     case MotionNotify: {
     }break;

     case KeymapNotify: {
      XRefreshKeyboardMapping(&Event.xmapping);
      OS_PrintErrorF("KeymapNotify\n");
     }break;

     default: {
      OS_PrintErrorF("Unknown Event\n");
     }break;
    }    
   }
  }
 }
 else
 {
  // TODO(hbr): error handling
 }

 return 0;
}