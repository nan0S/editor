#include <windows.h>
#include <gl/gl.h>

#include "base_ctx_crack.h"
#include "base_core.h"
#include "base_string.h"

#include "imgui_bindings.h"

#include "editor_memory.h"
#include "editor_math.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "os_core.h"

#include "win32_editor_renderer.h"
#include "win32_editor_renderer_opengl.h"
#include "win32_imgui_bindings.h"

#include "base_core.cpp"
#include "base_string.cpp"
#include "os_core.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"

platform_api Platform;

internal LARGE_INTEGER
Win32GetClock()
{
 LARGE_INTEGER Result;
 QueryPerformanceCounter(&Result);
 return Result;
}

global u64 GlobalWin32ClockFrequency;
internal f32
Win32SecondsElapsed(LARGE_INTEGER Begin, LARGE_INTEGER End)
{
 f32 Result = Cast(f32)(End.QuadPart - Begin.QuadPart) / GlobalWin32ClockFrequency;
 return Result;
}

#define WIN32_BEGIN_DEBUG_BLOCK(Name)
#define WIN32_END_DEBUG_BLOCK(Name)

#if 0
#define WIN32_BEGIN_DEBUG_BLOCK(Name) u64 Name##BeginTSC = OS_ReadCPUTimer();
#define WIN32_END_DEBUG_BLOCK(Name) do { \
u64 EndTSC = OS_ReadCPUTimer(); \
f32 Elapsed = Win32SecondsElapsed(Name##BeginTSC, EndTSC); \
OS_PrintDebugF("%s: %fms\n", #Name, Elapsed * 1000.0f); \
LARGE_INTEGER Name##Begin = Win32GetClock(); \
} while(0)
#endif

typedef void wgl_swap_interval_ext(int);

DLL_EXPORT
WIN32_RENDERER_INIT(Win32RendererInit)
{
 Platform = Memory->PlatformAPI;
 {
  LARGE_INTEGER PerfCounterFrequency;
  QueryPerformanceFrequency(&PerfCounterFrequency);
  GlobalWin32ClockFrequency = PerfCounterFrequency.QuadPart;
 }
 
 opengl *OpenGL = PushStruct(Arena, opengl);
 OpenGL->Window = Window;
 
 //- set pixel format
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
  
  int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
  
  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, SizeOf(SuggestedPixelFormat), &SuggestedPixelFormat);
  SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
 }
 
 //- create opengl context
 HGLRC OpenGLRC = wglCreateContext(WindowDC);
 if (wglMakeCurrent(WindowDC, OpenGLRC))
 {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  //wglSwapIntervalEXT(0);
  
  wgl_swap_interval_ext *wglSwapIntervalEXT = Cast(wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
  wglSwapIntervalEXT(0);
 }
 
 //- allocate texutre indices
 u32 TextureCount = Memory->Limits.MaxTextureCount;
 GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
 OpenGL->MaxTextureCount = TextureCount;
 OpenGL->Textures = Textures;
 
 glGenTextures(Cast(GLsizei)TextureCount, Textures);
 for (u32 TextureIndex = 0;
      TextureIndex < TextureCount;
      ++TextureIndex)
 {
  glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
 }
 
 return Cast(renderer *)OpenGL;
}

DLL_EXPORT
RENDERER_BEGIN_FRAME(Win32RendererBeginFrame)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = Cast(opengl *)Renderer;
 if (Memory->RendererCodeReloaded)
 {
  LARGE_INTEGER PerfCounterFrequency;
  QueryPerformanceFrequency(&PerfCounterFrequency);
  GlobalWin32ClockFrequency = PerfCounterFrequency.QuadPart;
 }
 
 render_frame *RenderFrame = &OpenGL->RenderFrame;
 RenderFrame->CommandCount = 0;
 RenderFrame->Commands = Memory->CommandBuffer;
 RenderFrame->MaxCommandCount = Memory->MaxCommandCount;
 RenderFrame->WindowDim = WindowDim;
 
 Memory->ImGuiBindings.NewFrame();
 
 return RenderFrame;
}

internal int RenderCommandCmp(render_command *A, render_command *B) { return Cmp(A->ZOffset, B->ZOffset); }

internal m4x4
M3x3ToM4x4OpenGL(m3x3 M)
{
 // NOTE(hbr): OpenGL matrices are column-major
 M = Transpose3x3(M);
 m4x4 R = {
  { {M.M[0][0], M.M[0][1], 0, M.M[0][2]},
   {M.M[1][0], M.M[1][1], 0, M.M[1][2]},
   {        0,         0, 1,         0},
   {M.M[2][0], M.M[2][1], 0, M.M[2][2]}}
 };
 
 return R;
}

