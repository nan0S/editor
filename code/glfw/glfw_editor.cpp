#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_thread_ctx.h"
#include "base/base_os.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_os.cpp"

#if 0
int main()
{
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0; ArenaIndex < ArrayCount(Arenas); ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena(Gigabytes(64));
 } 
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));

 arena *Arena = Arenas[0];


 return 0;
}
#endif

#include <X11/Xlib.h>

int main(int ArgCount, char *Argv[])
{
	Display *Display = XOpenDisplay(NULL);
 if (Display)
 {
  Screen *Screen = DefaultScreenOfDisplay(Display);
  int ScreenId = DefaultScreen(Display);
  int X = 0, Y = 0;
  unsigned int Width = 320;
  unsigned int Height = 200;
  unsigned int BorderWidth = 1;
  unsigned long BorderColor = BlackPixel(Display, ScreenId);
  unsigned long BackgroundColor = WhitePixel(Display, ScreenId);
  Window Parent = RootWindowOfScreen(Screen);
  Window Window = XCreateSimpleWindow(Display, Parent, X, Y, Width, Height, BorderWidth, BorderColor, BackgroundColor);  
  XClearWindow(Display, Window);
  XMapRaised(Display, Window);
  
  while (1)
  {
	  XEvent Event;
   XNextEvent(Display, &Event);
  }
 }
 else
 {
  // TODO(hbr): handle error
 }
 
	return 1;
}