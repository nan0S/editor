#include <windows.h>
#include <gl/gl.h>

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

#include "win32/win32_editor_renderer.h"
#include "win32/win32_editor_renderer_opengl.h"
#include "win32/win32_editor_imgui_bindings.h"
#include "win32/win32_shared.h"

#include "editor_renderer_opengl.h"
#include "editor_renderer_opengl.cpp"

#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"

internal void
Win32SetPixelFormat(win32_opengl_renderer *Win32OpenGL, HDC WindowDC)
{
 WIN32_BEGIN_DEBUG_BLOCK(SetPixelFormat);
 
 int SuggestedPixelFormatIndex = 0;
 UINT ExtendedPicked = 0;
 
 if (Win32OpenGL->wglChoosePixelFormatARB)
 {
  int AttribIList[] =
  {
   WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
   WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
   WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
   WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
   WGL_COLOR_BITS_ARB, 32,
   WGL_ALPHA_BITS_ARB, 8,
   WGL_DEPTH_BITS_ARB, 24,
   // NOTE(hbr): When I set WGL_SAMPLES_ARB to some values (e.g. 16), ImGui text becomes blurry.
   // I don't know why that is.
   WGL_SAMPLES_ARB, 8,
   // NOTE(hbr): I don't know if this is really needed, but Casey uses it
   WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
   0
  };
  
  Win32OpenGL->wglChoosePixelFormatARB(WindowDC,
                                       AttribIList,
                                       0,
                                       1,
                                       &SuggestedPixelFormatIndex,
                                       &ExtendedPicked);
 }
 
 if (!ExtendedPicked)
 {
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = SizeOf(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DesiredPixelFormat.dwFlags = (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER);
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.cDepthBits = 24;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
  
  SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
 }
 
 PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
 DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, SizeOf(SuggestedPixelFormat), &SuggestedPixelFormat);
 SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
 
 WIN32_END_DEBUG_BLOCK(SetPixelFormat);
}

#define OpenGLFunction(Name) OpenGL->Name = Cast(func_##Name *)wglGetProcAddress(#Name);
#define Win32OpenGLFunction(Name) Win32OpenGL->Name = Cast(func_##Name *)wglGetProcAddress(#Name);

internal opengl *
Win32OpenGLInit(arena *Arena, renderer_memory *Memory, HWND Window, HDC WindowDC)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = PushStruct(Arena, opengl);
 win32_opengl_renderer *Win32OpenGL = PushStruct(Arena, win32_opengl_renderer);
 Win32OpenGL->WindowDC = WindowDC;
 OpenGL->Header.Platform = Win32OpenGL;
 
 //- create false context to retrieve extensions pointers
 {
  WNDCLASSA WindowClass = {};
  { 
   WindowClass.lpfnWndProc = DefWindowProcA;
   WindowClass.hInstance = GetModuleHandle(0);
   WindowClass.lpszClassName = "Win32EditorWGLLoader";
  }
  
  if (RegisterClassA(&WindowClass))
  {
   HWND FalseWindow =
    CreateWindowExA(0,
                    WindowClass.lpszClassName,
                    "Parametric Curves Editor",
                    0,
                    CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                    0,
                    0,
                    WindowClass.hInstance,
                    0);
   
   HDC FalseWindowDC = GetWindowDC(FalseWindow);
   Win32SetPixelFormat(Win32OpenGL, FalseWindowDC);
   
   HGLRC FalseOpenGLRC = wglCreateContext(FalseWindowDC);
   if (wglMakeCurrent(FalseWindowDC, FalseOpenGLRC))
   {
    Win32OpenGLFunction(wglChoosePixelFormatARB);
   }
   
   wglDeleteContext(FalseOpenGLRC);
   ReleaseDC(FalseWindow, FalseWindowDC);
   DestroyWindow(FalseWindow);
  }
 }
 
 //- initialize opengl context 
 Win32SetPixelFormat(Win32OpenGL, WindowDC);
 HGLRC OpenGLRC = wglCreateContext(WindowDC);
 if (wglMakeCurrent(WindowDC, OpenGLRC))
 {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  // NOTE(hbr): So that glEnable,glEnd with glBindTexture works. When using shaders it is not needed.
  glEnable(GL_TEXTURE_2D);
  
  Win32OpenGLFunction(wglSwapIntervalEXT);
  OpenGLFunction(glGenVertexArrays);
  OpenGLFunction(glBindVertexArray);
  OpenGLFunction(glEnableVertexAttribArray);
  OpenGLFunction(glDisableVertexAttribArray);
  OpenGLFunction(glVertexAttribPointer);
  OpenGLFunction(glActiveTexture);
  OpenGLFunction(glGenBuffers);
  OpenGLFunction(glBindBuffer);
  OpenGLFunction(glBufferData);
  OpenGLFunction(glDrawBuffers);
  OpenGLFunction(glCreateProgram);
  OpenGLFunction(glCreateShader);
  OpenGLFunction(glAttachShader);
  OpenGLFunction(glCompileShader);
  OpenGLFunction(glShaderSource);
  OpenGLFunction(glLinkProgram);
  OpenGLFunction(glGetProgramInfoLog);
  OpenGLFunction(glGetShaderInfoLog);
  OpenGLFunction(glUseProgram);
  OpenGLFunction(glGetUniformLocation);
  OpenGLFunction(glUniform1f);
  OpenGLFunction(glUniform2fv);
  OpenGLFunction(glUniform3fv);
  OpenGLFunction(glUniform4fv);
  OpenGLFunction(glUniformMatrix3fv);
  OpenGLFunction(glUniformMatrix4fv);
  OpenGLFunction(glGetAttribLocation);
  OpenGLFunction(glValidateProgram);
  OpenGLFunction(glGetProgramiv);
  OpenGLFunction(glDeleteProgram);
  OpenGLFunction(glDeleteShader);
  OpenGLFunction(glDrawArrays);
  OpenGLFunction(glGenerateMipmap);
  OpenGLFunction(glDebugMessageCallback);
  OpenGLFunction(glVertexAttribDivisor);
  OpenGLFunction(glDrawArraysInstanced);
  
  if (Win32OpenGL->wglSwapIntervalEXT)
  {
   // NOTE(hbr): disable VSync
   Win32OpenGL->wglSwapIntervalEXT(0);
  }
  
  OpenGLInit(OpenGL, Arena, Memory);
 }
 
 return OpenGL;
}

#undef OpenGLFunction
#undef Win32OpenGLFunction

DLL_EXPORT
WIN32_RENDERER_INIT(Win32RendererInit)
{
 opengl *OpenGL = Win32OpenGLInit(Arena, Memory, Window, WindowDC);
 return Cast(renderer *)OpenGL;
}

DLL_EXPORT
RENDERER_BEGIN_FRAME(Win32RendererBeginFrame)
{
 render_frame *Result = OpenGLBeginFrame(Cast(opengl *)Renderer, Memory, WindowDim);
 return Result;
}

DLL_EXPORT
RENDERER_END_FRAME(Win32RendererEndFrame)
{
 OpenGLEndFrame(Cast(opengl *)Renderer, Memory, Frame);
 win32_opengl_renderer *Win32OpenGL = Cast(win32_opengl_renderer *)Renderer->Header.Platform;
 SwapBuffers(Win32OpenGL->WindowDC);
}