DLL_EXPORT
RENDERER_END_FRAME(Win32RendererEndFrame)
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 texture_transfer_queue *Queue = &Memory->TextureQueue;
 GLuint *Textures = OpenGL->Textures;
 m4x4 M = M3x3ToM4x4OpenGL(Frame->Proj);
 
 v4 Clear = Frame->ClearColor;
 glClearColor(Clear.R, Clear.G, Clear.B, Clear.A);
 glClear(GL_COLOR_BUFFER_BIT);
 glViewport(0, 0, Frame->WindowDim.X, Frame->WindowDim.Y);
 
 //- uploads textures into GPU
 for (u32 OpIndex = 0;
      OpIndex < Queue->OpCount;
      ++OpIndex)
 {
  texture_transfer_op *Op = Queue->Ops + OpIndex;
  Assert(Op->TextureIndex < OpenGL->MaxTextureCount);
  
  glBindTexture(GL_TEXTURE_2D, Textures[Op->TextureIndex]);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               Cast(u32)Op->Width,
               Cast(u32)Op->Height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               Op->Pixels);
 }
 Queue->OpCount = 0;
 Queue->TransferMemoryUsed = 0;
 
 //- draw
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, RenderCommandCmp);
 for (u32 CommandIndex = 0;
      CommandIndex < Frame->CommandCount;
      ++CommandIndex)
 {
  render_command *Command = Frame->Commands + CommandIndex;
  
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(Cast(f32 *)M.M);
  
  m4x4 Model = M3x3ToM4x4OpenGL(Command->ModelXForm);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(Cast(f32 *)Model.M);
  
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    int glPrimitive = 0;
    switch (Array->Primitive)
    {
     case Primitive_TriangleStrip: {glPrimitive = GL_TRIANGLE_STRIP;}break;
     case Primitive_Triangles:     {glPrimitive = GL_TRIANGLES;}break;
    }
    
    glBegin(glPrimitive);
    glColor4fv(Array->Color.E);
    for (u32 VertexIndex = 0;
         VertexIndex < Array->VertexCount;
         ++VertexIndex)
    {
     glVertex2fv(Array->Vertices[VertexIndex].Pos.E);
    }
    glEnd();
   }break;
   
   case RenderCommand_Circle: {
    render_command_circle *Circle = &Command->Circle;
    
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(Circle->Color.E);
    glVertex2f(0.0f, 0.0f);
    u32 VertexCount = 30;
    for (u32 VertexIndex = 0;
         VertexIndex <= VertexCount;
         ++VertexIndex)
    {
     f32 Radians = 2*Pi32 / VertexCount * VertexIndex;
     v2 P = Rotation2DFromRadians(Radians);
     glVertex2fv(P.E);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_STRIP);
    glColor4fv(Circle->OutlineColor.E);
    for (u32 VertexIndex = 0;
         VertexIndex <= VertexCount;
         ++VertexIndex)
    {
     f32 Radians = 2*Pi32 / VertexCount * VertexIndex;
     v2 P = Rotation2DFromRadians(Radians);
     v2 OP = Circle->OutlineThickness * P;
     glVertex2fv(P.E);
     glVertex2fv(OP.E);
    }
    glEnd();
   }break;
   
   case RenderCommand_Rectangle: {
    render_command_rectangle *Rect = &Command->Rectangle;
    
    glBegin(GL_QUADS);
    glColor4fv(Rect->Color.E);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
   }break;
   
   case RenderCommand_Triangle: {
    render_command_triangle *Tri = &Command->Triangle;
    
    glBegin(GL_TRIANGLES);
    glColor4fv(Tri->Color.E);
    glVertex2fv(Tri->P0.E);
    glVertex2fv(Tri->P1.E);
    glVertex2fv(Tri->P2.E);
    glEnd();
   }break;
   
   case RenderCommand_Image: {
    render_command_image *Image = &Command->Image;
    
    glBindTexture(GL_TEXTURE_2D, Textures[Image->TextureIndex]);
    
    glBegin(GL_QUADS);
    glColor4f(1, 1, 1, 1);
    
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
   }break;
  }
 }
 
#if 1
 m3x3 I = Identity3x3();
 m4x4 Model = M3x3ToM4x4OpenGL(I);
 glMatrixMode(GL_MODELVIEW);
 glLoadMatrixf(Cast(f32 *)Model.M);
 glBegin(GL_TRIANGLES);
 glColor4f(1, 0, 0, 1);
 glVertex2f(-0.5f, -0.5f);
 glVertex2f(+0.5f, -0.5f);
 glVertex2f( 0.0f, +0.5f);
 glEnd();
#endif
 
 Memory->ImGuiBindings.Render();
 SwapBuffers(wglGetCurrentDC());
}