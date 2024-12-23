#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"

#include "editor_memory.h"
#include "editor_math.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_third_party_inc.h"

#include "x11/x11_editor_renderer.h"
#include "x11/x11_editor_renderer_opengl.h"
#include "editor_renderer_opengl.h"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"
#include "editor_renderer_opengl.cpp"

platform_api Platform;

internal x11_renderer_init_result
X11CreateWindowAndInitOpenGL(arena *Arena, renderer_memory *Memory,
                             Display *Display,
                             u32 PosX, u32 PosY,
                             u32 WindowWidth, u32 WindowHeight,
                             u32 BorderWidth)
{
 Platform = Memory->PlatformAPI;

 Screen *Screen = DefaultScreenOfDisplay(Display);
 int ScreenID = DefaultScreen(Display);
 Window Root = RootWindow(Display, ScreenID);

 GLint glxAttribs[] = {
   GLX_X_RENDERABLE    , True,
   GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
   GLX_RENDER_TYPE     , GLX_RGBA_BIT,
   GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
   GLX_RED_SIZE        , 8,
   GLX_GREEN_SIZE      , 8,
   GLX_BLUE_SIZE       , 8,
   GLX_ALPHA_SIZE      , 8,
   GLX_DEPTH_SIZE      , 24,
   GLX_STENCIL_SIZE    , 8,
   GLX_DOUBLEBUFFER    , True,
   None
  };
  int FBCount;
  GLXFBConfig* FBConfigs = glXChooseFBConfig(Display, ScreenID, glxAttribs, &FBCount);

  // NOTE(hbr): We iterate through returned FBConfigs because we don't want to require
  // to have multisampling. If it's there, then use it, otherwise still procede without it.
  XVisualInfo *ChosenVisual = 0;
  GLXFBConfig ChosenFBConfig = 0;
  int ChosenSamples = -1;
  for (int FBIndex = 0;
       FBIndex < FBCount;
       ++FBIndex)
  {
    GLXFBConfig FBConfig = FBConfigs[FBIndex];
    XVisualInfo *Visual = glXGetVisualFromFBConfig(Display, FBConfig);
    if (Visual)
    {
     int SampleBuffers;
     int Samples;
     glXGetFBConfigAttrib(Display, FBConfig, GLX_SAMPLE_BUFFERS, &SampleBuffers);
     glXGetFBConfigAttrib(Display, FBConfig, GLX_SAMPLES, &Samples);

     if (Samples > ChosenSamples)
     {
      ChosenSamples = Samples;
      ChosenVisual = Visual;
      ChosenFBConfig = FBConfig;
     }
    }
  }

  XFree(FBConfigs);

 XSetWindowAttributes WindowAttributes = {};
 WindowAttributes.border_pixel = BlackPixel(Display, ScreenID);
 WindowAttributes.background_pixel = WhitePixel(Display, ScreenID);
 WindowAttributes.override_redirect = True;
 WindowAttributes.colormap = XCreateColormap(Display, Root, ChosenVisual->visual, AllocNone);
 WindowAttributes.event_mask  = ExposureMask;

 Window Window = XCreateWindow(Display, Root,
                               PosX, PosY,
                               WindowWidth, WindowHeight,
                               BorderWidth,
                               ChosenVisual->depth, InputOutput, ChosenVisual->visual,
                               CWBackPixel | CWColormap | CWBorderPixel | CWEventMask,
                               &WindowAttributes);
 
 func_glXCreateContextAttribsARB *glxCreateContextAttribsARB = Cast(func_glXCreateContextAttribsARB *)glXGetProcAddressARB(Cast(GLubyte const *)"glXCreateContextAttribsARB");
 GLXContext Context = 0;
 if (glxCreateContextAttribsARB)
 {
   int Attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if not(BUILD_DEBUG)
    |GLX_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
   };
   Context = glxCreateContextAttribsARB(Display, ChosenFBConfig, 0, true, Attribs);
 }
 else
 {
  Context = glXCreateNewContext(Display, ChosenFBConfig, GLX_RGBA_TYPE, 0, true);
 }

 if (!Context)
 {
  Context = glXCreateContext(Display, ChosenVisual, 0, GL_TRUE);  
 }
 XSync(Display, false);

 if (glXMakeCurrent(Display, Window, Context))
 {
  // TODO(hbr): retrieve OpenGL functions
 }

 glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

 opengl *OpenGL = PushStruct(Arena, opengl);
 x11_opengl_renderer *X11 = PushStruct(Arena, x11_opengl_renderer);
 X11->Display = Display;
 X11->X11Window = Window;
 OpenGL->Header.Platform = X11;
 
 x11_renderer_init_result Result = {};
 Result.Renderer = Cast(renderer *)OpenGL;
 Result.X11Window = Window;

 return Result;
}

DLL_EXPORT
X11_CREATE_WINDOW_AND_INIT_RENDERER(X11CreateWindowAndInitRenderer)
{
 x11_renderer_init_result Result = X11CreateWindowAndInitOpenGL(Arena, Memory, Window, WindowDC);
 return Result;
}

DLL_EXPORT
RENDERER_BEGIN_FRAME(X11RendererBeginFrame)
{
 render_frame *Result = OpenGLBeginFrame(Cast(opengl *)Renderer, Memory, WindowDim);
 return Result;
}

DLL_EXPORT
RENDERER_END_FRAME(X11RendererEndFrame)
{
 OpenGLEndFrame(Cast(opengl *)Renderer, Memory, Frame);
 x11_opengl_renderer *X11 = Cast(x11_opengl_renderer *)Renderer->Header.Platform;
 glXSwapBuffers(X11->Display, X11->X11Window);
}
